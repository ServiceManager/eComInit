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
 * Desc: This is the interface to the service repository. Consumers interact
 * with the repository generally by retrieving from it a merged service or
 * instance, which has had all the layers folded down.
 *
 */

#ifndef S16_DB_H_
#define S16_DB_H_

#include <string.h>

#include "s16clnt.h"
#include "s16note.h"
#include "s16srv.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define S16DB_CONFIGD_SOCKET_PATH "/var/tmp/configd"

    typedef struct s16note_sub_s s16note_sub_t;

    typedef enum s16db_errcode_e
    {
        /* Path is invalid */
        S16EBADPATH = 6000,
        S16ENOSUCHSVC = 6001,
        S16ENOSUCHINST = 6002,
    } s16db_errcode_t;

    typedef enum s16db_layer_e
    {
        L_MANIFEST,
        L_ADMIN,
    } s16db_layer_t;

    typedef struct s16db_scope_s
    {
        svc_list_t svcs;
    } s16db_scope_t;

    typedef struct s16db_hdl_s
    {
        /* Client to configd connection. */
        int fd;
        s16rpc_clnt_t clnt;
        /* If we are interested in notifications, then we are also an RPC
         * server, servicing only calls from configd. */
        s16rpc_srv_t * srv;
        /* Accordingly we have a notification queue. */
        s16note_list_t notes;
        /* A local set of all services (merged) is kept as it's sufficient for
         * most purposes. */
        s16db_scope_t scope;
    } s16db_hdl_t;

    typedef struct s16db_lookup_result_s
    {
        enum
        {
            NOTFOUND,
            SVC,
            INSTANCE,
        } type;

        union {
            svc_t * s;
            svc_instance_t * i;
        };

    } s16db_lookup_result_t;

    /* Repository functions. */
    /* Creates a new handle, connecting it to the repository.
     * Returns 0 if successful. */
    int s16db_hdl_new (s16db_hdl_t * hdl);

    /**********************************************************
     * Events related
     **********************************************************/
    /* If you are subscribed to notifications through an s16db handle, this
     * must be called whenever your KEvent loop receives an event. */
    void s16db_investigate_kevent (s16db_hdl_t * srv, struct kevent * ev);

    /* If subscribed to receive notes, calling this will take from the note
     * queue the last note received. */
    s16note_t * s16db_get_note (s16db_hdl_t * hdl);

    /* Messages to the repository: */
    /* Subscribe to receive notifications of the given kinds. */
    void s16db_subscribe (s16db_hdl_t * hdl, int kq,
                          int /* s16note_type_t */ kinds);

    /* Publish an event. */
    void s16db_publish (s16db_hdl_t * hdl, s16note_t * note);

    /**********************************************************
     * Immediate functions
     * These directly interface with the repository.
     **********************************************************/
    /* Imports a UCL-form service into the given layer. */
    int s16db_import_ucl_svc (s16db_hdl_t * hdl, struct ucl_object_s * usvc,
                              s16db_layer_t layer);
    /* Gets the state for the given instance path. */
    svc_state_t s16db_get_state (s16db_hdl_t * hdl, path_t * path);
    /* Sets the state for the given instance path.
     * Returns: 0 if successful. */
    int s16db_set_state (s16db_hdl_t * hdl, path_t * path, svc_state_t state);
    /* Disables the given path. If path is an instance, disables that
     * instance; if path is a service, disables all its instances. Otherwise
     * does nothing. Returns: 0 if successful. */
    int s16db_disable (s16db_hdl_t * hdl, path_t * path);

    /* Enables the given path. If path is an instance, enables that
     * instance; if path is a service, enables all its instances. Otherwise
     * does nothing. Returns: 0 if successful. */
    int s16db_enable (s16db_hdl_t * hdl, path_t * path);

    /**********************************************************
     * Permanent state
     * These work with the local cached scope.
     **********************************************************/
    /* Looks up whether the given instance is enabled. Returns true if it
     * is, and false if it isn't. */
    bool s16db_get_inst_enabled (s16db_hdl_t * hdl, path_t * path);

    /**********************************************************
     * Lookup
     **********************************************************/
    /* Retrieves a merged list of all services. */
    const svc_list_t * s16db_get_all_services (s16db_hdl_t * hdl);
    /* Retrieves the merged service or instance specified by the path.
     * r.type is ENOTFOUND if not found. */
    s16db_lookup_result_t s16db_lookup_path (s16db_hdl_t * hdl, path_t * path);

    /**********************************************************
     * Conversions
     **********************************************************/
    /* UCL to internal: */
    /* Converts a string path to a path. */
    path_t * s16db_string_to_path (const char * txt);
    path_t * s16db_ucl_to_path (const struct ucl_object_s * upath);
    /* Converts a UCL instance to an instance */
    svc_instance_t * s16db_ucl_to_inst (const struct ucl_object_s * obj);
    /* Converts a UCL manifest to a service. */
    svc_t * s16db_ucl_to_svc (const struct ucl_object_s * obj);
    /* Converts a UCL service array to a service list. */
    svc_list_t s16db_ucl_to_svcs (const struct ucl_object_s * usvcs);
    /* Converts a UCL notification to an S16 notification. */
    s16note_t * s16db_ucl_to_note (const struct ucl_object_s * unote);

    /* Internal to UCL: */
    struct ucl_object_s * s16db_path_to_ucl (path_t * path);
    struct ucl_object_s * s16db_inst_to_ucl (svc_instance_t * path);
    struct ucl_object_s * s16db_svc_to_ucl (svc_t * svc);
    struct ucl_object_s * s16db_note_to_ucl (const s16note_t * note);

#ifdef __cplusplus
}
#endif

#endif
