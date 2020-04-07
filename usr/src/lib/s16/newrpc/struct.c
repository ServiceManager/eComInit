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
    .len = sizeof (testStruct1),
    .fields = {{.name = "tD1A",
                .type = {.kind = S16R_KINT},
                .off = offsetof (testStruct1, a)},
               {.name = "tD1B",
                .type = {.kind = S16R_KSTRING},
                .off = offsetof (testStruct1, b)},
               {.name = NULL}}};

s16r_type testType1 = {.kind = S16R_KSTRUCT, .sdesc = &testDesc1};

s16r_struct_description testDesc2 = {
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

ucl_object_t * serialise (void ** src, s16r_type * field);

ucl_object_t * serialiseStruct (void * src, s16r_struct_description * desc)
{
    ucl_object_t * out = ucl_object_typed_new (UCL_OBJECT);
    s16r_field_description * field;

    for (int i = 0; (field = &desc->fields[i]) && field->name; i++)
    {
        void * member = ((char *)src + field->off);
        ucl_object_insert_key (out, serialise (member, &field->type),
                               field->name, 0, true);
    }

    return out;
}

ucl_object_t * serialiseList (void * src, s16r_type * type)
{
    ucl_object_t * out = ucl_object_typed_new (UCL_ARRAY);

    LL_each ((void_list_t *)src, el)
    {
        ucl_array_append (out, serialise (&el->val, type));
    }

    return out;
}

ucl_object_t * serialise (void ** src, s16r_type * type)
{
    switch (type->kind)
    {
    case S16R_KSTRING:
        return ucl_object_fromstring (*src);

    case S16R_KBOOL:
        return ucl_object_frombool (*(bool *)src);

    case S16R_KINT:
        return ucl_object_fromint (*(int *)src);

    case S16R_KSTRUCT:
        return serialiseStruct (*src, type->sdesc);

    case S16R_KLIST:
        return serialiseList (src, type->ltype);

    default:
        assert ("Should not be reached.");
    }
}

/*
 * Deserialisation
 */

void deserialise (const ucl_object_t * src, s16r_type * field, void * dest);

void deserialiseStruct (const ucl_object_t * src,
                        s16r_struct_description * desc, void * dest)
{
    s16r_field_description * field;
    void * struc = malloc (desc->len);

    for (int i = 0; (field = &desc->fields[i]) && field->name; i++)
    {
        const ucl_object_t * obj = ucl_object_lookup (src, field->name);
        void * member = ((char *)struc + field->off);

        deserialise (obj, &field->type, member);
    }

    *(void **)dest = struc;
}

void deserialiseList (const ucl_object_t * src, s16r_type * type, void * dest)
{
    const ucl_object_t * el;
    ucl_object_iter_t it = NULL;

    *(void_list_t *)dest = void_list_new ();

    while ((el = ucl_iterate_object (src, &it, true)))
    {
        /* n.b. what about lists? they aren't officially able to do this */
        void * val;
        deserialise (el, type, &val);
        void_list_add (dest, val);
    }
}

void deserialise (const ucl_object_t * src, s16r_type * type, void * dest)
{
    switch (type->kind)
    {
    case S16R_KSTRING:
        *(char **)dest = ucl_object_tostring (src);
        break;

    case S16R_KBOOL:
        *(bool *)dest = ucl_object_toboolean (src);
        break;

    case S16R_KINT:
        *(int *)dest = ucl_object_toint (src);
        break;

    case S16R_KSTRUCT:
        deserialiseStruct (src, type->sdesc, dest);
        break;

    case S16R_KLIST:
        deserialiseList (src, type->ltype, dest);
        break;

    default:
        assert ("Should not be reached.");
    }
}

int main ()
{
    testStruct1 test1 = {.a = 55, .b = "Hello"};
    testStruct2 test2 = {.a = &test1, .b = "World"};
    ucl_object_t * obj;
    testStruct2 * test3;

    void_list_lpush (&test2.c, &test1);

    obj = serialiseStruct (&test2, &testDesc2);

    printf ("Out: %s\n", ucl_object_emit (obj, UCL_EMIT_JSON));

    deserialiseStruct (obj, &testDesc2, &test3);

    obj = serialiseStruct (test3, &testDesc2);

    printf ("Out: %s\n", ucl_object_emit (obj, UCL_EMIT_JSON));

    return 0;
}