/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright (c) 1994, 2010, Oracle and/or its affiliates. All rights reserved.
 * Copyright 2012 DEY Storage Systems, Inc.  All rights reserved.
 */
/*
 * Copyright (c) 2013 Joyent, Inc.  All Rights reserved.
 */
/*
 * Pfiles-based utility to get process id attached with particular port in illumos 
 * Copyright (c) 2015 Nexenta Systems Inc
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <dirent.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/udp.h>
#include <arpa/inet.h>
#include <pwd.h>
#include "libproc.h"

static pid_t pid;
static psinfo_t psinfo;
static const char tcp_hdr[] =
"PROTO           IP               PORT   PID    USER     COMMAND\n"
"--------  -------------------    ----- ------ -------- ------------------\n";

static void
print_stat(struct sockaddr *sa, socklen_t len)
{
	struct sockaddr_in *so_in = (struct sockaddr_in *)(void *)sa;
	struct sockaddr_in6 *so_in6 = (struct sockaddr_in6 *)(void *)sa;
	char  abuf[INET6_ADDRSTRLEN];
	struct passwd *pwd;

	if (len == 0)
		return;

	pwd = getpwuid(psinfo.pr_euid);
	switch (sa->sa_family) {
		default:
			return;
		case AF_INET:
			(void) printf("%-8s  %-22s %-5u %-5d  %-8s  %s\n",
			    "AF_INET", inet_ntop(AF_INET, &so_in->sin_addr,
			    abuf, sizeof (abuf)), ntohs(so_in->sin_port),
			    (int)pid, pwd->pw_name, psinfo.pr_psargs);
			return;
		case AF_INET6:
			(void) printf("%-8s  %-22s %-5u %-5d  %-8s  %s\n",
			    "AF_INET6", inet_ntop(AF_INET6, &so_in6->sin6_addr,
			    abuf, sizeof (abuf)), ntohs(so_in->sin_port),
			    (int)pid, pwd->pw_name, psinfo.pr_psargs);
			return;
	}
}

static int
sockstat(void *data, prfdinfo_t *info)
{
	struct ps_prochandle *ph = data;
	long buf[(sizeof (short) + PATH_MAX + sizeof (long) - 1)
		/ sizeof (long)];
	struct sockaddr *sa = (struct sockaddr *)buf;
	socklen_t len;
	mode_t mode = info->pr_mode;

	if ((mode & S_IFMT) == S_IFSOCK && Pstate(ph) != PS_DEAD) {
		len = sizeof (buf);
		if (pr_getsockname(ph, info->pr_fd, sa, &len) == 0 &&
		    (sa->sa_family == AF_INET || sa->sa_family == AF_INET6))
			print_stat(sa, len);

		len = sizeof (buf);
		if (pr_getpeername(ph, info->pr_fd, sa, &len) == 0 &&
		    (sa->sa_family == AF_INET || sa->sa_family == AF_INET6))
			print_stat(sa, len);
		}

		return (0);
}

int
main(int argc, char **argv)
{
	struct ps_prochandle *ph;
	dirent_t *dent;
	DIR *dirp;
	int ret;

	(void) proc_initstdio();

	(void) printf(tcp_hdr);

	dirp = opendir("/proc");
	while ((dent = readdir(dirp)) != NULL) {
		if (dent->d_name[0] == '.')
			continue;

		if ((pid = proc_arg_psinfo(dent->d_name, PR_ARG_PIDS,
		    &psinfo, &ret)) == -1)
			continue;

		if ((ph = Pgrab(pid, PGRAB_FORCE, &ret)) != NULL) {
			if (Pcreate_agent(ph) == 0) {
				proc_unctrl_psinfo(&psinfo);
				(void) Pfdinfo_iter(ph, sockstat, ph);
				Pdestroy_agent(ph);
			}
			Prelease(ph, 0);
			ph = NULL;
		}
	}

	(void) proc_finistdio();

	return (0);
}
