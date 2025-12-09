// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#ifndef _PARSER_
#define _PARSER_

int parserFromData(Compiler* p, char* name, LB* data);
Type* parserFromIncludes(Compiler* c, char* name);

int parserNextchar(Compiler* p);
int parserGettoken(Compiler* p);
void parserRewind(Compiler* p);
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
