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

#ifdef __cplusplus
extern "C"
{
#endif

#include "S16/List.h"
#include "S16/NVRPC.h"

    typedef struct PBusMethod PBusMethod;
    typedef struct PBusObject PBusObject;
    typedef struct PBusClass PBusClass;
    typedef struct PBusInvocationContext PBusInvocationContext;
    typedef struct PBusServer PBusServer;

    S16ListType (PBusObject, PBusObject *);
    S16ListType (PBusPathElement, char *);

    typedef PBusObject * (*ResolveSubObjectFun) (
        PBusObject * self, void ** user, const char * fullPath,
        const char * selfPath, PBusPathElement_list_t * remainingPath,
        const char * selector);
    typedef nvlist_t * (*DispatchMessageFun) (PBusObject * self,
                                              PBusInvocationContext * ctx,
                                              nvlist_t * params);

    /*
     * A P-Bus handler function. Its arguments have all been automatically
     * deserialised, and its return type will be automatically serialised. But
     * it is the responsibility of the handler to duplicate any resources it
     * retains.
     */
    typedef void * (*PBusFun) (PBusObject * self, PBusInvocationContext * ctx,
                               ...);

    /*
     * A raw P-Bus handler function. It should return an NVList containing a key
     * "result" with the value being the result.
     */
    typedef nvlist_t * (*PBusRawFun) (PBusObject * self,
                                      PBusInvocationContext * ctx,
                                      nvlist_t * params);

    struct PBusMethod
    {
        S16NVRPCMessageSignature * messageSignature;
        PBusFun fnImplementation;
    };

    struct PBusClass
    {
        ResolveSubObjectFun fnResolveSubObject;
        DispatchMessageFun fnDispatchMessage;

        PBusMethod * (*methods)[]; /* terminated .name = NULL */
    };

    struct PBusObject
    {
        PBusClass * isA;
        char * name; /* NULL if root object */
        void * data;

        PBusObject_list_t subObjects;
    };

    struct PBusInvocationContext
    {
        const char * fromBusname;  /* NULL if direct */
        const char * fullSelfPath; /* Full object path of self */
        const char * selfPath;     /* Last component of object path of self */
        void * user;               /* User data set by custom resolveFun */
        const char * selector;     /* Message selector */
    };

    struct PBusServer
    {
        S16NVRPCServer * rpcServer;
        PBusObject * rootObject;
    };

    /*
     * Creates a new PBusServer with @rootObject as its root object.
     */
    PBusObject * PBusServerNew (PBusObject * rootObject);

    /*
     * Creates a new PBusObject of class @isa, name @name, and with user data
     * @data, which is included in every message invocation on that object.
     */
    PBusObject * PBusObjectNew (PBusClass * isa, char * name, void * data);
    /*
     * Destroys a PBusObject. Does not destroy any subobjects it may have.
     */
    void PBusObjectDestroy (PBusObject * obj);
    /*
     * Destroys a PBusObject and all its subobjects recursively.
     * But be aware that programmatic subobjects are not discovered.
     */
    void PBusObjectDeepDestroy (PBusObject * obj);
    /*
     * Adds a PBusObject @obj as a subobject of @parent
     */
    void PBusObjectAddSubObject (PBusObject * obj, PBusObject * subObj);

#ifdef __cplusplus
}
#endif

#endif
