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

#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/event.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "systemd/sd-daemon.h"

#include "s16clnt.h"
#include "s16db_priv.h"
#include "s16srv.h"

#include "configd.h"

/* System-global scope. */
s16db_scope_t global;
/* User scope. */
s16db_scope_t user;

subscriber_list_t subs;
s16note_list_t notes;

void clean_exit ()
{
    db_destroy ();
    unlink (S16DB_CONFIGD_SOCKET_PATH);
}

int main (int argc, char * argv[])
{
    int listener_s;
    int kq;
    struct sockaddr_un sun;
    struct kevent ev;
    s16rpc_srv_t * srv;
    bool run = true;

    if ((listener_s = socket (AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        perror ("Failed to create socket");
        exit (-1);
    }

    memset (&sun, 0, sizeof (struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strncpy (sun.sun_path, S16DB_CONFIGD_SOCKET_PATH, sizeof (sun.sun_path));

    if (bind (listener_s, (struct sockaddr *)&sun, SUN_LEN (&sun)) == -1)
    {
        perror ("Failed to bind socket");
        exit (-1);
    }

    listen (listener_s, 5);

    s16_log_init ("Service Repository");
    db_setup ();

    subs = subscriber_list_new ();
    notes = s16note_list_new ();

    kq = kqueue ();

    signal (SIGINT, SIG_IGN);

    EV_SET (&ev, SIGINT, EVFILT_SIGNAL, EV_ADD, 0, 0, 0);

    if (kevent (kq, &ev, 1, 0, 0, 0) == -1)
    {
        perror ("KQueue: Failed to set signal event\n");
        exit (EXIT_FAILURE);
    }

    srv = s16rpc_srv_new (kq, listener_s, NULL, false);
    rpc_setup (srv);

    /* make sure repo socket deleted after exit */
    atexit (clean_exit);

    sd_notify (0, "READY=1\nSTATUS=Service repository up and running");

    while (run)
    {
        s16note_t * note = NULL;

        memset (&ev, 0x00, sizeof (struct kevent));
        kevent (kq, NULL, 0, &ev, 1, NULL);

        s16rpc_investigate_kevent (srv, &ev);

        if (ev.flags & EV_EOF)
        {
            int fd = ev.ident;
            subscriber_t * sub = NULL;

            list_foreach (subscriber, &subs, it)
            {
                if (it->val->clnt.fd == fd)
                    sub = it->val;
            }

            /* one of our subscribers closed their socket, remove from
             * subscriber list */
            if (sub)
                subscriber_list_del (&subs, sub);
        }

        switch (ev.filter)
        {
        case EVFILT_SIGNAL:
            if (ev.ident == SIGINT)
                run = false;
            break;
        }

        while ((note = s16note_list_lpop (&notes)))
        {
            ucl_object_t * unote = s16db_note_to_ucl (note);
            list_foreach (subscriber, &subs, it)
            {
                if (it->val->kinds & note->note_type)
                    s16rpc_clnt_call_unsafe (&it->val->clnt, "notify", unote);
            }
        }
    }

    return 0;
}
