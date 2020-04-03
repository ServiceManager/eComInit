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

#include <atf-c.h>

ATF_TC (tester);
ATF_TC_HEAD (tester, tc) { atf_tc_set_md_var (tc, "descr", "This is a test"); }
ATF_TC_BODY (tester, tc) { ATF_CHECK_STREQ ("Test", "Tsest"); }

ATF_TP_ADD_TCS (tp)
{
    ATF_TP_ADD_TC (tp, tester);
    return atf_no_error ();
}