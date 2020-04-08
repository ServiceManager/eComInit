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

#include "nv.h"
#include "ucl.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef intptr_t boolptr_t;
    typedef intptr_t fdptr_t;

    typedef enum
    {
        S16R_KSTRING,     /* A pointer to const char*/
        S16R_KBOOL,       /* A boolptr_t */
        S16R_KINT,        /* An intptr_t */
        S16R_KNVLIST,     /* A pointer to an nvlist */
        S16R_KSTRUCT,     /* A pointer to a struct */
        S16R_KLIST,       /* An S16 list */
        S16R_KDESCRIPTOR, /* Unix rights - but cast into fdptr_t!! */
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
        size_t nargs;
        s16r_message_arg_signature args[];
    } s16r_message_signature;

    /* S16 RPC Error code */
    typedef enum
    {
        /* The nvlist sent is not a valid Request object. */
        S16R_EInvalidRequest = -32600,
        /* The method does not exist / is not available. */
        S16R_ENoSuchMethid = -32601,
        /* Invalid method parameter(s). */
        S16R_EInvalidParams = -32602,
        /* Internal NVList-RPC error. */
        S16R_EInternalError = -32603,
        /*
         * 32000 to -32099	Server error	Reserved for implementation-defined
         * server-errors.
         */
    } s16r_errcode_t;

    typedef struct
    {
        s16r_errcode_t code;
        char * message;
        size_t data_len;
        void * data;
    } s16r_error_t;

    typedef struct s16r_data_s
    {
        const char * method;
        s16r_error_t err;
        nvlist_t * result;
        void * extra;
    } s16r_data_t;

    typedef struct s16r_srv_t s16r_srv_t;

    typedef void * (*s16r_fun_t) (s16r_data_t *, ...);

    void serialise (nvlist_t * nvl, const char * name, void ** src,
                    s16r_type * type);

    /*
     * Deserialisation routines. They return 0 if they succeed.
     */
    int deserialiseStruct (nvlist_t * nvl, s16r_struct_description * desc,
                           void ** dest);
    int deserialiseMsgArgs (nvlist_t * nvl, s16r_message_signature * desc,
                            void ** dest);

    ucl_object_t * nvlist_to_ucl (const nvlist_t * nvl);

    s16r_srv_t * s16r_srv_new (void * extra);
    /* Registers a method with the server. */
    void s16rpc_srv_register_method (s16r_srv_t * srv, const char * name,
                                     s16r_fun_t fun);

    void testIt ();

#ifdef __cplusplus
}
#endif

#endif
