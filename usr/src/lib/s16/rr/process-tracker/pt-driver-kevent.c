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
#include <sys/event.h>
#include <sys/types.h>

#include "S16/List.h"
#include "S16/RestarterServices.h"

void * s16mem_alloc (unsigned long);
void s16mem_free (void *);

typedef struct process_tracker_s
{
    int kq;
    pid_list_t pids;
} S16ProcessTracker;

S16ProcessTracker * S16ProcessTrackerNew (int kq)
{
    S16ProcessTracker * pt = s16mem_alloc (sizeof (S16ProcessTracker));
    pt->kq = kq;
    pt->pids = pid_list_new ();
    return pt;
}

int S16ProcessTrackerWatchPID (S16ProcessTracker * pt, pid_t pid)
{
    int i;
    struct kevent ke;

    EV_SET (&ke, pid, EVFILT_PROC, EV_ADD, NOTE_EXIT | NOTE_TRACK, 0, NULL);
    i = kevent (pt->kq, &ke, 1, NULL, 0, NULL);

    if (i == -1)
        fprintf (stderr,
                 "Error: failed to watch PID %d: %s\n",
                 pid,
                 strerror (errno));
    else
        pid_list_add (&pt->pids, pid);

    return i == -1 ? 1 : 0;
}

void S16ProcessTrackerDisregardPID (S16ProcessTracker * pt, pid_t pid)
{
    struct kevent ke;
    pid_list_it it;

    EV_SET (&ke, pid, EVFILT_PROC, EV_ADD, NOTE_EXIT | NOTE_TRACK, 0, NULL);
    kevent (pt->kq, &ke, 1, NULL, 0, NULL);

    pid_list_del (&pt->pids, pid);

    return;
}

S16ProcessTrackerEvent *
S16ProcessTrackerInvestigateKEvent (S16ProcessTracker * pt, struct kevent * ke)
{
    S16ProcessTrackerEvent * result;
    S16ProcessTrackerEvent info;

    if (ke->filter != EVFILT_PROC)
        return 0;

    if (ke->fflags & NOTE_CHILD)
    {
        printf ("new pid %d has %d as parent\n", ke->ident, ke->data);
        info.event = kS16ProcessTrackerEventTypeChild;
        info.pid = ke->ident;
        info.ppid = ke->data;

        pid_list_add (&pt->pids, ke->ident);
    }
    else if (ke->fflags & NOTE_EXIT)
    {
        printf ("pid %d exited\n", ke->ident);
        info.event = kS16ProcessTrackerEventTypeExit;
        info.pid = ke->ident;
        info.ppid = 0;
        info.flags = ke->data;

        pid_list_del (&pt->pids, ke->ident);
    }
    else
        return 0;

    result = s16mem_alloc (sizeof (S16ProcessTrackerEvent));
    *result = info;

    return result;
}

void S16ProcessTrackerDestroy (S16ProcessTracker * pt)
{
    list_foreach (pid, &pt->pids, it)
    {
        S16ProcessTrackerDisregardPID (pt, it->val);
    }

    pid_list_destroy (&pt->pids);
    s16mem_free (pt);
}
