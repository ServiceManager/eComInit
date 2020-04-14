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
#include <stdlib.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <unistd.h>

#include "S16/JSONRPCClient.h"
#include "S16/JSONRPCServer.h"

#define S16_JSONRPC_VERSION "2.0"

#define MAX_LEN_MSG 16384

typedef ucl_object_t * (*s16rpc_fun0_t) (s16rpc_data_t *);
typedef ucl_object_t * (*s16rpc_fun1_t) (s16rpc_data_t *, const ucl_object_t *);
typedef ucl_object_t * (*s16rpc_fun2_t) (s16rpc_data_t *, const ucl_object_t *,
                                         const ucl_object_t *);
typedef ucl_object_t * (*s16rpc_fun3_t) (s16rpc_data_t *, const ucl_object_t *,
                                         const ucl_object_t *,
                                         const ucl_object_t *);
typedef ucl_object_t * (*s16rpc_fun4_t) (s16rpc_data_t *, const ucl_object_t *,
                                         const ucl_object_t *,
                                         const ucl_object_t *,
                                         const ucl_object_t *);

typedef struct
{
    const char * name;
    /* Parameter count */
    size_t nparams;
    void * fun;
} s16rpc_method_t;

S16List (s16rpc_method, s16rpc_method_t *);

typedef struct
{
    int fd;
    /* Every message from a client begins with 4 bytes representing the
     * length of the message. When we receive those 4 bytes, we allocate a
     * buffer to hold the rest of the message. */
    int32_t cur_msg_len;
    /* Offset into the current message being received. */
    int cur_msg_off;
    /* Message buffer. */
    char * cur_msg_buf;
} s16rpc_conn_t;

S16List (s16rpc_conn, s16rpc_conn_t *);

struct s16rpc_srv_s
{
    /* if this is true, don't try to accept() on fd */
    bool is_client;
    int kq;
    /* fd on which this server listens for connections, if not is_client */
    int fd;
    /* custom data */
    void * extra;
    s16rpc_method_list_t meths;
    s16rpc_conn_list_t conns;
};

static s16rpc_conn_t * conn_new (s16rpc_srv_t * srv, int fd)
{
    s16rpc_conn_t * res = calloc (1, sizeof (s16rpc_conn_t));
    res->fd = fd;
    s16rpc_conn_list_add (&srv->conns, res);
    return res;
}

static int conn_close (s16rpc_srv_t * srv, s16rpc_conn_t * con)
{
    int clos = close (con->fd);
    if (con->cur_msg_buf)
        free (con->cur_msg_buf);
    s16rpc_conn_list_del (&srv->conns, con);
    free (con);
    return clos;
}

bool match_fd (s16rpc_conn_t * a, int fd)
{
    if (a->fd == fd)
        return true;
    return false;
}

bool match_method (s16rpc_method_t * meth, const char * txt)
{
    return !strcmp (meth->name, txt);
}

int write_object (int fd, const ucl_object_t * obj)
{
    char * s = (char *)ucl_object_emit (obj, UCL_EMIT_JSON_COMPACT);
    int32_t len = strlen (s) + 1;

    write (fd, (char *)&len, sizeof (int32_t));
    write (fd, s, len);
    free (s);
    return 1;
}

void add_obj_el (ucl_object_t * msg, const char * name, ucl_object_t * value)
{
    ucl_object_insert_key (msg, value, name, 0, 0);
}

ucl_object_t * make_error (int code, const char * message, ucl_object_t * data)
{
    ucl_object_t * uerr = ucl_object_typed_new (UCL_OBJECT);
    ucl_object_insert_key (uerr, ucl_object_fromint (code), "code", 0, 0);
    ucl_object_insert_key (
        uerr, ucl_object_fromstring (message), "message", 0, 0);
    ucl_object_insert_key (
        uerr, data ? data : ucl_object_typed_new (UCL_NULL), "data", 0, 0);
    return uerr;
}

