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
#ifndef _PARSER_
#define _PARSER_

int parserFromData(Compiler* p, char* name, LB* data);
Type* parserFromIncludes(Compiler* c, char* name);

int parserNextchar(Compiler* p);
int parserGettoken(Compiler* p);
void parserGiveback(Compiler* p);
char* parserNext(Compiler* p);
int parserAssume(Compiler* p,char* keyword);
int parserUntil(Compiler* p, char* keyword);
int parserGetstring(Compiler* p,Buffer* b);
int parserIsFinal(Compiler* p);
void parserReset(Compiler* p);
void parserRestorechar(Compiler* p);
void parserJump(Compiler* c, int index);
int parserIndex(Compiler* c);
void parserRestoreFromDef(Compiler* c, Def* def);
#endif
