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
 * Desc: The interface for the S16 Notifications Framework.
 * The Repository acts as a central receiver and dispatcher of notifications to
 * the various daemons of S16. Requests by user-facing tools are sent to the
 * repository, and other components of S16 consume those notifications. The
 * repository also consumes these notifications.
 */

#ifndef S16_NOTE_H_
#define S16_NOTE_H_

#include "S16/Service.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        /* An administrative request to alter service state.
         * Usually sent by: svcadm
         * Relevant to: Graphing Services */
        N_ADMIN_REQ = 1,
        /* A request for a restarter to change service state.
         * Usually sent by: Graphing Services
         * Relevant to: Restarters */
        N_RESTARTER_REQ = 2,
        /* A notification that a service has changed state.
         * Usually sent by: Restarters
         * Relevant to: Graphing Services */
        N_STATE_CHANGE = 4,
        /* A notification that service configuration has changed.
         * Usually sent by: the Repository
         * Relevant to: Everyone */
        N_CONFIG = 8
    } s16note_type_t;

    typedef enum s16note_admin_type_e
    {
        A_ENABLE,
        A_DISABLE,
        A_START,
        A_STOP,
        A_RESTART,
        A_REFRESH,
        A_CLEAR,
        A_MARK,
    } s16note_admin_type_t;

    typedef enum s16note_rreq_type_e
    {
        /* Add this instance. */
        RR_ADDINSTANCE,
        /* Enable, but don't try to start yet. */
        RR_ENABLE,
        RR_DISABLE,
        /* Start instance. */
        RR_START,
        /* Stop instance. */
        RR_STOP,
        /* Move instance to maintenance mode. */
        RR_MAINTAIN,
    } s16note_rreq_type_t;

    /* State change type */
    typedef enum s16note_sc_type_e
    {
        SC_ONLINE,
        SC_OFFLINE,
        SC_DISABLED,
    } s16note_sc_type_t;

    typedef struct s16note_s
    {
        /* The type of the note. */
        s16note_type_t note_type;
        /* The particular note type for this note_type. */
        int type;
        /* The affected instance. */
        path_t * path;
        /* The relevant reason for this note_type. */
        int reason;
    } s16note_t;

    S16List (s16note, s16note_t *);

    s16note_t * s16note_new (s16note_type_t note_type, int type,
                             const path_t * path, int reason);
    void s16note_destroy (s16note_t * note);

#ifdef __cplusplus
}
#endif

#endif
