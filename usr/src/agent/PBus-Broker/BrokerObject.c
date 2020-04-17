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

#include "PBus-Broker.h"
#include "PBus_priv.h"

/*
 * Returns an ID by which this subscription rule will be known and can be
 * deleted by, and which is attached to all receiveNotification messages.
 */
static S16NVRPCMessageSignature sigSubscribeTo = {
    .name = "subscribeTo",
    .raw = false,
    .rtype = {.kind = S16R_KINT},
    .nargs = 2,
    .args = {
        {.name = "busNamePattern", .type = {.kind = S16R_KSTRING}},
        {.name = "notificationNamePattern", .type = {.kind = S16R_KSTRING}},
        {.name = NULL}}};

static intptr_t _subscribeTo (PBusObject * self, PBusInvocationContext * ctx,
                              const char * busNamePattern,
                              const char * notificationNamePattern)
{
    return 0;
}

static PBusMethod methSubscribeTo = {.messageSignature = &sigSubscribeTo,
                                     .fnImplementation = (PBusFun)_subscribeTo};

static PBusMethod * methods[] = {&methSubscribeTo};

static PBusClass brokerClass = {.methods = &methods};

PBusObject gBrokerObject = {.isA = &brokerClass, .name = NULL, .data = NULL};