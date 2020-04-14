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

/*
 * Desc: Repository access functions not of interest to consumers other than the
 * internal services.
 */

#ifndef S16_DB_PRIV_H_
#define S16_DB_PRIV_H_

#include "S16/Repository.h"

#ifdef __cplusplus
extern "C"
{
#endif

    /**********************************************************
     * Repository lookup
     **********************************************************/
    /* Retrieves from the repository a merged list of all services. A
     * repository isn't that big, so this is a standard way of interacting
     * with it for now. In the future, it might be good to investigate if
     * this is still a good idea. Do we really want to  */
    svc_list_t s16db_repo_get_all_services_merged (s16db_hdl_t * hdl);
    /* Retrieves from the repository the merged service or instance
     * specified by the path. r.type is ENOTFOUND if not found. */
    s16db_lookup_result_t s16db_repo_lookup_path_merged (s16db_hdl_t * hdl,
                                                         path_t * path);

    /**********************************************************
     * Scope lookup
     **********************************************************/
    s16db_lookup_result_t s16db_lookup_path_in_scope (s16db_scope_t scope,
                                                      path_t * path);

#ifdef __cplusplus
}
#endif

#endif
