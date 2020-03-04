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

#ifndef REPOSITORYD_H_
#define REPOSITORYD_H_

#include "s16db.h"

typedef struct
{
    s16rpc_clnt_t clnt;
    int kinds;
} subscriber_t;

S16List (subscriber, subscriber_t *);

/* rpc.c */
void rpc_setup (s16rpc_srv_t * srv);

/* db.c */
void db_setup ();
void db_destroy ();
void db_import (s16db_layer_t layer, svc_t * svc);
s16db_lookup_result_t db_lookup_path_merged (path_t * path);
svc_list_t * db_get_all_svcs_merged ();
int db_set_enabled (path_t * path, bool enabled);

extern s16db_scope_t global;
extern subscriber_list_t subs;
extern s16note_list_t notes;

#endif