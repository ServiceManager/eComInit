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
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2010 David MacKay.  All rights reserved.
 * Use is subject to license terms.
 */

/*
 * Desc: Path representation.
 */

package org.systemxvi.libs16;

/* Represents a System Resource Path. */
public class Path 
{
    private String service = null;
    private String instance = null;
    private String property  = null;

    public Path(java.net.URI uri)
    {
        String[] pathParts = uri.getPath().split(":");

        service = pathParts[0];
        instance = pathParts[1];
        property = uri.getFragment();
    }
}
