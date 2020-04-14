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

#ifndef S16SRV_H_
#define S16SRV_H_

#include "S16/JSONRPC.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct kevent;
    typedef struct s16rpc_srv_s s16rpc_srv_t;

    typedef struct s16rpc_data_s
    {
        int sock;
        const char * method;
        s16rpc_error_t err;
        void * extra;
    } s16rpc_data_t;

    typedef ucl_object_t * (*s16rpc_fun_t) (s16rpc_data_t *);

    /* Creates a new server on the given KQueue and socket. If is_client is
     * true, this server will only listen for messages on the given sock, and
     * not try to accept(). */
    s16rpc_srv_t * s16rpc_srv_new (int kq, int sock, void * extra,
                                   bool is_client);
    /* Registers a method with the server. */
    void s16rpc_srv_register_method (s16rpc_srv_t * srv, const char * name,
                                     size_t nparams, s16rpc_fun_t fun);
    /* Must be called when your KEvent event-loop receives an event. */
    void s16rpc_investigate_kevent (s16rpc_srv_t * srv, struct kevent * ev);

#ifdef __cplusplus
}
#endif

#endif
