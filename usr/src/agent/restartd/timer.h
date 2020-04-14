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

#ifndef TIMER___H___
#define TIMER___H___

#include "S16/List.h"

typedef struct tstimer_s
{
    long id;
    void * user;
    void (*cb) (long, void *);
} tstimer_t;

S16ListType (tstimer, tstimer_t *);

typedef struct timerset_s
{
    int kq;
    tstimer_list_t timers;
} timerset_t;

/* Initialises a timerset. */
void timerset_init (timerset_t * tset, int kq);
/* Finds the timer for a given ID, if it exists. */
tstimer_t * timerset_find (timerset_t * tset, long id);
/* Deletes the timer for the given ID, if it exists. */
void timerset_del (timerset_t * tset, long id);
/* Adds a new timer to be triggered in @msec milliseconds, whereupon it will
 * call @cb(timer_id, user_data). Returns the timer id. */
long /* timer_id */
timerset_add (timerset_t * tset, int msec, void * user_data,
              void (*cb) (long /* timer id */, void * /* user data */));
/* Investigates a KEvent, dispatching any timers that have rung. */
void timerset_investigate_kevent (timerset_t * tset, struct kevent * ke);

#endif
