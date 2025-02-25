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
#ifndef _SYSTEM_ASYNC_
#define _SYSTEM_ASYNC_

typedef struct {
	Def* READ_ONLY;
	Def* REWRITE;
	Def* READ_WRITE;
	Def* APPEND;
}FileModes;
extern FileModes FM;

int systemFileInit(Pkg* system);

#endif
