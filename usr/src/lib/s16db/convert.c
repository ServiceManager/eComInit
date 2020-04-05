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

#include "s16db.h"
#include "ucl.h"

#define UclFromStr ucl_object_fromstring

void ins_key (ucl_object_t * obj, const char * key, const char * val)
{
    ucl_object_insert_key (obj, UclFromStr (val), key, 0, 0);
}

ucl_object_t * ins_key_arr (ucl_object_t * obj, const char * key)
{
    ucl_object_t * arr = ucl_object_typed_new (UCL_ARRAY);
    ucl_object_insert_key (obj, arr, key, 0, 0);
    return arr;
}

ucl_object_t * s16db_path_to_ucl (path_t * path)
{
    char * spath = s16_path_to_string (path);
    ucl_object_t * upath = ucl_object_fromstring (spath);
    free (spath);
    return upath;
}

const char * s16db_depgroup_type_to_string (int t)
{
    /* clang-format off */
    return t == REQUIRE_ALL ? "require-all"
         : t == REQUIRE_ANY ? "require-any"
         : t == OPTIONAL_ALL ? "optional-all"
         : t == EXCLUDE_ALL ? "exclude-all"
         : "INVALID-TYPE";
    /* clang-format on */
}

const char * s16db_depgroup_restarton_to_string (int t)
{
    /* clang-format off */
    return t == ON_NONE ? "none"
         : t == ON_ERROR ? "error"
         : t == ON_REFRESH ? "refresh"
         : t == ON_RESTART ? "restart"
         : "INVALID";
    /* clang-format on */
}

ucl_object_t * s16db_depgroup_to_ucl (depgroup_t * prop)
{
    ucl_object_t * udep = ucl_object_typed_new (UCL_OBJECT);
    ins_key (udep, "grouping", s16db_depgroup_type_to_string (prop->type));
    ins_key (udep, "restart-on",
             s16db_depgroup_restarton_to_string (prop->restart_on));
    ucl_object_t * paths = ins_key_arr (udep, "paths");

    for (path_list_it it = path_list_begin (&prop->paths); it != NULL;
         it = path_list_it_next (it))
        ucl_array_append (paths, s16db_path_to_ucl (it->val));

    return udep;
}

ucl_object_t * s16db_prop_to_ucl (property_t * prop)
{
    ucl_object_t * uprop = ucl_object_typed_new (UCL_OBJECT);
    ins_key (uprop, "name", prop->name);
    ins_key (uprop, "value", prop->value.s);
    return uprop;
}

void u_add_props (ucl_object_t * obj, prop_list_t * props)
{
    ucl_object_t * arr = ins_key_arr (obj, "properties");
    for (prop_list_it it = prop_list_begin (props); it != NULL;
         it = prop_list_it_next (it))
        ucl_array_append (arr, s16db_prop_to_ucl (it->val));
}

ucl_object_t * s16db_meth_to_ucl (method_t * meth)
{
    ucl_object_t * umeth = ucl_object_typed_new (UCL_OBJECT);
    ins_key (umeth, "name", meth->name);
    if (!prop_list_empty (&meth->props))
        u_add_props (umeth, &meth->props);
    return umeth;
}

void u_add_depgroups (ucl_object_t * obj, depgroup_list_t * depgroups)
{
    ucl_object_t * arr = ins_key_arr (obj, "dependencies");
    for (depgroup_list_it it = depgroup_list_begin (depgroups); it != NULL;
         it = depgroup_list_it_next (it))
        ucl_array_append (arr, s16db_depgroup_to_ucl (it->val));
}

void u_add_meths (ucl_object_t * obj, meth_list_t * meths)
{
    ucl_object_t * arr = ins_key_arr (obj, "methods");
    for (meth_list_it it = meth_list_begin (meths); it != NULL;
         it = meth_list_it_next (it))
        ucl_array_append (arr, s16db_meth_to_ucl (it->val));
}

ucl_object_t * s16db_inst_to_ucl (svc_instance_t * inst)
{
    ucl_object_t * uinst = ucl_object_typed_new (UCL_OBJECT);
    char * path = s16_path_to_string (inst->path);

    ins_key (uinst, "path", path);

    if (!prop_list_empty (&inst->props))
        u_add_props (uinst, &inst->props);

    if (!meth_list_empty (&inst->meths))
        u_add_meths (uinst, &inst->meths);

    if (!depgroup_list_empty (&inst->depgroups))
        u_add_depgroups (uinst, &inst->depgroups);

    ucl_object_insert_key (uinst, ucl_object_frombool (inst->enabled),
                           "enabled", 0, 1);
    ucl_object_insert_key (uinst, ucl_object_fromint (inst->state), "state", 0,
                           1);

    free (path);

    return uinst;
}

