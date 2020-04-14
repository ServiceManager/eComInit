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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/event.h>
#include <sys/signal.h>
#include <time.h>
#include <unistd.h>

#include "graphd.h"
#include "S16/Repository.h"

int kq;

s16db_scope_t scope;
s16db_hdl_t hdl;
s16note_list_t notes;

int main (int argc, char * argv[])
{
    kq = kqueue ();
    struct timespec tmout = {3, 0};

    s16_log_init ("S16 Graphing Service");

    if (kq == -1)
        perror ("KQueue: Failed to open\n");

    notes = s16note_list_new ();
    if (s16db_hdl_new (&hdl))
        perror ("Failed to connect to repository");

    graph_init ();

    for (svc_list_it it = list_begin (s16db_get_all_services (&hdl));
         it != NULL;
         it = list_next (it))
        graph_install_service (it->val);

    graph_setup_all ();

    s16db_subscribe (
        &hdl, kq, N_ADMIN_REQ | N_RESTARTER_REQ | N_STATE_CHANGE | N_CONFIG);

    while (1)
    {
        s16note_t * note = NULL;
        struct kevent ev;

        memset (&ev, 0x00, sizeof (struct kevent));

        if (kevent (kq, NULL, 0, &ev, 1, &tmout) == -1)
        {
            perror ("KQueue: Failed to wait for events\n");
            exit (EXIT_FAILURE);
        }

        s16db_investigate_kevent (&hdl, &ev);

        while ((note = s16db_get_note (&hdl)))
            graph_process_note (note);

        /* for testing purposes, internal note queue */
        while ((note = s16note_list_lpop (&notes)))
            graph_process_note (note);
    }

    close (kq);

    return 0;
}
