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
#include <sys/socket.h>
#include <sys/un.h>
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

static int sd_notify_fd;

void sd_notify_srv_cleanup ()
{
#ifdef S16_ENABLE_SD_NOTIFY
    unlink (NOTIFY_SOCKET_PATH);
#endif
}

void sd_notify_srv_setup (int kq)
{
#ifdef S16_ENABLE_SD_NOTIFY
    int s;
    struct sockaddr_un sun;
    struct kevent ev;
    int yes = 1;

    if ((s = socket (AF_UNIX, SOCK_DGRAM, 0)) == -1)
    {
        perror ("Failed to create socket for sd_notify");
        exit (-1);
    }

    memset (&sun, 0, sizeof (struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strncpy (sun.sun_path, NOTIFY_SOCKET_PATH, sizeof (sun.sun_path));

    if (setsockopt (s, S16_SO_CREDOPT_LVL, S16_SO_CREDOPT, &yes, sizeof (int)) <
        0)
    {
        perror ("setsockopt for sd_notify!");
    }

    if (bind (s, (struct sockaddr *)&sun, SUN_LEN (&sun)) == -1)
    {
        perror ("Failed to bind socket for sd_notify");
        exit (-1);
    }

    s16_cloexec (s);

    EV_SET (&ev, s, EVFILT_READ, EV_ADD, 0, 0, NULL);
    if (kevent (kq, &ev, 1, 0, 0, 0) == -1)
    {
        perror ("KQueue: Failed to set read event for sd_notify\n");
        exit (EXIT_FAILURE);
    }

    sd_notify_fd = s;

#endif
}

#ifdef S16_ENABLE_SD_NOTIFY
static void dispatch_sd_notify (pid_t pid, char * buf)
{
    char *seg, *saveptr = NULL;
    /* parsed outputs */
    Unit * unit = manager_find_unit_for_pid (pid);

    if (!unit)
        s16_log (WARN,
                 "Notification message sent for PID %d which does not belong "
                 "to any known unit",
                 pid);

    seg = strtok_r (buf, "\n", &saveptr);
    while (seg)
    {
        size_t len = strlen (seg);

        if (len == 7 && !strncmp (seg, "READY=1", 7))
            unit_notify_ready (unit);
        else if (len > 7 && !strncmp (seg, "STATUS=", 7))
            unit_notify_status (unit, strndup (seg + 7, len - 7));
        else
            s16_log (WARN, "Unhandled component of notify message: \"%s\"\n",
                     seg);

        seg = strtok_r (NULL, "\n", &saveptr);
    }
}

#endif

void sd_notify_srv_investigate_kevent (struct kevent * ev)
{
#ifdef S16_ENABLE_SD_NOTIFY
    char buf[2048];
    struct sockaddr_storage sst;
    struct iovec iov[1];
    union {
        struct cmsghdr hdr;
        char cred[CMSG_SPACE (sizeof (CREDS))];
    } cmsg;
    CREDS * creds;

    if (!(ev->filter == EVFILT_READ && ev->ident == sd_notify_fd))
        return;

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
    ssize_t count = recvmsg (sd_notify_fd, &msg, 0);

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
        s16_log (DBG,
                 "Received SystemD-style notification from pid %lu: <%s>\n",
                 CREDS_PID (creds), buf);
        dispatch_sd_notify (CREDS_PID (creds), buf);
    }
#endif
}
