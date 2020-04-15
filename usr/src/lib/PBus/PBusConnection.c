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

#include <sys/socket.h>
#include <sys/un.h>

#include "S16/NVRPC.h"

#include "PBus/PBus.h"
#include "PBus_priv.h"

int PBusConnectToSystemBroker ()
{
    int fd;
    struct sockaddr_un sun;

    if ((fd = socket (AF_UNIX, SOCK_STREAM, 0)) == -1)
    {
        return -1;
    }

    S16CloseOnExec (fd);

    memset (&sun, 0, sizeof (struct sockaddr_un));
    sun.sun_family = AF_UNIX;
    strncpy (sun.sun_path, kPBusSocketPath, sizeof (sun.sun_path));

    if (connect (fd, (struct sockaddr *)&sun, SUN_LEN (&sun)) == -1)
        return -1;
    else
        return fd;
}
