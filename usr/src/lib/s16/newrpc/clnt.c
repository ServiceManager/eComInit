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
    S16NVRPCMessageSignature * sig;
} S16NVRPCMethod;

S16ListType (s16r_method, S16NVRPCMethod *);

struct S16NVRPCServer
{
    /* custom data */
    void * extra;
    s16r_method_list_t meths;
};

typedef void * (*s16r_fun0_t) (S16NVRPCCallContext *);
typedef void * (*s16r_fun1_t) (S16NVRPCCallContext *, const void *);
typedef void * (*s16r_fun2_t) (S16NVRPCCallContext *, const void *,
                               const void *);
typedef void * (*s16r_fun3_t) (S16NVRPCCallContext *, const void *,
                               const void *, const void *);
typedef void * (*s16r_fun4_t) (S16NVRPCCallContext *, const void *,
                               const void *, const void *, const void *);
typedef void * (*s16r_fun5_t) (S16NVRPCCallContext *, const void *,
                               const void *, const void *, const void *,
                               const void *);

static bool matchMeth (S16NVRPCMethod * meth, void * str)
{
    return !strcmp (meth->sig->name, str);
}

static S16NVRPCMethod * findMeth (S16NVRPCServer * srv, const char * name)
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
        nvlist_move_nvlist (req, "params", params);
    nvlist_add_number (req, "id", rand ());

    return req;
}

void * DispatchFunctionWithArgumentsConverted (S16NVRPCCallContext * dat,
                                               S16NVRPCImplementationFn fun,
                                               S16NVRPCMessageSignature * sig,
                                               nvlist_t * nvparams)
{
    void ** params;
    void * result;

    if (S16NVRPCMessageSignatureDeserialiseArguments (
            nvparams, sig, (void **)&params))
    {
        printf ("Error!\n");
        return NULL;
    }

#define Param(i) params[i]
    switch (sig->nargs)
    {
    case 0:
        result = ((s16r_fun0_t)fun) (dat);
        break;
    case 1:
        result = ((s16r_fun1_t)fun) (dat, Param (0));
        break;
    case 2:
        result = ((s16r_fun2_t)fun) (dat, Param (0), Param (1));
        break;
    case 3:
        result = ((s16r_fun3_t)fun) (dat, Param (0), Param (1), Param (2));
        break;
    case 4:
        result = ((s16r_fun4_t)fun) (
            dat, Param (0), Param (1), Param (2), Param (3));
        break;
    case 5:
        result = ((s16r_fun5_t)fun) (
            dat, Param (0), Param (1), Param (2), Param (3), Param (4));
        break;
    default:
        printf ("Unsupported parameter count!\n");
        return NULL;
    }
#undef Param

    S16NVRPCMessageSignatureDestroyArguments (params, sig);

    return result;
}

static nvlist_t * CreateNVError (S16NVRPCErrorCode code, const char * message,
                                 size_t data_len, void * data)
{
    nvlist_t * nverr = nvlist_create (0);

    nvlist_add_number (nverr, "code", code);
    assert (!nvlist_error (nverr));
    nvlist_add_string (nverr, "message", message);
    assert (!nvlist_error (nverr));
    if (data_len)
    {
        nvlist_add_binary (nverr, "data", data, data_len);
        assert (!nvlist_error (nverr));
    }

    return nverr;
}

static nvlist_t * CreateNVResponse (S16NVRPCType * rtype, void * result,
                                    nvlist_t * error, int id)
{
    nvlist_t * response = nvlist_create (0);

    nvlist_add_string (response, "nvrpc", "0.9");
    assert (!nvlist_error (response));
    if (result)
    {
        serialise (response, "result", &result, rtype);
        assert (!nvlist_error (response));
    }
    else
    {
        nvlist_move_nvlist (response, "error", error);
        assert (!nvlist_error (response));
    }
    nvlist_add_number (response, "id", id);
    assert (!nvlist_error (response));

    return response;
}

