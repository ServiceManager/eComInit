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
/* BSD style */
#define S16_SO_CREDOPT_LVL 0

#if defined(__FreeBSD__)
/* FreeBSD case */
#define CREDS struct cmsgcred
#define CREDS_PID(c) c->cmcred_pid
#define S16_SO_CREDOPT LOCAL_PEERCRED
#else
/* NetBSD case */
#define CREDS struct sockcred
#define CREDS_PID(c) c->sc_pid
#define S16_SO_CREDOPT LOCAL_CREDS
#endif

#elif defined(SCM_CREDENTIALS)
/* Linux style */
#define S16_SO_CREDOPT_LVL SOL_SOCKET
#define CREDS struct ucred
#define CREDS_PID(c) c->pid
#define S16_SO_CREDOPT SO_PASSCRED
#define SCM_CREDS SCM_CREDENTIALS

#else

#error "Unsupported platform - can't pass credentials over socket"

#endif

#include "manager.h"
#include "s16db.h"
#include "s16rr.h"

const int NOTE_RREQ = 18;

Manager manager;

void clean_exit () { sd_notify_srv_cleanup (); }

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

/* TODO: Evaluate if this is still useful now we have the notify functionality.
 * Connections to the repository shouldn't fail if it has notified as being up,
 * except with EINTR, which we should handle in libs16. */
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

/* TODO: Move to libs16 */
void handle_signal (int sig)
{
    struct kevent sigev;

#ifdef S16_PLAT_BSD
    /* On BSD, setting a signal event filter on a Kernel Queue does NOT
     * supersede ordinary signal disposition; with libkqueue on Linux at least,
     * however, it does. Therefore we ignore the signal; it'll be multiplexed
     * into our event loop instead. */
    signal (sig, SIG_IGN);
#endif

    EV_SET (&sigev, sig, EVFILT_SIGNAL, EV_ADD, 0, 0, 0);

    if (kevent (manager.kq, &sigev, 1, 0, 0, 0) == -1)
    {
        perror ("KQueue: Failed to set signal event\n");
        exit (EXIT_FAILURE);
    }
}

/* Sets up a manifest-import service to read services into the repository. */
void setup_manifest_import ()
{
    Unit * configd = unit_add (s16_path_configd ());
    s16note_t * note =
        s16note_new (N_RESTARTER_REQ, RR_START, configd->path, 0);

    configd->type = U_ONESHOT;
    configd->methods[UM_START] = "/opt/s16/etc/s16/method/manifest-import";
    configd->state = US_OFFLINE;

    unit_msg (configd, note);
    s16note_destroy (note);
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

    sd_notify_srv_setup (manager.kq);

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

        configd->type = U_NOTIFY;
        configd->methods[UM_PRESTART] = "/usr/bin/env echo PRESTART";
        configd->methods[UM_START] = "/opt/s16/libexec/s16.configd";
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
        sd_notify_srv_investigate_kevent (&ev);

        switch (ev.filter)
        {

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
