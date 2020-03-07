/*
 * CDDL HEADER START
 *
 * This file and its contents are supplied under the terms of the
 * Common Development and Distribution License ("CDDL"), version 1.0.
 * You may only use this file in accordance with the terms of version
 * 1.0 of the CDDL.
 *
 * A full copy of the text of the CDDL should have accompanied this
 * source.  A copy of the CDDL is also available via the Internet at
 * http://www.illumos.org/license/CDDL.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/SYSTEMXVI.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2020 David MacKay.  All rights reserved.
 * Use is subject to license terms.
 */

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <unistd.h>

#if defined(SCM_CREDS)
#define CREDS struct cmsgcred
#define CREDS_PID(c) c->cmcred_pid
#elif defined(SCM_CREDENTIALS)
#define CREDS struct ucred
#define CREDS_PID(c) c->pid
#define LOCAL_PEERCRED SO_PASSCRED
#define SCM_CREDS SCM_CREDENTIALS
#else
#error "Unsupported platform - can't pass credentials over socket"
#endif

#include "manager.h"
#include "s16db.h"
#include "s16rr.h"

const int NOTE_RREQ = 18;

Manager manager;

void clean_exit () { unlink (NOTIFY_SOCKET_PATH); }

void manager_fork_cleanup ()
{
    /* In principle, we don't need to do this. Everything of ours is CLOEXEC and
     * we *should* be able to close the Kernel Queue and accordingly have all
     * related FDs of that closed, as the KQueue manual pages say. But this
     * doesn't work on my Linux system; libkqueue does not mark its various FDs
     * as CLOEXEC, and what's worse, explicitly using EV_DELETE on our
     * EVFILT_SIGNAL handlers stops us, the parent, from receiving EVFILT_SIGNAL
     * events, but leaves the signalfds open! Something is rotten. */
    for (int fd = 3; fd < 256; fd++)
        close (fd);
}

void repo_retry_cb (long timer, void * unused)
{
    if ((s16db_hdl_new (&manager.h)))
    {
        perror ("Failed to connect to repository");
        if ((manager.repo_retry_delay = manager.repo_retry_delay * 2) > 3000)
            s16_log (ERR, "Repeatedly failed to connect to repository.\n");
        else
        {
            manager.repo_retry_timer = timerset_add (
                &manager.ts, manager.repo_retry_delay, NULL, repo_retry_cb);
        }
    }
    else
    {
        s16_log (DBG, "Connected to the repository.\n");
        manager.repo_up = true;
    }
}

void manager_configd_came_up ()
{
    if ((s16db_hdl_new (&manager.h)))
    {
        perror ("Failed to connect to repository");
        manager.repo_retry_delay = 5;
        manager.repo_retry_timer =
            timerset_add (&manager.ts, 5, NULL, repo_retry_cb);
    }
    else
    {
        manager.repo_up = true;
    }
}

Unit * manager_find_unit_for_pid (pid_t pid)
{
    LL_each (&manager.units, it)
    {
        if (unit_has_pid (it->val, pid))
            return it->val;
    }
    return NULL;
}

