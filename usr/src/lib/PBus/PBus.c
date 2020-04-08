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

#include "PBus.h"
#include "PBus_priv.h"

s16r_message_signature msgRecvSig = {
    .rtype = {.kind = S16R_KNVLIST},
    .nargs = 2,
    .args = {{.name = "objectPath", .type = {.kind = S16R_KSTRING}},
             {.name = "selector", .type = {.kind = S16R_KSTRING}},
             {.name = "params", .type = {.kind = S16R_KNVLIST}},
             {.name = NULL}}};

static bool matchObject (PBusObject * o, const char * n)
{
    return !strcmp (o->aName, n);
}

static nvlist_t * dispatchMessage (PBusObject * self, void * user,
                                   const char * selfPath, const char * selector,
                                   nvlist_t * params)
{
    PBusInvocationContext ctx = {
        .selfPath = selfPath, .user = user, .selector = selector};

    if (self->fnDispatchMessage)
        return self->fnDispatchMessage (self, &ctx, params);
    else
    {
        return NULL;
    }
}

static nvlist_t * findReceiver (PBusObject * self, void * extraData,
                                const char * selfPath,
                                PBusPathElement_list_t * remainingPath,
                                const char * selector, nvlist_t * params)
{
    if (LL_empty (remainingPath))
        return dispatchMessage (self, extraData, selfPath, selector, params);
    else
    {
        char * next = PBusPathElement_list_lpop (remainingPath);

        if (self->fnResolveSubObject)
        {
            self = self->fnResolveSubObject (
                self, &extraData, next, remainingPath, selector);
            if (!self)
                return /* ERROR */ NULL;
            return findReceiver (
                self, extraData, next, remainingPath, selector, params);
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
                                 next,
                                 remainingPath,
                                 selector,
                                 params);
        }
    }
}

static nvlist_t * findReceiver_root (PBusServer * srv,
                                     PBusPathElement_list_t * path,
                                     const char * selector, nvlist_t * params)
{
    if (!path)
        return dispatchMessage (srv->rootObject, NULL, NULL, selector, params);
    else
        return findReceiver (
            srv->rootObject, NULL, NULL, path, selector, params);
}

void testIt ()
{
    PBusServer * srv = calloc (1, sizeof (*srv));
    PBusPathElement_list_t path = PBusPathElement_list_new ();
    PBusObject *a, *b, *c;

#define makeObj(name)                                                          \
    name = calloc (1, sizeof (*name));                                         \
    name->aName = #name

    makeObj (a);
    makeObj (b);
    makeObj (c);

    PBusPathElement_list_add (&path, "a");
    PBusPathElement_list_add (&path, "b");

    srv->rootObject = calloc (1, sizeof (PBusObject));

#define addObjToObj(b, a) PBusObject_list_add (&a->subObjects, b)
    addObjToObj (a, srv->rootObject);
    addObjToObj (b, a);
    addObjToObj (c, b);

    findReceiver_root (srv, &path, "Hello", NULL);
}