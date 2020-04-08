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

#include <sys/event.h>
#include <sys/signal.h>

#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "s16.h"

/* State functions */
const char * s16_state_to_string (svc_state_t state)
{
#define Case(ST, T)                                                            \
    case S_##ST:                                                               \
        return T
    switch (state)
    {
        Case (NONE, "None");
        Case (UNINIT, "Uninitialised");
        Case (DISABLED, "Disabled");
        Case (OFFLINE, "Offline");
        Case (MAINTENANCE, "Maintenance");
        Case (ONLINE, "Online");
        Case (DEGRADED, "Degraded");
    default:
        return "<unknown-state>";
    }
#undef Case
}

/* Path functions */

path_t * s16_path_new (const char * svc, const char * inst)
{
    path_t * n = calloc (1, sizeof (path_t));
    assert (svc);
    n->full_qual = true;
    n->svc = strdup (svc);
    n->inst = inst ? strdup (inst) : NULL;
    return n;
}

void s16_path_destroy (path_t * path)
{
    /* don't destroy constant paths */
    if (path == s16_path_configd () || path == s16_path_graphd () ||
        path == s16_path_restartd ())
        return;
    if (path->svc)
        free (path->svc);
    if (path->inst)
        free (path->inst);
    free (path);
}

path_t * s16_path_copy (const path_t * path)
{
    return s16_path_new (path->svc, path->inst);
}

bool s16_path_equal (const path_t * a, const path_t * b)
{
    bool r;
    assert (a->svc && b->svc);
    r = !strcmp (a->svc, b->svc);
    if (!r)
        return r;
    if (a->inst || b->inst)
        r = (a->inst && b->inst) ? !strcmp (a->inst, b->inst) : 0;
    return r;
}

bool s16_path_is_inst (const path_t * path)
{
    assert (path->svc);
    return path->inst != NULL;
}

path_t * s16_svc_path_from_inst_path (const path_t * path)
{
    assert (path->inst);
    return s16_path_new (path->svc, NULL);
}

char * s16_path_to_string (const path_t * path)
{
    const char * qual = path->full_qual ? "svc:/" : "";
    const char * svc = path->svc ? path->svc : "";
    const char * svc_colon = path->inst ? ":" : "";
    const char * inst = path->inst ? path->inst : "";
    size_t size =
        snprintf (NULL, 0, "%s%s%s%s", qual, svc, svc_colon, inst) + 1;
    char * buf = malloc (size);
    sprintf (buf, "%s%s%s%s", qual, svc, svc_colon, inst);
    return buf;
}

path_t * s16_path_restartd ()
{
    static path_t path_restartd = {
        .full_qual = true, .svc = "system/svc/restarter", .inst = "default"};
    return &path_restartd;
}

path_t * s16_path_configd ()
{
    static path_t path_configd = {
        .full_qual = true, .svc = "system/svc/repository", .inst = "default"};
    return &path_configd;
}

path_t * s16_path_graphd ()
{
    static path_t path_graphd = {
        .full_qual = true, .svc = "system/svc/graph-engine", .inst = "default"};
    return &path_graphd;
}

/* Dependency group functions */
void s16_depgroup_destroy (depgroup_t * depgroup)
{
    if (depgroup->name)
        free (depgroup->name);
    path_list_deepdestroy (&depgroup->paths, s16_path_destroy);
    free (depgroup);
}
/* Makes a deep copy of a depgroup. */
depgroup_t * s16_depgroup_copy (const depgroup_t * depgroup)
{
    depgroup_t * r = malloc (sizeof (depgroup_t));
    r->name = depgroup->name ? strdup (depgroup->name) : NULL;
    r->type = depgroup->type;
    r->restart_on = depgroup->restart_on;
    r->paths = path_list_map (&depgroup->paths, s16_path_copy);
    return r;
}
/* Returns true if b's name matches that of a. */
bool s16_depgroup_name_equal (const depgroup_t * a, const depgroup_t * b)
{
    return a->name && b->name && !strcmp (a->name, b->name);
}

/* Property functions */
void s16_prop_destroy (property_t * prop)
{
    free (prop->name);
    if (prop->type == PROP_STRING)
        free (prop->value.s);
    free (prop);
}

property_t * s16_prop_copy (const property_t * prop)
{
    property_t * r = malloc (sizeof (property_t));
    r->name = strdup (prop->name);
    r->type = prop->type;
    if (prop->type == PROP_STRING)
        r->value.s = strdup (prop->value.s);
    else
        r->value.i = prop->value.i;
    return r;
}

bool s16_prop_name_equal (const property_t * a, const property_t * b)
{
    return !strcmp (a->name, b->name);
}

/* Method functions */
method_t * s16_meth_copy (const method_t * meth)
{
    method_t * r = malloc (sizeof (method_t));
    r->name = strdup (meth->name);
    r->props = prop_list_map (&meth->props, s16_prop_copy);
    return r;
}

bool s16_meth_name_equal (const method_t * a, const method_t * b)
{
    return !strcmp (a->name, b->name);
}

void s16_meth_destroy (method_t * meth)
{
    free (meth->name);
    prop_list_deepdestroy (&meth->props, s16_prop_destroy);
    free (meth);
}

