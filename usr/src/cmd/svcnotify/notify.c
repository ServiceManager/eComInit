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

#include <getopt.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "systemd/sd-daemon.h"
#include "utstring.h"

struct option options[] = {{"ready", no_argument, NULL, 'r'},
                           {"pid", optional_argument, NULL, 'p'},
                           {"uid", required_argument, NULL, 'u'},
                           {"status", required_argument, NULL, 's'},
                           {"booted", no_argument, NULL, 'b'},
                           {"help", no_argument, NULL, 'h'},
                           {"version", no_argument, NULL, 'v'},
                           {NULL, 0, NULL, 0}};

int main (int argc, char * argv[])
{
    int c;
    UT_string * str;

    utstring_new (str);

    while ((c = getopt_long (argc, argv, "rp::u:s:bhv", options, NULL)) >= 0)
    {
        switch (c)
        {
        case 'r':
            utstring_printf (str, "READY=1\n");
            break;

        case 'p':
            /* TODO: Validate this input. */
            if (optarg)
                utstring_printf (str, "MAINPID=%s\n", optarg);
            else
                utstring_printf (str, "MAINPID=%d\n", getppid ());

            break;

        case 'u':
            printf ("System XVI does not support spoofing UID.\n");
            exit (-1);
            break;

        case 's':
            utstring_printf (str, "STATUS=%s\n", optarg);
            break;

        case 'b':
            return sd_booted ();
            break;

        case 'h':
            printf ("TODO: Help to be written.\n");
            exit (-1);
            break;

        case 'v':
            printf ("System XVI\n");
            exit (0);
            break;
        }
    }

    if (sd_notify (false, utstring_body (str)) < 0)
    {
        perror ("sd_notify!");
    }

    return 0;
}
