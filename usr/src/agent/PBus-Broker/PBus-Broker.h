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

#ifndef PBUSBROKER_H_
#define PBUSBROKER_H_

#include <sys/types.h>

#include "S16/List.h"
#include "S16/NVRPC.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        uid_t aUID;
        gid_t aGID;
    } PBusCredentials;

    typedef struct
    {

        int aFD; /* FD on which this client is connected */
        int aID; /* Unique (per system session) ID of client */
        PBusCredentials aCredentials; /* Credentials of client */
    } PBusClient;

    S16ListType (PBusClient, PBusClient *);

    typedef struct
    {
        int aListenSocket;
        int aKQ;
        S16NVRPCServer * aRPCServer;

        PBusClient_list_t aClients;
    } PBusBroker;

    extern PBusBroker gBroker;

#ifdef __cplusplus
}
#endif

#endif
