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

#include <sys/event.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/un.h>

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "nv.h"
#include "s16.h"
#include "s16newrpc.h"
#include "systemd/sd-daemon.h"

#include "PBusBroker.h"
#include "PBus_priv.h"

PBusContext pbCtx;

void clean_exit () { unlink (kPBusSocketPath); }

static bool PBusClient_isForFD (PBusClient * pbc, int fd)
{
    return pbc->aFD == fd;
}

PBusClient * PBusClient_new (int fd)
{
    struct kevent ev;
    PBusClient * pbc = calloc (1, sizeof (*pbc));
    pbc->aFD = fd;

    EV_SET (&ev, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
    if (kevent (pbCtx.aListenSocket, &ev, 1, NULL, 0, NULL) == -1)
        perror ("kevent");

    return pbc;
}

void PBusClient_disconnect (PBusClient * pbc)
{
    struct kevent ev;

    EV_SET (&ev, pbc->aFD, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    if (kevent (pbCtx.aListenSocket, &ev, 1, NULL, 0, NULL) == -1)
        perror ("kevent");

    close (pbc->aFD);
    PBusClient_list_del (&pbCtx.aClients, pbc);
    free (pbc);
}

void PBusClient_recv (PBusClient * pbc)
{
    // nvlist_t * nvl = nvlist_recv (pbc->aFD, 0);
}

PBusClient * PBusContext_findClient (int fd)
{
    return PBusClient_list_find_int (&pbCtx.aClients, PBusClient_isForFD, fd)
        ->val;
}

int main ()
{
    struct sockaddr_un sun;
    struct kevent ev;
    bool run = true;

    /* make sure repo socket deleted after exit */
    atexit (clean_exit);
    s16_log_init ("P-Bus Broker");

    if ((pbCtx.aKQ = kqueue ()) == -1)
    {
        perror ("KQueue: Failed to open\n");
    }

    s16_handle_signal (pbCtx.aKQ, SIGINT);

    if ((pbCtx.aListenSocket = socket (AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        perror ("Failed to create socket");
        exit (-1);
    }

    memset (&sun, 0, sizeof (struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strncpy (sun.sun_path, kPBusSocketPath, sizeof (sun.sun_path));

    if (bind (pbCtx.aListenSocket, (struct sockaddr *)&sun, SUN_LEN (&sun)) ==
        -1)
    {
        perror ("Failed to bind socket");
        exit (-1);
    }

    listen (pbCtx.aListenSocket, 5);

    EV_SET (&ev, pbCtx.aListenSocket, EVFILT_READ, EV_ADD, 0, 0, NULL);

    if (kevent (pbCtx.aKQ, &ev, 1, NULL, 0, NULL) == -1)
    {
        perror ("KQueue: Failed to set read event\n");
        exit (EXIT_FAILURE);
    }

    sd_notify (0, "READY=1\nSTATUS=P-Bus Broker now accepting connections");

    while (run)
    {
        memset (&ev, 0x00, sizeof (struct kevent));
        kevent (pbCtx.aKQ, NULL, 0, &ev, 1, NULL);

        switch (ev.filter)
        {
        case EVFILT_READ:
        {
            int fd = ev.ident;

            if ((ev.flags & EV_EOF) && !(fd == pbCtx.aListenSocket))
            {
                perror ("Socket shut\n");
                PBusClient_disconnect (PBusContext_findClient (fd));
            }
            else if (ev.ident == pbCtx.aListenSocket)
            {
                printf ("Accept\n");
                fd = accept (fd, NULL, NULL);
                if (fd == -1)
                    perror ("accept");

                PBusClient_new (fd);
            }
            else
            {
                printf ("Recv\n");
                PBusClient_recv (PBusContext_findClient (fd));
            }

            break;
        }
        case EVFILT_SIGNAL:
            fprintf (
                stderr,
                "PBus-Broker: Signal received: %lu. Additional data: %ld\n",
                ev.ident,
                ev.data);
            if (ev.ident == SIGINT)
                run = false;
            break;
        }
    }

    return 1;
}