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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "S16/Repository_Private.h"

int s16db_hdl_new (s16db_hdl_t * hdl)
{
    struct sockaddr_un sun;

    if ((hdl->fd = socket (AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        perror ("Failed to create socket");
        exit (-1);
    }

    s16_cloexec (hdl->fd);

    memset (&sun, 0, sizeof (struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strncpy (sun.sun_path, S16DB_CONFIGD_SOCKET_PATH, sizeof (sun.sun_path));

    if (connect (hdl->fd, (struct sockaddr *)&sun, SUN_LEN (&sun)) == -1)
        return -1;

    hdl->clnt = s16rpc_clnt_new (hdl->fd);
    hdl->srv = NULL;
    hdl->notes = s16note_list_new ();
    hdl->scope.svcs = s16db_repo_get_all_services_merged (hdl);

    return 0;
}

void s16db_investigate_kevent (s16db_hdl_t * hdl, struct kevent * ev)
{
    if (hdl->srv)
        s16rpc_investigate_kevent (hdl->srv, ev);
}

s16note_t * s16db_get_note (s16db_hdl_t * hdl)
{
    return s16note_list_lpop (&hdl->notes);
}

const svc_list_t * s16db_get_all_services (s16db_hdl_t * hdl)
{
    return &hdl->scope.svcs;
}

s16db_lookup_result_t s16db_lookup_path (s16db_hdl_t * hdl, path_t * path)
{
    return s16db_lookup_path_in_scope (hdl->scope, path);
}

s16db_lookup_result_t s16db_lookup_path_in_scope (s16db_scope_t scope,
                                                  path_t * path)
{
    s16db_lookup_result_t res;

    res.s = NULL;
    res.type = SVC;

    if (path->svc)
    {
        for (svc_list_it it = svc_list_begin (&scope.svcs); it != NULL;
             it = svc_list_it_next (it))
        {
            if (!strcmp (path->svc, it->val->path->svc))
                res.s = it->val;
        }

        if (!res.s)
        {
            fprintf (stderr, "Failed to find service %s!\n", path->svc);
            return res;
        }
    }

    if (path->svc && path->inst)
    {
        svc_t * svc = res.s;
        res.i = NULL;
        res.type = INSTANCE;

        list_foreach (inst, &svc->insts, it)
        {
            if (!strcmp (path->inst, it->val->path->inst))
                res.i = it->val;
        }

        if (!res.i)
        {
            s16_log (INFO,
                     "Failed to find instance %s of service %s!\n",
                     path->inst,
                     path->svc);
            return res;
        }
    }
    else if (path->inst)
    {
        /* search all services' instances for this path. Count and refuse if
         * a path is ambiguous. */
        size_t cnt = 0;
        res.i = NULL;
        res.type = INSTANCE;

        for (inst_list_it it = inst_list_begin (&res.s->insts); it != NULL;
             it = inst_list_it_next (it))
            if (!strcmp (path->inst, it->val->path->inst))
            {
                res.i = it->val;
                cnt++;
            }

        if (!cnt)
        {
            s16_log (INFO, "Failed to find instance %s\n", path->inst);
        }
        else if (cnt > 1)
        {
            s16_log (INFO, "Too many instances found for %s\n", path->inst);
        }
    }

    return res;
}

s16note_t * s16note_new (s16note_type_t note_type, int type,
                         const path_t * path, int reason)
{
    s16note_t * note = malloc (sizeof (s16note_t));
    note->note_type = note_type;
    note->type = type;
    note->path = s16_path_copy (path);
    note->reason = reason;
    return note;
}

void s16note_destroy (s16note_t * note)
{
    s16_path_destroy (note->path);
    free (note);
}