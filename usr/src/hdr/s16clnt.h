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

#ifndef S16CLNT_H_
#define S16CLNT_H_

#include "s16rpc.h"

#ifdef __cplusplus
extern "C"
{
#endif

    typedef struct
    {
        int fd;
    } s16rpc_clnt_t;

    s16rpc_clnt_t s16rpc_clnt_new (int sock);

    ucl_object_t * s16rpc_i_clnt_call (s16rpc_clnt_t * clnt,
                                       s16rpc_error_t * err, size_t nparams,
                                       const char * meth_name, ...);
    ucl_object_t * s16rpc_i_clnt_call_unsafe (s16rpc_clnt_t * clnt,
                                              size_t nparams,
                                              const char * meth_name, ...);

#define GET_ARG_COUNT(...)                                                     \
    INTERNAL_GET_ARG_COUNT_PRIVATE (0, ##__VA_ARGS__, 10, 9, 8, 7, 6, 5, 4, 3, \
                                    2, 1, 0)
#define INTERNAL_GET_ARG_COUNT_PRIVATE(_0, _1_, _2_, _3_, _4_, _5_, _6_, _7_,  \
                                       _8_, _9_, _10_, count, ...)             \
    count

/* Makes a call.
 * @param Client
 * @param Method name
 * */
#define s16rpc_clnt_call(clnt, error, ...)                                     \
    s16rpc_i_clnt_call (clnt, error, GET_ARG_COUNT (__VA_ARGS__), ##__VA_ARGS__)

/* Makes an unsafe call.
 * @param Client
 * @param Method name
 * */
#define s16rpc_clnt_call_unsafe(clnt, ...)                                     \
    s16rpc_i_clnt_call_unsafe (clnt, GET_ARG_COUNT (__VA_ARGS__), ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif
