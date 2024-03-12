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

Type* compileBreak(Compiler* c)
{
	Type* t;
	if (!c->fmk->breakType) return compileError(c,"nothing to break!\n");
	if (!(t=compileExpression(c))) return NULL;
	if (typeUnify(c,t,c->fmk->breakType)) return NULL;
	if (bufferAddChar(c->th, c->bytecode,OPbreak)) return NULL;
	c->fmk->breakUse=1;
	return t;
}

Type* compileWhile(Compiler* c)
{
	Type* t;
	LINT bc_while,bc_end;

	Type* backupTypeBreak;
	int backupBreakUse;
	LINT bc_break;
	Type* tresult = typeAllocUndef(c->th); if (!tresult) return NULL;

	if (bufferAddChar(c->th, c->bytecode,OPmark)) return NULL;
	bc_break= bytecodeAddEmptyJump(c);
	if (bc_break < 0) return NULL;
	backupTypeBreak = c->fmk->breakType;
	backupBreakUse = c->fmk->breakUse;
	c->fmk->breakType = tresult;
	c->fmk->breakUse = 0;

	if (bufferAddChar(c->th, c->bytecode,OPnil)) return NULL;

	bc_while=bytecodePin(c);
	if (!(t=compileExpression(c))) return NULL;

	if (typeUnify(c,t,MM.Boolean)) return NULL;

	if (parserAssume(c,"do")) return NULL;

	if (bufferAddChar(c->th, c->bytecode,OPelse)) return NULL;
	bc_end=bytecodeAddEmptyJump(c);
	if (bc_end < 0) return NULL;
	if (bufferAddChar(c->th, c->bytecode,OPdrop)) return NULL;
	if (!(t=compileExpression(c))) return NULL;
	if (typeUnify(c,t,tresult)) return NULL;

	if (bufferAddChar(c->th, c->bytecode,OPgoto)) return NULL;
	if (bytecodeAddJump(c,bc_while)) return NULL;
	
	bytecodeSetJump(c,bc_end,bytecodePin(c));

	if (c->fmk->breakUse)
	{
		if (bufferAddChar(c->th, c->bytecode, OPunmark)) return NULL;
		bytecodeSetJump(c, bc_break, bytecodePin(c));
	}
	else
	{
		bufferSetChar(c->bytecode, bc_break - 1, OPgoto);
		bytecodeSetJump(c, bc_break, bc_break + 3);
	}
	c->fmk->breakType = backupTypeBreak;
	c->fmk->breakUse = backupBreakUse;

	return t;
}


