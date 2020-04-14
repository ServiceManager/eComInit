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

#ifndef S16_H_
#define S16_H_

#ifdef __cplusplus
extern "C"
{
#endif

#include <stddef.h>
#include <stdint.h>

#include "S16/List.h"
#include "S16/PlatformDefinitions.h"

#ifndef S16_MEM_
#define S16_MEM_
    void * s16mem_alloc (unsigned long nbytes);
    void * s16mem_calloc (size_t cnt, unsigned long nbytes);
    char * s16mem_strdup (const char * str);
    void s16mem_free (void * ap);
#endif

    typedef struct path_s S16Path;
    typedef struct svc_instance_s S16ServiceInstance;
    typedef struct svc_s S16Service;

    /* Logging functionality */
    /* Log level: At which level of verbosity should this be emitted? */
    typedef enum
    {
        kS16LogDebug,
        kS16LogInfo,
        kS16LogWarn,
        kS16LogError,
    } S16LogLevel;

    /*
     * Initialises the log system.
     * name: Name of your program.
     */
    void S16LogInit (const char * name);

    void S16Log (S16LogLevel level, const char * fmt, ...);
    void S16LogPath (S16LogLevel level, const S16Path * path, const char * fmt,
                     ...);
    void S16LogService (S16LogLevel level, const S16Service * svc,
                        const char * fmt, ...);
    void S16Loginst (S16LogLevel level, const S16ServiceInstance * inst,
                     const char * fmt, ...);

    /* Misc functionality */
    /* Set FD_CLOEXEC on some FD, preserving old flags. */
    void S16CloseOnExec (int fd);
    /*
     * Handle a signal with a Kernel Queue. You must check for EVFILT_SIGNAL in
     * your event loop.
     */
    void S16HandleSignalWithKQueue (int kq, int sig);

#ifdef __cplusplus
}
#endif

#endif
