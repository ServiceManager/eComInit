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

#ifndef GRAPHD_H_
#define GRAPHD_H_

#include "S16/Repository.h"

/* Vertex type */
typedef enum
{
    V_INST,
    V_SVC,
    V_DEPGROUP
} vertex_type_t;

typedef struct vertex_s vertex_t;
typedef struct edge_s edge_t;

S16List (vertex, vertex_t *);
S16List (edge, edge_t *);

/* Vertex */
struct vertex_s
{
    path_t * path;

    /* Type of vertex */
    vertex_type_t type;

    /* Dependency grouping type.
     * REQUIRE_ALL for instances (they need all their depgroups up.)
     * REQUIRE_ANY for services (they need at least one instance up - for now.)
     */
    depgroup_type_t dg_type;
    /* Restart-on type. */
    depgroup_restarton_t restart_on;

    edge_list_t dependencies;
    edge_list_t dependents;

    /* State of vertex */
    svc_state_t state;

    /* Has this vertex been set up with its dependencies? */
    bool is_setup : 1;
    /* Is this vertex enabled? */
    bool is_enabled : 1;
    /* Is this vertex to go offline? */
    bool to_offline : 1;
    /* Is this vertex to be disabled? */
    bool to_disable : 1;
};

struct edge_s
{
    vertex_t *from, *to;
};

/* Initialises the graph engine. */
void graph_init ();
/* Adds a new service to the graph. */
vertex_t * graph_install_service (svc_t * svc);
/* Sets up all vertices */
void graph_setup_all ();
/* Processes incoming notes. */
void graph_process_note (s16note_t * note);

extern s16db_hdl_t hdl;
/* Notifications received */
extern s16note_list_t notes;

#endif