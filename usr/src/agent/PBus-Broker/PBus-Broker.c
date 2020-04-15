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

#include <S16/NVRPC.h>
#include <S16/Service.h>
#include <nv.h>
#include <systemd/sd-daemon.h>

#include "PBus-Broker.h"
#include "PBus_priv.h"

PBusBroker gBroker;

void clean_exit () { unlink (kPBusSocketPath); }

static bool PBusClient_isForFD (PBusClient * pbc, int fd)
{
    return pbc->aFD == fd;
}

PBusClient * PBusClient_new (int fd)
{
    char buf[6];
    struct kevent ev;
    PBusClient * pbc = calloc (1, sizeof (*pbc));
    pbc->aFD = fd;

    EV_SET (&ev, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
    if (kevent (gBroker.aKQ, &ev, 1, NULL, 0, NULL) == -1)
        perror ("Add KEvent for new client:");

    PBusClient_list_add (&gBroker.aClients, pbc);
    S16Log (kS16LogInfo, "[FD %d] New client accepted.\n", pbc->aFD);

    return pbc;
}

void PBusClient_disconnect (PBusClient * pbc)
{
    struct kevent ev;

    EV_SET (&ev, pbc->aFD, EVFILT_READ, EV_DELETE, 0, 0, NULL);
    if (kevent (gBroker.aKQ, &ev, 1, NULL, 0, NULL) == -1)
        perror ("kevent");

    close (pbc->aFD);
    PBusClient_list_del (&gBroker.aClients, pbc);
    S16Log (kS16LogInfo, "[FD %d] Client disconnected.\n", pbc->aFD);
    free (pbc);
}

void PBusClient_recv (PBusClient * pbc)
{
    nvlist_t * nvl;

    S16Log (kS16LogInfo, "[FD %d] Receiving data from client %d.\n", pbc->aFD);
    S16NVRPCServerReceiveFromFileDescriptor (gBroker.aRPCServer, pbc->aFD);
}

PBusClient * PBusBroker_findClient (int fd)
{
    return PBusClient_list_find_int (&gBroker.aClients, PBusClient_isForFD, fd)
        ->val;
}

void * msgRecv (S16NVRPCCallContext * ctx, const char * fromBusname,
                const char * toBusname, const char * objectPath,
                const char * selector, nvlist_t * params)
{
    printf ("Receive from busname %s to busname %s\n", fromBusname, toBusname);

    return nvlist_create (0);
}

int main ()
{
    struct sockaddr_un sun;
    struct kevent ev;
    bool run = true;

    /* make sure repo socket deleted after exit */
    atexit (clean_exit);
    S16LogInit ("P-Bus Broker");

    unlink (kPBusSocketPath);

    if ((gBroker.aKQ = kqueue ()) == -1)
    {
        perror ("KQueue: Failed to open\n");
    }

    S16HandleSignalWithKQueue (gBroker.aKQ, SIGINT);

    if ((gBroker.aListenSocket = socket (AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        perror ("Failed to create socket");
        exit (-1);
    }

    memset (&sun, 0, sizeof (struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strncpy (sun.sun_path, kPBusSocketPath, sizeof (sun.sun_path));

    if (bind (gBroker.aListenSocket, (struct sockaddr *)&sun, SUN_LEN (&sun)) ==
        -1)
    {
        perror ("Failed to bind socket");
        exit (-1);
    }

    listen (gBroker.aListenSocket, 5);

    EV_SET (&ev, gBroker.aListenSocket, EVFILT_READ, EV_ADD, 0, 0, NULL);

    if (kevent (gBroker.aKQ, &ev, 1, NULL, 0, NULL) == -1)
    {
        perror ("KQueue: Failed to set read event\n");
        exit (EXIT_FAILURE);
    }

    gBroker.aRPCServer = S16NVRPCServerNew (NULL);
    S16NVRPCServerRegisterMethod (
        gBroker.aRPCServer, &msgSendSig, (S16NVRPCImplementationFn)msgRecv);

    sd_notify (0, "READY=1\nSTATUS=P-Bus Broker now accepting connections");

    while (run)
    {
        memset (&ev, 0x00, sizeof (struct kevent));
        kevent (gBroker.aKQ, NULL, 0, &ev, 1, NULL);

        switch (ev.filter)
        {
        case EVFILT_READ:
        {
            int fd = ev.ident;
            printf ("Event!\n");

            if ((ev.flags & EV_EOF) && !(fd == gBroker.aListenSocket))
            {
                PBusClient_disconnect (PBusBroker_findClient (fd));
            }
            else if (ev.ident == gBroker.aListenSocket)
            {
                int newFd = accept (fd, NULL, NULL);
                if (newFd == -1)
                    perror ("accept");
                else
                    PBusClient_new (newFd);
            }
            else
            {
                PBusClient_recv (PBusBroker_findClient (fd));
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