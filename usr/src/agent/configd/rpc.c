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
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "S16/JSONRPCClient.h"
#include "S16/Repository.h"
#include "S16/JSONRPCServer.h"

#include "configd.h"

/* Fun: disable/enable
 * Desc: (Dis/en)ables a service by setting its enabled flag to (false/true) and
 * dispatching an administrative event. Sig: int (path_t * path) */
ucl_object_t * handle_disable (s16rpc_data_t * dat, const ucl_object_t * upath)
{
    int e = 0;
    bool enable = !strcmp (dat->method, "enable");
    path_t * path = s16db_ucl_to_path (upath);

    if (!path)
    {
        e = S16EBADPATH;
        goto ret;
    }

    e = db_set_enabled (path, enable);
    s16_path_destroy (path);

ret:
    return ucl_object_fromint (e);
}

/* Fun: import-service
 * Desc: Import a full manifest to the given layer.
 * Sig: int (svc_t * svc, enum layer) */
ucl_object_t * handle_import_service (s16rpc_data_t * dat,
                                      const ucl_object_t * umanifest,
                                      const ucl_object_t * ulayer)
{
    int e = 0;
    svc_t * svc = s16db_ucl_to_svc (umanifest);
    s16db_layer_t layer = ucl_object_toint (ulayer);

    if (!svc)
        fprintf (stderr, "Received invalid service.");
    else
    {
        s16_log_svc (INFO, svc, "Service loaded into repository.\n");
        db_import (layer, svc);
    }

    return ucl_object_fromint (e);
}

/* Fun: get-all-services-merged
 * Desc: Get a merged list of all services.
 * Sig: int (svc_t * svc, enum layer) */
ucl_object_t * handle_get_all_services_merged (s16rpc_data_t * dat)
{
    ucl_object_t * ureply = ucl_object_typed_new (UCL_ARRAY);

    s16_log (WARN, "Received request on FD %d\n", dat->sock);

    for (svc_list_it it = list_begin (db_get_all_svcs_merged ()); it != NULL;
         it = list_next (it))
        ucl_array_append (ureply, s16db_svc_to_ucl (it->val));

    return ureply;
}

/* Fun: get-path-merged
 * Desc: Get the merged service or instance for the given path.
 * Sig: {type, value?} (path_t * path) */
ucl_object_t * handle_get_path_merged (s16rpc_data_t * dat,
                                       const ucl_object_t * upath)
{
    ucl_object_t * reply = ucl_object_typed_new (UCL_OBJECT);
    path_t * path = s16db_ucl_to_path (upath);
    s16db_lookup_result_t res = db_lookup_path_merged (path);

    if (res.type == SVC && res.s)
    {
        ucl_object_t * usvc = s16db_svc_to_ucl (res.s);
        ucl_object_insert_key (
            reply, ucl_object_fromstring ("svc"), "type", 0, 1);
        ucl_object_insert_key (reply, usvc, "value", 0, 1);
    }
    else if (res.type == INSTANCE && res.i)
    {
        ucl_object_t * uinst = s16db_inst_to_ucl (res.i);
        ucl_object_insert_key (
            reply, ucl_object_fromstring ("svc"), "type", 0, 1);
        ucl_object_insert_key (reply, uinst, "value", 0, 1);
    }
    else
    {
        ucl_object_insert_key (
            reply, ucl_object_fromstring ("error"), "type", 0, 1);
    }

    s16_path_destroy (path);

    return reply;
}

/* Fun: subscribe
 * Desc: Subscribe to the given set of notifications.
 * Sig: int (s16note_type_t) */
ucl_object_t * handle_subscribe (s16rpc_data_t * dat,
                                 const ucl_object_t * utypes)
{
    int types = ucl_object_toint (utypes);
    subscriber_t * sub = malloc (sizeof (*sub));

    sub->clnt = s16rpc_clnt_new (dat->sock);
    sub->kinds = types;

    subscriber_list_add (&subs, sub);

    return ucl_object_fromint (0);
}

void rpc_setup (s16rpc_srv_t * srv)
{
    s16rpc_srv_register_method (
        srv, "disable", 1, (s16rpc_fun_t)handle_disable);
    s16rpc_srv_register_method (srv, "enable", 1, (s16rpc_fun_t)handle_disable);
    s16rpc_srv_register_method (
        srv, "import-service", 2, (s16rpc_fun_t)handle_import_service);
    s16rpc_srv_register_method (
        srv, "get-all-services-merged", 0, handle_get_all_services_merged);
    s16rpc_srv_register_method (
        srv, "get-path-merged", 1, (s16rpc_fun_t)handle_get_path_merged);

    s16rpc_srv_register_method (
        srv, "subscribe", 1, (s16rpc_fun_t)handle_subscribe);
}
