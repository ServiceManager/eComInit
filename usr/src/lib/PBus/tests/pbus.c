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
#include "PBus_priv.h"

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
    ATF_CHECK_STREQ ("a/b/c", ctx->selfPath);
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
    atf_tc_set_md_var (
        tc,
        "descr",
        "Test conversion of a service structure to and from JSON form.");
}
ATF_TC_BODY (send_message, tc)
{
    PBusServer * srv = calloc (1, sizeof (*srv));
    PBusObject *a, *b, *c;
    nvlist_t * params = nvlist_create (0);
    nvlist_t * res;

    nvlist_add_string (params, "argA", "Hello");
    nvlist_add_string (params, "argB", "World");

#define makeObj(obj)                                                           \
    obj = calloc (1, sizeof (*obj));                                           \
    obj->name = #obj;                                                          \
    obj->isA = &testCls

    makeObj (a);
    makeObj (b);
    makeObj (c);
    makeObj (srv->rootObject);

#define addObjToObj(b, a) PBusObject_list_add (&a->subObjects, b)
    addObjToObj (a, srv->rootObject);
    addObjToObj (b, a);
    addObjToObj (c, b);

    res = findReceiver_root (
        srv, "a/b/c", "pbus:/system/testSvc", "TestMeth", params);
    // printf ("%s\n", ucl_object_emit (S16NVRPCNVListToUCL (res),
    // UCL_EMIT_JSON));
    nvlist_destroy (res);

    nvlist_destroy (params);

#define destroyObj(a)                                                          \
    PBusObject_list_destroy (&a->subObjects);                                  \
    free (a)

    destroyObj (srv->rootObject);
    destroyObj (a);
    destroyObj (b);
    destroyObj (c);

    free (srv);
}

ATF_TP_ADD_TCS (tp)
{
    ATF_TP_ADD_TC (tp, send_message);
    return atf_no_error ();
}