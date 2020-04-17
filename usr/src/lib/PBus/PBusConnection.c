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
#include <sys/socket.h>
#include <sys/un.h>

#include "S16/NVRPC.h"

#include "PBus/PBus.h"
#include "PBus/PBus_Private.h"

/*
 * Creates a new PBusConnection with @rootObject as its root object. You may
 * specify NULL if you don't want to respond to anything.
 */
PBusConnection * PBusConnectionNew (PBusObject * rootObject)
{
    PBusConnection * conn = malloc (sizeof (*conn));

    conn->fd = -1;
    conn->rpcServer = S16NVRPCServerNew (conn);
    conn->rootObject = rootObject;
    conn->brokerObject = NULL;

    return conn;
}

bool PBusConnectionIsConnected (PBusConnection * connection)
{
    return connection->fd != -1;
}

bool PBusConnectionIsDirect (PBusConnection * connection)
{
    return PBusConnectionIsConnected (connection) && connection->brokerObject;
}

int PBusConnectionConnectToSystemBroker (PBusConnection * connection)
{
    int fd;
    struct sockaddr_un sun;

    if ((fd = socket (AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        return -1;
    }

    S16CloseOnExec (fd);

    memset (&sun, 0, sizeof (struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strncpy (sun.sun_path, kPBusSocketPath, sizeof (sun.sun_path));

    if (connect (fd, (struct sockaddr *)&sun, SUN_LEN (&sun)) == -1)
        return -1;
    else
    {
        connection->fd = fd;
        connection->brokerObject =
            PBusDistantObjectNew (connection, "PBus-Broker", "");
        return fd;
    }
}

static S16NVRPCError * ConnectionSendMessageSynchronous (
    PBusConnection * connection, void ** result, const char * toBusname,
    const char * objectPath, const char * selector, nvlist_t * params)
{
    return S16NVRPCClientCall (connection->fd,
                               result,
                               &msgSendSig,
                               "",
                               toBusname,
                               objectPath,
                               selector,
                               params);
}

PBusDistantObject * PBusConnectionGetBrokerObject (PBusConnection * connection)
{
    return connection->brokerObject;
}

PBusDistantObject * PBusDistantObjectNew (PBusConnection * conn,
                                          const char * busName,
                                          const char * objectPath)
{

    PBusDistantObject * obj = malloc (sizeof (*obj));
    obj->connection = conn;
    obj->busName = busName;
    obj->objectPath = objectPath;
    return obj;
}

/*
 * PBusInvocation
 */

PBusInvocation *
PBusInvocationNewWithSignature (S16NVRPCMessageSignature * signature)
{
    PBusInvocation * invoc = malloc (sizeof (*invoc));
    invoc->arguments = NULL;
    invoc->wasSent = false;
    invoc->signature = signature;
    return invoc;
}

void _PBusInvocationSetArgumentsInternal (size_t nParams,
                                          PBusInvocation * invocation, ...)
{
    va_list args;
    nvlist_t * arguments = nvlist_create (0);

    nParams--;
    assert (invocation->signature->nargs == nParams);

    va_start (args, invocation);
    for (size_t i = 0; i < nParams; ++i)
    {
        void * argument = va_arg (args, void *);
        S16NVRPCMessageParameter * param = &invocation->signature->args[i];
        serialise (arguments, param->name, &argument, &param->type);
    }
    va_end (args);

    invocation->arguments = arguments;
}

S16NVRPCError * PBusInvocationSendTo (PBusInvocation * invocation,
                                      PBusDistantObject * object)
{
    invocation->wasSent = true;
    return ConnectionSendMessageSynchronous (object->connection,
                                             &invocation->result,
                                             object->busName,
                                             object->objectPath,
                                             invocation->signature->name,
                                             invocation->arguments);
}