void reply_error (s16rpc_conn_t * conn, const ucl_object_t * id, int code,
                  const char * message, ucl_object_t * data)
{
    ucl_object_t * msg = ucl_object_typed_new (UCL_OBJECT);
    ucl_object_insert_key (
        msg, ucl_object_fromstring (S16_JSONRPC_VERSION), "jsonrpc", 0, 0);
    ucl_object_insert_key (
        msg, make_error (code, message, data), "error", 0, 0);
    /* must copy id as it will be torn down in unref of original msg */
    ucl_object_insert_key (msg,
                           id ? ucl_object_copy (id)
                              : ucl_object_typed_new (UCL_NULL),
                           "id",
                           0,
                           0);
    write_object (conn->fd, msg);
    ucl_object_unref (msg);
}

void reply_result (s16rpc_conn_t * conn, const ucl_object_t * id,
                   ucl_object_t * result)
{
    ucl_object_t * msg = ucl_object_typed_new (UCL_OBJECT);
    ucl_object_insert_key (
        msg, ucl_object_fromstring (S16_JSONRPC_VERSION), "jsonrpc", 0, 0);
    ucl_object_insert_key (msg, result, "result", 0, 0);
    /* must copy id as it will be torn down in unref of original msg */
    ucl_object_insert_key (msg,
                           id ? ucl_object_copy (id)
                              : ucl_object_typed_new (UCL_NULL),
                           "id",
                           0,
                           0);
    write_object (conn->fd, msg);
    ucl_object_unref (msg);
}

ucl_object_t * dispatch_method (s16rpc_data_t * dat, void * fun, size_t nparams,
                                const ucl_object_t * params)
{
#define Param(i) ucl_array_find_index (params, i)
    switch (nparams)
    {
    case 0:
        return ((s16rpc_fun0_t)fun) (dat);
    case 1:
        return ((s16rpc_fun1_t)fun) (dat, Param (0));
    case 2:
        return ((s16rpc_fun2_t)fun) (dat, Param (0), Param (1));
    case 3:
        return ((s16rpc_fun3_t)fun) (dat, Param (0), Param (1), Param (2));
    case 4:
        return ((s16rpc_fun4_t)fun) (
            dat, Param (0), Param (1), Param (2), Param (3));
    }
#undef Param
    assert ("This should not be reached");
    return NULL;
}

void handle_msg (s16rpc_srv_t * srv, s16rpc_conn_t * conn)
{
    struct ucl_parser * parser = ucl_parser_new (0);
    ucl_object_t * obj = NULL;

    ucl_parser_add_string (parser, conn->cur_msg_buf, conn->cur_msg_len);

    if (ucl_parser_get_error (parser))
    {
        printf ("RPC error: Malformed message: %s\n",
                ucl_parser_get_error (parser));
        reply_error (conn, NULL, 1, "Error parsing JSON", NULL);
        goto cleanup;
    }

    obj = ucl_parser_get_object (parser);

    if (ucl_object_type (obj) == UCL_OBJECT)
    {
        const ucl_object_t *method = ucl_object_lookup (obj, "method"),
                           *params = ucl_object_lookup (obj, "params"),
                           *id = ucl_object_lookup (obj, "id");
        ucl_object_t * result = NULL;
        const char * txt = method ? ucl_object_tostring (method) : NULL;
        size_t nparams = params ? ucl_array_size (params) : 0;
        s16rpc_data_t dat;

        s16rpc_method_t * cand;

        if (!method || !txt)
        {
            printf (
                "RPC error: Malformed message: Mising or malformed method\n");
            reply_error (conn, id, 1, "Missing or malformed method", NULL);
            goto cleanup;
        }

        cand = list_it_val (
            s16rpc_method_list_find (&srv->meths,
                                     (s16rpc_method_list_find_fn)match_method,
                                     (void *)txt));

        if (!cand)
        {
            printf ("RPC error: Server cannot handle method %s\n", txt);
            reply_error (
                conn, id, S16ENOSUCHMETH, "Server cannot handle method", NULL);
            goto cleanup;
        }

        if (nparams != cand->nparams)
        {
            printf ("RPC error: Parameter count mismatch for method %s "
                    "(expected %ld, got %ld)\n",
                    txt,
                    cand->nparams,
                    nparams);
            reply_error (conn, id, 1, "Incorrect parameter count", NULL);
            goto cleanup;
        }

        dat.sock = conn->fd;
        dat.method = cand->name;
        dat.err.code = 0;
        dat.err.data = 0;
        dat.err.message = NULL;
        dat.extra = srv->extra;

        result = dispatch_method (&dat, cand->fun, nparams, params);

        if (dat.err.code)
        {
            assert (!result);
            reply_error (conn, id, dat.err.code, dat.err.message, dat.err.data);
            /* err.data is auto-freed by unref in reply_error; no need to do
             * away with it. */
            if (dat.err.message)
                free (dat.err.message);
        }
        else
        {
            assert (!dat.err.code && !dat.err.message && !dat.err.data);
            reply_result (conn, id, result);
        }
    }

cleanup:
    if (obj)
        ucl_obj_unref (obj);
    ucl_parser_free (parser);
}

