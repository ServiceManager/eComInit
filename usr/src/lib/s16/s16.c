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

#include "S16/Service.h"

/* State functions */
const char * S16StateToString (S16ServiceState state)
{
#define Case(ST, T)                                                            \
    case kS16State##ST:                                                        \
        return T
    switch (state)
    {
        Case (None, "None");
        Case (Uninitialised, "Uninitialised");
        Case (Disabled, "Disabled");
        Case (Offline, "Offline");
        Case (Maintenance, "Maintenance");
        Case (Online, "Online");
        Case (Degraded, "Degraded");
    default:
        return "<unknown-state>";
    }
#undef Case
}

/* Path functions */

S16Path * S16PathNew (const char * svc, const char * inst)
{
    S16Path * n = calloc (1, sizeof (S16Path));
    assert (svc);
    n->full_qual = true;
    n->svc = strdup (svc);
    n->inst = inst ? strdup (inst) : NULL;
    return n;
}

void S16PathDestroy (S16Path * path)
{
    /* don't destroy constant paths */
    if (path == S16PathOfRepository () || path == S16PathOfGrapher () ||
        path == S16PathOfMainRestarter ())
        return;
    if (path->svc)
        free (path->svc);
    if (path->inst)
        free (path->inst);
    free (path);
}

S16Path * S16PathCopy (const S16Path * path)
{
    return S16PathNew (path->svc, path->inst);
}

bool S16PathEqual (const S16Path * a, const S16Path * b)
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

bool S16PathIsInstance (const S16Path * path)
{
    assert (path->svc);
    return path->inst != NULL;
}

S16Path * S16ServicePathFromInstancePath (const S16Path * path)
{
    assert (path->inst);
    return S16PathNew (path->svc, NULL);
}

char * S16PathToString (const S16Path * path)
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

S16Path * S16PathOfMainRestarter ()
{
    static S16Path path_restartd = {
        .full_qual = true, .svc = "system/svc/restarter", .inst = "default"};
    return &path_restartd;
}

S16Path * S16PathOfRepository ()
{
    static S16Path path_configd = {
        .full_qual = true, .svc = "system/svc/repository", .inst = "default"};
    return &path_configd;
}

S16Path * S16PathOfGrapher ()
{
    static S16Path path_graphd = {
        .full_qual = true, .svc = "system/svc/graph-engine", .inst = "default"};
    return &path_graphd;
}

/* Dependency group functions */
void s16_depgroup_destroy (S16DependencyGroup * depgroup)
{
    if (depgroup->name)
        free (depgroup->name);
    path_list_deepdestroy (&depgroup->paths, S16PathDestroy);
    free (depgroup);
}
/* Makes a deep copy of a depgroup. */
S16DependencyGroup * s16_depgroup_copy (const S16DependencyGroup * depgroup)
{
    S16DependencyGroup * r = malloc (sizeof (S16DependencyGroup));
    r->name = depgroup->name ? strdup (depgroup->name) : NULL;
    r->type = depgroup->type;
    r->restart_on = depgroup->restart_on;
    r->paths = path_list_map (&depgroup->paths, S16PathCopy);
    return r;
}
/* Returns true if b's name matches that of a. */
bool s16_depgroup_name_equal (const S16DependencyGroup * a,
                              const S16DependencyGroup * b)
{
    return a->name && b->name && !strcmp (a->name, b->name);
}

/* Property functions */
void s16_prop_destroy (S16Property * prop)
{
    free (prop->name);
    if (prop->type == PROP_STRING)
        free (prop->value.s);
    free (prop);
}

S16Property * s16_prop_copy (const S16Property * prop)
{
    S16Property * r = malloc (sizeof (S16Property));
    r->name = strdup (prop->name);
    r->type = prop->type;
    if (prop->type == PROP_STRING)
        r->value.s = strdup (prop->value.s);
    else
        r->value.i = prop->value.i;
    return r;
}

bool s16_prop_name_equal (const S16Property * a, const S16Property * b)
{
    return !strcmp (a->name, b->name);
}

/* Method functions */
S16ServiceMethod * s16_meth_copy (const S16ServiceMethod * meth)
{
    S16ServiceMethod * r = malloc (sizeof (S16ServiceMethod));
    r->name = strdup (meth->name);
    r->props = prop_list_map (&meth->props, s16_prop_copy);
    return r;
}

bool s16_meth_name_equal (const S16ServiceMethod * a,
                          const S16ServiceMethod * b)
{
    return !strcmp (a->name, b->name);
}

void s16_meth_destroy (S16ServiceMethod * meth)
{
    free (meth->name);
    prop_list_deepdestroy (&meth->props, s16_prop_destroy);
    free (meth);
}