static nvlist_t * s16r_handle_request (S16NVRPCServer * srv, nvlist_t * req)
{
    int id;
    const char * methname;
    bool isNote = false;

    S16NVRPCMethod * meth;
    S16NVRPCCallContext dat;
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
        nverr = CreateNVError (
            kS16NVRPCErrorInvalidRequest, "Missing nvrpc field", 0, NULL);
        goto error;
    }
    if (!(methname = dnvlist_get_string (req, "method", NULL)))
    {
        printf ("Invalid request: Missing method name.\n");
        nverr = CreateNVError (
            kS16NVRPCErrorInvalidRequest, "Missing method name", 0, NULL);
        goto error;
    }

    params = (nvlist_t *)dnvlist_get_nvlist (req, "params", NULL);

    if (!(meth = findMeth (srv, methname)))
    {
        printf ("Invalid request: Didn't find method.\n");
        nverr = CreateNVError (
            kS16NVRPCErrorInvalidRequest, "Method not found", 0, NULL);
        goto error;
    }

    dat.err.code = 0;
    dat.err.data = NULL;
    dat.err.data_len = 0;
    dat.err.message = NULL;
    dat.extra = srv->extra;
    dat.method = methname;
    result = DispatchFunctionWithArgumentsConverted (
        &dat, meth->fun, meth->sig, params);

    if (!isNote)
    {
        response =
            CreateNVResponse (&meth->sig->rtype,
                              result,
                              dat.err.code ? CreateNVError (dat.err.code,
                                                            dat.err.message,
                                                            dat.err.data_len,
                                                            dat.err.data)
                                           : NULL,
                              id);
    }

    goto done;

error:
    if (isNote)
    {
        if (nverr)
            nvlist_destroy (nverr);
    }
    else
        response = CreateNVResponse (NULL, NULL, nverr, id);

done:
    return response;
}

void S16NVRPCErrorDestroy (S16NVRPCError * err)
{
    free (err->message);
    free (err->data);
    free (err);
}

S16NVRPCServer * S16NVRPCServerNew (void * extra)
{
    S16NVRPCServer * srv = malloc (sizeof (*srv));

    srv->extra = extra;
    srv->meths = s16r_method_list_new ();

    return srv;
}

void S16NVRPCServerRegisterMethod (S16NVRPCServer * srv,
                                   S16NVRPCMessageSignature * sig,
                                   S16NVRPCImplementationFn fun)
{
    S16NVRPCMethod * meth = malloc (sizeof (*meth));
    meth->fun = fun;
    meth->sig = sig;
    s16r_method_list_add (&srv->meths, meth);
}

void S16NVRPCServerReceiveFromFileDescriptor (S16NVRPCServer * server, int fd)
{
    nvlist_t * request = nvlist_recv (fd, 0);
    nvlist_t * response;

    response = s16r_handle_request (server, request);
    nvlist_destroy (request);
    assert (nvlist_send (fd, response) != -1);
    nvlist_destroy (response);
}

/* Returns ID. */
static int clientCallInternal (int fd, const char * methodName,
                               nvlist_t * params)
{
    int id = rand ();
    nvlist_t * message = nvlist_create (0);

    nvlist_add_string (message, "nvrpc", "0.9");
    assert (!nvlist_error (message));
    nvlist_add_string (message, "method", methodName);
    assert (!nvlist_error (message));
    nvlist_move_nvlist (message, "params", params);
    assert (!nvlist_error (message));
    nvlist_add_number (message, "id", id);
    assert (!nvlist_error (message));

    assert (!nvlist_send (fd, message));
    nvlist_destroy (message);

    return id;
}

S16NVRPCError * S16NVRPCClientCallRaw (int fd, nvlist_t ** result,
                                       const char * methodName,
                                       nvlist_t * params);

