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

#ifndef S16_CORE_H_
#define S16_CORE_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <time.h>

#include "S16/Core.h"

#define S16_CONFIGD_BINARY S16_LIBEXECDIR "/s16.configd"

    typedef enum svc_state_e
    {
        S_NONE,
        S_UNINIT,
        S_DISABLED,
        S_OFFLINE,
        S_MAINTENANCE,
        S_ONLINE,
        S_DEGRADED,
        S_MAX,
    } svc_state_t;

    struct path_s
    {
        bool full_qual;
        char * svc;
        char * inst;
    };

    S16List (path, path_t *);

    typedef enum depgroup_type_s
    {
        REQUIRE_ANY,
        REQUIRE_ALL,
        OPTIONAL_ALL,
        EXCLUDE_ALL,
    } depgroup_type_t;

    typedef enum depgroup_restarton_s
    {
        ON_NONE,
        ON_ERROR,
        ON_RESTART,
        ON_REFRESH,
        ON_ANY,
    } depgroup_restarton_t;

    typedef struct depgroup_s
    {
        char * name;
        depgroup_type_t type;
        depgroup_restarton_t restart_on;
        path_list_t paths;
    } depgroup_t;

    S16List (depgroup, depgroup_t *);

    typedef struct property_s
    {
        char * name;

        enum
        {
            PROP_STRING,
            PROP_NUMBER,
            PROP_BOOLEAN,
        } type;

        union {
            long i;
            char * s;
        } value;
    } property_t;

    S16List (prop, property_t *);

    /* TODO: think about how delegated restarters may want their methods to
     * look. */
    typedef struct method_s
    {
        char * name;
        // char * exec;
        prop_list_t props;
    } method_t;

    S16List (meth, method_t *);

    struct svc_instance_s
    {
        path_t * path;

        prop_list_t props;
        meth_list_t meths;
        depgroup_list_t depgroups;

        bool enabled;
        svc_state_t state;
    };

    S16List (inst, svc_instance_t *);

    struct svc_s
    {
        /* Each service has a path. */
        path_t * path;
        /* It also has a default instance. If unset, the default is 'default';
         * and if no instance named 'default' exists, then one is created. */
        char * def_inst;
        /* It also has a list of properties, */
        prop_list_t props;
        /* a list of methods, */
        meth_list_t meths;
        /* a list of instances, */
        inst_list_t insts;
        /* a list of dependency groups, */
        depgroup_list_t depgroups;

        /* and finally, a state. */
        svc_state_t state;
    };

    S16List (svc, svc_t *);

    /* State functions. */
    /* Gets string describing state. */
    const char * s16_state_to_string (svc_state_t state);

    /* Path functions */
    /* Creates a new path. */
    path_t * s16_path_new (const char * svc, const char * inst);
    /* Destroys a path. */
    void s16_path_destroy (path_t * path);
    /* Copies a path. */
    path_t * s16_path_copy (const path_t * path);
    /* Tests whether two paths are equal. */
    bool s16_path_equal (const path_t * a, const path_t * b);
    /* Tests whether a path leads to an instance. */
    bool s16_path_is_inst (const path_t * p);
    /* Creates a new path to the parent service underlying an instance path. */
    path_t * s16_svc_path_from_inst_path (const path_t * path);
    /* Creates a string path from a path. */
    char * s16_path_to_string (const path_t * path);

    /* Retrieves the path of the master restarter. */
    path_t * s16_path_restartd ();
    /* Retrieves the path of the service repository. */
    path_t * s16_path_configd ();
    /* Retrieves the path of the graphing service. */
    path_t * s16_path_graphd ();

    /* Dependency-group functions */
    /* Destroys a depgroup. */
    void s16_depgroup_destroy (depgroup_t * depgroup);
    /* Makes a deep copy of a depgroup. */
    depgroup_t * s16_depgroup_copy (const depgroup_t * depgroup);
    /* Returns true if b's name matches that of a. */
    bool s16_depgroup_name_equal (const depgroup_t * a, const depgroup_t * b);

    /* Property functions */
    /* Destroys a property. */
    void s16_prop_destroy (property_t * prop);
    /* Makes a deep copy of a property. */
    property_t * s16_prop_copy (const property_t * prop);
    /* Returns true if b's name matches that of a. */
    bool s16_prop_name_equal (const property_t * a, const property_t * b);

    /* Method functions */
    /* Destroys a method. */
    void s16_meth_destroy (method_t * meth);
    /* Makes a deep copy of a method. */
    method_t * s16_meth_copy (const method_t * meth);
    /* Returns true if b's name matches that of a. */
    bool s16_meth_name_equal (const method_t * a, const method_t * b);

    /* Instance functions */
    /* Destroys an instance. */
    void s16_inst_destroy (svc_instance_t * inst);
    /* Makes a deep copy of a instance. */
    svc_instance_t * s16_inst_copy (const svc_instance_t * inst);
    /* Returns true if b's name matches that of a. */
    bool s16_inst_name_equal (const svc_instance_t * a,
                              const svc_instance_t * b);

    /* Service functions */
    /* Allocates and initialises fields of a new service.*/
    svc_t * s16_svc_alloc ();
    /* Makes a deep copy of a service. */
    svc_t * s16_svc_copy (const svc_t * svc);
    /* Destroys a service. */
    void s16_svc_destroy (svc_t * svc);
    /* Returns true if b's name matches that of a. */
    bool s16_svc_name_equal (const svc_t * a, const svc_t * b);

#ifdef __cplusplus
}
#endif

#endif