ucl_object_t * s16db_svc_to_ucl (svc_t * svc)
{
    ucl_object_t * usvc = ucl_object_typed_new (UCL_OBJECT);
    char * path = s16_path_to_string (svc->path);
    ins_key (usvc, "path", path);

    if (svc->def_inst)
        ins_key (usvc, "default-instance", svc->def_inst);

    if (!prop_list_empty (&svc->props))
        u_add_props (usvc, &svc->props);

    if (!meth_list_empty (&svc->meths))
        u_add_meths (usvc, &svc->meths);

    if (!inst_list_empty (&svc->insts))
    {
        ucl_object_t * arr = ins_key_arr (usvc, "instances");
        for (inst_list_it it = inst_list_begin (&svc->insts); it != NULL;
             it = inst_list_it_next (it))
            ucl_array_append (arr, s16db_inst_to_ucl (it->val));
    }
    if (!depgroup_list_empty (&svc->depgroups))
        u_add_depgroups (usvc, &svc->depgroups);

    ucl_object_insert_key (usvc, ucl_object_fromint (svc->state), "state", 0,
                           1);

    free (path);

    return usvc;
}

ucl_object_t * s16db_note_to_ucl (const s16note_t * note)
{
    ucl_object_t * unote = ucl_object_typed_new (UCL_OBJECT);
    char * path = s16_path_to_string (note->path);

    ucl_object_insert_key (unote, ucl_object_fromint (note->note_type),
                           "note-type", 0, 1);
    ucl_object_insert_key (unote, ucl_object_fromint (note->type), "type", 0,
                           1);
    ins_key (unote, "path", path);
    ucl_object_insert_key (unote, ucl_object_fromint (note->reason), "reason",
                           0, 1);

    free (path);

    return unote;
}

/******************************************************
 * Conversions from UCL to internal representation
 ******************************************************/

path_t * s16db_string_to_path (const char * txt)
{
    path_t * path = calloc (1, sizeof (path_t));
    size_t len;
    size_t svc_len = 0;

    len = strlen (txt);

    if (len > 4 && !strncmp (txt, "svc:/", 4))
    {
        path->full_qual = 1;
        txt += 5;
        len -= 5;
    }

    for (size_t pos = 0; pos < len && txt[pos] != ':'; pos++, svc_len++)
        ;

    path->svc = strndup (txt, svc_len);

    if (len > svc_len)
        path->inst = strndup (txt + svc_len + 1, len - 1);

    return path;
}

#define IS(x) (!strcmp (txt, x))

int s16db_string_to_depgroup_type (const char * txt)
{
    /* clang-format off */
    return IS("require-all") ? REQUIRE_ALL
         : IS("require-any") ? REQUIRE_ANY
         : IS("optional-all") ? OPTIONAL_ALL
         : IS("exclude-all") ? EXCLUDE_ALL
         : -1;
    /* clang-format on */
}

int s16db_string_to_depgroup_restarton (const char * txt)
{
    /* clang-format off */
    return IS("none") ? ON_NONE
         : IS("error") ? ON_ERROR
         : IS("refresh") ? ON_REFRESH 
         : IS("restart") ? ON_RESTART
         : -1;
    /* clang-format on */
}

#undef IS

path_t * s16db_ucl_to_path (const ucl_object_t * upath)
{
    assert (ucl_object_type (upath) == UCL_STRING);
    return s16db_string_to_path (ucl_object_tostring (upath));
}

depgroup_t * s16db_ucl_to_depgroup (const ucl_object_t * obj)
{
    depgroup_t * depgroup = calloc (1, sizeof (depgroup_t));
    const ucl_object_t *type, *restart_on, *paths, *upath;
    ucl_object_iter_t it = NULL;

    type = ucl_object_lookup (obj, "grouping");
    restart_on = ucl_object_lookup (obj, "restart-on");
    paths = ucl_object_lookup (obj, "paths");

    assert (type && ucl_object_type (type) == UCL_STRING);
    assert (restart_on && ucl_object_type (restart_on) == UCL_STRING);
    assert (paths && ucl_object_type (paths) == UCL_ARRAY);

    depgroup->type = s16db_string_to_depgroup_type (ucl_object_tostring (type));
    depgroup->restart_on =
        s16db_string_to_depgroup_restarton (ucl_object_tostring (restart_on));

    while ((upath = ucl_iterate_object (paths, &it, true)))
    {
        path_t * path = s16db_ucl_to_path (upath);
        assert (path);
        path_list_add (&depgroup->paths, path);
    }

    return depgroup;
}

