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

#include "S16/List.h"
#include "S16/NVRPC.h"
#include "nv.h"

typedef struct
{
    intptr_t a;
    const char * b;
    intptr_t c;
    boolptr_t d;
} testStruct1;

S16ListType (test1, testStruct1 *);

typedef struct
{
    testStruct1 * a;
    const char * b;
    test1_list_t c;
} testStruct2;

S16NVRPCStruct testDesc1 = {.len = sizeof (testStruct1),
                            .fields = {{.name = "tD1A",
                                        .type = {.kind = S16R_KINT},
                                        .off = offsetof (testStruct1, a)},
                                       {.name = "tD1B",
                                        .type = {.kind = S16R_KSTRING},
                                        .off = offsetof (testStruct1, b)},
                                       {.name = "tD1C",
                                        .type = {.kind = S16R_KDESCRIPTOR},
                                        .off = offsetof (testStruct1, c)},
                                       {.name = "tD1D",
                                        .type = {.kind = S16R_KBOOL},
                                        .off = offsetof (testStruct1, d)},
                                       {.name = NULL}}};

S16NVRPCType testType1 = {.kind = S16R_KSTRUCT, .sdesc = &testDesc1};

S16NVRPCStruct testDesc2 = {
    .len = sizeof (testStruct2),
    .fields = {{.name = "tD2A",
                .type = {.kind = S16R_KSTRUCT, .sdesc = &testDesc1},
                .off = offsetof (testStruct2, a)},
               {.name = "tD2B",
                .type = {.kind = S16R_KSTRING},
                .off = offsetof (testStruct2, b)},
               {.name = "tD2C",
                .type =
                    {
                        .kind = S16R_KLIST,
                        .ltype = &testType1,
                    },
                .off = offsetof (testStruct2, c)},
               {.name = NULL}}};

ATF_TC (deserialise_twice);
ATF_TC_HEAD (deserialise_twice, tc)
{
    atf_tc_set_md_var (
        tc,
        "descr",
        "Test conversion of a service structure to and from JSON form.");
}
ATF_TC_BODY (deserialise_twice, tc)
{
    testStruct1 test1 = {.a = 55, .b = "Hello", .c = 1, .d = false};
    testStruct2 test2 = {.a = &test1, .b = "World"};
    nvlist_t * nvl;
    testStruct2 * test3;
    char * str;
    ucl_object_t * obj;

    test1_list_lpush (&test2.c, &test1);

    ATF_REQUIRE ((nvl = S16NVRPCStructSerialise (&test2, &testDesc2)));
    ATF_REQUIRE ((obj = S16NVRPCNVListToUCL (nvl)));
    ATF_REQUIRE ((str = (char *)ucl_object_emit (obj, UCL_EMIT_JSON)));
    printf ("First serialisation: %s\n", str);
    ucl_object_unref (obj);
    free (str);

    S16NVRPCStructDeserialise (nvl, &testDesc2, (void **)&test3);
    nvlist_destroy (nvl);

    nvl = S16NVRPCStructSerialise (test3, &testDesc2);
    obj = S16NVRPCNVListToUCL (nvl);
    str = (char *)ucl_object_emit (obj, UCL_EMIT_JSON);
    printf ("Second serialisation: %s\n", str);
    ucl_object_unref (obj);
    free (str);

    /* ATF_CHECK_STREQ (converted, correct) */

    nvlist_destroy (nvl);
}

ATF_TP_ADD_TCS (tp)
{
    ATF_TP_ADD_TC (tp, deserialise_twice);
    return atf_no_error ();
}