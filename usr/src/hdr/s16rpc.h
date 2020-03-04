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

#ifndef S16RPC_H_
#define S16RPC_H_

#include "s16.h"
#include "ucl.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /* S16 RPC Error code */
    typedef enum
    {
        S16ENOSUCHMETH = 5000,
    } s16rpc_errcode_t;

    typedef struct
    {
        s16rpc_errcode_t code;
        char * message;
        ucl_object_t * data;
    } s16rpc_error_t;

    void s16rpc_error_destroy (s16rpc_error_t * rerr);

#ifdef __cplusplus
}
#endif

#endif
