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

/* Desc: This is a POSIX-compliant process-tracker driver.
 * It can only detect immediate childrens' exits, as it uses
 * SIGCHLD only. */

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/event.h>
#include <sys/types.h>

#include "S16/RestarterServices.h"
#include "list.h"

ListGenForNameType (pid, pid_t);

void * s16mem_alloc (unsigned long);
void s16mem_free (void *);

typedef struct process_tracker_s
{
    int kq;
    pid_list_t pids;
} process_tracker_t;

process_tracker_t * pt_new (int kq)
{
    process_tracker_t * pt = s16mem_alloc (sizeof (process_tracker_t));
    pt->kq = kq;
    pt->pids = pid_list_new ();
    return pt;
}

int pt_watch_pid (process_tracker_t * pt, pid_t pid)
{
    pid_list_add (&pt->pids, pid);
    return 0;
}

void pt_disregard_pid (process_tracker_t * pt, pid_t pid) { return; }

pt_info_t * pt_investigate_kevent (process_tracker_t * pt, struct kevent * ke)
{
    pt_info_t * result;
    pt_info_t info;

    if (ke->filter != EVFILT_SIGNAL)
        return 0;

    if (ke->ident == SIGCHLD)
    {
        info.event = PT_EXIT;
        info.pid = waitpid ((pid_t) (-1), (int *)&info.flags, WNOHANG);
        info.ppid = 0;
    }
    else
        return 0;

    result = s16mem_alloc (sizeof (pt_info_t));
    *result = info;

    return result;
}

void pt_destroy (process_tracker_t * pt)
{
    for (pid_list_it it = pid_list_begin (&pt->pids); it != NULL;
         it = pid_list_it_next (it))
    {
        pt_disregard_pid (pt, it->val);
    }

    pid_list_destroy (&pt->pids);
    s16mem_free (pt);
}
