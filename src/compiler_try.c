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


Type* compileTry(Compiler* c)
{
	Type* tresult;
	LINT bc_catch,bc_next,end;

	if (bufferAddchar(c->th, c->bytecode,OPtry)) return NULL;
	bc_catch= bytecodeAddEmptyJump(c);
	if (bc_catch < 0) return NULL;

	if (!(tresult=compileProgram(c))) return NULL;
	if (bufferAddchar(c->th, c->bytecode,OPunmark)) return NULL;

	if (bufferAddchar(c->th, c->bytecode,OPgoto)) return NULL;
	bc_next=bytecodeAddEmptyJump(c);
	if (bc_next < 0) return NULL;

	if (parserAssume(c,"catch")) return NULL;
	bytecodeSetJump(c,bc_catch,bytecodePin(c));

	if (!compileMatchChoice(c,MM.Exception,tresult,&end,1)) return NULL;

	bytecodeSetJump(c,bc_next,end);
	return tresult;
}

// parsing of throw ... ('throw' has already been read)
Type* compileThrow(Compiler* c)
{
	Type* t;

	if (!(t=compileExpression(c))) return NULL;
	if (typeUnify(c,t,MM.Exception)) return NULL;
	if (bufferAddchar(c->th, c->bytecode,OPthrow)) return NULL;

	return typeAllocUndef(c->th);
}

