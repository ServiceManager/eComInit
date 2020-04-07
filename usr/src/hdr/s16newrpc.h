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

#ifndef S16NEWRPC_H_
#define S16NEWRPC_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

    typedef enum
    {
        S16R_KSTRING,
        S16R_KBOOL,
        S16R_KINT,
        S16R_KSTRUCT,
        S16R_KLIST,       /* An S16 list */
        S16R_KDESCRIPTOR, /* Unix rights */
        S16R_KMAX
    } s16r_kind;

    typedef struct s16r_type
    {
        s16r_kind kind;
        union {
            /* If kind is STRUCT, this points to the struct description. */
            struct s16r_struct_description * sdesc;
            /* If kind is LIST, this points to the type of its elements. */
            struct s16r_type * ltype;
        };
    } s16r_type;

    typedef struct
    {
        const char * name;
        s16r_type type;
        /* Offset of this field into the corresponding struct. */
        size_t off;
    } s16r_field_description;

    typedef struct s16r_struct_description
    {
        /* Total size of this struct. */
        size_t len;
        s16r_field_description fields[];
    } s16r_struct_description;

    typedef struct s16r_message_arg_signature
    {
        const char * name;
        s16r_type type;
    } s16r_message_arg_signature;

    typedef struct s16r_message_signature
    {
        /* Return type of the message. */
        s16r_type rtype;
        /* Arguments */
        s16r_message_arg_signature args[];
    } s16r_message_signature;

#ifdef __cplusplus
}
#endif

#endif
