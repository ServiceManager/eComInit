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

#include "S16/Repository.h"

struct svcs_s
{
    s16db_hdl_t h;
    const svc_list_t * svcs;
};

struct svcs_s svcs;

static void print_svc (svc_state_t state, path_t * path)
{
    const char * sstate = s16_state_to_string (state);
    size_t need_t = strlen (sstate) < 8;
    char * spath = s16_path_to_string (path);

    printf ("%s%s\tDate\t\t%s\n", sstate, need_t ? "\t" : "", spath);

    free (spath);
}

int main (int argc, char * argv[])
{
    if (s16db_hdl_new (&svcs.h))
        perror ("Failed to connect to repository");
    svcs.svcs = s16db_get_all_services (&svcs.h);

    printf ("STATE\t\tSTIME\t\tPATH\n");

    list_foreach (svc, svcs.svcs, sit)
    {
        print_svc (sit->val->state, sit->val->path);

        list_foreach (inst, &sit->val->insts, iit)
        {
            print_svc (iit->val->state, iit->val->path);
        }
    }

    return 0;
}
