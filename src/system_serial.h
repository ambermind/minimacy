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
#ifndef _SERIAL_
#define _SERIAL_

int fun_serialOpen(Thread* th);
int fun_serialClose(Thread* th);
int fun_serialWrite(Thread* th);
int fun_serialRead(Thread* th);
int fun_serialSocket(Thread* th);

int sysSerialInit(Pkg* system);

#endif
