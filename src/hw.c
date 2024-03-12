/* Copyright (c) 2022, Sylvain Huet, Ambermind
   This program is free software: you can redistribute it and/or modify it
   under the terms of the GNU General Public License, version 2.0, as
   published by the Free Software Foundation.
   This program is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
   FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License,
   version 2.0, for more details.
   You should have received a copy of the GNU General Public License along
   with this program. If not, see <https://www.gnu.org/licenses/>. */
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