property_t * s16db_ucl_to_prop (const ucl_object_t * obj)
{
    property_t * prop = calloc (1, sizeof (property_t));
    const ucl_object_t *name, *val;

    name = ucl_object_lookup (obj, "name");
    val = ucl_object_lookup (obj, "value");

    if (!name)
    {
        fprintf (stderr, "Error: Unnamed property\n");
        goto fail;
    }

    if (!val)
    {
        fprintf (stderr, "Error: Property without value\n");
        goto fail;
    }

    assert (ucl_object_type (name) == UCL_STRING);
    assert (ucl_object_type (val) == UCL_STRING);

    prop->type = PROP_STRING;
    prop->name = strdup (ucl_object_tostring (name));
    prop->value.s = strdup (ucl_object_tostring (val));

    return prop;

fail:
    free (prop);
    return NULL;
}

void add_props (const ucl_object_t * props, prop_list_t * list)
{
    const ucl_object_t * ucl_prop;
    ucl_object_iter_t it = NULL;

    while ((ucl_prop = ucl_iterate_object (props, &it, true)))
    {
        property_t * prop = s16db_ucl_to_prop (ucl_prop);
        if (!prop)
            fprintf (stderr, "Bad property\n");
        else
            prop_list_add (list, prop);
    }
}

method_t * s16db_ucl_to_meth (const ucl_object_t * obj)
{
    method_t * meth = calloc (1, sizeof (method_t));
    const ucl_object_t *name, *props;

    name = ucl_object_lookup (obj, "name");
    props = ucl_object_lookup (obj, "properties");

    if (!name)
    {
        fprintf (stderr, "Error: Unnamed method.\n");
        goto fail;
    }

    if (!props)
    {
        fprintf (stderr, "Error: method without properties\n");
        goto fail;
    }

    assert (ucl_object_type (name) == UCL_STRING);
    assert (ucl_object_type (props) == UCL_ARRAY);

    meth->name = strdup (ucl_object_tostring (name));
    add_props (props, &meth->props);

    return meth;

fail:
    free (meth);
    return NULL;
}

static void add_meths (const ucl_object_t * meths, meth_list_t * list)
{
    const ucl_object_t * ucl_meth;
    ucl_object_iter_t it = NULL;

    while ((ucl_meth = ucl_iterate_object (meths, &it, true)))
    {
        method_t * meth = s16db_ucl_to_meth (ucl_meth);
        if (!meth)
            fprintf (stderr, "Bad method\n");
        else
            meth_list_add (list, meth);
    }
}

static void add_depgroups (const ucl_object_t * depgroups,
                           depgroup_list_t * list)
{
    const ucl_object_t * ucl_depgroup;
    ucl_object_iter_t it = NULL;

    while ((ucl_depgroup = ucl_iterate_object (depgroups, &it, true)))
    {
        depgroup_t * depgroup = s16db_ucl_to_depgroup (ucl_depgroup);
        if (!depgroup)
            fprintf (stderr, "Bad depgroup\n");
        else
            depgroup_list_add (list, depgroup);
    }
}

svc_instance_t * s16db_ucl_to_inst (const ucl_object_t * obj)
{
    svc_instance_t * inst = calloc (1, sizeof (svc_t));
    const ucl_object_t *path, *props, *meths, *depgroups, *enabled, *state;

    inst->props = prop_list_new ();
    inst->meths = meth_list_new ();

    path = ucl_object_lookup (obj, "path");
    props = ucl_object_lookup (obj, "properties");
    meths = ucl_object_lookup (obj, "methods");
    depgroups = ucl_object_lookup (obj, "dependencies");
    enabled = ucl_object_lookup (obj, "enabled");
    state = ucl_object_lookup (obj, "state");

    if (!path)
    {
        fprintf (stderr, "Error: Unnamed instance\n");
        goto fail;
    }

    assert (ucl_object_type (path) == UCL_STRING);
    assert (!props || ucl_object_type (props) == UCL_ARRAY);
    assert (!meths || ucl_object_type (meths) == UCL_ARRAY);
    assert (!depgroups || ucl_object_type (depgroups) == UCL_ARRAY);
    assert (!enabled || ucl_object_type (enabled) == UCL_BOOLEAN);
    assert (!state || ucl_object_type (state) == UCL_INT);

    inst->path = s16db_ucl_to_path (path);

    if (props)
        add_props (props, &inst->props);
    if (meths)
        add_meths (meths, &inst->meths);
    if (depgroups)
        add_depgroups (depgroups, &inst->depgroups);
    inst->state = state ? ucl_object_toint (state) : S_NONE;
    inst->enabled = enabled ? ucl_object_toboolean (enabled) : true;

    return inst;

fail:
    free (inst);
    return NULL;
}

