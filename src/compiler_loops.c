// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

Type* compileBreak(Compiler* c)
{
	Type* t;
	LINT pin;
	if (!c->fmk->loopType) return compileError(c,"nothing to break!\n");
	if (!(t=compileExpression(c))) return NULL;
	if (typeUnify(c,t,c->fmk->loopType)) return NULL;
	if (bufferAddChar(c->bytecode,OPbreak)) return NULL;
	if (bufferAddChar(c->bytecode, OPgoto)) return NULL;
	pin = bytecodePin(c);
	if (bytecodeAddJumpList(c, c->fmk->breakList)) return NULL;
	c->fmk->breakList=pin;
	return t;
}

Type* compileContinue(Compiler* c)
{
	Type* t;
	LINT pin;
	if (!c->fmk->loopType) return compileError(c, "nothing to continue!\n");
	if (!(t = compileExpression(c))) return NULL;
	if (typeUnify(c, t, c->fmk->loopType)) return NULL;
	if (bufferAddChar(c->bytecode, OPcontinue)) return NULL;
	if (bufferAddChar(c->bytecode, OPgoto)) return NULL;
	pin = bytecodePin(c);
	if (bytecodeAddJumpList(c, c->fmk->continueList)) return NULL;
	c->fmk->continueList = pin;
	return t;
}

Type* compileWhile(Compiler* c)
{
	Type* t;
	LINT bc_while,bc_end;

	Type* backupLoopType= c->fmk->loopType;
	LINT backupBreakList = c->fmk->breakList;
	LINT backupContinueList = c->fmk->continueList;
	LINT bc_loop;
	Type* tresult = typeAllocUndef(); if (!tresult) return NULL;
	c->fmk->loopType = NULL;
	c->fmk->breakList = 0;
	c->fmk->continueList = 0;

	bc_loop = bytecodePin(c);
	if (bufferAddChar(c->bytecode,OPloop)) return NULL;

	bc_while=bytecodePin(c);
	if (!(t=compileExpression(c))) return NULL;

	if (typeUnify(c,t,MM.Boolean)) return NULL;

	if (parserAssume(c,"do")) return NULL;

	c->fmk->loopType = tresult;

	if (bufferAddChar(c->bytecode,OPelse)) return NULL;
	bc_end=bytecodeAddEmptyJump(c);
	if (bc_end < 0) return NULL;
	if (bufferAddChar(c->bytecode,OPdrop)) return NULL;
	if (!(t=compileExpression(c))) return NULL;
	if (typeUnify(c,t,tresult)) return NULL;

	if (bufferAddChar(c->bytecode,OPgoto)) return NULL;
	if (bytecodeAddJump(c,bc_while)) return NULL;
	
	bytecodeSetJump(c,bc_end,bytecodePin(c));

	if (c->fmk->breakList || c->fmk->continueList)
	{
		if (bufferAddChar(c->bytecode, OPbreak)) return NULL;
	}
	else
	{
		bufferSetChar(c->bytecode, bc_loop, OPnil);
	}

	if (c->fmk->breakList)
	{
		bytecodeSetJumpList(c, c->fmk->breakList, bytecodePin(c));
	}
	if (c->fmk->continueList)
	{
		bytecodeSetJumpList(c, c->fmk->continueList, bc_while);
	}

	c->fmk->loopType = backupLoopType;
	c->fmk->breakList = backupBreakList;
	c->fmk->continueList = backupContinueList;

	return t;
}

