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

#ifndef PBUS_H_
#define PBUS_H_

#include "s16list.h"
#include "s16newrpc.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct PBusObject PBusObject;
    typedef struct PBusInvocationContext PBusInvocationContext;

    S16List (PBusObject, PBusObject *);
    S16List (PBusPathElement, char *);

    typedef PBusObject * (*ResolveSubObjectFun) (
        PBusObject * self, void ** user, const char * selfPath,
        PBusPathElement_list_t * remainingPath, const char * selector);
    typedef nvlist_t * (*DispatchMessageFun) (PBusObject * self,
                                              PBusInvocationContext * ctx,
                                              nvlist_t * params);

    struct PBusObject
    {
        /* If root object, then this is NULL. */
        char * aName;
        void * aData;

        PBusObject_list_t subObjects;

        ResolveSubObjectFun fnResolveSubObject;
        DispatchMessageFun fnDispatchMessage;
    };

    typedef struct PBusServer
    {
        PBusObject * rootObject;
    } PBusServer;

    typedef struct PBusInvocationContext
    {
        const char * selfPath;
        void * user;
        const char * selector;
    } PBusInvocationContext;

    nvlist_t * PBusGetExtraData (nvlist_t ** extraData);

    void testIt ();

#ifdef __cplusplus
}
#endif

#endif
