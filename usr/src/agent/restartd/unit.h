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

#ifndef UNIT_H_
#define UNIT_H_

#include "S16/Repository.h"
#include "S16/RestarterServices.h"

typedef enum
{
    U_SIMPLE,
    U_FORKS,
    U_ONESHOT,
    U_GROUP,
    U_NOTIFY,
} UnitType;

typedef enum
{
    /* Don't restart this unit if a method fails. */
    UR_NO,
    /* Restart if a method fails, but only if with a zero exit code. */
    UR_OK,
    /* Restart for any reason. */
    UR_YES,
} UnitRestart;

typedef enum
{
    US_NONE,
    US_UNINITIALISED,
    US_DISIABLED,
    US_OFFLINE,
    US_PRESTART,
    US_START,
    US_POSTSTART,
    US_ONLINE,
    US_EXITED, /* for Oneshot services */
    US_STOP,
    US_STOPTERM,
    US_STOPKILL,
    US_POSTSTOP,
    US_MAINTENANCE,
} UnitState;

typedef enum
{
    UM_PRESTART,
    UM_START,
    UM_POSTSTART,
    UM_STOP,
    UM_POSTSTOP,
    UM_MAX,
} UnitMethodType;

typedef struct
{
    UnitMethodType type;
    const char * exec;
} UnitMethod;

S16List (UnitMethod, UnitMethod *);

typedef struct
{
    /* Path of this instance. */
    path_t * path;
    UnitType type;
    UnitRestart to_restart;
    bool is_enabled;

    const char * methods[UM_MAX];

    /* Transient state */
    /* Current state of unit */
    UnitState state;
    /* Target state of unit */
    UnitState target;

    /* All PIDs associated with the unit. */
    pid_list_t pids;

    /* The main PID:
     * Immediate child at first, then in case of U_FORKING where a Pidfile is
     * available, read from there. */
    pid_t main_pid;
    /* Secondary PID. Just used for the poststart method. */
    pid_t secondary_pid;

    /* Failure count for each method. */
    int fail_cnt[UM_MAX];

    /* ID of primary watchdog timer */
    unsigned timer_id;
    /* ID of method-fail-restart delay timer */
    unsigned meth_restart_timer_id;
} Unit;

S16List (Unit, Unit *);

/* Adds a unit for the given path. If it already exists, returns that unit. */
Unit * unit_add (path_t * path);
/* Configures the unit with fresh data from the handle. */
void unit_setup (Unit * unit);

/* Sends a process-tracker event to the given unit. */
void unit_ptevent (Unit * unit, pt_info_t * info);
/* Sends a note to the given unit. */
void unit_msg (Unit * unit, s16note_t * note);
/* Notifies given unit that a readiness notification was received. */
void unit_notify_ready (Unit * unit);
/*
 * Notifies given unit of a new status message from the daemon.
 * Arg @status is owned by receiver. */
void unit_notify_status (Unit * unit, char * status);

/* Returns true if the given unit is in charge of the given PID. */
bool unit_has_pid (Unit * unit, pid_t pid);

#endif