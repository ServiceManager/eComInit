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

#include "s16db.h"

svc_t make_svc () {}

ATF_TC (convert_svc);
ATF_TC_HEAD (convert_svc, tc)
{
    atf_tc_set_md_var (
        tc,
        "descr",
        "Test conversion of a service structure to and from JSON form.");
}
ATF_TC_BODY (convert_svc, tc)
{
    path_list_t paths = path_list_new ();
    depgroup_list_t depgroups = depgroup_list_new ();
    prop_list_t props = prop_list_new ();
    meth_list_t meths = meth_list_new ();
    inst_list_t insts = inst_list_new ();

    const char * correct =
        "{\"path\":\"svc:/"
        "ex\",\"default-instance\":\"notdefault\",\"properties\":[{\"name\":"
        "\"prop\",\"value\":\"propval\"}],\"methods\":[{\"name\":\"meth\","
        "\"properties\":[{\"name\":\"prop\",\"value\":\"propval\"}]}],"
        "\"instances\":[{\"path\":\"svc:/"
        "ex:inst\",\"properties\":[{\"name\":\"prop\",\"value\":\"propval\"}],"
        "\"methods\":[{\"name\":\"meth\",\"properties\":[{\"name\":\"prop\","
        "\"value\":\"propval\"}]}],\"dependencies\":[{\"grouping\":\"optional-"
        "all\",\"restart-on\":\"refresh\",\"paths\":[\"svc:/"
        "dep:depinst\"]}],\"enabled\":false,\"state\":4}],\"dependencies\":[{"
        "\"grouping\":\"optional-all\",\"restart-on\":\"refresh\",\"paths\":["
        "\"svc:/dep:depinst\"]}],\"state\":0}";

    depgroup_t dg = {
        .name = "dg",
        .type = OPTIONAL_ALL,
        .restart_on = ON_REFRESH,
        .paths = *path_list_add (&paths, s16_path_new ("dep", "depinst"))};

    property_t prop = {
        .name = "prop", .type = PROP_STRING, .value.s = "propval"};

    method_t meth = {.name = "meth", .props = *prop_list_add (&props, &prop)};

    svc_instance_t inst = {.path = s16_path_new ("ex", "inst"),
                           .props = props,
                           .meths = *meth_list_add (&meths, &meth),
                           .depgroups = *depgroup_list_add (&depgroups, &dg),
                           .state = S_MAINTENANCE};

    svc_t svc = {.path = s16_path_new ("ex", NULL),
                 .def_inst = "notdefault",
                 .props = props,
                 .meths = meths,
                 .insts = *inst_list_add (&insts, &inst),
                 .depgroups = depgroups};

    char * converted = (char *)ucl_object_emit (s16db_svc_to_ucl (&svc),
                                                UCL_EMIT_JSON_COMPACT);

    ATF_CHECK_STREQ (converted, correct);
}

ATF_TP_ADD_TCS (tp)
{
    ATF_TP_ADD_TC (tp, convert_svc);
    return atf_no_error ();
}