// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
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
		parserRewind(c);
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

