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

#include <assert.h>
#include <err.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <unistd.h>

#include "s16list.h"
#include "s16newrpc.h"
#include "ucl.h"

S16List (void, void *);

const char * s16r_kind_str[S16R_KMAX] = {"String", "Boolean", "Int", "Struct",
                                         "List"};

typedef struct
{
    int a;
    const char * b;
} testStruct1;

S16List (test1, testStruct1 *);

typedef struct
{
    testStruct1 * a;
    const char * b;
    test1_list_t c;
} testStruct2;

s16r_struct_description testDesc1 = {
    .fields = {{.name = "tD1A",
                .type = {.kind = INT},
                .off = offsetof (testStruct1, a)},
               {.name = "tD1B",
                .type = {.kind = STRING},
                .off = offsetof (testStruct1, b)},
               {.name = NULL}}};

s16r_type testType1 = {.kind = STRUCT, .sdesc = &testDesc1};

s16r_struct_description testDesc2 = {
    .fields = {{.name = "tD2A",
                .type = {.kind = STRUCT, .sdesc = &testDesc1},
                .off = offsetof (testStruct2, a)},
               {.name = "tD2B",
                .type = {.kind = STRING},
                .off = offsetof (testStruct2, b)},
               {.name = "tD2C",
                .type =
                    {
                        .kind = LIST,
                        .ltype = &testType1,
                    },
                .off = offsetof (testStruct2, c)},
               {.name = NULL}}};

ucl_object_t * convert (void ** src, s16r_type * field);

ucl_object_t * convertStruct (void * src, s16r_struct_description * desc)
{
    ucl_object_t * out = ucl_object_typed_new (UCL_OBJECT);
    s16r_field_description * field;

    for (int i = 0; (field = &desc->fields[i]) && field->name; i++)
    {
        void * member = ((char *)src + field->off);
        ucl_object_insert_key (out, convert (member, &field->type), field->name,
                               0, true);
    }

    return out;
}

ucl_object_t * convertList (void * src, s16r_type * type)
{
    ucl_object_t * out = ucl_object_typed_new (UCL_ARRAY);

    LL_each ((void_list_t *)src, el)
    {
        ucl_array_append (out, convert (&el->val, type));
    }

    return out;
}

ucl_object_t * convert (void ** src, s16r_type * type)
{
    switch (type->kind)
    {
    case STRING:
        return ucl_object_fromstring (*src);

    case BOOL:
        return ucl_object_frombool (*(bool *)src);

    case INT:
        return ucl_object_fromint (*(int *)src);

    case STRUCT:
        return convertStruct (*src, type->sdesc);

    case LIST:
        return convertList (src, type->ltype);

    default:
        assert ("Should not be reached.");
    }
}

int main ()
{
    testStruct1 test1 = {.a = 55, .b = "Hello"};
    testStruct2 test2 = {.a = &test1, .b = "World"};

    void_list_lpush (&test2.c, &test1);

    printf ("Out: %s\n", ucl_object_emit (convertStruct (&test2, &testDesc2),
                                          UCL_EMIT_JSON));

    return 0;
}