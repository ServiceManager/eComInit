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

/*Returns: struct { error: number, result: variable } SendResult; */

#include "PBus/PBus.h"

typedef void * (*PB0ParamFun) (PBusObject *, PBusInvocationContext *);
typedef void * (*PB1ParamFun) (PBusObject *, PBusInvocationContext *,
                               const void *);
typedef void * (*PB2ParamFun) (PBusObject *, PBusInvocationContext *,
                               const void *, const void *);
typedef void * (*PB3ParamFun) (PBusObject *, PBusInvocationContext *,
                               const void *, const void *, const void *);
typedef void * (*PB4ParamFun) (PBusObject *, PBusInvocationContext *,
                               const void *, const void *, const void *,
                               const void *);

S16NVRPCMessageSignature msgSendSig = {
    .name = "msgSend",
    .rtype = {.kind = S16R_KNVLIST},
    .nargs = 5,
    .args = {{.name = "fromBusname", .type = {.kind = S16R_KSTRING}},
             {.name = "toBusname", .type = {.kind = S16R_KSTRING}},
             {.name = "objectPath", .type = {.kind = S16R_KSTRING}},
             {.name = "selector", .type = {.kind = S16R_KSTRING}},
             {.name = "params", .type = {.kind = S16R_KNVLIST}},
             {.name = NULL}}};

static bool matchObject (PBusObject * o, const char * n)
{
    return !strcmp (o->name, n);
}

static void * dispatchFun (PBusObject * self, PBusInvocationContext * ctx,
                           PBusMethod * meth, nvlist_t * nvparams)
{
    void * res;
    void ** params;

    if (S16NVRPCMessageSignatureDeserialiseArguments (
            nvparams, meth->messageSignature, (void **)&params))
    {
        printf ("Error deserialising message args!\n");
        return NULL;
    }

#define Param(i) params[i]
    switch (meth->messageSignature->nargs)
    {
    case 0:
        res = ((PB0ParamFun)meth->fnImplementation) (self, ctx);
        break;

    case 1:
        res = ((PB1ParamFun)meth->fnImplementation) (self, ctx, Param (0));
        break;

    case 2:
        res = ((PB2ParamFun)meth->fnImplementation) (
            self, ctx, Param (0), Param (1));
        break;

    case 3:
        res = ((PB3ParamFun)meth->fnImplementation) (
            self, ctx, Param (0), Param (1), Param (2));
        break;

    case 4:
        res = ((PB4ParamFun)meth->fnImplementation) (
            self, ctx, Param (0), Param (1), Param (2), Param (3));
        break;

    default:
        printf ("Unsupported parameter count!\n");
        return NULL;
    }
#undef Param

    S16NVRPCMessageSignatureDestroyArguments (params, meth->messageSignature);
    return res;
}

static nvlist_t * sendMessage (PBusObject * self, PBusInvocationContext * ctx,
                               PBusMethod * meth, nvlist_t * params)
{
    nvlist_t * response = nvlist_create (0);

    /* check if notes in future */
    if (!meth->messageSignature->raw)
    {
        void * result = dispatchFun (self, ctx, meth, params);
        serialise (response, "result", &result, &meth->messageSignature->rtype);
    }

    return response;
}

static nvlist_t * dispatchMessage (PBusObject * self, void * user,
                                   const char * fullPath, const char * selfPath,
                                   const char * fromBusname,
                                   const char * selector, nvlist_t * params)
{
    PBusInvocationContext ctx = {.fullSelfPath = fullPath,
                                 .selfPath = selfPath,
                                 .user = user,
                                 .selector = selector,
                                 .fromBusname = fromBusname};

    if (self->isA->fnDispatchMessage)
        return self->isA->fnDispatchMessage (self, &ctx, params);
    else
    {
        PBusMethod * meth;
        for (int i = 0; (meth = (*self->isA->methods)[i]); i++)
        {
            printf ("Candidate: %s. Tofind: %s\n",
                    meth->messageSignature->name,
                    selector);
            if (!strcmp (selector, meth->messageSignature->name))
                return sendMessage (self, &ctx, meth, params);
        }

        printf ("Failed to find handler for %s in object %s!\n",
                selector,
                fullPath);
        return NULL;
    }
}

static nvlist_t * findReceiver (PBusObject * self, void * extraData,
                                const char * fullPath, const char * selfPath,
                                PBusPathElement_list_t * remainingPath,
                                const char * fromBusname, const char * selector,
                                nvlist_t * params)
{
    if (LL_empty (remainingPath))
        return dispatchMessage (
            self, extraData, fullPath, selfPath, fromBusname, selector, params);
    else
    {
        char * next = PBusPathElement_list_lpop (remainingPath);

        if (self->isA->fnResolveSubObject)
        {
            self = self->isA->fnResolveSubObject (
                self, &extraData, fullPath, next, remainingPath, selector);
            if (!self)
                return /* kS16LogErrorOR */ NULL;
            return findReceiver (self,
                                 extraData,
                                 fullPath,
                                 next,
                                 remainingPath,
                                 fromBusname,
                                 selector,
                                 params);
        }
        else
        {
            PBusObject_list_it it =
                PBusObject_list_find (&self->subObjects,
                                      (PBusObject_list_find_fn)matchObject,
                                      (void *)next);
            if (!it)
            {
                printf ("%s: Failed to find subobject %s\n", selfPath, next);
                return NULL;
            }
            return findReceiver (list_it_val (it),
                                 extraData,
                                 fullPath,
                                 next,
                                 remainingPath,
                                 fromBusname,
                                 selector,
                                 params);
        }
    }
}

nvlist_t * findReceiver_root (PBusConnection * srv, const char * path,
                              const char * fromBusname, const char * selector,
                              nvlist_t * params)
{
    PBusPathElement_list_t pathEls = PBusPathElement_list_new ();
    char *pathCopy = NULL, *pathToSplit = NULL, *seg;
    char * saveptr = NULL;
    nvlist_t * result;

    if (path)
    {
        pathCopy = strdup (path);
        pathToSplit = strdup (path);

        seg = strtok_r (pathToSplit, "/", &saveptr);

        while (seg)
        {
            PBusPathElement_list_add (&pathEls, seg);
            seg = strtok_r (NULL, "/", &saveptr);
        }
    }

    if (!path)
        result = dispatchMessage (
            srv->rootObject, NULL, NULL, NULL, fromBusname, selector, params);
    else
        result = findReceiver (srv->rootObject,
                               NULL,
                               path,
                               NULL,
                               &pathEls,
                               fromBusname,
                               selector,
                               params);

    if (path)
    {
        free (pathCopy);
        free (pathToSplit);
        PBusPathElement_list_destroy (&pathEls);
    }

    return result;
}
