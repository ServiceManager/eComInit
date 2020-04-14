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

#include <stdlib.h>
#include <string.h>

#include "S16/List.h"
#include "S16/NVRPC.h"
#include "dnv.h"
#include "nv.h"

typedef struct
{
    const char * name;
    void * fun;
    s16r_message_signature * sig;
} S16JSONRPCMethod;

S16ListType (s16r_method, S16JSONRPCMethod *);

struct s16r_srv_t
{
    /* custom data */
    void * extra;
    s16r_method_list_t meths;
};

typedef void * (*s16r_fun0_t) (s16r_data_t *);
typedef void * (*s16r_fun1_t) (s16r_data_t *, const void *);
typedef void * (*s16r_fun2_t) (s16r_data_t *, const void *, const void *);
typedef void * (*s16r_fun3_t) (s16r_data_t *, const void *, const void *,
                               const void *);
typedef void * (*s16r_fun4_t) (s16r_data_t *, const void *, const void *,
                               const void *, const void *);

static bool matchMeth (S16JSONRPCMethod * meth, void * str)
{
    return !strcmp (meth->sig->name, str);
}

static S16JSONRPCMethod * findMeth (s16r_srv_t * srv, const char * name)
{
    s16r_method_list_it it =
        s16r_method_list_find (&srv->meths, matchMeth, (void *)name);
    return list_it_val (it);
}

nvlist_t * s16r_make_request (const char * meth_name, nvlist_t * params)
{
    nvlist_t * req = nvlist_create (0);

    nvlist_add_string (req, "nvrpc", "0.9");
    nvlist_add_string (req, "method", meth_name);
    if (params)
        nvlist_add_nvlist (req, "params", params);
    nvlist_add_number (req, "id", rand ());

    return req;
}

void * s16r_dispatch_fun (s16r_data_t * dat, S16JSONRPCMethod * meth,
                          nvlist_t * nvparams)
{
    void ** params;

    if (deserialiseMsgArgs (nvparams, meth->sig, (void **)&params))
    {
        printf ("Error!\n");
        return NULL;
    }

#define Param(i) params[i]
    switch (meth->sig->nargs)
    {
    case 0:
        return ((s16r_fun0_t)meth->fun) (dat);
    case 1:
        return ((s16r_fun1_t)meth->fun) (dat, Param (0));
    case 2:
        return ((s16r_fun2_t)meth->fun) (dat, Param (0), Param (1));
    case 3:
        return ((s16r_fun3_t)meth->fun) (dat, Param (0), Param (1), Param (2));
    case 4:
        return ((s16r_fun4_t)meth->fun) (
            dat, Param (0), Param (1), Param (2), Param (3));
    default:
        printf ("Unsupported parameter count!\n");
        return NULL;
    }
#undef Param
}

static nvlist_t * create_error (s16r_errcode_t code, const char * message,
                                size_t data_len, void * data)
{
    nvlist_t * nverr = nvlist_create (0);
    nvlist_add_number (nverr, "code", code);
    nvlist_add_string (nverr, "message", message);
    if (data_len)
        nvlist_add_binary (nverr, "data", data, data_len);
    return nverr;
}

nvlist_t * s16r_handle_request (s16r_srv_t * srv, nvlist_t * req)
{
    int id;
    const char * methname;
    bool isNote = false;

    S16JSONRPCMethod * meth;
    s16r_data_t * dat;
    nvlist_t * params;
    void * result;

    nvlist_t *response = NULL, *nverr = NULL;

    if (!(id = dnvlist_get_number (req, "id", 0)))
    {
        printf ("Missing ID field: notification assumed.\n");
        isNote = true;
    }
    if (!nvlist_exists_string (req, "nvrpc"))
    {
        printf ("Invalid request: Missing nvrpc field.\n");
        return NULL;
    }
    if (!(methname = dnvlist_take_string (req, "method", NULL)))
    {
        printf ("Invalid request: Missing method name.\n");
        return NULL;
    }

    params = dnvlist_take_nvlist (req, "params", NULL);

    if (!(meth = findMeth (srv, methname)))
    {
        printf ("Invalid request: Didn't find method.\n");
        return NULL;
    }

    dat = calloc (1, sizeof (*dat));
    dat->extra = srv->extra;
    dat->method = methname;
    result = s16r_dispatch_fun (NULL, meth, params);

    if (!isNote)
    {
        response = nvlist_create (0);
        nvlist_add_string (response, "nvrpc", "0.9");
        if (result)
            serialise (response, "result", &result, &meth->sig->rtype);
        else
            nvlist_add_nvlist (response, "error", nverr);
        nvlist_add_number (response, "id", id);
    }

    return response;
}

void s16r_srv_register_method (s16r_srv_t * srv, s16r_message_signature * sig,
                               s16r_fun_t fun)
{
    S16JSONRPCMethod * meth = malloc (sizeof (*meth));
    meth->fun = fun;
    meth->sig = sig;
    s16r_method_list_add (&srv->meths, meth);
}

s16r_srv_t * s16r_srv_new (void * extra)
{
    s16r_srv_t * srv = malloc (sizeof (*srv));

    srv->extra = extra;
    srv->meths = s16r_method_list_new ();

    return srv;
}

void * testFun (void * dat, char * parA, char * parB)
{
    printf ("%s %s\n", parA, parB);
    return "No Way";
}

typedef struct
{
    char * a;
    intptr_t b;
} argStruct;

s16r_message_signature testMethSig = {
    .name = "TestMeth",
    .rtype = {.kind = S16R_KSTRING},
    .nargs = 2,
    .args = {{.name = "argA", .type = {.kind = S16R_KSTRING}},
             {.name = "argB", .type = {.kind = S16R_KSTRING}},
             {.name = NULL}}};

void testIt ()
{
    s16r_srv_t * srv = s16r_srv_new (NULL);
    nvlist_t *req = nvlist_create (0), *params = nvlist_create (0), *res = NULL;

    s16r_srv_register_method (srv, &testMethSig, testFun);

    nvlist_add_string (params, "argA", "Hello");
    nvlist_add_string (params, "argB", "World");
    nvlist_move_nvlist (req, "params", params);

    req = s16r_make_request ("TestMeth", params);

    res = s16r_handle_request (srv, req);

    printf ("%s\n", ucl_object_emit (nvlist_to_ucl (res), UCL_EMIT_JSON));
}