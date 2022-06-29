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


Type* compileDefCons1(Compiler* c)
{
	while(1)
	{
		Type* consType;
		Ref* consRef;
		LB* consName;
		LINT argc=0;

		if ((!parserNext(c))||(!islabel(c->parser->token)))
			return compileError(c,"Compiler: constructor name expected (found '%s')\n",compileToken(c));
		if (pkgFirstGet(c->pkg, c->parser->token)) return compileError(c, "Compiler: '%s' already defined\n", compileToken(c));
		consName=memoryAllocStr(c->th, c->parser->token,-1); if (!consName) return NULL;
		while(parserNext(c)&& strcmp(c->parser->token, ",") && strcmp(c->parser->token, ";;"))
		{
			parserGiveback(c); 
			if (compilerSkipTypeDef(c)) return NULL;
			TYPEPUSH_NULL(c,typeAllocUndef(c->th));
			argc++;
		}
		parserGiveback(c);

		if (argc)
		{
			TYPEPUSH_NULL(c, typeAllocUndef(c->th));
			consType = typeAllocFromStack(c->th, NULL, TYPECODE_FUN, argc + 1);  if (!consType) return NULL;
			consRef = refAlloc(c->th, REFCODE_CONS, 0, NIL, consType); if (!consRef) return NULL;
		}
		else
		{
			consRef = refAlloc(c->th, REFCODE_CONS0, 0, NIL, typeAllocUndef(c->th)); if (!consRef) return NULL;
		}
		consRef->proto = 1;
		
		if (pkgAddRef(c->th, c->pkg, consName, consRef)) return NULL;

		if ((!parserNext(c))||(strcmp(c->parser->token,",")&&strcmp(c->parser->token,";;")))
			return compileError(c,"Compiler: ',' or ';;' expected (found '%s')\n",compileToken(c));
		if (!strcmp(c->parser->token, ";;")) return MM.I;
	}
}

Type* compileDefCons2(Compiler* c,Ref* sumRef, Locals* labels0)
{
	Type* sumType=sumRef->type;
	while(1)
	{
		Type* consType;
		Ref* consRef;
		LINT argc=0;
		// this can be used either with type or extend
		if ((!parserNext(c))||(!islabel(c->parser->token)))
			return compileError(c,"Compiler: constructor name expected (found '%s')\n",compileToken(c));
		consRef = pkgFirstGet(c->pkg, c->parser->token);
		if (!consRef) return compileError(c, "Compiler: xcannot find prototype of '%s' in '%s'\n", compileToken(c),pkgName(c->pkg));

		while(parserNext(c)&& strcmp(c->parser->token, ",") && strcmp(c->parser->token, ";;"))
		{
			Type* t;
			Locals* labels = labels0;
			parserGiveback(c); 
			t = compilerParseTypeDef(c, 1, &labels);
			if (!t) return NULL;
			TYPEPUSH_NULL(c,t);
			argc++;
		}
		parserGiveback(c);

		if (argc)
		{
			TYPEPUSH_NULL(c, sumType);
			consType = typeAllocFromStack(c->th, NULL, TYPECODE_FUN, argc + 1);  if (!consType) return NULL;
			if (typeUnify(c, consType, consRef->type)) return NULL;
		}
		else if (typeUnify(c, sumType, consRef->type)) return NULL;
		consRef->proto = 0;

		consRef->index = sumRef->index++;
		consRef->val = sumRef->val;
		sumRef->val = PNTTOVAL((LB*)consRef);
		consRef->parent = sumRef;

		if ((!parserNext(c))||(strcmp(c->parser->token,",")&&strcmp(c->parser->token,";;")))
			return compileError(c,"Compiler: ',' or ';;' expected (found '%s')\n",compileToken(c));

		if (!strcmp(c->parser->token, ";;"))
		{
			sumRef->proto = 0;
			return sumType;
		}
	}
}

Type* compileCons0(Compiler* c, Ref* ref)
{
	LINT global;
	if (funMakerNeedGlobal(c->fmk, (LB*)ref, &global)) return NULL;
	if (bc_byte_or_int(c, global, OPconstb, OPconst)) return NULL;
//	return typeCopy(ref->type);
	return typeInstance(c, ref);
}

Type* compileCons(Compiler* c,Ref* ref)
{
	Type* t;
	Type* t0;
	LINT i;
	LINT global;
	
	if (ref->type->nb<2) return compileError(c, "Compiler: constructor without value, this should never happen\n");
	t0= typeInstance(c, ref); if (!t0) return NULL;

	if (bcint_byte_or_int(c, ref->index)) return NULL;
	for (i = 0; i < t0->nb - 1; i++)
	{
		if (!(t = compileExpression(c))) return NULL;
		if (typeUnify(c, t, t0->child[i])) return NULL;
	}

	if (funMakerNeedGlobal(c->fmk, (LB*)ref, &global)) return NULL;
	if (bcint_byte_or_int(c, global)) return NULL;
	if (bufferAddchar(c->th, c->bytecode, OPsum)) return NULL;
	return t0->child[t0->nb - 1];
}