svc_t * s16db_ucl_to_svc (const ucl_object_t * obj)
{
    svc_t * svc = calloc (1, sizeof (svc_t));
    const ucl_object_t *path, *def_inst, *props, *meths, *instances, *depgroups,
        *state;

    svc->insts = inst_list_new ();
    svc->props = prop_list_new ();
    svc->meths = meth_list_new ();
    svc->depgroups = depgroup_list_new ();

    path = ucl_object_lookup (obj, "path");
    def_inst = ucl_object_lookup (obj, "default-instance");
    props = ucl_object_lookup (obj, "properties");
    meths = ucl_object_lookup (obj, "methods");
    instances = ucl_object_lookup (obj, "instances");
    depgroups = ucl_object_lookup (obj, "dependencies");
    state = ucl_object_lookup (obj, "state");

    if (!path)
    {
        fprintf (stderr, "Error: Unnamed service\n");
        goto fail;
    }

    assert (ucl_object_type (path) == UCL_STRING);
    assert (!def_inst || ucl_object_type (def_inst) == UCL_STRING);
    assert (!props || ucl_object_type (props) == UCL_ARRAY);
    assert (!meths || ucl_object_type (meths) == UCL_ARRAY);
    assert (!instances || ucl_object_type (instances) == UCL_ARRAY);
    assert (!depgroups || ucl_object_type (depgroups) == UCL_ARRAY);
    assert (!state || ucl_object_type (state) == UCL_INT);

    svc->path = s16db_ucl_to_path (path);

    if (def_inst)
        svc->def_inst = strdup (ucl_object_tostring (def_inst));

    if (props)
        add_props (props, &svc->props);
    if (meths)
        add_meths (meths, &svc->meths);
    if (depgroups)
        add_depgroups (depgroups, &svc->depgroups);
    svc->state = state ? ucl_object_toint (state) : S_NONE;

    if (instances)
    {
        const ucl_object_t * ucl_inst;
        ucl_object_iter_t it = NULL;

        while ((ucl_inst = ucl_iterate_object (instances, &it, true)))
        {
            svc_instance_t * inst = s16db_ucl_to_inst (ucl_inst);
            if (!inst)
                goto fail;
            else
                inst_list_add (&svc->insts, inst);
        }
    }

    return svc;

fail:
    free (svc);
    return NULL;
}

svc_list_t s16db_ucl_to_svcs (const ucl_object_t * usvcs)
{
    const ucl_object_t * ucl_svc;
    ucl_object_iter_t it = NULL;
    svc_list_t svcs = svc_list_new ();

    while ((ucl_svc = ucl_iterate_object (usvcs, &it, true)))
    {
        svc_t * svc = s16db_ucl_to_svc (ucl_svc);
        assert (svc);
        svc_list_add (&svcs, svc);
    }

    return svcs;
}

s16note_t * s16db_ucl_to_note (const ucl_object_t * unote)
{
    s16note_t * note = malloc (sizeof (s16note_t));
    const ucl_object_t *unote_type, *utype, *upath, *ureason;

    unote_type = ucl_object_lookup (unote, "note-type");
    utype = ucl_object_lookup (unote, "type");
    upath = ucl_object_lookup (unote, "path");
    ureason = ucl_object_lookup (unote, "reason");

    assert (unote_type && ucl_object_type (unote_type) == UCL_INT);
    assert (utype && ucl_object_type (utype) == UCL_INT);
    assert (upath && ucl_object_type (upath) == UCL_STRING);
    assert (ureason && ucl_object_type (ureason) == UCL_INT);

    note->note_type = ucl_object_toint (unote_type);
    note->type = ucl_object_toint (utype);
    note->path = s16db_ucl_to_path (upath);
    note->reason = ucl_object_toint (ureason);

    return note;
}