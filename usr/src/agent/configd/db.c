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

/*
 * Desc: The repository underlying configd is implemented here. Service data is
 * stored in several layers, ordered by increasing height in the hierarchy:
 *
 * - manifest: Service data as serialised in from the system service manifests.
 * - admin: Customisations made using svcadm, or by importing from non-system
 *   locations.
 *
 * The repository exports two kinds of interface for interacting with services.
 * One of these delivers data to consumers with the data from the layers merged
 * together. For example, if the admin layer contains an entry marked deleted,
 * then that entry is excluded from the data sent. This interface is consumed
 * consumed by daemons like startd and graphd.
 *
 * svcadm, however, stays in communication with the repository and consumes data
 * live from the system. While the first interface will, for example, generate
 * a 'default' instance for services lacking a default in their manifest, this
 * latter interface will not do so automatically.
 */

#include <assert.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>

#include "S16/Repository_Private.h"

#include "configd.h"

s16db_scope_t merged;

/* Manifest scope. */
s16db_scope_t manifest;
/* User scope. */
s16db_scope_t admin;

int merge_depgroup_into_list (S16DependencyGroup * depgroup,
                              depgroup_list_t * list)
{
    depgroup_list_it cand =
        depgroup_list_find_cmp (list, S16DependencyGroupNamesEqual, depgroup);

    if (cand)
    {
        S16DependencyGroupDestroy (cand->val);
        cand->val = S16DependencyGroupCopy (depgroup);
    }
    else
        depgroup_list_add (list, depgroup);

    return 0;
}

int merge_prop_into_list (S16Property * prop, prop_list_t * list)
{
    prop_list_it cand = prop_list_find_cmp (list, S16PropertyNamesEqual, prop);

    if (cand)
    {
        S16PropertyDestroy (cand->val);
        cand->val = S16PropertyCopy (prop);
    }
    else
        prop_list_add (list, prop);

    return 0;
}

int merge_meth_into_list (S16ServiceMethod * meth, meth_list_t * list)
{
    meth_list_it cand = meth_list_find_cmp (list, S16MethodNamesEqual, meth);

    if (cand)
    {
        S16MethodDestroy (cand->val);
        cand->val = S16MethodCopy (meth);
    }
    else
        meth_list_add (list, meth);

    return 0;
}

void merge_inst_into_inst (S16ServiceInstance * to, S16ServiceInstance * from)
{
    prop_list_walk (&from->props,
                    (prop_list_walk_fun)merge_prop_into_list,
                    (void *)&to->props);
    meth_list_walk (&from->meths,
                    (meth_list_walk_fun)merge_meth_into_list,
                    (void *)&to->meths);
    depgroup_list_walk (&from->depgroups,
                        (depgroup_list_walk_fun)merge_depgroup_into_list,
                        (void *)&to->depgroups);
}

int merge_inst_into_list (S16ServiceInstance * inst, inst_list_t * list)
{
    inst_list_it cand = inst_list_find_cmp (list, S16InstanceNamesEqual, inst);

    if (cand)
    {
        merge_inst_into_inst (inst, cand->val);
    }
    else
        inst_list_add (list, inst);

    return 0;
}

void merge_svc_into_svc (S16Service * to, S16Service * from)
{
    if (from->def_inst)
    {
        if (to->def_inst)
            free (to->def_inst);
        to->def_inst = strdup (from->def_inst);
    }
    prop_list_walk (&from->props,
                    (prop_list_walk_fun)merge_prop_into_list,
                    (void *)&to->props);
    meth_list_walk (&from->meths,
                    (meth_list_walk_fun)merge_meth_into_list,
                    (void *)&to->meths);
    depgroup_list_walk (&from->depgroups,
                        (depgroup_list_walk_fun)merge_depgroup_into_list,
                        (void *)&to->depgroups);
}

int merge_svc_into_list (S16Service * svc, svc_list_t * svcs)
{
    S16Service * cand =
        list_it_val (svc_list_find_cmp (svcs, S16ServiceNamesEqual, svc));

    if (!cand)
        svc_list_add (svcs, S16ServiceCopy (svc));
    else
        merge_svc_into_svc (cand, svc);

    return 0;
}

/* TODO: Optimise this. */
static void update_merged ()
{
    svc_list_deepdestroy (&merged.svcs, S16ServiceDestroy);

    merged.svcs = svc_list_map (&manifest.svcs, S16ServiceCopy);

    svc_list_walk (&admin.svcs,
                   (svc_list_walk_fun)merge_svc_into_list,
                   (void *)&merged.svcs);
}

void db_setup ()
{
    merged.svcs = svc_list_new ();
    manifest.svcs = svc_list_new ();
}
void db_destroy () { svc_list_deepdestroy (&manifest.svcs, S16ServiceDestroy); }

int db_set_enabled (S16Path * path, bool enabled)
{
    s16db_lookup_result_t lu = db_lookup_path_merged (path);

    if (lu.type == NOTFOUND)
        return S16ENOSUCHSVC;
    else if (lu.type == SVC)
    {
        list_foreach (inst, &lu.s->insts, it)
        {
            it->val->enabled = false;
            s16note_list_add (&notes,
                              s16note_new (N_ADMIN_REQ,
                                           enabled ? A_ENABLE : A_DISABLE,
                                           it->val->path,
                                           0));
        }
    }
    else
    {
        lu.i->enabled = enabled;
        s16note_list_add (
            &notes,
            s16note_new (N_ADMIN_REQ, enabled ? A_ENABLE : A_DISABLE, path, 0));
    }
    return 0;
}

void db_import (s16db_layer_t layer, S16Service * svc)
{
    svc_list_add (&manifest.svcs, svc);
    update_merged ();
}

s16db_lookup_result_t db_lookup_path_merged (S16Path * path)
{
    s16db_lookup_result_t r = s16db_lookup_path_in_scope (manifest, path);
    return r;
}

/* Retrieves a list of all services, fully merged. */
svc_list_t * db_get_all_svcs_merged () { return &merged.svcs; }
