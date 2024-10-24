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
#ifndef _TERM_
#define _TERM_

typedef struct
{
	int mask;
	int showBiosListing;
	int showPkgListing;
}Term;

extern Term MainTerm;
#define LOG_SYS 1
#define LOG_USER 2
#define LOG_ALL 3
#define LOG_DEV 1

#define TERM_LIMIT_ENUM 50
#define TERM_LIMIT_BIN 256
void termInit(void);
void termEnd(void);
void termSetMask(int i);
int termGetMask(void);
int termCheckMask(int mask);
void termShowBiosListing(int val);
void termShowPkgListing(int val);

void termWrite(int mask, char* src, LINT len);
void termPrintfv(int mask,char *format,va_list arglist);
void termPrintf(int mask, char* format, ...);

void itemDump(int mask,LW v, int type);
void itemDumpDirect(int mask,LW v, int type);
void threadDump(int mask,Thread* th,LINT n);
void itemEcho(int mask, LW v, int type, int ln);
void itemHeader(int mask,LB* p);

void termEchoSourceLine(Pkg* pkg, int mask,char* srcName, char* src, int index);

void _hexDump(int mask, char* src, LINT len, LINT offsetDisplay);
#define PRINTF termPrintf

#endif
