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
#include <err.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <unistd.h>

#include "S16/List.h"
#include "S16/NVRPC.h"
#include "nv.h"
#include "ucl.h"

S16ListType (void, void *);

const char * S16NVRPCTypeKind_str[S16R_KMAX] = {
    "String", "Boolean", "Int", "Struct", "List", "Right"};

void serialise (nvlist_t * nvl, const char * name, void ** src,
                S16NVRPCType * field);

nvlist_t * S16NVRPCStructSerialise (void * src, S16NVRPCStruct * desc)
{
    nvlist_t * nvl = nvlist_create (0);
    S16NVRPCField * field;

    for (int i = 0; (field = &desc->fields[i]) && field->name; i++)
    {
        void * member = ((char *)src + field->off);
        serialise (nvl, field->name, member, &field->type);
    }

    return nvl;
}

nvlist_t * serialiseList (void * src, S16NVRPCType * type)
{
    nvlist_t * nvl = nvlist_create (0);
    char namebuf[8];
    int i = 0;

    LL_each ((void_list_t *)src, el)
    {
        snprintf (namebuf, 8, "%i", i++);
        serialise (nvl, namebuf, &el->val, type);
    }

    return nvl;
}

/*
 * Serialises the data at @src, of type described by @type, into the nvlist
 * @nvl, under the name @name.
 */
void serialise (nvlist_t * nvl, const char * name, void ** src,
                S16NVRPCType * type)
{
    switch (type->kind)
    {
    case S16R_KSTRING:
        nvlist_add_string (nvl, name, *(char **)src);
        break;

    case S16R_KBOOL:
        nvlist_add_bool (nvl, name, *(boolptr_t *)src);
        break;

    case S16R_KINT:
        nvlist_add_number (nvl, name, *(intptr_t *)src);
        break;

    case S16R_KNVLIST:
        nvlist_add_nvlist (nvl, name, *(nvlist_t **)src);
        break;

    case S16R_KSTRUCT:
        nvlist_move_nvlist (
            nvl, name, S16NVRPCStructSerialise (*src, type->sdesc));
        break;

    case S16R_KLIST:
        nvlist_move_nvlist (nvl, name, serialiseList (src, type->ltype));
        break;

    case S16R_KDESCRIPTOR:
        nvlist_add_descriptor (nvl, name, *(fdptr_t *)src);
        break;

    default:
        assert (!"Should not be reached.");
    }
}

/*
 * Deserialisation functions. They return zero if they succed.
 */

/* FIXME: Memory can leak if deserialisation fails halfway through! Leak of
FDs is also possible. We should consider implementing in LibS16 some kind of
resource pools feature, where we can release the lot if something goes
wrong. */

int deserialiseList (nvlist_t * nvl, S16NVRPCType * type, void ** dest);
int S16NVRPCStructDeserialise (nvlist_t * nvl, S16NVRPCStruct * desc,
                               void ** dest);

int deserialiseMember (nvlist_t * nvl, const char * name, S16NVRPCType * type,
                       void ** dest)
{
    int err = -1;

    switch (type->kind)
    {
    case S16R_KSTRING:
        if (!nvlist_exists_string (nvl, name))
            goto err;
        *(char **)dest = nvlist_take_string (nvl, name);
        break;

    case S16R_KBOOL:
        if (!nvlist_exists_bool (nvl, name))
            goto err;
        *(boolptr_t *)dest = nvlist_get_bool (nvl, name);
        break;

    case S16R_KINT:
        if (!nvlist_exists_number (nvl, name))
            goto err;
        *(intptr_t *)dest = nvlist_get_number (nvl, name);
        break;

    case S16R_KNVLIST:
        if (!nvlist_exists_nvlist (nvl, name))
            goto err;
        *(nvlist_t **)dest = nvlist_take_nvlist (nvl, name);

    case S16R_KSTRUCT:
        /* 'Take' cannot be used here. */
        err = S16NVRPCStructDeserialise (
            (nvlist_t *)nvlist_get_nvlist (nvl, name), type->sdesc, dest);
        if (err)
            goto err;
        break;

    case S16R_KLIST:
        err = deserialiseList (
            (nvlist_t *)nvlist_get_nvlist (nvl, name), type->ltype, dest);
        if (err)
            goto err;
        break;

    case S16R_KDESCRIPTOR:
        if (!nvlist_exists_descriptor (nvl, name))
            goto err;
        *(fdptr_t *)dest = nvlist_take_descriptor (nvl, name);
        break;

    default:
        assert (!"Should not be reached.");
    }

    err = 0;

err:
    return err;
}