static S16NVRPCError * ProcessReply (nvlist_t * reply)
{
    assert (nvlist_exists_string (reply, "nvrpc"));
    assert (nvlist_exists_number (reply, "id"));
    if (!nvlist_exists (reply, "result"))
    {
        S16NVRPCError * err;
        nvlist_t * nverr =
            (nvlist_t *)dnvlist_get_nvlist (reply, "error", NULL);

        assert (nverr);
        assert (nvlist_exists_number (nverr, "code"));

        err = malloc (sizeof (*err));
        err->code = nvlist_take_number (nverr, "code");
        err->message = nvlist_take_string (nverr, "message");
        if (nvlist_exists_number (nverr, "data_len"))
        {
            size_t data_len;
            assert (nvlist_exists_binary (nverr, "data"));
            err->data_len = nvlist_take_number (nverr, "data_len");
            err->data = nvlist_take_binary (nverr, "data", &data_len);
            assert (err->data_len == data_len);
        }
        else
        {
            err->data_len = 0;
            err->data = NULL;
        }

        return err;
    }
    return NULL;
}

S16NVRPCError * S16NVRPCClientCallInternal (int fd, void ** result,
                                            size_t nparams,
                                            S16NVRPCMessageSignature * sig, ...)
{
    va_list args;
    nvlist_t * params = nvlist_create (0);
    nvlist_t * reply;
    S16NVRPCError * err = NULL;
    int id;

    nparams--;

    va_start (args, sig);
    for (size_t i = 0; i < nparams; ++i)
    {
        void * arg = va_arg (args, void *);
        S16NVRPCMessageParameter * param = &sig->args[i];
        serialise (params, param->name, &arg, &param->type);
        assert (!nvlist_error (params));
    }
    va_end (args);

    id = clientCallInternal (fd, sig->name, params);

    reply = nvlist_recv (fd, 0);
    err = ProcessReply (reply);

    if (!err)
        S16NVRPCMemberDeserialise (reply, "result", &sig->rtype, result);

    nvlist_destroy (reply);

    return err;
}

S16NVRPCAsyncCall *
S16NVRPCClientCallAsyncInternal (S16NVRPCAsyncContext * asyncContext, int fd,
                                 size_t nparams, S16NVRPCMessageSignature * sig,
                                 ...)
{
    va_list args;
    nvlist_t * params = nvlist_create (0);
    S16NVRPCAsyncCall * asyncCall = calloc (1, sizeof (&asyncCall));

    nparams--;

    va_start (args, sig);
    for (size_t i = 0; i < nparams; ++i)
    {
        void * arg = va_arg (args, void *);
        S16NVRPCMessageParameter * param = &sig->args[i];
        serialise (params, param->name, &arg, &param->type);
    }
    va_end (args);

    printf (">%s<\n",
            ucl_object_emit (S16NVRPCNVListToUCL (params), UCL_EMIT_YAML));

    asyncCall->id = clientCallInternal (fd, sig->name, params);

    return asyncCall;
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

S16NVRPCMessageSignature testMethSig = {
    .name = "TestMeth",
    .rtype = {.kind = S16R_KSTRING},
    .nargs = 2,
    .args = {{.name = "argA", .type = {.kind = S16R_KSTRING}},
             {.name = "argB", .type = {.kind = S16R_KSTRING}},
             {.name = NULL}}};

void testIt ()
{
    S16NVRPCServer * srv = S16NVRPCServerNew (NULL);
    nvlist_t *req = nvlist_create (0), *params = nvlist_create (0), *res = NULL;

    S16NVRPCServerRegisterMethod (
        srv, &testMethSig, (S16NVRPCImplementationFn)testFun);

    nvlist_add_string (params, "argA", "Hello");
    nvlist_add_string (params, "argB", "World");
    nvlist_move_nvlist (req, "params", params);

    req = s16r_make_request ("TestMeth", params);

    res = s16r_handle_request (srv, req);

    printf ("%s\n", ucl_object_emit (S16NVRPCNVListToUCL (res), UCL_EMIT_JSON));
}