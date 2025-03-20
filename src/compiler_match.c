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


Type* compileMatchNext(Compiler* c,Type* tval,Type* tresult,LINT* end,LINT bc_else)
{
	LINT bc_goto;
	Type* t;
	if (bufferAddChar(c->bytecode,OPgoto)) return NULL;
	bc_goto=bytecodeAddEmptyJump(c);
	if (bc_goto < 0) return NULL;
	bytecodeSetJump(c,bc_else,bytecodePin(c));
	if ((parserNext(c))&&(!strcmp(c->parser->token,",")))
	{
		if (!(t=compileMatchChoice(c,tval,tresult,end))) return NULL;
		bytecodeSetJump(c,bc_goto,*end);
		return tresult;
	}
	parserGiveback(c);
	if (bufferAddChar(c->bytecode,OPdrop)) return NULL;
	if (bufferAddChar(c->bytecode,OPnil)) return NULL;
	*end=bytecodePin(c);
	bytecodeSetJump(c,bc_goto,*end);
	return tresult;
}
Type* compileMatchValue(Compiler* c,Type* tval,Type* tresult,LINT* end)
{
	Type* t;
	LINT bc_else;
	parserGiveback(c);
	if (bufferAddChar(c->bytecode,OPdup)) return NULL;
	if (!(t=compileExpression(c))) return NULL;	// read the expression
	if (typeUnify(c,t,tval)) return NULL;

	if (bufferAddChar(c->bytecode,OPeq)) return NULL;
	if (bufferAddChar(c->bytecode,OPelse)) return NULL;
	bc_else= bytecodeAddEmptyJump(c);
	if (bc_else < 0) return NULL;

	if (parserAssume(c,"->")) return NULL;
	if (bufferAddChar(c->bytecode,OPdrop)) return NULL;
	if (!(t= compileExpression(c))) return NULL;
	if (typeUnify(c,t,tresult)) return NULL;

	return compileMatchNext(c,tval,tresult,end,bc_else);

}
Type* compileMatchStruct(Compiler* c, Type* tval, Type* tresult, LINT* end, Def* def)
{
	LINT i;
	Def* root;
	Type* t;
	Type* t0;
	Type* tchild;
	LINT bc_else;
	Locals* locals;
	Locals* localsBefore;

	root = def;
	while (root->parent) root = root->parent;
	if (def->type->nb!=root->type->nb) return compileError(c,"no cast between %s and %s (different number of parameters)\n", defName(def), defName(root));

	t0 = typeCopy(root->type); if (!t0) return NULL;
	tchild = typeCopy(def->type); if (!tchild) return NULL;
	for (i = 0; i < tchild->nb; i++) if (typeUnify(c, tchild->child[i], t0->child[i])) return NULL;

	t0 = typeDerivate(t0); if (!t0) return NULL;
	if (typeUnify(c, t0, tval)) return NULL;

	if (bufferAddChar(c->bytecode, OPdup)) return NULL;

	root = def;
	while (root->parent)
	{
		if (bc_byte_or_int(c, root->parent->index, OPpickb, OPpick)) return NULL;
		if (bc_byte_or_int(c, root->dI, OPcastb, OPcast)) return NULL;
		root = root->parent;
	}
	if (bufferAddChar(c->bytecode, OPnil)) return NULL;
	if (bufferAddChar(c->bytecode, OPne)) return NULL;
	if (bufferAddChar(c->bytecode, OPelse)) return NULL;
	bc_else = bytecodeAddEmptyJump(c);
	if (bc_else < 0) return NULL;

	localsBefore = c->fmk->locals;

	if ((!parserNext(c)) || !islabel(c->parser->token)) return compileError(c,"label expected (found '%s')\n", compileToken(c));

	locals = funMakerAddLocal(c, c->parser->token); if (!locals) return NULL;

	if (bc_byte_or_int(c, locals->index, OPslocb, OPsloc)) return NULL;
	if (typeUnify(c, tchild, locals->type)) return NULL;

	if (parserAssume(c, "->")) return NULL;
	if (!(t = compileExpression(c))) return NULL;
	if (typeUnify(c, t, tresult)) return NULL;

	c->fmk->locals = localsBefore;

	return compileMatchNext(c, tval, tresult, end, bc_else);
}
Type* compileMatchCons0(Compiler* c, Type* tval, Type* tresult, LINT* end, Def* def)
{
	Type* t;
	Type* t0;
	LINT bc_else;

	if (bufferAddChar(c->bytecode, OPdup)) return NULL;
	t0 = compileCons0(c,def);

	if (typeUnify(c, t0, tval)) return NULL;
	if (bufferAddChar(c->bytecode, OPeq)) return NULL;
	if (bufferAddChar(c->bytecode, OPelse)) return NULL;
	bc_else = bytecodeAddEmptyJump(c);
	if (bc_else < 0) return NULL;

	if (parserAssume(c, "->")) return NULL;
	if (bufferAddChar(c->bytecode, OPdrop)) return NULL;
	if (!(t = compileExpression(c))) return NULL;
	if (typeUnify(c, t, tresult)) return NULL;

	return compileMatchNext(c, tval, tresult, end, bc_else);
}
Type* compileMatchCons(Compiler* c,Type* tval,Type* tresult,LINT* end, Def* def)
{
	Type* t;
	Type* t0;
	LINT bc_else,argc,i;
	Locals* localsBefore;

	argc=def->type->nb-1;	// remove result
	t0= typeInstance(c, def); if (!t0) return NULL;

	if (bufferAddChar(c->bytecode,OPfirst)) return NULL;
	if (bcint_byte_or_int(c,def->index)) return NULL;
	if (bufferAddChar(c->bytecode,OPeq)) return NULL;
	if (bufferAddChar(c->bytecode,OPelse)) return NULL;
	bc_else= bytecodeAddEmptyJump(c);
	if (bc_else < 0) return NULL;

	localsBefore=c->fmk->locals;

	for(i=0;i<argc;i++)
	{
		if (!parserNext(c)) return compileError(c,"unexpected end of file\n");

		if (strcmp(c->parser->token,"_"))
		{
			parserGiveback(c);
			if (bufferAddChar(c->bytecode,OPdup)) return NULL;
			if (bufferAddChar(c->bytecode,OPfetchb)) return NULL;
			if (bufferAddChar(c->bytecode,(char)i+1)) return NULL;	// skip index

			t = compileLocals(c,NULL); if (!t) return NULL;
			if (typeUnify(c, t0->child[i], t)) return NULL;
		}
	}
	bufferAddChar(c->bytecode,OPdrop);
	if (typeUnify(c, t0->child[t0->nb - 1],tval)) return NULL;

	if (parserAssume(c,"->")) return NULL;
	if (!(t= compileExpression(c))) return NULL;
	if (typeUnify(c,t,tresult)) return NULL;

	c->fmk->locals=localsBefore;

	return compileMatchNext(c,tval,tresult,end,bc_else);
}
Type* compileMatchChoice(Compiler* c,Type* tval,Type* tresult,LINT* end)
{
	Type* t;
	Def* def;

	if (!parserNext(c)) return compileError(c,"constructor or expression or '_' expected (found '%s')\n",compileToken(c));

	if (!strcmp(c->parser->token,"_"))
	{
		Locals* locals= funMakerAddLocal(c, c->parser->token); if (!locals) return NULL;
		if (bc_byte_or_int(c, locals->index, OPslocb, OPsloc)) return NULL;
		locals->type = tval;

		if (parserAssume(c,"->")) return NULL;
		if (!(t= compileExpression(c))) return NULL;
		if (typeUnify(c,t,tresult)) return NULL;
		*end=bytecodePin(c);
		return tresult;
	}

	if (islabel(c->parser->token))
	{
		def=compileGetDef(c);
		if (def &&(def->code==DEF_CODE_CONS)) return compileMatchCons(c,tval,tresult,end,def);
		if (def && (def->code == DEF_CODE_CONS0)) return compileMatchCons0(c, tval, tresult, end, def);
		if (def && (def->code == DEF_CODE_STRUCT)) return compileMatchStruct(c, tval, tresult, end, def);
	}
	return compileMatchValue(c,tval,tresult,end);

}

Type* compileMatch(Compiler* c)
{
	Type* tval;
	Type* tresult;
	LINT end;

	if (!(tval=compileExpression(c))) return NULL;
	tresult = typeAllocUndef(); if (!tresult) return NULL;

	if (parserAssume(c,"with")) return NULL;

	return compileMatchChoice(c,tval,tresult,&end);
}