void setup_sd_notify ()
{
    int s;
    struct sockaddr_un sun;
    struct kevent ev;

    if ((s = socket (AF_UNIX, SOCK_DGRAM, 0)) == -1)
    {
        perror ("Failed to create socket for sd_notify");
        exit (-1);
    }

    memset (&sun, 0, sizeof (struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strncpy (sun.sun_path, NOTIFY_SOCKET_PATH, sizeof (sun.sun_path));

    if (bind (s, (struct sockaddr *)&sun, SUN_LEN (&sun)) == -1)
    {
        perror ("Failed to bind socket for sd_notify");
        exit (-1);
    }

    s16_cloexec (s);

    EV_SET (&ev, s, EVFILT_READ, EV_ADD, 0, 0, NULL);
    if (kevent (manager.kq, &ev, 1, 0, 0, 0) == -1)
    {
        perror ("KQueue: Failed to set read event for sd_notify\n");
        exit (EXIT_FAILURE);
    }

    manager.sd_notify_s = s;
}

void handle_sd_notify_recv ()
{
    char buf[2048];
    struct sockaddr_storage sst;
    struct iovec iov[1];
    union {
        struct cmsghdr hdr;
        char cred[CMSG_SPACE (sizeof (CREDS))];
    } cmsg;
    CREDS * creds;
    int yes = 1;

    if (setsockopt (manager.sd_notify_s, SOL_SOCKET, LOCAL_PEERCRED, &yes,
                    sizeof (int)) < 0)
    {
        perror ("setsockopt for sd_notify!");
    }

    iov[0].iov_base = buf;
    iov[0].iov_len = sizeof (buf);

    struct msghdr msg;
    msg.msg_name = &sst;
    msg.msg_namelen = sizeof (sst);
    msg.msg_iov = iov;
    msg.msg_iovlen = 1;
    msg.msg_control = &cmsg;
    msg.msg_controllen = sizeof (cmsg);

    memset (buf, 0, sizeof (buf));
    ssize_t count = recvmsg (manager.sd_notify_s, &msg, 0);

    if (count == -1)
    {
        perror ("Receive sd_notify message!");
    }
    else if (msg.msg_flags & MSG_TRUNC)
    {
        s16_log (WARN, "Truncated message; not processing.");
    }
    else
    {
        if (cmsg.hdr.cmsg_type != SCM_CREDS)
        {
            s16_log (
                WARN,
                "Missing credentials in sd_notify message; not processing.\n");
            return;
        }

        creds = (CREDS *)CMSG_DATA (&cmsg.hdr);
        s16_log (DBG, "Notification from pid %lu of <%s>\n", CREDS_PID (creds),
                 buf);
    }
}

void handle_signal (int sig)
{
    struct kevent sigev;

    EV_SET (&sigev, sig, EVFILT_SIGNAL, EV_ADD, 0, 0, 0);

    if (kevent (manager.kq, &sigev, 1, 0, 0, 0) == -1)
    {
        perror ("KQueue: Failed to set signal event\n");
        exit (EXIT_FAILURE);
    }
}

int main (int argc, char * argv[])
{
    bool run = true;
    struct timespec tmout = {0, 0}; /* return at once initially */

    s16_log_init ("Master Restarter");

    atexit (clean_exit);

    manager.kq = kqueue ();
    if (manager.kq == -1)
        perror ("KQueue: Failed to open\n");
    s16_cloexec (manager.kq);

    setup_sd_notify ();

    manager.pt = pt_new (manager.kq);
    timerset_init (&manager.ts, manager.kq);

    if ((s16db_hdl_new (&manager.h)))
    {
        perror ("Failed to connect to repository");
        manager.repo_up = false;
    }

    handle_signal (SIGHUP);
    handle_signal (SIGCHLD);
    handle_signal (SIGINT);

    if (!manager.repo_up)
    {
        Unit * configd = unit_add (s16_path_configd ());
        s16note_t * note =
            s16note_new (N_RESTARTER_REQ, RR_START, configd->path, 0);

        configd->type = U_SIMPLE;
        configd->methods[UM_PRESTART] = "/usr/bin/env echo PRESTART";
        configd->methods[UM_START] = "/usr/bin/env sleep 90";
        configd->methods[UM_POSTSTART] = "/usr/bin/env echo POSTSTART";
        configd->methods[UM_STOP] = "/usr/bin/env echo STOP";
        configd->state = US_OFFLINE;

        unit_msg (configd, note);
        s16note_destroy (note);
    }

    while (run)
    {
        struct kevent ev;
        pt_info_t * info;

        memset (&ev, 0x00, sizeof (struct kevent));

        if (kevent (manager.kq, NULL, 0, &ev, 1, &tmout) == -1)
        {
            perror ("KQueue: Failed to wait for events\n");
            exit (EXIT_FAILURE);
        }

        tmout.tv_sec = 3;

        if ((info = pt_investigate_kevent (manager.pt, &ev)))
        {
            Unit * unit = manager_find_unit_for_pid (info->pid);

            if (!unit)
                unit = manager_find_unit_for_pid (info->ppid);

            if (!unit)
                fprintf (stderr, "error: no unit associated with pid %d\n",
                         info->pid);
            else
                unit_ptevent (unit, info);

            s16mem_free (info);
        }

        timerset_investigate_kevent (&manager.ts, &ev);

        switch (ev.filter)
        {
        case EVFILT_READ:
            if (ev.ident == manager.sd_notify_s)
                handle_sd_notify_recv ();
            break;

        case EVFILT_SIGNAL:
            fprintf (stderr,
                     "Restartd: Signal received: %lu. Additional data: %ld\n",
                     ev.ident, ev.data);
            if (ev.ident == SIGCHLD)
                while (waitpid ((pid_t) (-1), 0, WNOHANG) > 0)
                    ;
            else if (ev.ident == SIGINT)
                run = false;
            break;
        }
    }

    pt_destroy (manager.pt);
    close (manager.kq);

    return 0;
}
