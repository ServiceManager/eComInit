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

#ifndef PBUSPRIV_H_
#define PBUSPRIV_H_

#include "s16newrpc.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define kPBusSocketPath "/var/run/PBus.sock"

    /*
     * NVList(SendResult) msgRecv(fromBusname: String, objectPath: String,
     *                            selector: String, params: NVList)
     * Or, in C form:
     * nvlist_t * msgRecv(const char * fromBusname, const char * objectPath,
     *                    const char * selector, nvlist_t * params)
     *
     * Note that fromBusname is always set NULL for a direct connection.
     */
    extern s16r_message_signature msgRecvSig;

    /*Returns: struct { error: number, result: variable } SendResult;

    To Bus: SendResult msgSend( endPoint: string, objectPath: list[string],
    params: nvlist)

    To endpoint:
    SendResult msgSend(objectPath: string, params: nvlist) */

#ifdef __cplusplus
}
#endif

#endif
