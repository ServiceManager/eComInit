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

#ifndef S16_RR_H_
#define S16_RR_H_

#include "S16/List.h"

#include <sys/event.h>
#include <sys/types.h>
#include <sys/wait.h>

#ifdef __cplusplus
extern "C"
{
#endif

    S16ListType (pid, pid_t);

    /* process tracking structures */
    typedef enum
    {
        kS16ProcessTrackerEventTypeChild,
        kS16ProcessTrackerEventTypeExit,
    } S16ProcessTrackerEventType;

    /* This structure contains details of a process tracker event.
     * In the case of child, the field 'ppid' contains the parent pid.
     * In the case of exit, the field 'flags' contains the exit data. */
    typedef struct pt_info_s
    {
        S16ProcessTrackerEventType event;
        pid_t pid, ppid;
        long flags;
    } S16ProcessTrackerEvent;

    typedef struct process_wait_s
    {
        int fd[2];
        pid_t pid;
    } S16PendingProcess;

    typedef struct process_tracker_s S16ProcessTracker;

    /* subreaping routines */

    int S16SubreapingAcquire ();
    int S16SubreapingRelinquish ();
    int S16SubreapingStatus ();

    /* process basic routines */

    /* Forks a process for the command specified.
     * This process waits until S16PendingProcessContinue()
     * is called. */
    S16PendingProcess * S16ProcessForkAndWait (const char * cmd_,
                                               void (*cleanup_cb) (void *),
                                               void * cleanup_cb_arg);
    /* Tells the child process to continue.
     * This also frees the S16PendingProcess structure. */
    void S16PendingProcessContinue (S16PendingProcess * pwait);

    /* process tracking routines */

    /* Creates a new process tracker and returns a handle to it.
     * Passed the KQueue descriptor that it may register its events. */
    S16ProcessTracker * S16ProcessTrackerNew (int kq);

    /* Adds a PID to the watchlist. */
    int S16ProcessTrackerWatchPID (S16ProcessTracker * pt, pid_t pid);

    /* Removes a PID from the watchlist. */
    void S16ProcessTrackerDisregardPID (S16ProcessTracker * pt, pid_t pid);

    /*
     * Investigates a kevent, that it may determine if it contains
     * an event that is relevant to the process tracker.
     *
     * Returns null for irrelevant, and a pointer to a S16ProcessTrackerEvent
     * for a relevant process tracker event.
     *
     * In the event of a child or an exit, there is no need to manually
     * add or remove, respectively, that PID from the watchlist - this is
     * automatically done by the tracker.
     */
    S16ProcessTrackerEvent *
    S16ProcessTrackerInvestigateKEvent (S16ProcessTracker * pt,
                                        struct kevent * ke);

    /* Destroys a process tracker.
     * Requires KQueue descriptor to remove itself from the queue. */
    void S16ProcessTrackerDestroy (S16ProcessTracker * pt);

    /* misc routines */

    /* Returns 0 for a healthy exit, and either signal number or return code if
     * not.
     */
    int S16ExitWasAbnormal (int wstat);

    /* Reads a PID file.
     * Returns PID for success and 0 for fail. */
    pid_t S16PIDReadFromPIDFile (const char * path);

#ifdef __cplusplus
}
#endif

#endif
