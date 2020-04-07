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

#include "nv.h"
#include "s16list.h"
#include "s16newrpc.h"
#include "ucl.h"

S16List (void, void *);

const char * s16r_kind_str[S16R_KMAX] = {"String", "Boolean", "Int",
                                         "Struct", "List",    "Right"};

typedef struct
{
    int a;
    const char * b;
    int c;
    bool d;
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
               {.name = "tD1C",
                .type = {.kind = S16R_KDESCRIPTOR},
                .off = offsetof (testStruct1, c)},
               {.name = "tD1D",
                .type = {.kind = S16R_KBOOL},
                .off = offsetof (testStruct1, d)},
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

void serialise (nvlist_t * nvl, const char * name, void ** src,
                s16r_type * field);

nvlist_t * serialiseStruct (void * src, s16r_struct_description * desc)
{
    nvlist_t * nvl = nvlist_create (0);
    s16r_field_description * field;

    for (int i = 0; (field = &desc->fields[i]) && field->name; i++)
    {
        void * member = ((char *)src + field->off);
        serialise (nvl, field->name, member, &field->type);
    }

    return nvl;
}

nvlist_t * serialiseList (void * src, s16r_type * type)
{
    nvlist_t * nvl = nvlist_create (0);
    char namebuf[8];
    int i = 0;

    LL_each ((void_list_t *)src, el)
    {
        snprintf (namebuf, 8, "%i", i++);
        serialise (nvl, namebuf, &el->val, type);
    }

    return nvl;
}

/*
 * Serialises the data at @src, of type described by @type, into the nvlist
 * @nvl, under the name @name.
 */
void serialise (nvlist_t * nvl, const char * name, void ** src,
                s16r_type * type)
{
    switch (type->kind)
    {
    case S16R_KSTRING:
        nvlist_add_string (nvl, name, *(char **)src);
        break;

    case S16R_KBOOL:
        nvlist_add_bool (nvl, name, *(bool *)src);
        break;

    case S16R_KINT:
        nvlist_add_number (nvl, name, *(int *)src);
        break;

    case S16R_KSTRUCT:
        nvlist_move_nvlist (nvl, name, serialiseStruct (*src, type->sdesc));
        break;

    case S16R_KLIST:
        nvlist_move_nvlist (nvl, name, serialiseList (src, type->ltype));
        break;

    case S16R_KDESCRIPTOR:
        nvlist_add_descriptor (nvl, name, *(int *)src);
        break;

    default:
        assert (!"Should not be reached.");
    }
}

/*
 * Deserialisation
 */

void deserialiseList (nvlist_t * nvl, s16r_type * type, void ** dest);
void deserialiseStruct (nvlist_t * nvl, s16r_struct_description * desc,
                        void ** dest);

void deserialiseMember (nvlist_t * nvl, const char * name, s16r_type * type,
                        void ** dest)
{
    switch (type->kind)
    {
    case S16R_KSTRING:
        *(char **)dest = nvlist_take_string (nvl, name);
        break;

    case S16R_KBOOL:
        *(bool *)dest = nvlist_get_bool (nvl, name);
        break;

    case S16R_KINT:
        *(int *)dest = nvlist_get_number (nvl, name);
        break;

    case S16R_KSTRUCT:
        /* 'Take' cannot be used here. */
        deserialiseStruct ((nvlist_t *)nvlist_get_nvlist (nvl, name),
                           type->sdesc, dest);
        break;

    case S16R_KLIST:
        deserialiseList ((nvlist_t *)nvlist_get_nvlist (nvl, name), type->ltype,
                         dest);
        break;

    case S16R_KDESCRIPTOR:
        *(int *)dest = nvlist_take_descriptor (nvl, name);
        break;

    default:
        assert (!"Should not be reached.");
    }
}

void deserialiseStruct (nvlist_t * nvl, s16r_struct_description * desc,
                        void ** dest)
{
    s16r_field_description * field;
    void * struc = malloc (desc->len);

    for (int i = 0; (field = &desc->fields[i]) && field->name; i++)
    {
        void * member = ((char *)struc + field->off);

        deserialiseMember (nvl, field->name, &field->type, member);
    }

    *dest = struc;
}

void deserialiseList (nvlist_t * nvl, s16r_type * type, void ** dest)
{
    const char * name;
    void * cookie = NULL;
    int nvtype;
    void_list_t * list = (void_list_t *)dest;

    *list = void_list_new ();

    while ((name = nvlist_next (nvl, &nvtype, &cookie)))
    {
        void * dat;
        deserialiseMember (nvl, name, type, &dat);
        void_list_add (list, dat);
    }
}

ucl_object_t * nvlist_to_ucl (const nvlist_t * nvl)
{
    ucl_object_t * obj = ucl_object_typed_new (UCL_OBJECT);
    const char * name;
    void * cookie = NULL;
    int type;

    while ((name = nvlist_next (nvl, &type, &cookie)) != NULL)
    {
        switch (type)
        {
        case NV_TYPE_STRING:
            ucl_object_insert_key (
                obj, ucl_object_fromstring (nvlist_get_string (nvl, name)),
                name, 0, false);
            break;

        case NV_TYPE_BOOL:
            ucl_object_insert_key (
                obj, ucl_object_frombool (nvlist_get_bool (nvl, name)), name, 0,
                false);
            break;

        case NV_TYPE_NUMBER:
            ucl_object_insert_key (
                obj, ucl_object_fromint (nvlist_get_number (nvl, name)), name,
                0, false);
            break;

        case NV_TYPE_NVLIST:
            ucl_object_insert_key (
                obj, nvlist_to_ucl (nvlist_get_nvlist (nvl, name)), name, 0,
                false);
            break;

        case NV_TYPE_DESCRIPTOR:
            ucl_object_insert_key (
                obj, ucl_object_fromint (nvlist_get_descriptor (nvl, name)),
                name, 0, false);
            break;

        default:
            assert (!"Unsupported NVList entry type.");
            break;
        }
    }

    return obj;
}

int main ()
{
    testStruct1 test1 = {.a = 55, .b = "Hello", .c = 1, .d = false};
    testStruct2 test2 = {.a = &test1, .b = "World"};
    nvlist_t * nvl;
    testStruct2 * test3;
    char * str;
    ucl_object_t * obj;

    test1_list_lpush (&test2.c, &test1);

    nvl = serialiseStruct (&test2, &testDesc2);
    obj = nvlist_to_ucl (nvl);
    str = (char *)ucl_object_emit (obj, UCL_EMIT_JSON);
    printf ("First serialisation: %s\n", str);
    ucl_object_unref (obj);
    free (str);

    deserialiseStruct (nvl, &testDesc2, (void **)&test3);
    nvlist_destroy (nvl);

    nvl = serialiseStruct (test3, &testDesc2);
    obj = nvlist_to_ucl (nvl);
    str = (char *)ucl_object_emit (obj, UCL_EMIT_JSON);
    printf ("Second serialisation: %s\n", str);
    ucl_object_unref (obj);
    free (str);

    nvlist_destroy (nvl);

    return 0;
}