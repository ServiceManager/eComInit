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

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>
#include <time.h>
#include <unistd.h>

#include "manager.h"
#include "s16db.h"
#include "s16rr.h"

const int NOTE_RREQ = 18;

Manager manager;

void manager_configd_came_up ()
{
    /* FIXME: Use an EVFILT_VNODE on the repo dir or something instead of
     * sleeping. */
    usleep (10000);
    if ((s16db_hdl_new (&manager.h)))
    {
        perror ("Failed to connect to repository");
    }
}

Unit * manager_find_unit_for_pid (pid_t pid)
{
    LL_each (&manager.units, it)
    {
        if (unit_has_pid (it->val, pid))
            return it->val;
    }
    return NULL;
}

int main (int argc, char * argv[])
{
    bool repo_up;
    struct kevent sigev;
    struct timespec tmout = {0, 0}; /* return at once initially */

    s16_log_init ("Master Restarter");

    manager.kq = kqueue ();
    if (manager.kq == -1)
        perror ("KQueue: Failed to open\n");

    manager.pt = pt_new (manager.kq);
    timerset_init (&manager.ts, manager.kq);

    if ((s16db_hdl_new (&manager.h)))
    {
        perror ("Failed to connect to repository");
        repo_up = false;
    }

    signal (SIGCHLD, SIG_IGN);

    EV_SET (&sigev, SIGHUP, EVFILT_SIGNAL, EV_ADD, 0, 0, 0);

    if (kevent (manager.kq, &sigev, 1, 0, 0, 0) == -1)
    {
        perror ("KQueue: Failed to set signal event\n");
        exit (EXIT_FAILURE);
    }

    if (!repo_up)
    {
        Unit * configd = unit_add (s16_path_configd ());
        s16note_t * note =
            s16note_new (N_RESTARTER_REQ, RR_START, configd->path, 0);

        configd->type = U_SIMPLE;
        configd->methods[UM_PRESTART] = "/usr/bin/echo PRESTART";
        configd->methods[UM_START] = "/usr/bin/sleep 10";
        configd->methods[UM_POSTSTART] = "/usr/bin/echo POSTSTART";
        configd->methods[UM_STOP] = "/usr/bin/echo STOP";
        configd->state = US_OFFLINE;

        unit_msg (configd, note);
        s16note_destroy (note);
    }

    while (1)
    {
        struct kevent ev;
        pt_info_t * info;

        memset (&ev, 0x00, sizeof (struct kevent));

        if (kevent (manager.kq, NULL, 0, &ev, 1, &tmout) == -1)
        {
            perror ("KQueue: Failed to wait for events\n");
            exit (EXIT_FAILURE);
        }

        tmout.tv_sec = 3;

        if ((info = pt_investigate_kevent (manager.pt, &ev)))
        {
            Unit * unit = manager_find_unit_for_pid (info->pid);

            if (!unit)
                unit = manager_find_unit_for_pid (info->ppid);

            if (!unit)
                fprintf (stderr, "error: no unit associated with pid %d\n",
                         info->pid);
            else
                unit_ptevent (unit, info);

            s16mem_free (info);
        }

        timerset_investigate_kevent (&manager.ts, &ev);

        switch (ev.filter)
        {
        case EVFILT_SIGNAL:
            fprintf (stderr, "Signal received: %lu. Additional data: %ld\n",
                     ev.ident, ev.data);
            if (ev.ident == SIGCHLD)
                while (waitpid ((pid_t) (-1), 0, WNOHANG) > 0)
                    ;
            break;
        }
    }

    pt_destroy (manager.pt);
    close (manager.kq);

    return 0;
}
