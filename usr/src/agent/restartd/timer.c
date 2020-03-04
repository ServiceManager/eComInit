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

#include <limits.h>
#include <stdlib.h>

#include "manager.h"
#include "timer.h"

void timerset_init (timerset_t * tset, int kq)
{
    tset->kq = kq;
    tset->timers = tstimer_list_new ();
}

tstimer_t * timerset_find (timerset_t * tset, long id)
{
    LL_each (&tset->timers, it)
    {
        if (it->val->id == id)
            return it->val;
    }
    return NULL;
}

void timerset_del (timerset_t * tset, long id)
{
    tstimer_t * toDel;
    struct kevent ev;

    if ((toDel = timerset_find (tset, id)))
    {
        tstimer_list_del (&tset->timers, toDel);
        EV_SET (&ev, toDel->id, EVFILT_TIMER, EV_DELETE, 0, 0, 0);
        kevent (tset->kq, &ev, 1, NULL, 0, NULL);
        s16mem_free (toDel);
    }
}

long timerset_add (timerset_t * tset, int msec, void * user_data,
                   void (*cb) (long /* timer id */, void * /* user data */))
{
    struct kevent ev;
    long ident = rand () % UINT_MAX;
    tstimer_t * newtimer;
    int i;

    while (timerset_find (tset, ident))
        ident = rand () % UINT_MAX;

    EV_SET (&ev, ident, EVFILT_TIMER, EV_ADD | EV_ONESHOT, 0, msec, 0);
    i = kevent (tset->kq, &ev, 1, NULL, 0, NULL);

    if (i == -1)
    {
        perror ("timer kevent! (add)");
        return 0;
    }

    newtimer = malloc (sizeof (tstimer_t));

    newtimer->id = ident;
    newtimer->user = user_data;
    newtimer->cb = cb;

    tstimer_list_add (&tset->timers, newtimer);
    return ident;
}

void timerset_investigate_kevent (timerset_t * tset, struct kevent * ke)
{
    if (ke->filter == EVFILT_TIMER)
    {
        tstimer_t * timer;
        printf ("Timer %lu goes off\n", ke->ident);
        if ((timer = timerset_find (tset, ke->ident)))
            timer->cb (ke->ident, timer->user);
    }
}