/* Instance functions */
/* Destroys an instance. */
void s16_inst_destroy (S16ServiceInstance * inst)
{
    S16PathDestroy (inst->path);
    prop_list_deepdestroy (&inst->props, s16_prop_destroy);
    meth_list_deepdestroy (&inst->meths, s16_meth_destroy);
    depgroup_list_deepdestroy (&inst->depgroups, s16_depgroup_destroy);
    free (inst);
}
/* Makes a deep copy of a instance. */
S16ServiceInstance * s16_inst_copy (const S16ServiceInstance * inst)
{
    S16ServiceInstance * r = malloc (sizeof (S16ServiceInstance));
    r->path = S16PathCopy (inst->path);
    r->props = prop_list_map (&inst->props, s16_prop_copy);
    r->meths = meth_list_map (&inst->meths, s16_meth_copy);
    r->depgroups = depgroup_list_map (&inst->depgroups, s16_depgroup_copy);
    r->state = inst->state;
    return r;
}
/* Returns true if b's name matches that of a. */
bool s16_inst_name_equal (const S16ServiceInstance * a,
                          const S16ServiceInstance * b)
{
    return !strcmp (a->path->inst, b->path->inst);
}

/* Service functions */

S16Service * s16_svc_alloc ()
{
    S16Service * r = calloc (1, sizeof (S16Service));
    r->props = prop_list_new ();
    r->meths = meth_list_new ();
    r->insts = inst_list_new ();
    r->depgroups = depgroup_list_new ();
    return r;
}

S16Service * s16_svc_copy (const S16Service * svc)
{
    S16Service * r = malloc (sizeof (S16Service));
    r->path = S16PathCopy (svc->path);
    r->def_inst = svc->def_inst ? strdup (svc->def_inst) : NULL;
    r->props = prop_list_map (&svc->props, s16_prop_copy);
    r->meths = meth_list_map (&svc->meths, s16_meth_copy);
    r->insts = inst_list_map (&svc->insts, s16_inst_copy);
    r->depgroups = depgroup_list_map (&svc->depgroups, s16_depgroup_copy);
    r->state = svc->state;
    return r;
}

void s16_svc_destroy (S16Service * svc)
{
    S16PathDestroy (svc->path);
    if (svc->def_inst)
        free (svc->def_inst);
    prop_list_deepdestroy (&svc->props, s16_prop_destroy);
    meth_list_deepdestroy (&svc->meths, s16_meth_destroy);
    inst_list_deepdestroy (&svc->insts, s16_inst_destroy);
    depgroup_list_deepdestroy (&svc->depgroups, s16_depgroup_destroy);
    free (svc);
}

bool s16_svc_name_equal (const S16Service * a, const S16Service * b)
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

void S16LogInit (const char * name) { progname = name; }

static void print_prefix (S16LogLevel level, const S16Path * svc)
{
    time_t rawtime;
    struct tm timeinfo;
    char time_str[26];
    char * spath = svc ? S16PathToString (svc) : NULL;
    const char * pfx = "";

    if (level == kS16LogError)
        pfx = KRED "ERROR: " KNRM;
    else if (level == kS16LogWarn)
        pfx = KYEL "WARNING: " KNRM;

    time (&rawtime);
    localtime_r (&rawtime, &timeinfo);
    asctime_r (&timeinfo, time_str);
    time_str[strlen (time_str) - 1] = '\0';

    if (spath)
    {
        printf (KWHT "[%s] "
                     "%s " KBLU "(%s)" KNRM ": %s ",
                time_str,
                progname,
                spath,
                pfx);
        free (spath);
    }
    else
        printf (KWHT "[%s] " KNRM "%s: " KNRM "%s", time_str, progname, pfx);
}

void S16Log (S16LogLevel level, const char * fmt, ...)
{
    va_list args;
    va_start (args, fmt);
    print_prefix (level, NULL);
    vprintf (fmt, args);
    va_end (args);
}

void S16LogPath (S16LogLevel level, const S16Path * path, const char * fmt, ...)
{
    va_list args;

    va_start (args, fmt);
    print_prefix (level, path);
    vprintf (fmt, args);
    va_end (args);
}

void S16LogService (S16LogLevel level, const S16Service * svc, const char * fmt,
                    ...)
{
    va_list args;

    va_start (args, fmt);
    print_prefix (level, svc->path);
    vprintf (fmt, args);
    va_end (args);
}

void S16LogInstance (S16LogLevel level, const S16ServiceInstance * inst,
                     const char * fmt, ...)
{
    va_list args;

    va_start (args, fmt);
    print_prefix (level, inst->path);
    vprintf (fmt, args);
    va_end (args);
}

void S16CloseOnExec (int fd)
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

void S16HandleSignalWithKQueue (int kq, int sig)
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