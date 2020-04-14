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
        kS16StateNone,
        kS16StateUninitialised,
        kS16StateDisabled,
        kS16StateOffline,
        kS16StateMaintenance,
        kS16StateOnline,
        kS16StateDegraded,
        kS16StateEnumMaximum,
    } S16ServiceState;

    struct path_s
    {
        bool full_qual;
        char * svc;
        char * inst;
    };

    S16ListType (path, S16Path *);

    typedef enum S16DependencyGroupype_s
    {
        kS16RequireAny,
        kS16RequireAll,
        kS16OptionalAll,
        kS16ExcludeAll,
    } S16DependencyGroupType;

    typedef enum depgroup_restarton_s
    {
        kS16RestartOnNone,
        kS16RestartOnError,
        kS16RestartOnRestart,
        kS16RestartOnRefresh,
        kS16RestartOnAny,
    } S16DependencyGroupRestartOnCondition;

    typedef struct depgroup_s
    {
        char * name;
        S16DependencyGroupType type;
        S16DependencyGroupRestartOnCondition restart_on;
        path_list_t paths;
    } S16DependencyGroup;

    S16ListType (depgroup, S16DependencyGroup *);

    typedef enum
    {
        kS16PropertyTypeString,
        kS16PropertyTypeNumber,
        kS16PropertyTypeBoolean,
    } S16PropertyType;

    typedef struct property_s
    {
        char * name;
        S16PropertyType type;

        union {
            long i;
            char * s;
        } value;
    } S16Property;

    S16ListType (prop, S16Property *);

    /* TODO: think about how delegated restarters may want their methods to
     * look. */
    typedef struct method_s
    {
        char * name;
        // char * exec;
        prop_list_t props;
    } S16ServiceMethod;

    S16ListType (meth, S16ServiceMethod *);

    struct svc_instance_s
    {
        S16Path * path;

        prop_list_t props;
        meth_list_t meths;
        depgroup_list_t depgroups;

        bool enabled;
        S16ServiceState state;
    };

    S16ListType (inst, S16ServiceInstance *);

    struct svc_s
    {
        /* Each service has a path. */
        S16Path * path;
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
        S16ServiceState state;
    };

    S16ListType (svc, S16Service *);

    /* State functions. */
    /* Gets string describing state. */
    const char * S16StateToString (S16ServiceState state);

    /* Path functions */
    /* Creates a new path. */
    S16Path * S16PathNew (const char * svc, const char * inst);
    /* Destroys a path. */
    void S16PathDestroy (S16Path * path);
    /* Copies a path. */
    S16Path * S16PathCopy (const S16Path * path);
    /* Tests whether two paths are equal. */
    bool S16PathEqual (const S16Path * a, const S16Path * b);
    /* Tests whether a path leads to an instance. */
    bool S16PathIsInstance (const S16Path * p);
    /* Creates a new path to the parent service underlying an instance path. */
    S16Path * S16ServicePathFromInstancePath (const S16Path * path);
    /* Creates a string path from a path. */
    char * S16PathToString (const S16Path * path);

    /* Retrieves the path of the master restarter. */
    S16Path * S16PathOfMainRestarter ();
    /* Retrieves the path of the service repository. */
    S16Path * S16PathOfRepository ();
    /* Retrieves the path of the graphing service. */
    S16Path * S16PathOfGrapher ();

    /* Dependency-group functions */
    /* Destroys a depgroup. */
    void S16DependencyGroupDestroy (S16DependencyGroup * depgroup);
    /* Makes a deep copy of a depgroup. */
    S16DependencyGroup *
    S16DependencyGroupCopy (const S16DependencyGroup * depgroup);
    /* Returns true if b's name matches that of a. */
    bool S16DependencyGroupNamesEqual (const S16DependencyGroup * a,
                                       const S16DependencyGroup * b);

    /* Property functions */
    /* Destroys a property. */
    void S16PropertyDestroy (S16Property * prop);
    /* Makes a deep copy of a property. */
    S16Property * S16PropertyCopy (const S16Property * prop);
    /* Returns true if b's name matches that of a. */
    bool S16PropertyNamesEqual (const S16Property * a, const S16Property * b);

    /* Method functions */
    /* Destroys a method. */
    void S16MethodDestroy (S16ServiceMethod * meth);
    /* Makes a deep copy of a method. */
    S16ServiceMethod * S16MethodCopy (const S16ServiceMethod * meth);
    /* Returns true if b's name matches that of a. */
    bool S16MethodNamesEqual (const S16ServiceMethod * a,
                              const S16ServiceMethod * b);

    /* Instance functions */
    /* Destroys an instance. */
    void S16InstanceDestroy (S16ServiceInstance * inst);
    /* Makes a deep copy of a instance. */
    S16ServiceInstance * S16InstanceCopy (const S16ServiceInstance * inst);
    /* Returns true if b's name matches that of a. */
    bool S16InstanceNamesEqual (const S16ServiceInstance * a,
                                const S16ServiceInstance * b);

    /* Service functions */
    /* Allocates and initialises fields of a new service.*/
    S16Service * S16ServiceAlloc ();
    /* Makes a deep copy of a service. */
    S16Service * S16ServiceCopy (const S16Service * svc);
    /* Destroys a service. */
    void S16ServiceDestroy (S16Service * svc);
    /* Returns true if b's name matches that of a. */
    bool S16ServiceNamesEqual (const S16Service * a, const S16Service * b);

#ifdef __cplusplus
}
#endif

#endif
