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
 * Desc: This file holds the logic for 'units', which are the master restarter's
 * representation of service instances.
 * Some things to consider:
 *  - If we fail to get a PID from a fork operation, we go straight to
 *   maintenance as failing to fork is an exceptional case. It may be better not
 *   to do so.
 */

#include <assert.h>
#include <sys/signal.h>

#include "manager.h"
#include "unit.h"

#define UnitTimerReg()                                                         \
    unit->timer_id = timerset_add (&manager.ts, 2000, unit, unit_timer_event);

#define DbgEnteringState(x)                                                    \
    s16_log_path (INFO, unit->path, "Unit targetting state %s\n", #x);
#define DbgEnteredState(x)                                                     \
    s16_log_path (INFO, unit->path, "Unit arrived at state %s\n", #x);

#define EStateProto(state) void unit_enter_##state (Unit * unit)

EStateProto (maintenance);
EStateProto (offline);
EStateProto (prestart);
EStateProto (start);
EStateProto (poststart);
EStateProto (online);
EStateProto (stop);
EStateProto (stopterm);
EStateProto (stopkill);

void unit_enter_state (Unit * unit, UnitState state);
void unit_timer_event (long id, void * data);

void fork_cleanup_cb (void * data)
{
    setenv ("NOTIFY_SOCKET", NOTIFY_SOCKET_PATH, 1);
    manager_fork_cleanup ();
}

UnitMethodType state_to_method_type (UnitState state)
{
    /* clang-format off */
    return state == US_PRESTART  ? UM_PRESTART
         : state == US_START     ? UM_START
         : state == US_POSTSTART ? UM_POSTSTART
         : state == UM_STOP      ? UM_STOP
         : state == UM_POSTSTOP  ? UM_POSTSTOP
         : -1;
    /* clang-format on */
}

bool unit_stopping (Unit * unit)
{
    return unit->state == US_STOP || unit->state == US_STOPTERM ||
           unit->state == US_STOPKILL || unit->state == US_POSTSTOP;
}

/* Remove the given PID from our watch. */
void unit_deregister_pid (Unit * unit, pid_t pid)
{
    pt_disregard_pid (manager.pt, pid);
    pid_list_del (&unit->pids, pid);
}

/* 0 for failure, valid PID for success */
pid_t unit_fork_and_register (Unit * unit, const char * cmd)
{
    process_wait_t * pwait = process_fork_wait (cmd, fork_cleanup_cb, NULL);
    pid_t ret = 0;

    if (pwait == NULL || pwait->pid == 0)
    {
        s16_log_path (ERR, unit->path, "failed to fork for command %s\n", cmd);
        return ret;
    }

    ret = pwait->pid;
    s16_log_path (DBG, unit->path, "Child PID: %d\n", ret);
    pt_watch_pid (manager.pt, ret);
    pid_list_add (&unit->pids, ret);
    process_fork_continue (pwait);

    return ret;
}

/* Purge all PIDs if necessary then enter the given target.
 * Does not execute the stop method. */
void unit_purge_and_target (Unit * unit)
{
    LL_print (&unit->pids);
    if (!LL_empty (&unit->pids))
    {
        printf ("unit_purge_and_target: First clearing all old PIDs.\n");
        unit_enter_stop /*term*/ (unit);
    }
    else
        unit_enter_state (unit, unit->target);
}

static void unit_restart_begin_cb (long id, void * data)
{
    Unit * unit = data;
    unit->meth_restart_timer_id = 0;
    timerset_del (&manager.ts, id);
    unit_enter_prestart (unit);
}

/* As unit_purge_and_target, but delayed. */
void unit_retry_start (Unit * unit, int msecs)
{
    printf ("UNit retry start\n");
    unit->target = US_NONE;
    unit_purge_and_target (unit);
    unit->meth_restart_timer_id =
        timerset_add (&manager.ts, 500, unit, unit_restart_begin_cb);
}

void unit_enter_maintenance (Unit * unit)
{
    DbgEnteredState (Maintenance);
    unit->state = US_MAINTENANCE;
}

void unit_enter_offline (Unit * unit)
{
    DbgEnteredState (Offline);
    unit->state = US_OFFLINE;
}

void unit_enter_prestart (Unit * unit)
{
    if (unit->methods[UM_PRESTART])
    {
        DbgEnteredState (PreStart);
        unit->state = US_PRESTART;
        UnitTimerReg ();
        unit->main_pid =
            unit_fork_and_register (unit, unit->methods[UM_PRESTART]);
        if (!unit->main_pid)
        {
            unit->target = US_MAINTENANCE;
            unit_purge_and_target (unit);
            return;
        }
    }
    else
        unit_enter_start (unit);
}

void unit_enter_start (Unit * unit)
{
    /* deal with forking case later */

    DbgEnteredState (Start);

    /*if (unit->type == U_FORKS)
    {
        if (unit->pidfile)
        {
            UnitPIDFileWatchReg ();
            if (!unit->dirwatch_id)
            {
                printf ("Erecting dirwatch for unit failed\n");
                timer_del (unit->timer_id);
                unit->target = US_MAINTENANCE;
                unit_purge_and_target (unit);
            }
        }
    }*/

    unit->state = US_START;
    unit->main_pid = unit_fork_and_register (unit, unit->methods[UM_START]);
    if (!unit->main_pid)
    {
        unit->target = US_MAINTENANCE;
        unit_purge_and_target (unit);
        return;
    }
    if (unit->type == U_SIMPLE || unit->type == U_ONESHOT ||
        unit->type == U_GROUP)
        /* in these kinds of service, we consider it online as soon as the
         * process specified in the start method is running */
        unit_enter_poststart (unit);
    else
    {
        /* begin the timer for the process to fork within */
        // UnitTimerReg ();
    }
}

void unit_enter_poststart (Unit * unit)
{
    if (unit->methods[UM_POSTSTART])
    {
        DbgEnteredState (Poststart);
        unit->state = US_POSTSTART;
        UnitTimerReg ();
        unit->secondary_pid =
            unit_fork_and_register (unit, unit->methods[UM_POSTSTART]);
        if (!unit->secondary_pid)
        {
            unit->target = US_MAINTENANCE;
            unit_purge_and_target (unit);
        }
    }
    else
        unit_enter_online (unit);
}

void unit_enter_online (Unit * unit)
{
    DbgEnteredState (Online);
    /* Special logic for core services. */
    if (unit->path == s16_path_configd ())
        manager_configd_came_up ();
}

void unit_enter_stop (Unit * unit)
{
    /* if the unit has a `stop` method, try that */
    if (unit->methods[UM_STOP])
    {
        DbgEnteredState (Stop);
        unit->state = US_STOP;
        UnitTimerReg ();
        unit->secondary_pid =
            unit_fork_and_register (unit, unit->methods[UM_STOP]);
        /* if no pid emerged, fork failed; immediately go to maintenance after
         * clearing any remaining processes */
        if (!unit->secondary_pid)
        {
            unit->target = US_MAINTENANCE;
            unit_purge_and_target (unit);
            return;
        }
    }
    else
        unit_enter_stopterm (unit);
}

void unit_enter_stopterm (Unit * unit)
{
    if (LL_empty (&unit->pids))
    {
        unit_enter_state (unit, unit->target);
        return;
    }

    DbgEnteredState (Stopterm);
    unit->state = US_STOPTERM;
    /* first, just try to kill the main PID */
    if (unit->main_pid)
        kill (unit->main_pid, SIGTERM);
    UnitTimerReg ();
    /* now the rest */
    LL_each (&unit->pids, it) { kill (it->val, SIGTERM); }

    /* FIXME: do we need this? note_awake (); */ /* prepare for more events */
}

void unit_enter_stopkill (Unit * unit)
{
    if (LL_empty (&unit->pids))
    {
        unit_enter_state (unit, unit->target);
        return;
    }

    DbgEnteredState (Stopkill);
    unit->state = US_STOPKILL;

    /* first, just try to kill the main PID */
    if (unit->main_pid)
        kill (unit->main_pid, SIGKILL);

    /* We should NEVER time out in this state, so if we do, something bad has
     * happened */
    UnitTimerReg ();
    /* now the rest */
    LL_each (&unit->pids, it) { kill (it->val, SIGKILL); }

    /* FIXME: do we need this? note_awake (); */ /* prepare for more events */
}

void unit_enter_state (Unit * unit, UnitState state)
{
    switch (state)
    {
    case US_OFFLINE:
        unit_enter_offline (unit);
        break;
    case US_MAINTENANCE:
        unit_enter_maintenance (unit);
        break;
    case US_PRESTART:
        unit_enter_prestart (unit);
        break;
    case US_START:
        unit_enter_start (unit);
        break;
    case US_POSTSTART:
        unit_enter_poststart (unit);
        break;
    case US_ONLINE:
        unit_enter_online (unit);
        break;
    case US_NONE:
        unit->state = US_NONE;
    }
}

void unit_ptevent (Unit * unit, pt_info_t * info)
{
    /* First of all, make sure to add/remove the PID from our list. */
    if (info->event == PT_CHILD)
    {
        if (unit_has_pid (unit, info->pid))
        {
            printf ("Not tracking %lu as we already do.\n",
                    (uintptr_t)info->pid);
        }
        else
        {
            /* isn't this done anyway by ptracker? */
            pt_watch_pid (manager.pt, info->pid);
            pid_list_add (&unit->pids, info->pid);
        }
    }
    else if (info->event == PT_EXIT)
    {
        printf ("Deregistering %lu...\n", (uintptr_t)info->pid);
        unit_deregister_pid (unit, info->pid);
    }

    /* first of all, handle stop-related transitions */
    if (info->event == PT_EXIT && unit_stopping (unit))
    {
        switch (unit->state)
        {
        case US_STOP:
            if (LL_empty (&unit->pids))
            {
                /* clear the stop method timer */
                timerset_del (&manager.ts, unit->timer_id);
                unit_enter_stopterm (unit);
            }
            break;
        case US_STOPTERM:
            if (LL_empty (&unit->pids))
            {
                /* clear the stop method timer */
                timerset_del (&manager.ts, unit->timer_id);
                unit_enter_stopkill (unit);
            }
        }
    }
    else if (info->event == PT_EXIT && info->pid == unit->main_pid)
    { /* main PID has exited */
        unit->main_pid = 0;
        if (unit->timer_id)
            timerset_del (&manager.ts, unit->timer_id);

        printf ("Main PID exited\n");
        /* if exit was S16_FATAL, go to maintenance instead - add this later
         */
        if (exit_was_abnormal (info->flags))
        {
            fprintf (stderr, "Bad exit in a main pid\n");
            /*if (unit->rtype == R_NO)
                unit->target = US_OFFLINE;*/
            /* if we were online, we'll go to offline, and then the graph
             * engine will tell us what to do. */
            /*else*/
            if (unit->state == US_ONLINE)
            {
                /* add fail time to ringbuffer */
                unit->target = US_OFFLINE;
                unit_purge_and_target (unit);
            }
            else
            {
                int * fail_cnt =
                    &unit->fail_cnt[state_to_method_type (unit->state)];

                (*fail_cnt)++;

                if (*fail_cnt > 5)
                {
                    s16_log_path (
                        ERR, unit->path,
                        "Transitioning to maintenance because: Method "
                        "failed more than 5 times\n");
                    unit->target = US_MAINTENANCE;
                    unit_purge_and_target (unit);
                }
                else
                {
                    unit->target = unit->state;
                    unit_retry_start (unit, 5000);
                }
            }
        }
        else
        {
            switch (unit->state)
            {
            case US_PRESTART:
                /* the main PID exited from prestart; we'll clean up any
                 * remnants and then enter the start state. */
                unit->target = US_START;
                printf ("Entering start as prestart is done.\n");

                unit_purge_and_target (unit);
                break;

            case US_ONLINE:
            case US_POSTSTART:
                /* we're in poststart or online, and the main PID of our
                 * service has quit.
                 */
                if (unit->type == U_SIMPLE)
                {
                    printf ("Online and SIMPLE and Main PID exited.\n");
                    unit->target = US_OFFLINE;
                    unit_enter_stop (unit);
                }
                else if (unit->type != U_GROUP && LL_empty (&unit->pids))
                {
                    /*if (unit->rtype == R_YES)
                        unit->target = US_PRESTART;
                    else*/
                    unit->target = US_OFFLINE;
                    unit_enter_stop (unit);
                }
                break;
            }
            return;
        }
    }
    else if (info->event == PT_EXIT && info->pid == unit->secondary_pid)
    {
        if (unit->state == US_POSTSTART)
        {
            if (unit->timer_id)
                timerset_del (&manager.ts, unit->timer_id);
            unit->secondary_pid = 0;

            if (exit_was_abnormal (info->flags))
            {
                fprintf (stderr, "Bad exit in a secondary pid\n");
                /*if (unit->rtype == R_NO)
                    unit->target = US_OFFLINE;*/
                int * fail_cnt =
                    &unit->fail_cnt[state_to_method_type (unit->state)];

                (*fail_cnt)++;

                if (*fail_cnt > 5)
                {
                    s16_log_path (
                        ERR, unit->path,
                        "Transitioning to maintenance because: Method "
                        "failed more than 5 times\n");
                    unit->target = US_MAINTENANCE;
                    unit_purge_and_target (unit);
                }
                else
                {
                    unit->target = US_PRESTART;
                    unit_retry_start (unit, 5000);
                }
            }
            else /* successful exit from PostStart method */
            {
                unit_enter_online (unit);
            }
        }
    }
}

void unit_timer_event (long id, void * data)
{
    Unit * unit = data;
    unit->timer_id = 0;
    timerset_del (&manager.ts, id);
    printf ("Timer in state %d\n", unit->state);

    switch (unit->state)
    {

    case US_STOP:
    {
        s16_log_path (WARN, unit->path, "Stop method timed out.\n");
        unit_enter_stopterm (unit);
        return;
    }

    case US_STOPTERM:
    {
        fprintf (stderr, "timeout in stopterm\n");
        // unit_enter_stopkill (unit);
        return;
    }
    case US_STOPKILL:
    {
        fprintf (stderr, "timeout in stopkill(!)\n");
        unit_enter_state (unit, unit->target);
        return;
    }
    case US_PRESTART:
    {
        fprintf (stderr, "timeout in prestart\n");
        unit->fail_cnt[UM_PRESTART]++;

        if (unit->fail_cnt[UM_PRESTART] > 5)
        {
            s16_log_path (ERR, unit->path,
                          "Transitioning to maintenance because: Method "
                          "timedout/failed 5 times in a row.");
            unit->target = US_MAINTENANCE;
            unit_purge_and_target (unit);
        }
        else
        {
            unit->target = US_PRESTART;
            unit_purge_and_target (unit);
        }

        /*if (unit->rtype == R_YES)
        {
            fprintf (stderr, "restarting for prestart\n");
            unit_enter_prestart (unit);
        }*/
        // else
        /*{

        }*/
        return;
    }
    case US_START:
    {
        fprintf (stderr, "timeout in start\n");
    }
    }
}

bool unit_has_pid (Unit * unit, pid_t pid)
{
    LL_each (&unit->pids, it)
    {
        if (it->val == pid)
            return true;
    }
    return false;
}

Unit * unit_add (path_t * path)
{
    Unit * unit = calloc (1, sizeof (Unit));

    unit->path = path;
    unit->pids = pid_list_new ();
    unit->state = US_UNINITIALISED;

    Unit_list_add (&manager.units, unit);

    return unit;
}

/* Sends a note to the given unit. */
void unit_msg (Unit * unit, s16note_t * note)
{
    s16note_rreq_type_t type = note->type;
    switch (type)
    {
    case RR_START:
    {
        s16_log_path (INFO, unit->path, "Received request to bring up.\n");
        unit_enter_prestart (unit);
    }
    }
}
