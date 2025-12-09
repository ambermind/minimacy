// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

#ifdef USE_FS_ANSI_UNIX
#include<unistd.h>
#include<sys/types.h>
#include<pwd.h>
#endif

int hwSetHostUser(char* user)
{
#ifdef USE_FS_ANSI_UNIX
	struct passwd* userinfo = getpwnam(user);
	if (!userinfo) return -1;
	PRINTF(LOG_SYS,"> Change user <%s> uid=%d gid=%d\n", user, userinfo->pw_uid, userinfo->pw_gid);
	if (setgid(userinfo->pw_gid)) return -1;
	if (setuid(userinfo->pw_uid)) return -1;
	return 0;
#else
	return 1;
#endif
}

int hwInit(void)
{
	hwTimeInit();
	hwRandomInit();
	return 0;
}
