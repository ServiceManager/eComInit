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
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "s16db_priv.h"

void add_param (ucl_object_t * msg, const char * name, ucl_object_t * value)
{
    ucl_object_insert_key (msg, value, name, 0, 0);
}

ucl_object_t * new_msg (const char * op_str)
{
    ucl_object_t * msg = ucl_object_typed_new (UCL_OBJECT);
    ucl_object_t * op = ucl_object_fromstring (op_str);
    add_param (msg, "method", op);
    return msg;
}

/* Fun: notify
 * Desc: Handles a notification.
 * Sig: int (s16note_t * note) */
ucl_object_t * handle_notify (s16rpc_data_t * dat, const ucl_object_t * unote)
{
    s16note_t * note = s16db_ucl_to_note (unote);
    printf ("Got a note: %s, %d, %d, %d\n", s16_path_to_string (note->path),
            note->note_type, note->type, note->reason);
    s16note_list_add (&((s16db_hdl_t *)dat->extra)->notes, note);
    return ucl_object_fromint (0);
}

void s16db_subscribe (s16db_hdl_t * hdl, int kq, int /* s16note_type_t */ kinds)
{
    s16rpc_error_t rerr;
    ucl_object_t * ukinds = ucl_object_fromint (kinds);
    ucl_object_t * reply;

    if (!hdl->srv)
    {
        hdl->srv = s16rpc_srv_new (kq, hdl->fd, (void *)hdl, 1);
        s16rpc_srv_register_method (hdl->srv, "notify", 1,
                                    (s16rpc_fun_t)handle_notify);
    }

    reply = s16rpc_clnt_call (&hdl->clnt, &rerr, "subscribe", ukinds);

    if (!reply)
    {
        s16_log (ERR, "Failed to send subscribe message: code %d: %s\n",
                 rerr.code, rerr.message);
        s16rpc_error_destroy (&rerr);
    }
    else
    {

        ucl_object_unref (reply);
    }
}

void s16db_publish (s16db_hdl_t * hdl, s16note_t * note)
{
    s16rpc_error_t rerr;
    ucl_object_t * unote = s16db_note_to_ucl (note);
    ucl_object_t * reply;

    reply = s16rpc_clnt_call (&hdl->clnt, &rerr, "publish", unote);
    ucl_object_unref (unote);

    if (!reply)
    {
        s16_log (ERR, "Failed to send publish message: code %d: %s\n",
                 rerr.code, rerr.message);
        s16rpc_error_destroy (&rerr);
    }
    else
    {
        ucl_object_unref (reply);
    }
}

/* Disables the given path. If path is an instance, disables that instance;
 * if path is a service, disables all its instances. */
#define DIS_OR_EN_FUN(MODE_)                                                   \
    int s16db_##MODE_ (s16db_hdl_t * hdl, path_t * path)                       \
    {                                                                          \
        s16rpc_error_t rerr;                                                   \
        ucl_object_t * upath = s16db_path_to_ucl (path);                       \
        ucl_object_t * reply;                                                  \
        int errc;                                                              \
                                                                               \
        reply = s16rpc_clnt_call (&hdl->clnt, &rerr, #MODE_, upath);           \
        ucl_object_unref (upath);                                              \
                                                                               \
        if (!reply)                                                            \
        {                                                                      \
            s16_log (ERR, "Failed to send %s message: code %d: %s\n", #MODE_,  \
                     rerr.code, rerr.message);                                 \
            errc = rerr.code;                                                  \
            s16rpc_error_destroy (&rerr);                                      \
        }                                                                      \
        else                                                                   \
        {                                                                      \
            errc = ucl_object_toint (reply);                                   \
            ucl_object_unref (reply);                                          \
        }                                                                      \
        printf ("Errc: %d\n", errc);                                           \
        return errc;                                                           \
    }

DIS_OR_EN_FUN (disable);
DIS_OR_EN_FUN (enable);

int s16db_set_state (s16db_hdl_t * hdl, path_t * path, svc_state_t state)
{
    s16rpc_error_t rerr;
    ucl_object_t * upath = s16db_path_to_ucl (path);
    ucl_object_t * ustate = ucl_object_fromint (state);
    ucl_object_t * reply;
    int errc;

    reply = s16rpc_clnt_call (&hdl->clnt, &rerr, "set-state", upath, ustate);
    ucl_object_unref (upath);
    ucl_object_unref (ustate);

    if (!reply)
    {
        s16_log (ERR, "Failed to send set-state message: code %d: %s\n",
                 rerr.code, rerr.message);
        s16rpc_error_destroy (&rerr);
        errc = rerr.code;
    }
    else
    {
        errc = ucl_object_toint (reply);
        ucl_object_unref (reply);
    }

    return errc;
}

svc_list_t s16db_repo_get_all_services_merged (s16db_hdl_t * hdl)
{
    s16rpc_error_t rerr;
    ucl_object_t * reply;
    svc_list_t svcs;

    reply = s16rpc_clnt_call (&hdl->clnt, &rerr, "get-all-services-merged");

    if (!reply)
    {
        s16_log (ERR, "Failed to send get-all-services message: code %d: %s\n",
                 rerr.code, rerr.message);
        s16rpc_error_destroy (&rerr);
    }
    else
    {
        svcs = s16db_ucl_to_svcs (reply);
        ucl_object_unref (reply);
    }

    return svcs;
}

s16db_lookup_result_t s16db_repo_get_path_merged (s16db_hdl_t * hdl,
                                                  path_t * path)
{
    s16rpc_error_t rerr;
    s16db_lookup_result_t res;
    ucl_object_t * upath = s16db_path_to_ucl (path);
    /* reply */
    ucl_object_t * reply = NULL;
    /* reply elements */
    const ucl_object_t *type, *value;

    reply = s16rpc_clnt_call (&hdl->clnt, &rerr, "get-path-merged", upath);
    ucl_object_unref (upath);

    if (!reply)
    {
        res.type = rerr.code;
        s16_log (ERR, "Failed to send get-path-merged message: code %d: %s\n",
                 rerr.code, rerr.message);
        s16rpc_error_destroy (&rerr);
    }
    else
    {
        type = ucl_object_lookup (reply, "type");
        value = ucl_object_lookup (reply, "value");

        if (!strcmp (ucl_object_tostring (type), "error"))
        {
            res.type = NOTFOUND;
            res.s = NULL;
        }
        else if (!strcmp (ucl_object_tostring (type), "svc"))
        {
            res.type = SVC;
            res.s = s16db_ucl_to_svc (value);
        }
        else
        {
            res.type = INSTANCE;
            res.i = s16db_ucl_to_inst (value);
        }

        ucl_object_unref (reply);
    }

    return res;
}

int s16db_import_ucl_svc (s16db_hdl_t * hdl, ucl_object_t * usvc,
                          s16db_layer_t layer)
{
    int e = 0;
    s16rpc_error_t rerr;
    ucl_object_t * ulayer = ucl_object_fromint (layer);
    ucl_object_t * reply;

    reply =
        s16rpc_clnt_call (&hdl->clnt, &rerr, "import-service", usvc, ulayer);

    ucl_object_unref (ulayer);

    if (!reply)
    {
        e = rerr.code;
        s16_log (ERR, "Failed to send import-service message: code %d: %s",
                 rerr.code, rerr.message);
        s16rpc_error_destroy (&rerr);
    }
    else
    {
        e = ucl_object_toint (reply);
        ucl_object_unref (reply);
    }

    return e;
}