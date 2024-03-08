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
		Def* consDef;
		LB* consName;
		LINT argc=0;
		LINT pIndex;
		LB* labelList;
		LB* labelNext;
		if ((!parserNext(c))||(!islabel(c->parser->token)))
			return compileError(c,"Compiler: constructor name expected (found '%s')\n",compileToken(c));
		pIndex = parserIndex(c);

		labelList = exportLabelList(c, c->parser->token);
		labelNext = labelList ? TABPNT(labelList, LIST_NXT) : NULL;

		if (pkgFirstGet(c->pkg, c->parser->token)) return compileError(c, "Compiler: '%s' already defined\n", compileToken(c));
		consName=memoryAllocStr(c->th, c->parser->token,-1); if (!consName) return NULL;
		while(parserNext(c)&& strcmp(c->parser->token, ",") && strcmp(c->parser->token, ";;"))
		{
			if (labelList) {
				if (!labelNext) return compileError(c, "Compiler: more arguments than in the export declaration\n");
				if (strcmp(STRSTART(TABPNT(labelNext, LIST_VAL)), c->parser->token))
					return compileError(c, "Compiler: argument name '%s' does not match with export declaration '%s'\n", c->parser->token, STRSTART(TABPNT(labelNext, LIST_VAL)));
				labelNext = TABPNT(labelNext, LIST_NXT);
			}
			// we accept label, label@Type, @Type
			if (strcmp(c->parser->token, "@")) {	// there should be a label
				if ((!islabel(c->parser->token))||(!parserNext(c))) return compileError(c, "Compiler: ',' or ';;' expected (found '%s')\n", compileToken(c));
			}
			if (!strcmp(c->parser->token, "@")) {
				if (compilerSkipTypeDef(c)) return NULL;
			}
			else parserGiveback(c); 
			TYPEPUSH_NULL(c,typeAllocUndef(c->th));
			argc++;
		}
		if (labelList && labelNext) return compileError(c, "Compiler: less arguments than in the export declaration\n");
		parserGiveback(c);

		if (argc)
		{
			if (!startsWithLowercase(STRSTART(consName))) return compileError(c, "Compiler: constructors names should start with lowercase (found '%s')\n", STRSTART(consName));
			TYPEPUSH_NULL(c, typeAllocUndef(c->th));
			consType = typeAllocFromStack(c->th, NULL, TYPECODE_FUN, argc + 1);  if (!consType) return NULL;
			consDef = defAlloc(c->th, DEF_CODE_CONS, 0, NIL, VAL_TYPE_PNT, consType); if (!consDef) return NULL;
		}
		else
		{
//			if (!startsWithUppercase(STRSTART(consName))) return compileError(c, "Compiler: constant constructors names should start with lowercase (found '%s')\n", STRSTART(consName));
			consDef = defAlloc(c->th, DEF_CODE_CONS0, 0, NIL, VAL_TYPE_PNT, typeAllocUndef(c->th)); if (!consDef) return NULL;
		}
		consDef->proto = 1;
		defSetParser(consDef, c, pIndex);
		if (pkgAddDef(c->th, c->pkg, consName, consDef)) return NULL;

		if ((!parserNext(c))||(strcmp(c->parser->token,",")&&strcmp(c->parser->token,";;")))
			return compileError(c,"Compiler: ',' or ';;' expected (found '%s')\n",compileToken(c));
		if (!strcmp(c->parser->token, ";;")) return MM.I;
	}
}

Type* compileDefCons2(Compiler* c,Def* sumDef, Locals* labels0)
{
	Type* sumType=sumDef->type;
	while(1)
	{
		Type* consType;
		Def* consDef;
		LINT argc=0;
		// this can be used either with type or extend
		if ((!parserNext(c))||(!islabel(c->parser->token)))
			return compileError(c,"Compiler: constructor name expected (found '%s')\n",compileToken(c));
		consDef = pkgFirstGet(c->pkg, c->parser->token);
		if (!consDef) return compileError(c, "Compiler: cannot find prototype of '%s' in '%s'\n", compileToken(c),pkgName(c->pkg));

		while(parserNext(c)&& strcmp(c->parser->token, ",") && strcmp(c->parser->token, ";;"))
		{
			Type* t;
			Locals* labels = labels0;

			// we have either label, label@Type, @Type
			if (islabel(c->parser->token)) {
				if (!parserNext(c)) return compileError(c, "Compiler: ',' or ';;' expected (found '%s')\n", compileToken(c));
			}
			if (!strcmp(c->parser->token, "@")) {
				t = compilerParseTypeDef(c, 1, &labels);
				if (!t) return NULL;
				TYPEPUSH_NULL(c, t);
			}
			else {
				parserGiveback(c);
				TYPEPUSH_NULL(c, typeAllocWeak(c->th));
			}
			argc++;
		}
		parserGiveback(c);

		if (argc)
		{
			TYPEPUSH_NULL(c, sumType);
			consType = typeAllocFromStack(c->th, NULL, TYPECODE_FUN, argc + 1);  if (!consType) return NULL;
			if (typeUnify(c, consType, consDef->type)) return NULL;
		}
		else if (typeUnify(c, sumType, consDef->type)) return NULL;
		consDef->proto = 0;

		consDef->index = sumDef->index++;
		defSet(consDef,sumDef->val,sumDef->valType);
		defSet(sumDef,PNTTOVAL((LB*)consDef),VAL_TYPE_PNT);
		consDef->parent = sumDef;

		if ((!parserNext(c))||(strcmp(c->parser->token,",")&&strcmp(c->parser->token,";;")))
			return compileError(c,"Compiler: ',' or ';;' expected (found '%s')\n",compileToken(c));

		if (!strcmp(c->parser->token, ";;"))
		{
			sumDef->proto = 0;
			return sumType;
		}
	}
}

Type* compileCons0(Compiler* c, Def* def)
{
	LINT global;
	if (funMakerNeedGlobal(c->fmk, (LB*)def, &global)) return NULL;
	if (bc_byte_or_int(c, global, OPconstb, OPconst)) return NULL;
//	return typeCopy(def->type);
	return typeInstance(c, def);
}

Type* compileCons(Compiler* c,Def* def)
{
	Type* t;
	Type* t0;
	LINT i;
	LINT global;
	
	if (def->type->nb<2) return compileError(c, "Compiler: constructor without value, this should never happen\n");
	t0= typeInstance(c, def); if (!t0) return NULL;

	if (bcint_byte_or_int(c, def->index)) return NULL;
	for (i = 0; i < t0->nb - 1; i++)
	{
		if (!(t = compileExpression(c))) return NULL;
		if (typeUnify(c, t, t0->child[i])) return NULL;
	}
	if (t0->nb>1) {
		if (!parserNext(c)) return compileError(c, "Compiler: unexpected end of file\n");
		if (!parserIsFinal(c)) return compileError(c, "Compiler: too many argument(s) for function '%s' (should be "LSD")\n", defName(def), t0->nb-1);
		parserGiveback(c);
	}

	if (funMakerNeedGlobal(c->fmk, (LB*)def, &global)) return NULL;
	if (bcint_byte_or_int(c, global)) return NULL;
	if (bufferAddChar(c->th, c->bytecode, OPsum)) return NULL;
	return t0->child[t0->nb - 1];
}