static void handle_recv (s16rpc_srv_t * srv, s16rpc_conn_t * conn)
{
    int len_to_recv;

    if (!conn->cur_msg_len)
    {
        size_t len =
            recv (conn->fd, (char *)&conn->cur_msg_len, sizeof (int32_t), 0);
        assert (len == 4);
        conn->cur_msg_buf = malloc (conn->cur_msg_len);
        conn->cur_msg_off = 0;
    }

    len_to_recv = conn->cur_msg_len - conn->cur_msg_off;
    conn->cur_msg_off += recv (conn->fd, conn->cur_msg_buf, len_to_recv, 0);

    if (conn->cur_msg_len && (conn->cur_msg_off == conn->cur_msg_len))
    {
        /* message fully received */
        conn->cur_msg_len = 0;
        handle_msg (srv, conn);
        free (conn->cur_msg_buf);
        conn->cur_msg_buf = NULL;
    }
}

void s16rpc_error_destroy (s16rpc_error_t * rerr)
{
    if (rerr->message)
        free (rerr->message);
    if (rerr->data)
        ucl_object_unref (rerr->data);
}

void s16rpc_investigate_kevent (s16rpc_srv_t * srv, struct kevent * ev)
{
    struct kevent nev;
    if (ev->flags & EV_EOF)
    {
        int fd = ev->ident;
        s16rpc_conn_t * cand =
            list_it_val (s16rpc_conn_list_find_int (&srv->conns, match_fd, fd));

        if (cand)
        {
            EV_SET (&nev, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
            if (kevent (srv->kq, &nev, 1, NULL, 0, NULL) == -1)
                err (1, "kevent");
            conn_close (srv, cand);
        }
    }
    else if (!srv->is_client && ev->ident == srv->fd)
    {
        int fd = accept (ev->ident, NULL, NULL);
        if (fd == -1)
            err (1, "accept");
        conn_new (srv, fd);
        EV_SET (&nev, fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
        if (kevent (srv->kq, &nev, 1, NULL, 0, NULL) == -1)
            err (1, "kevent");
    }
    else if (ev->filter == EVFILT_READ)
    {
        int fd = ev->ident;
        s16rpc_conn_t * cand =
            list_it_val (s16rpc_conn_list_find_int (&srv->conns, match_fd, fd));

        if (cand)
            handle_recv (srv, cand);
    }
}

void s16rpc_srv_register_method (s16rpc_srv_t * srv, const char * name,
                                 size_t nparams, s16rpc_fun_t fun)
{
    s16rpc_method_t * meth = malloc (sizeof (s16rpc_method_t));
    meth->name = name;
    meth->nparams = nparams;
    meth->fun = fun;
    s16rpc_method_list_add (&srv->meths, meth);
}

s16rpc_srv_t * s16rpc_srv_new (int kq, int sock, void * extra, bool is_client)
{
    s16rpc_srv_t * srv = malloc (sizeof (s16rpc_srv_t));
    struct kevent ev;

    srv->is_client = is_client;
    srv->kq = kq;
    srv->fd = sock;
    srv->extra = extra;
    srv->conns = s16rpc_conn_list_new ();
    srv->meths = s16rpc_method_list_new ();

    EV_SET (&ev, sock, EVFILT_READ, EV_ADD, 0, 0, NULL);
    if (kevent (kq, &ev, 1, NULL, 0, NULL) == -1)
        err (1, "kevent");

    if (is_client)
    {
        conn_new (srv, sock);
    }

    return srv;
}

/******************************************************************************
 * CLIENT SIDE
 ******************************************************************************/

char * recv_text (int fd)
{
    size_t len_read;
    int32_t len;
    char * buf;

    len_read = recv (fd, (char *)&len, sizeof (int32_t), 0);
    assert (len_read == sizeof (int32_t));

    if (len > MAX_LEN_MSG)
    {
        s16_log (ERR, "ignoring message of excessive length\n");
        buf = strdup ("INVALID-MESSAGE");
    }
    else
    {
        buf = malloc (len);
        recv (fd, buf, len, 0);
    }

    return buf;
}

ucl_object_t * recv_reply (int fd, s16rpc_error_t * rerror)
{
    char * reply = recv_text (fd);
    struct ucl_parser * parser = ucl_parser_new (0);
    ucl_object_t * obj = NULL;
    const ucl_object_t *error, *result;
    ucl_object_t * ret = NULL;

    ucl_parser_add_string (parser, reply, strlen (reply));

    if (ucl_parser_get_error (parser))
    {
        printf ("RPC error: Malformed reply: %s\n",
                ucl_parser_get_error (parser));
    }

    obj = ucl_parser_get_object (parser);

    result = ucl_object_lookup (obj, "result");
    error = ucl_object_lookup (obj, "error");

    if (error)
    {
        const ucl_object_t *code = ucl_object_lookup (error, "code"),
                           *message = ucl_object_lookup (error, "message"),
                           *data = ucl_object_lookup (error, "data");
        printf ("RPC Error: code %ld, message %s\n",
                ucl_object_toint (code),
                ucl_object_tostring (message));
        rerror->code = ucl_object_toint (code);
        rerror->message =
            message ? strdup (ucl_object_tostring (message)) : NULL;
        rerror->data = data ? ucl_object_ref (data) : NULL;
    }
    else if (result)
    {
        ret = ucl_object_ref (result);
    }
    else
    {
        printf ("RPC Error: Malformed reply\n");
    }

    free (reply);

    if (obj)
        ucl_object_unref (obj);
    ucl_parser_free (parser);

    return ret;
}

s16rpc_clnt_t s16rpc_clnt_new (int sock)
{
    s16rpc_clnt_t clnt;
    clnt.fd = sock;
    return clnt;
}

ucl_object_t * s16rpc_i_clnt_call_arr (s16rpc_clnt_t * clnt,
                                       s16rpc_error_t * rerror,
                                       const char * meth_name,
                                       ucl_object_t * params)
{
    ucl_object_t * msg = ucl_object_typed_new (UCL_OBJECT);

    ucl_object_insert_key (
        msg, ucl_object_fromstring (S16_JSONRPC_VERSION), "jsonrpc", 0, 1);
    ucl_object_insert_key (
        msg, ucl_object_fromstring (meth_name), "method", 0, 1);
    ucl_object_insert_key (msg, params, "params", 0, 1);

    write_object (clnt->fd, msg);
    ucl_object_unref (msg);

    return recv_reply (clnt->fd, rerror);
}

ucl_object_t * s16rpc_i_clnt_call (s16rpc_clnt_t * clnt,
                                   s16rpc_error_t * rerror, size_t nparams,
                                   const char * meth_name, ...)
{
    va_list args;
    ucl_object_t * params = ucl_object_typed_new (UCL_ARRAY);

    nparams--;

    va_start (args, meth_name);
    for (size_t i = 0; i < nparams; ++i)
    {
        ucl_object_t * arg = va_arg (args, ucl_object_t *);
        ucl_array_append (params, ucl_object_ref (arg));
    }
    va_end (args);

    return s16rpc_i_clnt_call_arr (clnt, rerror, meth_name, params);
}

ucl_object_t * s16rpc_i_clnt_call_unsafe (s16rpc_clnt_t * clnt, size_t nparams,
                                          const char * meth_name, ...)
{
    va_list args;
    ucl_object_t *params = ucl_object_typed_new (UCL_ARRAY), *ret;
    s16rpc_error_t rerror;

    nparams--;

    rerror.code = 0;
    rerror.message = NULL;
    rerror.data = NULL;

    va_start (args, meth_name);
    for (size_t i = 0; i < nparams; ++i)
    {
        ucl_object_t * arg = va_arg (args, ucl_object_t *);
        ucl_array_append (params, ucl_object_ref (arg));
    }
    va_end (args);

    ret = s16rpc_i_clnt_call_arr (clnt, &rerror, meth_name, params);

    if (rerror.message)
        free (rerror.message);
    if (rerror.data)
        ucl_object_unref (rerror.data);

    return ret;
}