/* Instance functions */
/* Destroys an instance. */
void s16_inst_destroy (svc_instance_t * inst)
{
    s16_path_destroy (inst->path);
    prop_list_deepdestroy (&inst->props, s16_prop_destroy);
    meth_list_deepdestroy (&inst->meths, s16_meth_destroy);
    depgroup_list_deepdestroy (&inst->depgroups, s16_depgroup_destroy);
    free (inst);
}
/* Makes a deep copy of a instance. */
svc_instance_t * s16_inst_copy (const svc_instance_t * inst)
{
    svc_instance_t * r = malloc (sizeof (svc_instance_t));
    r->path = s16_path_copy (inst->path);
    r->props = prop_list_map (&inst->props, s16_prop_copy);
    r->meths = meth_list_map (&inst->meths, s16_meth_copy);
    r->depgroups = depgroup_list_map (&inst->depgroups, s16_depgroup_copy);
    r->state = inst->state;
    return r;
}
/* Returns true if b's name matches that of a. */
bool s16_inst_name_equal (const svc_instance_t * a, const svc_instance_t * b)
{
    return !strcmp (a->path->inst, b->path->inst);
}

/* Service functions */

svc_t * s16_svc_alloc ()
{
    svc_t * r = calloc (1, sizeof (svc_t));
    r->props = prop_list_new ();
    r->meths = meth_list_new ();
    r->insts = inst_list_new ();
    r->depgroups = depgroup_list_new ();
    return r;
}

svc_t * s16_svc_copy (const svc_t * svc)
{
    svc_t * r = malloc (sizeof (svc_t));
    r->path = s16_path_copy (svc->path);
    r->def_inst = svc->def_inst ? strdup (svc->def_inst) : NULL;
    r->props = prop_list_map (&svc->props, s16_prop_copy);
    r->meths = meth_list_map (&svc->meths, s16_meth_copy);
    r->insts = inst_list_map (&svc->insts, s16_inst_copy);
    r->depgroups = depgroup_list_map (&svc->depgroups, s16_depgroup_copy);
    r->state = svc->state;
    return r;
}

void s16_svc_destroy (svc_t * svc)
{
    s16_path_destroy (svc->path);
    if (svc->def_inst)
        free (svc->def_inst);
    prop_list_deepdestroy (&svc->props, s16_prop_destroy);
    meth_list_deepdestroy (&svc->meths, s16_meth_destroy);
    inst_list_deepdestroy (&svc->insts, s16_inst_destroy);
    depgroup_list_deepdestroy (&svc->depgroups, s16_depgroup_destroy);
    free (svc);
}

bool s16_svc_name_equal (const svc_t * a, const svc_t * b)
{
    return !strcmp (a->path->svc, b->path->svc);
}

/* logging functionality */

#define KNRM "\x1B[0m"
#define KRED "\x1B[31m"
#define KGRN "\x1B[32m"
#define KYEL "\x1B[33m"
#define KBLU "\x1B[34m"
#define KMAG "\x1B[35m"
#define KCYN "\x1B[36m"
#define KWHT "\x1B[37m"

static const char * progname = "library";

void s16_log_init (const char * name) { progname = name; }

static void print_prefix (s16_log_level_t level, const path_t * svc)
{
    time_t rawtime;
    struct tm timeinfo;
    char time_str[26];
    char * spath = svc ? s16_path_to_string (svc) : NULL;
    const char * pfx = "";

    if (level == ERR)
        pfx = KRED "ERROR: " KNRM;
    else if (level == WARN)
        pfx = KYEL "WARNING: " KNRM;

    time (&rawtime);
    localtime_r (&rawtime, &timeinfo);
    asctime_r (&timeinfo, time_str);
    time_str[strlen (time_str) - 1] = '\0';

    if (spath)
    {
        printf (KWHT "[%s] "
                     "%s " KBLU "(%s)" KNRM ": %s ",
                time_str, progname, spath, pfx);
        free (spath);
    }
    else
        printf (KWHT "[%s] " KNRM "%s: " KNRM "%s", time_str, progname, pfx);
}

void s16_log (s16_log_level_t level, const char * fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    print_prefix (level, NULL);
    vprintf (fmt, args);
    va_end (args);
}

void s16_log_path (s16_log_level_t level, const path_t * path, const char * fmt,
                   ...)
{
    va_list args;

    va_start (args, fmt);
    print_prefix (level, path);
    vprintf (fmt, args);
    va_end (args);
}

void s16_log_svc (s16_log_level_t level, const svc_t * svc, const char * fmt,
                  ...)
{
    va_list args;

    va_start (args, fmt);
    print_prefix (level, svc->path);
    vprintf (fmt, args);
    va_end (args);
}

void s16_log_inst (s16_log_level_t level, const svc_instance_t * inst,
                   const char * fmt, ...)
{
    va_list args;

    va_start (args, fmt);
    print_prefix (level, inst->path);
    vprintf (fmt, args);
    va_end (args);
}

void s16_cloexec (int fd)
{
    int flags;
    flags = fcntl (fd, F_GETFD);
    if (flags != -1)
        fcntl (fd, F_SETFD, flags | FD_CLOEXEC);
    else
    {
        perror ("Fcntl!");
    }
}

void s16_handle_signal (int kq, int sig)
{
    struct kevent sigev;

#ifdef S16_PLAT_BSD
    /* On BSD, setting a signal event filter on a Kernel Queue does NOT
     * supersede ordinary signal disposition; with libkqueue on Linux at least,
     * however, it does. Therefore we ignore the signal; it'll be multiplexed
     * into our event loop instead. */
    signal (sig, SIG_IGN);
#endif

    EV_SET (&sigev, sig, EVFILT_SIGNAL, EV_ADD, 0, 0, 0);

    if (kevent (kq, &sigev, 1, 0, 0, 0) == -1)
    {
        perror ("KQueue: Failed to set signal event\n");
        exit (EXIT_FAILURE);
    }
}