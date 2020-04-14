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

/* This is a process-tracker driver using Linux's NetLink Proc connector.
 * The Proc connector allows us to be informed of process forks and exits.
 * Thanks to http://netsplit.com/the-proc-connector-and-socket-filters for
 * demystifying this.
 * Be aware: this requires root. */

#include <errno.h>
#include <fcntl.h>
#include <linux/cn_proc.h>
#include <linux/connector.h>
#include <linux/netlink.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/event.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <unistd.h>

#include "S16/List.h"
#include "S16/RestarterServices.h"

void * s16mem_alloc (unsigned long);
void s16mem_free (void *);

struct process_tracker_s
{
    int kq;
    int sock;
    pid_list_t pids;
};

S16ProcessTracker * S16ProcessTrackerNew (int kq)
{
    int i;
    struct sockaddr_nl addr;
    S16ProcessTracker * pt = s16mem_alloc (sizeof (S16ProcessTracker));
    struct iovec iov[3];
    char nlmsghdrbuf[NLMSG_LENGTH (0)];
    struct nlmsghdr * nlmsghdr = (struct nlmsghdr *)nlmsghdrbuf;
    struct cn_msg cn_msg;
    enum proc_cn_mcast_op op;
    struct kevent ke;

    pt->kq = kq;
    pt->pids = pid_list_new ();

    pt->sock = socket (PF_NETLINK,
                       SOCK_DGRAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                       NETLINK_CONNECTOR);

    addr.nl_family = AF_NETLINK;
    addr.nl_pid = getpid ();
    addr.nl_groups = CN_IDX_PROC;

    bind (pt->sock, (struct sockaddr *)&addr, sizeof addr);

    nlmsghdr->nlmsg_len = NLMSG_LENGTH (sizeof cn_msg + sizeof op);
    nlmsghdr->nlmsg_type = NLMSG_DONE;
    nlmsghdr->nlmsg_flags = 0;
    nlmsghdr->nlmsg_seq = 0;
    nlmsghdr->nlmsg_pid = getpid ();

    iov[0].iov_base = nlmsghdrbuf;
    iov[0].iov_len = NLMSG_LENGTH (0);

    cn_msg.id.idx = CN_IDX_PROC;
    cn_msg.id.val = CN_VAL_PROC;
    cn_msg.seq = 0;
    cn_msg.ack = 0;
    cn_msg.len = sizeof op;

    iov[1].iov_base = &cn_msg;
    iov[1].iov_len = sizeof cn_msg;

    op = PROC_CN_MCAST_LISTEN;

    iov[2].iov_base = &op;
    iov[2].iov_len = sizeof op;

    writev (pt->sock, iov, 3);

    EV_SET (&ke, pt->sock, EVFILT_READ, EV_ADD, 0, 0, NULL);
    i = kevent (pt->kq, &ke, 1, NULL, 0, NULL);

    if (i == -1)
        fprintf (stderr,
                 "Error: failed to watch NetLink socket %d: %s\n",
                 pt->sock,
                 strerror (errno));
    return pt;
}

int S16ProcessTrackerWatchPID (S16ProcessTracker * pt, pid_t pid)
{
    pid_list_add (&pt->pids, pid);
    return 0;
}

/* The NetLink driver gets every process event straight from the kernel, as
 * such we cannot disregard a PID. Only delete it from our watch. */
void S16ProcessTrackerDisregardPID (S16ProcessTracker * pt, pid_t pid)
{
    pid_list_del (&pt->pids, pid);
    return;
}

int pid_relevant (S16ProcessTracker * pt, pid_t pid, pid_t ppid)
{
    return pid_list_find_eq (&pt->pids, pid) ||
           pid_list_find_eq (&pt->pids, ppid);
}

/* FIXME: I need to find out whether we should support sending back multiple
 * S16ProcessTrackerEvent; is it not possible, after all, that we can get
 * multiple messages from netlink? */
S16ProcessTrackerEvent *
S16ProcessTrackerInvestigateKEvent (S16ProcessTracker * pt, struct kevent * ke)
{
    S16ProcessTrackerEvent * result;
    S16ProcessTrackerEvent info;

    struct msghdr msghdr;
    struct sockaddr_nl addr;
    struct iovec iov[1];
    char buf[NLMSG_SPACE (
        NLMSG_LENGTH (sizeof (struct cn_msg) + sizeof (struct proc_event)))];
    ssize_t len;

    if (ke->filter != EVFILT_READ || ke->ident != pt->sock)
        return 0;

    msghdr.msg_name = &addr;
    msghdr.msg_namelen = sizeof addr;
    msghdr.msg_iov = iov;
    msghdr.msg_iovlen = 1;
    msghdr.msg_control = NULL;
    msghdr.msg_controllen = 0;
    msghdr.msg_flags = 0;

    iov[0].iov_base = buf;
    iov[0].iov_len = sizeof buf;

    len = recvmsg (pt->sock, &msghdr, 0);

    if (addr.nl_pid != 0)
        return 0;

    for (struct nlmsghdr * nlmsghdr = (struct nlmsghdr *)buf;
         NLMSG_OK (nlmsghdr, len);
         nlmsghdr = NLMSG_NEXT (nlmsghdr, len))
    {
        struct cn_msg * cn_msg;
        struct proc_event * ev;

        cn_msg = NLMSG_DATA (nlmsghdr);
        if ((cn_msg->id.idx != CN_IDX_PROC) || (cn_msg->id.val != CN_VAL_PROC))
            continue;

        ev = (struct proc_event *)cn_msg->data;

        switch (ev->what)
        {
        case PROC_EVENT_FORK:
            info.event = kS16ProcessTrackerEventTypeChild;
            info.pid = ev->event_data.fork.child_tgid;
            info.ppid = ev->event_data.fork.parent_tgid;
            info.flags = 0;

            if (pid_relevant (pt, info.pid, info.ppid))
            {
                pid_list_add (&pt->pids, info.pid);
                goto reply;
            }
            break;
        case PROC_EVENT_EXIT:
            info.event = kS16ProcessTrackerEventTypeExit;
            info.pid = ev->event_data.exit.process_tgid;
            info.ppid = 0;
            info.flags = ev->event_data.exit.exit_code;

            if (pid_relevant (pt, info.pid, 0))
            {
                S16ProcessTrackerDisregardPID (pt, info.pid);
                goto reply;
            }
            break;
        default:
            return 0;
        }
    }
    return 0;

reply:
    result = s16mem_alloc (sizeof (S16ProcessTrackerEvent));
    *result = info;

    return result;
}

void S16ProcessTrackerDestroy (S16ProcessTracker * pt)
{
    pid_list_destroy (&pt->pids);
    close (pt->sock);
    s16mem_free (pt);
}
