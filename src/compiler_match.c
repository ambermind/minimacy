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


Type* compileMatchNext(Compiler* c,Type* tval,Type* tresult,LINT* end,LINT bc_else,int trycatch)
{
	LINT bc_goto;
	Type* t;
	if (bufferAddchar(c->th, c->bytecode,OPgoto)) return NULL;
	bc_goto=bytecodeAddEmptyJump(c);
	if (bc_goto < 0) return NULL;
	bytecodeSetJump(c,bc_else,bytecodePin(c));
	if ((parserNext(c))&&(!strcmp(c->parser->token,",")))
	{
		if (!(t=compileMatchChoice(c,tval,tresult,end,trycatch))) return NULL;
		bytecodeSetJump(c,bc_goto,*end);
		return tresult;
	}
	parserGiveback(c);
	if (trycatch)
	{
		if (bufferAddchar(c->th, c->bytecode,OPthrow)) return NULL;
	}
	else
	{
		if (bufferAddchar(c->th, c->bytecode,OPdrop)) return NULL;
		if (bufferAddchar(c->th, c->bytecode,OPnil)) return NULL;
	}
	*end=bytecodePin(c);
	bytecodeSetJump(c,bc_goto,*end);
	return tresult;
}
Type* compileMatchValue(Compiler* c,Type* tval,Type* tresult,LINT* end,int trycatch)
{
	Type* t;
	LINT bc_else;
	parserGiveback(c);
	if (bufferAddchar(c->th, c->bytecode,OPdup)) return NULL;
	if (!(t=compileExpression(c))) return NULL;	// read the expression
	if (typeUnify(c,t,tval)) return NULL;

	if (bufferAddchar(c->th, c->bytecode,OPeq)) return NULL;
	if (bufferAddchar(c->th, c->bytecode,OPelse)) return NULL;
	bc_else= bytecodeAddEmptyJump(c);
	if (bc_else < 0) return NULL;

	if (parserAssume(c,"->")) return NULL;
	if (bufferAddchar(c->th, c->bytecode,OPdrop)) return NULL;
	if (!(t= compileExpression(c))) return NULL;
	if (typeUnify(c,t,tresult)) return NULL;

	return compileMatchNext(c,tval,tresult,end,bc_else,trycatch);

}
Type* compileMatchStruct(Compiler* c, Type* tval, Type* tresult, LINT* end, Ref* ref, int trycatch)
{
	LINT i;
	Ref* root;
	Type* t;
	Type* t0;
	Type* tchild;
	LINT bc_else;
	Locals* locals;
	Locals* localsBefore;

	root = ref;
	while (root->parent) root = root->parent;
	if (ref->type->nb!=root->type->nb) return compileError(c, "Compiler: no cast between %s and %s (different number of parameters)\n", refName(ref), refName(root));

	t0 = typeCopy(c->th, root->type); if (!t0) return NULL;
	tchild = typeCopy(c->th, ref->type); if (!tchild) return NULL;
	for (i = 0; i < tchild->nb; i++) if (typeUnify(c, tchild->child[i], t0->child[i])) return NULL;

	t0 = typeDerivate(c->th, t0, 0); if (!t0) return NULL;
	if (typeUnify(c, t0, tval)) return NULL;

	if (bufferAddchar(c->th, c->bytecode, OPdup)) return NULL;

	root = ref;
	while (root->parent)
	{
		if (bc_byte_or_int(c, root->parent->index, OPpickb, OPpick)) return NULL;
		if (bc_byte_or_int(c, root->dI, OPcastb, OPcast)) return NULL;
		root = root->parent;
	}
	if (bufferAddchar(c->th, c->bytecode, OPnil)) return NULL;
	if (bufferAddchar(c->th, c->bytecode, OPne)) return NULL;
	if (bufferAddchar(c->th, c->bytecode, OPelse)) return NULL;
	bc_else = bytecodeAddEmptyJump(c);
	if (bc_else < 0) return NULL;

	localsBefore = c->fmk->locals;

	if ((!parserNext(c)) || !islabel(c->parser->token)) return compileError(c, "Compiler: label expected (found '%s')\n", compileToken(c));

	locals = funMakerAddLocal(c, c->parser->token); if (!locals) return NULL;

	if (bc_byte_or_int(c, locals->index, OPslocb, OPsloc)) return NULL;
	if (typeUnify(c, tchild, locals->type)) return NULL;

	if (parserAssume(c, "->")) return NULL;
	if (!(t = compileExpression(c))) return NULL;
	if (typeUnify(c, t, tresult)) return NULL;

	c->fmk->locals = localsBefore;

	return compileMatchNext(c, tval, tresult, end, bc_else, trycatch);
}
Type* compileMatchCons0(Compiler* c, Type* tval, Type* tresult, LINT* end, Ref* ref, int trycatch)
{
	Type* t;
	Type* t0;
	LINT bc_else;

	if (bufferAddchar(c->th, c->bytecode, OPdup)) return NULL;
	t0 = compileCons0(c,ref);

	if (typeUnify(c, t0, tval)) return NULL;
	if (bufferAddchar(c->th, c->bytecode, OPeq)) return NULL;
	if (bufferAddchar(c->th, c->bytecode, OPelse)) return NULL;
	bc_else = bytecodeAddEmptyJump(c);
	if (bc_else < 0) return NULL;

	if (parserAssume(c, "->")) return NULL;
	if (bufferAddchar(c->th, c->bytecode, OPdrop)) return NULL;
	if (!(t = compileExpression(c))) return NULL;
	if (typeUnify(c, t, tresult)) return NULL;

	return compileMatchNext(c, tval, tresult, end, bc_else, trycatch);
}
Type* compileMatchCons(Compiler* c,Type* tval,Type* tresult,LINT* end, Ref* ref,int trycatch)
{
	Type* t;
	Type* t0;
	LINT bc_else,argc,i;
	Locals* localsBefore;

	argc=ref->type->nb-1;	// remove result
	t0= typeInstance(c, ref); if (!t0) return NULL;

	if (bufferAddchar(c->th, c->bytecode,OPfirst)) return NULL;
	if (bcint_byte_or_int(c,ref->index)) return NULL;
	if (bufferAddchar(c->th, c->bytecode,OPeq)) return NULL;
	if (bufferAddchar(c->th, c->bytecode,OPelse)) return NULL;
	bc_else= bytecodeAddEmptyJump(c);
	if (bc_else < 0) return NULL;

	localsBefore=c->fmk->locals;

	for(i=0;i<argc;i++)
	{
		if (!parserNext(c)) return compileError(c,"Compiler: unexpected end of file\n");

		if (strcmp(c->parser->token,"_"))
		{
			parserGiveback(c);
			if (bufferAddchar(c->th, c->bytecode,OPdup)) return NULL;
			if (bufferAddchar(c->th, c->bytecode,OPfetchb)) return NULL;
			if (bufferAddchar(c->th, c->bytecode,(char)i+1)) return NULL;	// skip index

			t = compileLocals(c,NULL); if (!t) return NULL;
			if (typeUnify(c, t0->child[i], t)) return NULL;
		}
	}
	bufferAddchar(c->th, c->bytecode,OPdrop);
	if (typeUnify(c, t0->child[t0->nb - 1],tval)) return NULL;

	if (parserAssume(c,"->")) return NULL;
	if (!(t= compileExpression(c))) return NULL;
	if (typeUnify(c,t,tresult)) return NULL;

	c->fmk->locals=localsBefore;

	return compileMatchNext(c,tval,tresult,end,bc_else,trycatch);
}
Type* compileMatchChoice(Compiler* c,Type* tval,Type* tresult,LINT* end,int trycatch)
{
	Type* t;
	Ref* ref;

	if (!parserNext(c)) return compileError(c,"Compiler: constructor or expression or '_' expected (found '%s')\n",compileToken(c));

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
		ref=compileGetRef(c);
		if (ref &&(ref->code==REFCODE_CONS)) return compileMatchCons(c,tval,tresult,end,ref,trycatch);
		if (ref && (ref->code == REFCODE_CONS0)) return compileMatchCons0(c, tval, tresult, end, ref, trycatch);
		if (ref && (ref->code == REFCODE_STRUCT)) return compileMatchStruct(c, tval, tresult, end, ref, trycatch);
	}
//	if (trycatch) return compileError(c,"Compiler: constructor or '_' expected (found '%s')\n",compileToken(c));
	return compileMatchValue(c,tval,tresult,end,trycatch);

}

Type* compileMatch(Compiler* c)
{
	Type* tval;
	Type* tresult;
	LINT end;

	if (!(tval=compileExpression(c))) return NULL;
	tresult = typeAllocUndef(c->th); if (!tresult) return NULL;

	if (parserAssume(c,"with")) return NULL;

	return compileMatchChoice(c,tval,tresult,&end,0);
}