Type* compileRepeat(Compiler* c)
{
	Type* t;
	LINT bc_repeat, bc_cond;

	Type* backupLoopType = c->fmk->loopType;
	LINT backupBreakList = c->fmk->breakList;
	LINT backupContinueList = c->fmk->continueList;
	LINT bc_loop;
	Type* tresult = typeAllocUndef(); if (!tresult) return NULL;
	c->fmk->loopType = tresult;
	c->fmk->breakList = 0;
	c->fmk->continueList = 0;

	bc_loop = bytecodePin(c);
	if (bufferAddChar(c->bytecode, OPloop)) return NULL;

	bc_repeat = bytecodePin(c);
	if (bufferAddChar(c->bytecode, OPdrop)) return NULL;
	if (!(t = compileExpression(c))) return NULL;
	if (typeUnify(c, t, tresult)) return NULL;

	if (parserAssume(c, "until")) return NULL;
	bc_cond = bytecodePin(c);
	c->fmk->loopType = NULL;
	if (!(t = compileExpression(c))) return NULL;
	if (typeUnify(c, t, MM.Boolean)) return NULL;
	if (bufferAddChar(c->bytecode, OPelse)) return NULL;
	if (bytecodeAddJump(c, bc_repeat)) return NULL;

	if (c->fmk->breakList || c->fmk->continueList)
	{
		if (bufferAddChar(c->bytecode, OPbreak)) return NULL;
	}
	else
	{
		bufferSetChar(c->bytecode, bc_loop, OPnil);
	}

	if (c->fmk->breakList)
	{
		bytecodeSetJumpList(c, c->fmk->breakList, bytecodePin(c));
	}
	if (c->fmk->continueList)
	{
		bytecodeSetJumpList(c, c->fmk->continueList, bc_cond);
	}

	c->fmk->loopType = backupLoopType;
	c->fmk->breakList = backupBreakList;
	c->fmk->continueList = backupContinueList;

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
	Type* backupLoopType = c->fmk->loopType;
	LINT backupBreakList = c->fmk->breakList;
	LINT backupContinueList = c->fmk->continueList;
	LINT bc_loop, bc_iterator;
	Type* tresult=typeAllocUndef(); if (!tresult) return NULL;

	backupLoopType = c->fmk->loopType;
	backupBreakList = c->fmk->breakList;
	c->fmk->loopType = NULL;
	c->fmk->breakList = 0;
	c->fmk->continueList = 0;

	localsBefore = c->fmk->locals;

	if ((parserNext(c)) && (isLabel(c->parser->token))) oneLabelOnly = 1;
	parserRewind(c);

	// parse and isolate locals retrieving bytecode
	bc_locals = bufferSize(c->bytecode);
	localsType = compileLocals(c,NULL); if (!localsType) return NULL;
	localsBytecode = memoryAllocBin(bufferStart(c->bytecode) + bc_locals, bufferSize(c->bytecode) - bc_locals, DBG_BYTECODE); if (!localsBytecode) return NULL;
	bufferCut(c->bytecode, bc_locals);

	if ((!parserNext(c))||( strcmp(c->parser->token,"in") && strcmp(c->parser->token,"=") && strcmp(c->parser->token, "of") && strcmp(c->parser->token, ",")))
		return compileError(c,"'in' or 'of' or '=' expected (found '%s')\n",compileToken(c));

	if (!strcmp(c->parser->token, ","))
	{
		if ((!parserNext(c)) || (!isLabel(c->parser->token)))
			return compileError(c,"label expected (found '%s')\n", compileToken(c));
		iterator = funMakerAddLocal(c, c->parser->token); if (!iterator) return NULL;	// this one for the list iterator (type list u0), we need to keep it
		if ((!parserNext(c)) || (strcmp(c->parser->token, "in") && strcmp(c->parser->token, "=") && strcmp(c->parser->token, "of")))
			return compileError(c,"'in' or 'of' or '=' expected (found '%s')\n", compileToken(c));

	}
	if (!strcmp(c->parser->token,"in"))
	{
		Type* tlist = typeAlloc(TYPECODE_LIST, NULL, 1, localsType); if (!tlist) return NULL;

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

		bc_loop = bytecodePin(c);
		if (bufferAddChar(c->bytecode, OPloop)) return NULL;

		bc_cond=bytecodePin(c);
		if (bc_byte_or_int(c, iterator->index,OPrlocb,OPrloc)) return NULL;
		if (bufferAddChar(c->bytecode,OPnil)) return NULL;
		if (bufferAddChar(c->bytecode,OPne)) return NULL;
	
		if (bufferAddChar(c->bytecode,OPelse)) return NULL;
		bc_end=bytecodeAddEmptyJump(c);
		if (bc_end < 0) return NULL;
		if (parserAssume(c,"do")) return NULL;
		c->fmk->loopType = tresult;
		if (bufferAddChar(c->bytecode,OPdrop)) return NULL;

		if (bc_byte_or_int(c, iterator->index, OPrlocb, OPrloc)) return NULL;
		if (bufferAddChar(c->bytecode, OPhd)) return NULL;
		if (bufferAddBin(c->bytecode, BIN_START(localsBytecode),BIN_LENGTH(localsBytecode))) return NULL;

		if (!(t=compileExpression(c))) return NULL;
		if (typeUnify(c,t,tresult)) return NULL;

		bc_iterator = bytecodePin(c);
		if (bc_byte_or_int(c, iterator->index, OPrlocb, OPrloc)) return NULL;
		if (bufferAddChar(c->bytecode,OPtl)) return NULL;
		if (bc_byte_or_int(c, iterator->index,OPslocb,OPsloc)) return NULL;	// update list iterator
		if (bufferAddChar(c->bytecode,OPgoto)) return NULL;
		if (bytecodeAddJump(c,bc_cond)) return NULL;
	}
	else if (!strcmp(c->parser->token, "of"))
	{
		Locals* array, * arrayLen;
		Type* tarray = typeAlloc(TYPECODE_ARRAY, NULL, 1, localsType); if (!tarray) return NULL;

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

		if (bufferAddChar(c->bytecode, OPdup)) return NULL;	// so that we can chain sloc and len
		if (bc_byte_or_int(c, array->index, OPslocb, OPsloc)) return NULL;	// set array iterator
		if (bufferAddChar(c->bytecode, OParraylen)) return NULL;
		if (bc_byte_or_int(c, arrayLen->index, OPslocb, OPsloc)) return NULL;	// set array len

		if (bcint_byte_or_int(c, 0)) return NULL;	// set array index
		if (bc_byte_or_int(c, iterator->index, OPslocb, OPsloc)) return NULL;	// set array index

		bc_loop = bytecodePin(c);
		if (bufferAddChar(c->bytecode, OPloop)) return NULL;

		bc_cond = bytecodePin(c);
		if (bc_byte_or_int(c, iterator->index, OPrlocb, OPrloc)) return NULL;
		if (bc_byte_or_int(c, arrayLen->index, OPrlocb, OPrloc)) return NULL;
		if (bufferAddChar(c->bytecode, OPlt)) return NULL;

		if (bufferAddChar(c->bytecode, OPelse)) return NULL;
		bc_end = bytecodeAddEmptyJump(c);
		if (bc_end < 0) return NULL;
		if (parserAssume(c, "do")) return NULL;
		c->fmk->loopType = tresult;
		if (bufferAddChar(c->bytecode, OPdrop)) return NULL;
		if (bc_byte_or_int(c, array->index, OPrlocb, OPrloc)) return NULL;	// set array index
		if (bc_byte_or_int(c, iterator->index, OPrlocb, OPrloc)) return NULL;
		if (bufferAddChar(c->bytecode, OPfetch)) return NULL;

		if (bufferAddBin(c->bytecode, BIN_START(localsBytecode), BIN_LENGTH(localsBytecode))) return NULL;
		
		if (!(t = compileExpression(c))) return NULL;
		if (typeUnify(c, t, tresult)) return NULL;

		bc_iterator = bytecodePin(c);
		if (bc_byte_or_int(c, iterator->index, OPrlocb, OPrloc)) return NULL;	// i+1
		if (bufferAddChar(c->bytecode, OPintb)) return NULL;
		if (bufferAddChar(c->bytecode, 1)) return NULL;
		if (bufferAddChar(c->bytecode, OPadd)) return NULL;
		if (bc_byte_or_int(c, iterator->index, OPslocb, OPsloc)) return NULL;
		if (bufferAddChar(c->bytecode, OPgoto)) return NULL;
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

		if (bufferAddBin(c->bytecode, BIN_START(localsBytecode), BIN_LENGTH(localsBytecode))) return NULL;

		bc_loop = bytecodePin(c);
		if (bufferAddChar(c->bytecode, OPloop)) return NULL;

		bc_cond = bytecodePin(c);
		if ((!parserNext(c)) || strcmp(c->parser->token, ";")) return compileError(c,"';' expected (found %s)\n", compileToken(c));

		if (!(t = compileExpression(c))) return NULL;
		if (typeUnify(c, t, MM.Boolean)) return NULL;

		if (bufferAddChar(c->bytecode, OPelse)) return NULL;
		bc_end = bytecodeAddEmptyJump(c);
		if (bc_end < 0) return NULL;

		if (!parserNext(c)) return compileError(c,"';'  or 'do' expected (found %s)\n", compileToken(c));

		if (!strcmp(c->parser->token, ";"))	// with a "next" expression: for i=v0; cond; next do ...
		{
			Type* tinc;
			LINT bc_expr;
			if (bufferAddChar(c->bytecode, OPgoto)) return NULL;
			bc_expr = bytecodeAddEmptyJump(c);
			if (bc_expr < 0) return NULL;

			bc_iterator = bytecodePin(c);
			if (!(tinc = compileExpression(c))) return NULL;

			if (typeUnify(c, localsType, tinc)) return NULL;
			if (bufferAddBin(c->bytecode, BIN_START(localsBytecode), BIN_LENGTH(localsBytecode))) return NULL;

			if (bufferAddChar(c->bytecode, OPgoto)) return NULL;
			if (bytecodeAddJump(c, bc_cond)) return NULL;

			if (parserAssume(c, "do")) return NULL;
			c->fmk->loopType = tresult;

			bytecodeSetJump(c, bc_expr, bytecodePin(c));
			if (bufferAddChar(c->bytecode, OPdrop)) return NULL;

			if (!(t = compileExpression(c))) return NULL;
			if (typeUnify(c, t, tresult)) return NULL;

			if (bufferAddChar(c->bytecode, OPgoto)) return NULL;
			if (bytecodeAddJump(c, bc_iterator)) return NULL;
		}
		else	// without "next" expression: for i=v0; cond do ...
		{
			if (!oneLabelOnly) return compileError(c,"simple loops don't accept a composite iterator\n");
			parserRewind(c);
			if (typeUnify(c, localsType, MM.Int)) return NULL;
			if (parserAssume(c, "do")) return NULL;
			c->fmk->loopType = tresult;
			if (bufferAddChar(c->bytecode, OPdrop)) return NULL;

			if (!(t = compileExpression(c))) return NULL;
			if (typeUnify(c, t, tresult)) return NULL;

			bc_iterator = bytecodePin(c);
			if (bc_byte_or_int(c, iterator->index, OPrlocb, OPrloc)) return NULL;	// i+1
			if (bufferAddChar(c->bytecode, OPintb)) return NULL;
			if (bufferAddChar(c->bytecode, 1)) return NULL;
			if (bufferAddChar(c->bytecode, OPadd)) return NULL;
			if (bc_byte_or_int(c, iterator->index, OPslocb, OPsloc)) return NULL;
			if (bufferAddChar(c->bytecode, OPgoto)) return NULL;
			if (bytecodeAddJump(c, bc_cond)) return NULL;
		}
	}
	bytecodeSetJump(c,bc_end,bytecodePin(c));
	c->fmk->locals=localsBefore;

	if (c->fmk->breakList || c->fmk->continueList)
	{
		if (bufferAddChar(c->bytecode, OPbreak)) return NULL;
	}
	else
	{
		bufferSetChar(c->bytecode, bc_loop, OPnil);
	}

	if (c->fmk->breakList)
	{
		bytecodeSetJumpList(c, c->fmk->breakList, bytecodePin(c));
	}
	if (c->fmk->continueList)
	{
		bytecodeSetJumpList(c, c->fmk->continueList, bc_iterator);
	}

	c->fmk->loopType = backupLoopType;
	c->fmk->breakList = backupBreakList;
	c->fmk->continueList = backupContinueList;

	return tresult;
}
