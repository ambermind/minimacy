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
	Type* telse;
	LINT bc_catch,bc_next;

	if (bufferAddChar(c->bytecode,OPtry)) return NULL;
	bc_catch= bytecodeAddEmptyJump(c);
	if (bc_catch < 0) return NULL;

	if (!(tresult= compileExpression(c))) return NULL;
	if (bc_byte_or_int(c, 2, OPskipb, OPskip)) return NULL;

	if ((!parserNext(c)) || (strcmp(compileToken(c), "else"))) {
		parserGiveback(c);
		bytecodeSetJump(c, bc_catch, bytecodePin(c));
		return tresult;
	}
	if (bufferAddChar(c->bytecode, OPgoto)) return NULL;
	bc_next = bytecodeAddEmptyJump(c);
	if (bc_next < 0) return NULL;
	bytecodeSetJump(c, bc_catch, bytecodePin(c));

	if (bufferAddChar(c->bytecode, OPdrop)) return NULL;	// remove nil

	if (!(telse = compileExpression(c))) return NULL;
	if (typeUnify(c, tresult, telse)) return NULL;
	bytecodeSetJump(c,bc_next, bytecodePin(c));
	return tresult;
}

// parsing of abort ... ('abort' has already been read)
Type* compileAbort(Compiler* c)
{
	if (bufferAddChar(c->bytecode, OPnil)) return NULL;	// ensure there is at least one value in the stack after the callstack
	if (bufferAddChar(c->bytecode, OPabort)) return NULL;
	return typeAllocUndef();
}