Type* compileFor(Compiler* c)
{
	Type* t;
	Type* tlb;
	Locals* localsBefore;
	Locals* localsAfter;
	Locals* iterator=NULL;
	LB* localsBytecode = NULL;
	Type* localsType;
	LINT bc_locals;
	int oneLabelOnly = 0;

	LINT bc_cond,bc_end;
	Type* backupTypeBreak;
	int backupBreakUse;
	LINT bc_break;
	Type* tresult=typeAllocUndef(c->th); if (!tresult) return NULL;

	if (bufferAddChar(c->th, c->bytecode, OPmark)) return NULL;
	bc_break = bytecodeAddEmptyJump(c);
	if (bc_break < 0) return NULL;
	backupTypeBreak = c->fmk->breakType;
	backupBreakUse = c->fmk->breakUse;
	c->fmk->breakType = tresult;
	c->fmk->breakUse = 0;

	localsBefore = c->fmk->locals;

	if ((parserNext(c)) && (islabel(c->parser->token))) oneLabelOnly = 1;
	parserGiveback(c);

	// parse and isolate locals retrieving bytecode
	bc_locals = bufferSize(c->bytecode);
	localsType = compileLocals(c,NULL); if (!localsType) return NULL;
	localsBytecode = memoryAllocBin(c->th, bufferStart(c->bytecode) + bc_locals, bufferSize(c->bytecode) - bc_locals, DBG_BYTECODE); if (!localsBytecode) return NULL;
	bufferCut(c->bytecode, bc_locals);

	if ((!parserNext(c))||( strcmp(c->parser->token,"in") && strcmp(c->parser->token,"=") && strcmp(c->parser->token, "of") && strcmp(c->parser->token, ",")))
		return compileError(c,"'in' or 'of' or '=' expected (found '%s')\n",compileToken(c));

	if (!strcmp(c->parser->token, ","))
	{
		if ((!parserNext(c)) || (!islabel(c->parser->token)))
			return compileError(c,"label expected (found '%s')\n", compileToken(c));
		iterator = funMakerAddLocal(c, c->parser->token); if (!iterator) return NULL;	// this one for the list iterator (type list u0), we need to keep it
		if ((!parserNext(c)) || (strcmp(c->parser->token, "in") && strcmp(c->parser->token, "=") && strcmp(c->parser->token, "of")))
			return compileError(c,"'in' or 'of' or '=' expected (found '%s')\n", compileToken(c));

	}
	if (!strcmp(c->parser->token,"in"))
	{
		Type* tlist = typeAlloc(c->th, TYPECODE_LIST, NULL, 1, localsType); if (!tlist) return NULL;

		if (!iterator) {
			iterator = funMakerAddLocal(c, ""); if (!iterator) return NULL;	// this one for the list iterator (type list u0), we need to keep it
		}
		iterator->type = tlist;
		localsAfter = c->fmk->locals;
		c->fmk->locals=localsBefore;
		if (!(tlb=compileExpression(c))) return NULL;
		if (typeUnify(c,tlb,tlist)) return NULL;

		c->fmk->locals=localsAfter;
		if (bc_byte_or_int(c, iterator->index,OPslocb,OPsloc)) return NULL;	// set list iterator

		if (bufferAddChar(c->th, c->bytecode,OPnil)) return NULL;
		bc_cond=bytecodePin(c);
		if (bc_byte_or_int(c, iterator->index,OPrlocb,OPrloc)) return NULL;
		if (bufferAddChar(c->th, c->bytecode,OPnil)) return NULL;
		if (bufferAddChar(c->th, c->bytecode,OPne)) return NULL;
	
		if (bufferAddChar(c->th, c->bytecode,OPelse)) return NULL;
		bc_end=bytecodeAddEmptyJump(c);
		if (bc_end < 0) return NULL;
		if (parserAssume(c,"do")) return NULL;
		if (bufferAddChar(c->th, c->bytecode,OPdrop)) return NULL;

		if (bc_byte_or_int(c, iterator->index, OPrlocb, OPrloc)) return NULL;
		if (bufferAddChar(c->th, c->bytecode, OPhd)) return NULL;
		if (bufferAddBin(c->th, c->bytecode, BIN_START(localsBytecode),BIN_LENGTH(localsBytecode))) return NULL;

		if (!(t=compileExpression(c))) return NULL;
		if (typeUnify(c,t,tresult)) return NULL;

		if (bc_byte_or_int(c, iterator->index,OPrlocb,OPrloc)) return NULL;
		if (bufferAddChar(c->th, c->bytecode,OPtl)) return NULL;
		if (bc_byte_or_int(c, iterator->index,OPslocb,OPsloc)) return NULL;	// update list iterator
		if (bufferAddChar(c->th, c->bytecode,OPgoto)) return NULL;
		if (bytecodeAddJump(c,bc_cond)) return NULL;
	}
	else if (!strcmp(c->parser->token, "of"))
	{
		Locals* array, * arrayLen;
		Type* tarray = typeAlloc(c->th, TYPECODE_ARRAY, NULL, 1, localsType); if (!tarray) return NULL;

		if (!iterator) {
			iterator = funMakerAddLocal(c, ""); if (!iterator) return NULL;	// this one for the list iterator (type list u0), we need to keep it
		}
		iterator->type = MM.Int;
		array = funMakerAddLocal(c, ""); if (!array) return NULL;
		arrayLen = funMakerAddLocal(c, ""); if (!arrayLen) return NULL;
		array->type = tarray;
		arrayLen->type = MM.Int;

		localsAfter = c->fmk->locals;
		c->fmk->locals = localsBefore;
		if (!(tlb = compileExpression(c))) return NULL;
		if (typeUnify(c, tlb, tarray)) return NULL;

		c->fmk->locals = localsAfter;

		if (bufferAddChar(c->th, c->bytecode, OPdup)) return NULL;	// so that we can chain sloc and len
		if (bc_byte_or_int(c, array->index, OPslocb, OPsloc)) return NULL;	// set array iterator
		if (bufferAddChar(c->th, c->bytecode, OParraylen)) return NULL;
		if (bc_byte_or_int(c, arrayLen->index, OPslocb, OPsloc)) return NULL;	// set array len

		if (bcint_byte_or_int(c, 0)) return NULL;	// set array index
		if (bc_byte_or_int(c, iterator->index, OPslocb, OPsloc)) return NULL;	// set array index

		if (bufferAddChar(c->th, c->bytecode, OPnil)) return NULL;
		bc_cond = bytecodePin(c);
		if (bc_byte_or_int(c, iterator->index, OPrlocb, OPrloc)) return NULL;
		if (bc_byte_or_int(c, arrayLen->index, OPrlocb, OPrloc)) return NULL;
		if (bufferAddChar(c->th, c->bytecode, OPlt)) return NULL;

		if (bufferAddChar(c->th, c->bytecode, OPelse)) return NULL;
		bc_end = bytecodeAddEmptyJump(c);
		if (bc_end < 0) return NULL;
		if (parserAssume(c, "do")) return NULL;
		if (bufferAddChar(c->th, c->bytecode, OPdrop)) return NULL;
		if (bc_byte_or_int(c, array->index, OPrlocb, OPrloc)) return NULL;	// set array index
		if (bc_byte_or_int(c, iterator->index, OPrlocb, OPrloc)) return NULL;
		if (bufferAddChar(c->th, c->bytecode, OPfetch)) return NULL;

		if (bufferAddBin(c->th, c->bytecode, BIN_START(localsBytecode), BIN_LENGTH(localsBytecode))) return NULL;
		
		if (!(t = compileExpression(c))) return NULL;
		if (typeUnify(c, t, tresult)) return NULL;

		if (bc_byte_or_int(c, iterator->index, OPrlocb, OPrloc)) return NULL;	// i+1
		if (bufferAddChar(c->th, c->bytecode, OPintb)) return NULL;
		if (bufferAddChar(c->th, c->bytecode, 1)) return NULL;
		if (bufferAddChar(c->th, c->bytecode, OPadd)) return NULL;
		if (bc_byte_or_int(c, iterator->index, OPslocb, OPsloc)) return NULL;
		if (bufferAddChar(c->th, c->bytecode, OPgoto)) return NULL;
		if (bytecodeAddJump(c, bc_cond)) return NULL;
	}
	else
	{
		if (iterator) return compileError(c,"simple loops don't accept a second iterator\n");
		
		// case for x= ...
		iterator = c->fmk->locals;
		localsAfter = c->fmk->locals;
		c->fmk->locals = localsBefore;
		if (!(tlb = compileExpression(c))) return NULL;
		if (typeUnify(c, tlb, localsType)) return NULL;
		c->fmk->locals = localsAfter;

		if (bufferAddBin(c->th, c->bytecode, BIN_START(localsBytecode), BIN_LENGTH(localsBytecode))) return NULL;

		if (bufferAddChar(c->th, c->bytecode, OPnil)) return NULL;
		bc_cond = bytecodePin(c);
		if ((!parserNext(c)) || strcmp(c->parser->token, ";")) return compileError(c,"';' expected (found %s)\n", compileToken(c));

		if (!(t = compileExpression(c))) return NULL;
		if (typeUnify(c, t, MM.Boolean)) return NULL;

		if (bufferAddChar(c->th, c->bytecode, OPelse)) return NULL;
		bc_end = bytecodeAddEmptyJump(c);
		if (bc_end < 0) return NULL;

		if (!parserNext(c)) return compileError(c,"';'  or 'do' expected (found %s)\n", compileToken(c));

		if (!strcmp(c->parser->token, ";"))	// with a "next" expression: for i=v0; cond; next do ...
		{
			Type* tinc;
			LINT bc_expr, bc_next;
			if (bufferAddChar(c->th, c->bytecode, OPgoto)) return NULL;
			bc_expr = bytecodeAddEmptyJump(c);
			if (bc_expr < 0) return NULL;

			bc_next = bytecodePin(c);
			if (!(tinc = compileExpression(c))) return NULL;

			if (typeUnify(c, localsType, tinc)) return NULL;
			if (bufferAddBin(c->th, c->bytecode, BIN_START(localsBytecode), BIN_LENGTH(localsBytecode))) return NULL;

			if (bufferAddChar(c->th, c->bytecode, OPgoto)) return NULL;
			if (bytecodeAddJump(c, bc_cond)) return NULL;

			if (parserAssume(c, "do")) return NULL;

			bytecodeSetJump(c, bc_expr, bytecodePin(c));
			if (bufferAddChar(c->th, c->bytecode, OPdrop)) return NULL;

			if (!(t = compileExpression(c))) return NULL;
			if (typeUnify(c, t, tresult)) return NULL;

			if (bufferAddChar(c->th, c->bytecode, OPgoto)) return NULL;
			if (bytecodeAddJump(c, bc_next)) return NULL;
		}
		else	// without "next" expression: for i=v0; cond do ...
		{
			if (!oneLabelOnly) return compileError(c,"simple loops don't accept a composite iterator\n");
			parserGiveback(c);
			if (typeUnify(c, localsType, MM.Int)) return NULL;
			if (parserAssume(c, "do")) return NULL;
			if (bufferAddChar(c->th, c->bytecode, OPdrop)) return NULL;

			if (!(t = compileExpression(c))) return NULL;
			if (typeUnify(c, t, tresult)) return NULL;

			if (bc_byte_or_int(c, iterator->index, OPrlocb, OPrloc)) return NULL;	// i+1
			if (bufferAddChar(c->th, c->bytecode, OPintb)) return NULL;
			if (bufferAddChar(c->th, c->bytecode, 1)) return NULL;
			if (bufferAddChar(c->th, c->bytecode, OPadd)) return NULL;
			if (bc_byte_or_int(c, iterator->index, OPslocb, OPsloc)) return NULL;
			if (bufferAddChar(c->th, c->bytecode, OPgoto)) return NULL;
			if (bytecodeAddJump(c, bc_cond)) return NULL;
		}
	}
	bytecodeSetJump(c,bc_end,bytecodePin(c));
	c->fmk->locals=localsBefore;

	if (c->fmk->breakUse)
	{
		if (bufferAddChar(c->th, c->bytecode, OPunmark)) return NULL;
		bytecodeSetJump(c, bc_break, bytecodePin(c));
	}
	else
	{
		bufferSetChar(c->bytecode, bc_break - 1, OPgoto);
		bytecodeSetJump(c, bc_break, bc_break + 3);
	}
	c->fmk->breakType = backupTypeBreak;
	c->fmk->breakUse = backupBreakUse;

	return tresult;
}
