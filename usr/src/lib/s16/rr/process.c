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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/types.h>
#include <unistd.h>

#if defined(__FreeBSD__) || defined(__DragonFly__)
#include <kvm.h>
#include <sys/procctl.h>
#include <sys/sysctl.h>
#include <sys/user.h>
#elif defined(__linux__)
#include <proc/readproc.h>
#include <sys/prctl.h>
#endif

#include "S16/RestarterServices.h"

/* It is forbidden to include Service.h in this file.
 * Doing so leads to a FreeBSD-internal header conflict! */
void * s16mem_alloc (unsigned long);
void s16mem_free (void *);

int S16SubreapingAcquire ()
{
#if defined(__FreeBSD__) || defined(__DragonFly__)
    return procctl (P_PID, getpid (), PROC_REAP_ACQUIRE, NULL);
#elif defined(__linux__)
    return prctl (PR_SET_CHILD_SUBREAPER, 1);
#endif
}

int S16SubreapingRelinquish ()
{
#if defined(__FreeBSD__) || defined(__DragonFly__)
    return procctl (P_PID, getpid (), PROC_REAP_RELEASE, NULL);
#elif defined(__linux__)
    return prctl (PR_SET_CHILD_SUBREAPER, 0);
#endif
}

int S16SubreapingStatus ()
{
#if defined(__FreeBSD__) || defined(__DragonFly__)
    struct procctl_reaper_status pctl;
    procctl (P_PID, getpid (), PROC_REAP_STATUS, &pctl);
    return (pctl.rs_flags & REAPER_STATUS_OWNED);
#elif defined(__linux__)
    int status;
    prctl (PR_GET_CHILD_SUBREAPER, &status);
    return status;
#endif
}

S16PendingProcess * S16ProcessForkAndWait (const char * cmd_,
                                           void (*cleanup_cb) (void *),
                                           void * cleanup_cb_arg)
{
    S16PendingProcess * pwait = s16mem_alloc (sizeof (S16PendingProcess));
    int n_spaces = 0;
    char *cmd = strdup (cmd_), *tofree = cmd, **argv = NULL, *saveptr = NULL;
    pid_t newPid;

    strtok_r (cmd, " ", &saveptr);

    while (cmd)
    {
        argv = (char **)realloc (argv, sizeof (char *) * ++n_spaces);
        argv[n_spaces - 1] = cmd;
        cmd = strtok_r (NULL, " ", &saveptr);
    }

    argv = (char **)realloc (argv, sizeof (char *) * (n_spaces + 1));
    argv[n_spaces] = 0;

    pipe (pwait->fd);
    newPid = fork ();

    if (newPid == 0) /* child */
    {
        char dispose;
        close (pwait->fd[1]);
        read (pwait->fd[0], &dispose, 1);
        close (pwait->fd[0]);
        cleanup_cb (cleanup_cb_arg);
        execvp (argv[0], argv);
        perror ("Execvp failed!");
        exit (999);
    }
    else if (newPid < 0) /* fail */
    {
        fprintf (stderr, "FAILED TO FORK\n");
        pwait->pid = 0;
    }
    else
    {
        close (pwait->fd[0]);
        pwait->pid = newPid;
    }

    free (argv);
    free (tofree);

    return pwait;
}

void S16PendingProcessContinue (S16PendingProcess * pwait)
{
    write (pwait->fd[1], "0", 1);
    close (pwait->fd[1]);
    s16mem_free (pwait);
}

int S16ExitWasAbnormal (int wstat)
{
    if (WIFEXITED (wstat))
        return WEXITSTATUS (wstat);
    else if (WIFSIGNALED (wstat))
    {
        int sig = WTERMSIG (wstat);
        if ((sig == SIGPIPE) || (sig == SIGTERM) || (sig == SIGHUP) ||
            (sig == SIGINT))
            return 0;
        else
            return sig;
    }
    else
        return 1;
}

void discard_signal (int no) {}

/* Borrowed from Epoch: http://universe2.us/epoch.html */
pid_t S16PIDReadFromPIDFile (const char * path)
{
    FILE * PIDFileDescriptor = fopen (path, "r");
    char PIDBuf[200], *TW = NULL, *TW2 = NULL;
    unsigned long Inc = 0;
    pid_t InPID;

    if (!PIDFileDescriptor)
        return 0; // Zero for failure.

    for (int TChar;
         (TChar = getc (PIDFileDescriptor)) != EOF && Inc < (200 - 1);
         ++Inc)
        *(unsigned char *)&PIDBuf[Inc] = (unsigned char)TChar;

    PIDBuf[Inc] = '\0';
    fclose (PIDFileDescriptor);

    for (TW = PIDBuf; *TW == '\n' || *TW == '\t' || *TW == ' '; ++TW)
        ;
    for (TW2 = TW; *TW2 != '\0' && *TW2 != '\t' && *TW2 != '\n' &&
                   *TW2 != '%' && *TW2 != ' ';
         ++TW2)
        ; // Delete any following the number.

    *TW2 = '\0';
    InPID = strtol (TW, 0, 10);
    return InPID;
}
