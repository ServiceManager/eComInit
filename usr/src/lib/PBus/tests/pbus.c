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

#include "S16/NVRPC.h"

#include "PBus/PBus.h"
#include "PBus/PBus_Private.h"

S16NVRPCMessageSignature testMethSig = {
    .name = "TestMeth",
    .raw = false,
    .rtype = {.kind = S16R_KSTRING},
    .nargs = 2,
    .args = {{.name = "argA", .type = {.kind = S16R_KSTRING}},
             {.name = "argB", .type = {.kind = S16R_KSTRING}},
             {.name = NULL}}};

static void * testFun (PBusObject * self, PBusInvocationContext * ctx,
                       const char * strA, const char * strB)
{
    printf ("String A: %s, b: %s\n", strA, strB);
    ATF_CHECK_STREQ (strA, "Hello");
    ATF_CHECK_STREQ (strB, "World");
    ATF_CHECK_STREQ ("pbus:/system/testSvc", ctx->fromBusname);
    ATF_CHECK_STREQ ("b/c", ctx->fullSelfPath);
    ATF_CHECK_STREQ ("TestMeth", ctx->selector);

    return "Done!";
}

PBusMethod testMeth = {.fnImplementation = (PBusFun)testFun,
                       .messageSignature = &testMethSig};

PBusMethod * testMethSigs[2] = {&testMeth, NULL};

PBusClass testCls = {.methods = &testMethSigs};

ATF_TC (send_message);
ATF_TC_HEAD (send_message, tc)
{
    atf_tc_set_md_var (tc, "descr", "Test message send and resolution.");
}
ATF_TC_BODY (send_message, tc)
{
    PBusObject *a, *b, *c;
    nvlist_t * params = nvlist_create (0);
    nvlist_t * res;
    S16NVRPCError err;

    nvlist_add_string (params, "argA", "Hello");
    nvlist_add_string (params, "argB", "World");

#define makeObj(obj)                                                           \
    obj = calloc (1, sizeof (*obj));                                           \
    obj->name = #obj;                                                          \
    obj->isA = &testCls

    makeObj (a);
    makeObj (b);
    makeObj (c);

#define addObjToObj(b, a) PBusObject_list_add (&a->subObjects, b)
    addObjToObj (b, a);
    addObjToObj (c, b);

    res = PBusFindReceiver_Root (
        a, &err, "b/c", "pbus:/system/testSvc", "TestMeth", params);
    nvlist_destroy (params);
    nvlist_destroy (res);

#define destroyObj(a)                                                          \
    PBusObject_list_destroy (&a->subObjects);                                  \
    free (a)

    destroyObj (a);
    destroyObj (b);
    destroyObj (c);
}

ATF_TP_ADD_TCS (tp)
{
    ATF_TP_ADD_TC (tp, send_message);
    return atf_no_error ();
}