int S16NVRPCStructDeserialise (nvlist_t * nvl, S16NVRPCStruct * desc,
                               void ** dest)
{
    S16NVRPCField * field;
    void * struc = malloc (desc->len);

    for (int i = 0; (field = &desc->fields[i]) && field->name; i++)
    {
        int err;
        void * member = ((char *)struc + field->off);

        if (!nvlist_exists (nvl, field->name))
            return -1;
        if ((err = deserialiseMember (nvl, field->name, &field->type, member)))
            return err;
    }

    *dest = struc;
    return 0;
}

int deserialiseList (nvlist_t * nvl, S16NVRPCType * type, void ** dest)
{
    const char * name;
    void * cookie = NULL;
    int nvtype;
    void_list_t * list = (void_list_t *)dest;

    *list = void_list_new ();

    while ((name = nvlist_next (nvl, &nvtype, &cookie)))
    {
        void * dat;
        int err = deserialiseMember (nvl, name, type, &dat);
        if (err)
            return err;
        void_list_add (list, dat);
    }

    return 0;
}

int S16NVRPCMessageSignatureDeserialiseArguments (
    nvlist_t * nvl, S16NVRPCMessageSignature * desc, void ** dest)
{
    S16NVRPCMessageParameter * field;
    void ** struc = malloc (sizeof (void *) * desc->nargs);

    for (int i = 0; (field = &desc->args[i]) && field->name; i++)
    {
        int err = deserialiseMember (nvl, field->name, &field->type, &struc[i]);
        if (err)
            return err;
    }

    *dest = struc;
    return 0;
}

void destroy (void ** src, S16NVRPCType * field);

void destroyStruct (void * src, S16NVRPCStruct * desc)
{
    S16NVRPCField * field;

    for (int i = 0; (field = &desc->fields[i]) && field->name; i++)
    {
        void * member = ((char *)src + field->off);
        destroy (member, &field->type);
    }
}

void destroyList (void * src, S16NVRPCType * type)
{
    LL_each ((void_list_t *)src, el) { destroy (&el->val, type); }
    void_list_destroy ((void_list_t *)src);
}

void S16NVRPCMessageSignatureDestroyArguments (void ** src,
                                               S16NVRPCMessageSignature * desc)
{
    S16NVRPCMessageParameter * field;

    for (int i = 0; (field = &desc->args[i]) && field->name; i++)
    {
        destroy (&src[i], &field->type);
    }

    free (src);
}

void destroy (void ** src, S16NVRPCType * type)
{
    switch (type->kind)
    {
    case S16R_KSTRING:
        free (*(char **)src);
        break;
    case S16R_KNVLIST:
        nvlist_destroy (*(nvlist_t **)src);
        break;
    case S16R_KSTRUCT:
        destroyStruct (*src, type->sdesc);
        break;
    case S16R_KLIST:
        destroyList (src, type->ltype);
        break;
    case S16R_KDESCRIPTOR:
        close (*(fdptr_t *)src);
        break;

    case S16R_KINT:
    case S16R_KBOOL:
        break;

    default:
        assert (!"Should not be reached.");
    }
}

ucl_object_t * S16NVRPCNVListToUCL (const nvlist_t * nvl)
{
    ucl_object_t * obj = ucl_object_typed_new (UCL_OBJECT);
    const char * name;
    void * cookie = NULL;
    int type;

    while ((name = nvlist_next (nvl, &type, &cookie)) != NULL)
    {
        switch (type)
        {
        case NV_TYPE_STRING:
            ucl_object_insert_key (
                obj,
                ucl_object_fromstring (nvlist_get_string (nvl, name)),
                name,
                0,
                false);
            break;

        case NV_TYPE_BOOL:
            ucl_object_insert_key (
                obj,
                ucl_object_frombool (nvlist_get_bool (nvl, name)),
                name,
                0,
                false);
            break;

        case NV_TYPE_NUMBER:
            ucl_object_insert_key (
                obj,
                ucl_object_fromint (nvlist_get_number (nvl, name)),
                name,
                0,
                false);
            break;

        case NV_TYPE_NVLIST:
            ucl_object_insert_key (
                obj,
                S16NVRPCNVListToUCL (nvlist_get_nvlist (nvl, name)),
                name,
                0,
                false);
            break;

        case NV_TYPE_DESCRIPTOR:
            ucl_object_insert_key (
                obj,
                ucl_object_fromint (nvlist_get_descriptor (nvl, name)),
                name,
                0,
                false);
            break;

        default:
            assert (!"Unsupported NVList entry type.");
            break;
        }
    }

    return obj;
}
