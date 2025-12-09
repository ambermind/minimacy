// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"


// parsing ([Term]/.field)* (. or [ is to be read again)
Type* compileSetPoint(Compiler* c, Type* t0, int* opstore)
{
	LINT index;
	Type* t;

	*opstore = OPstore;
	while (1)
	{
		Def* def;

		if ((!parserNext(c)) || (strcmp(c->parser->token, ".") && strcmp(c->parser->token, "[")))
			return compileError(c, "'=', '.' or '[' expected (found '%s')\n", compileToken(c));
		index = -1;

		if (!strcmp(c->parser->token, ".")) {
			Type* field;
			if ((!parserNext(c))||(!isLabel(c->parser->token))||(!(def = compileGetDef(c)))||(def->code != DEF_CODE_FIELD))
				return compileError(c, "field name expected (found '%s')\n", compileToken(c));
			field = typeInstance(c, def); if (!field) return NULL;
			if (typeUnify(c, t0, field->child[TYPEFIELD_MAIN])) return NULL;
			t0 = field->child[TYPEFIELD_FIELD];

			index = def->index;
		}
		else {
			Type* u;

			if (!(t = compileExpression(c))) return NULL;

			TYPE_PUSH_NULL(t0);
			TYPE_PUSH_NULL(t);
			u = typeCopy(MM.fun_array_u0_I_u0); if (!u) return NULL;
			if (!(t0 = typeUnifyFromStack(c, u))) return NULL;
			if (parserAssume(c, "]")) return NULL;
		}
		if (!parserNext(c)) return t0;
		if ((strcmp(c->parser->token, "."))&& (strcmp(c->parser->token, "[")))
		{
			parserRewind(c);
			if (index >= 0)
			{
				if (bcint_byte_or_int(c, index)) return NULL;
			}
			return t0;
		}
		if (index >= 0) {
			if (bc_byte_or_int(c, index, OPfetchb, OPfetch)) return NULL;
		}
		else if (bufferAddChar(c->bytecode, OPfetch)) return NULL;
		parserRewind(c);
	}
}

Type* compileSetDefOrLocal(Compiler* c,Type* t0,int local,LINT index,int* opstore)
{
	if (!parserNext(c)) return 0;
	if (strcmp(c->parser->token,".")&& strcmp(c->parser->token, "["))
	{
		if (bcint_byte_or_int(c,index)) return NULL;
		if (local) *opstore=OPsloci;
		else *opstore=OPsglobi;
		parserRewind(c);
		return t0;
	}
	parserRewind(c);
	if (local) {
		if (bc_byte_or_int(c, index, OPrlocb, OPrloc)) return NULL;
	}
	else if (bc_byte_or_int(c,index,OPrglobb,OPrglob)) return NULL;
	return compileSetPoint(c, t0, opstore);
}

// parsing of set ... = ... ('set' has already been read)
Type* compileSet(Compiler* c)
{
	Type* t=NULL;
	Type* tsrc;
	Locals* lb;
	int opstore=-1;

	if (!parserNext(c)) return compileError(c,"definition name expected (found '%s')\n",compileToken(c));
	if (!strcmp(c->parser->token,"("))
	{
		if (!(t = compileTerm(c))) return NULL;
		if (parserAssume(c, ")")) return NULL;
		if ((!parserNext(c))||(strcmp(c->parser->token,".") && strcmp(c->parser->token, "[")))
			return compileError(c, "'=', '.' or '[' expected (found '%s')\n", compileToken(c));
		parserRewind(c);
		if (!(t = compileSetPoint(c, t, &opstore))) return NULL;
	}
	else if (isLabel(c->parser->token))
	{
		lb = localsGet(c->fmk->locals, c->fmk->level, c->parser->token);
		if (lb)
		{
			if (!(t = compileSetDefOrLocal(c, lb->type, 1, lb->index, &opstore))) return NULL;
		}
		else
		{
			Def* def = compileGetDef(c);
			if (def)
			{
				LINT code = def->code;
				if (code == DEF_CODE_CONST) return compileError(c,"constant '%s' cannot be modified\n", compileToken(c));

				if (code == DEF_CODE_VAR)	// variable
				{
					LINT global;
					if (funMakerNeedGlobal(c->fmk, (LB*)def, &global)) return NULL;
					if (!(t = compileSetDefOrLocal(c, def->type, 0, global, &opstore))) return NULL;
				}
			}
		}
	}
	if (opstore==-1) return compileError(c,"variable expected (found '%s')\n",compileToken(c));

	if (parserAssume(c,"=")) return NULL;
	if (!(tsrc=compileExpression(c))) return NULL;
	if (bufferAddChar(c->bytecode,opstore)) return NULL;

	if (typeUnify(c,t,tsrc)) return NULL;
	return t;
}

// return NULL (ie error) or MM.Boolean (because we need something not null)
Type* compileSkipLocal(Compiler* c);
Type* compileSkipLocalsTerm(Compiler* c)
{
	Type* t;
	if (!parserNext(c)) return compileError(c,"term expected (found '%s')\n", compileToken(c));
//	PRINTF(LOG_DEV,"------compileSkipLocalsTerm %s\n", compileToken(c));
	if (!strcmp(c->parser->token, "("))
	{
		t = compileSkipLocal(c); if (!t) return NULL;
		if (parserAssume(c, ")")) return NULL;
		return t;
	}
	if (!strcmp(c->parser->token, "["))
	{
		while (1)
		{
			if (!parserNext(c)) return compileError(c,"locals or ']' expected (found '%s')\n", compileToken(c));

			if (!strcmp(c->parser->token, "]")) return MM.Boolean;
			if (!strcmp(c->parser->token, "_"))
			{
			}
			else
			{
				parserRewind(c);
				if (!(t = compileSkipLocal(c))) return NULL;
			}
			if ((!parserNext(c)) || (strcmp(c->parser->token, ",") && strcmp(c->parser->token, "]")))
				return compileError(c, "',' or ']' expected (found '%s')\n", compileToken(c));
			if (!strcmp(c->parser->token, "]")) parserRewind(c);
		}
	}
	if (!strcmp(c->parser->token, "_")) return MM.Boolean;
	if (isLabel(c->parser->token)) {
		if (!parserNext(c)) return compileError(c,"unexpected end of file\n");
		if (!strcmp(c->parser->token, ":")) {
			if (compilerSkipTypeDef(c)) return NULL;
		}
		else parserRewind(c);
		return MM.Boolean;
	}
	return compileError(c,"unexpected token '%s'\n", compileToken(c));
}

Type* compileSkipLocal(Compiler* c)
{
	Type* t;
//	PRINTF(LOG_DEV,"------compileSkipLocal %d again=%d\n", c->parser->index, c->parser->again);

	t= compileSkipLocalsTerm(c); if (!t) return NULL;

	if (parserNext(c) && !strcmp(c->parser->token, "::")) return compileSkipLocal(c);
	parserRewind(c);

	return t;
}


Type* compileLocals(Compiler* c, int* simple);
Type* compileLocalsTerm(Compiler* c, int* simple)
{
	Type* t;
	if (!parserNext(c)) return compileError(c,"term expected (found '%s')\n", compileToken(c));
//	PRINTF(LOG_DEV,"------compileLocalsTerm %s\n", compileToken(c));

	if (!strcmp(c->parser->token, "("))
	{
		Type* t = compileLocals(c,simple); if (!t) return NULL;
		if (parserAssume(c, ")")) return NULL;
		return t;
	}
	if (!strcmp(c->parser->token, "["))
	{
		int n = 0;
		if (simple) *simple = 0;
		while (1)
		{
			if (!parserNext(c)) return compileError(c,"locals or ']' expected (found '%s')\n", compileToken(c));
			if (!strcmp(c->parser->token, "]"))
			{
				if (bufferAddChar(c->bytecode, OPdrop)) return NULL;
				return typeAllocFromStack(NULL, TYPECODE_TUPLE, n);
			}
			if (!strcmp(c->parser->token, "_"))
			{
				Type* u = typeAllocUndef(); if (!u) return NULL;
				TYPE_PUSH_NULL(u);
				n++;
			}
			else
			{
				parserRewind(c);
				if (bufferAddChar(c->bytecode, OPdup)) return NULL;
				if (bc_byte_or_int(c, n, OPfetchb, OPfetch)) return NULL;
				if (!(t = compileLocals(c, simple))) return NULL;
				TYPE_PUSH_NULL(t);
				n++;
			}
			if ((!parserNext(c)) || (strcmp(c->parser->token, ",") && strcmp(c->parser->token, "]")))
				return compileError(c, "',' or ']' expected (found '%s')\n", compileToken(c));
			if (!strcmp(c->parser->token, "]")) parserRewind(c);

		}
	}
	if (!strcmp(c->parser->token, "_"))
	{
		if (bufferAddChar(c->bytecode, OPdrop)) return NULL;
		return typeAllocUndef();
	}
	if (isLabel(c->parser->token))
	{
		Locals* l;
		l = funMakerAddLocal(c, c->parser->token); if (!l) return NULL;
		if (!parserNext(c)) return compileError(c,"unexpected end of file\n");
		if (!strcmp(c->parser->token, ":")) {
			Type* t = compilerParseTypeDef(c, 0, &c->fmk->typeLabels);
			if (!t) return NULL;
			if (typeUnify(c, l->type, t)) return NULL;
		}
		else parserRewind(c);

		if (bc_byte_or_int(c, l->index, OPslocb, OPsloc)) return NULL;
		return l->type;
	}
	return compileError(c,"unexpected term '%s'\n", compileToken(c));
}

Type* compileLocals(Compiler* c, int* simple)
{
	Type* t;
	LINT firstOpcode = bytecodePin(c);
//	PRINTF(LOG_DEV,"------compileLocals %d again=%d\n", c->parser->index, c->parser->again);

	if (bufferAddChar(c->bytecode, OPnop)) return NULL;
	t = compileLocalsTerm(c, simple); if (!t) return NULL;

	if (parserNext(c) && !strcmp(c->parser->token, "::"))
	{
		Type* tlist = typeAlloc(TYPECODE_LIST, NULL, 1, t); if (!tlist) return NULL;
		if (simple) *simple = 0;
		if ((bytecodePin(c) == firstOpcode + 2) && (bufferGetChar(c->bytecode, firstOpcode + 1) == OPdrop))
			bufferDelete(c->bytecode, firstOpcode, 2);	// remove OPnop, OPdrop
		else
			bufferSetChar(c->bytecode, firstOpcode, OPfirst);
		if (bufferAddChar(c->bytecode, OPfetchb)) return NULL;
		if (bufferAddChar(c->bytecode, 1)) return NULL; 
		
		t = compileLocals(c, simple); if (!t) return NULL;
		if (typeUnify(c, tlist, t)) return NULL;
		return tlist;
	}
	bufferDelete(c->bytecode, firstOpcode, 1);	// remove the OPnop

	parserRewind(c);
	return t;
}


// compile let ... -> ... in ('let' has already been read)
Type* compileLet(Compiler* c)
{
	Type* tsrc;
	Type* t;
	Locals* locals;

//	if (!(tsrc=compileExpression(c))) return NULL;
	if (!(tsrc= compileProgram(c))) return NULL;

	if ((!parserNext(c)) || (strcmp(c->parser->token, "->") && strcmp(c->parser->token, "as")))
		return compileError(c, "'->' or 'as' expected (found '%s')\n", compileToken(c));

	locals=c->fmk->locals;	// we save the list of locals

	if (!(t=compileLocals(c,NULL))) return NULL;
	if (typeUnify(c,tsrc,t)) return NULL;

	if (parserAssume(c,"in")) return NULL;

	if (!(t=compileExpression(c))) return NULL;

	c->fmk->locals=locals;	// we restore the list of locals
	return t;
}
// compile lt ... -> ... in ('let' has already been read)
Type* compileLt(Compiler* c)
{
	Type* tsrc;
	Type* t;
	LB* localsBytecode = NULL;
	Type* localsType;
	LINT bc_locals;
	Locals* localsBefore;
	Locals* localsAfter;

	localsBefore = c->fmk->locals;
	bc_locals = bufferSize(c->bytecode);
	localsType = compileLocals(c, NULL); if (!localsType) return NULL;
	localsBytecode = memoryAllocBin(bufferStart(c->bytecode) + bc_locals, bufferSize(c->bytecode) - bc_locals, DBG_BYTECODE); if (!localsBytecode) return NULL;
	bufferCut(c->bytecode, bc_locals);
	if (parserAssume(c, "=")) return NULL;

	localsAfter = c->fmk->locals;
	c->fmk->locals = localsBefore;
	if (!(tsrc = compileProgram(c))) return NULL;
	if (typeUnify(c, tsrc, localsType)) return NULL;
	c->fmk->locals = localsAfter;

	if (bufferAddBin(c->bytecode, BIN_START(localsBytecode), BIN_LENGTH(localsBytecode))) return NULL;

	if (parserAssume(c, "in")) return NULL;

	if (!(t = compileExpression(c))) return NULL;

	c->fmk->locals = localsBefore;	// we restore the list of locals
	return t;
}


// parsing ([Term]/.champ)* on reading
Type* compileGetPoint(Compiler* c,Type* t0)
{
	Type* t;
	while(1)
	{
		Def* def;
		if (!parserNext(c)) return 0;
		if (!strcmp(c->parser->token, ".")) {
			Type* field;
			if ((!parserNext(c)) || (!isLabel(c->parser->token)) || (!(def = compileGetDef(c))) || (def->code != DEF_CODE_FIELD))
				return compileError(c, "field name expected (found '%s')\n", compileToken(c));
			field = typeInstance(c, def); if (!field) return NULL;

			if (bc_byte_or_int(c,def->index,OPfetchb,OPfetch)) return NULL;
			if (typeUnify(c,t0,field->child[TYPEFIELD_MAIN])) return NULL;
			t0=field->child[TYPEFIELD_FIELD];
		}
		else if (!strcmp(c->parser->token, "[")) {
			Type* u;

			if (!(t=compileExpression(c))) return NULL;

			TYPE_PUSH_NULL(t0);
			TYPE_PUSH_NULL(t);
			u = typeCopy(MM.fun_array_u0_I_u0); if (!u) return NULL;
			if (!(t0=typeUnifyFromStack(c,u))) return NULL;
			if (bufferAddChar(c->bytecode,OPfetch)) return NULL;
			if (parserAssume(c, "]")) return NULL;
		}
		else {
			parserRewind(c);
			return t0;
		}
	}
}

Type* compileUpCast(Compiler* c, Def* to)
{
	Type* t;
	Type* derivate;
	Type* u;
	if (!(t = compileExpression(c))) return NULL;
	u = typeCopy(to->type); if (!u) return NULL;
	if (!(derivate = typeDerivate(u))) return NULL;
	if (typeUnify(c, t, derivate)) return NULL;
	return u;
}
Type* compileDownCast(Compiler* c, Def* from, Def* to)
{
	int i;
	Type* t;
	Type* u;
	if (!typePrimaryIsChild(to, from)) return compileError(c,"no downcast from %s to %s\n", defName(from), defName(to));

	// we need the following condition, else there are typechecking traps
	if (from->type->nb != to->type->nb) return compileError(c,"no downcast from %s to %s (different number of parameters)\n", defName(from), defName(to));
	if (!(t = compileExpression(c))) return NULL;
	u = typeCopy(from->type); if (!u) return NULL;
	if (typeUnify(c, t, u)) return NULL;
	u = typeCopy(to->type); if (!u) return NULL;
	while (t->actual) t = t->actual;
	for (i = 0; i < t->nb; i++) if (typeUnify(c, t->child[i], u->child[i])) return NULL;

	while (to != from)
	{
		if (bc_byte_or_int(c, to->parent->index, OPpickb, OPpick)) return NULL;
		if (bc_byte_or_int(c, to->dI, OPcastb, OPcast)) return NULL;

		to = to->parent;
	}
	return u;
}
Type* compileCast(Compiler* c, Def* to)
{
	int parserSave;
	if (parserAssume(c, "<")) return NULL;
	parserSave = parserIndex(c);
	if (!parserNext(c)) return compileError(c,"unexpected end of file\n");
	if (isLabel(c->parser->token))
	{
		Def*  from = compileGetDef(c);
		if ((from) && (from->code == DEF_CODE_STRUCT)) return compileDownCast(c, from, to);
	}
	parserJump(c, parserSave);
	return compileUpCast(c, to);
}
Type* compileDef(Compiler* c)
{
	LINT code;
	Type* t;
	Def* def;
	Locals* lb=localsGet(c->fmk->locals,c->fmk->level,c->parser->token);
//	PRINTF(LOG_DEV,"token=%s\n", c->parser->token);
	if (lb)
	{
		if (bc_byte_or_int(c,lb->index,OPrlocb,OPrloc)) return NULL;
		return compileGetPoint(c,lb->type);
	}
	if (!strcmp(c->parser->token, "pkg"))	// this is done here instead of in compileTerm so that 'pkg' can be used as a local
	{
		LINT global;
		if (funMakerAddGlobal(c->fmk, (LB*)c->pkg,&global)) return NULL;
		if (bc_byte_or_int(c, global, OPconstb, OPconst)) return NULL;
		return MM.Package;
	}
	def=compileGetDef(c);
	if (!def) return compileError(c,"unknown label '%s'\n",compileToken(c));

	code=def->code;
	if (code == DEF_CODE_STRUCT) return compileCast(c, def);
	if (code == DEF_CODE_CONS) return compileCons(c,def);
	if (code == DEF_CODE_CONS0) return compileCons0(c, def);
	if (code>=0)
	{
		int i;
		LINT global;
		LINT index = def->index;
		LW val = def->val;
		char* name = index== DEF_INDEX_STATIC?(char*)def->name:defName(def);
		char* noParenthesis = strstr(" echo echoLn dump void hexDump hexDumpBytes echoTime arrayDump echoHexLn ", name);
		Type* q = def->type;
		if (noParenthesis && (noParenthesis[-1]!=32 || noParenthesis[strlen(name)]!=32)) noParenthesis=NULL;

		if ((index != DEF_INDEX_STATIC) && (def != c->fmk->def)) q = typeInstance(c, def);
		if (!q) return NULL;

		if ((index!=DEF_INDEX_OPCODE)&&(index!= DEF_INDEX_STATIC))
		{
			if (funMakerNeedGlobal(c->fmk, (LB*)def, &global)) return NULL;
			if (bc_byte_or_int(c,global,OPrglobb,OPrglob)) return NULL;	// or store val and use OPconst ? => no, because val may change during compilation (if prototype)
		}
		i = 0;
		if (noParenthesis) {
			if (!(t = compileExpression(c))) return NULL;
			TYPE_PUSH_NULL(t);
			i = 1;
		} 
		else {
			if (parserAssume(c, "(")) return NULL;
			while (1) {
				if (!parserNext(c)) return compileError(c, "expression or ')' expected (found '%s')\n", compileToken(c));
				if (!strcmp(c->parser->token, ")")) break;
				if (!strcmp(c->parser->token, "*")) {
					if (parserAssume(c, ")")) return NULL;
					if (!(t = compileExpression(c))) return NULL;
					TYPE_PUSH_NULL(t);
					i++;
					break;
				}
				parserRewind(c);
				if (!(t = compileExpression(c))) return NULL;
				TYPE_PUSH_NULL(t);
				i++;
				if ((!parserNext(c)) || (strcmp(c->parser->token, ",") && strcmp(c->parser->token, ")")))
					return compileError(c, "',' or ')' expected (found '%s')\n", compileToken(c));
				if (!strcmp(c->parser->token, ")")) break;
			}
		}
		if (i != code) return compileError(c, "wrong number of argument(s) for function '%s' (%d, should be "LSD")\n", name, i, code);
		if ((index != DEF_INDEX_OPCODE) && (index != DEF_INDEX_STATIC)) {
			if (def->header.pkg == c->pkg) {
				if (bc_byte_or_int(c, code, OPtfcb, OPtfc)) return NULL;
			}
			else
				if (bc_byte_or_int(c, code, OPexecb, OPexec)) return NULL;
		}
		else {
			LINT opcode = (index == DEF_INDEX_OPCODE)?ARRAY_INT(PNT_FROM_VAL(val), FUN_NATIVE_OPCODE):INT_FROM_VAL(val);
			if (bc_opcode(c, opcode)) return NULL;
		}
//		PRINTF(LOG_DEV,"def=%s\n", name);
		return typeUnifyFromStack(c,q);
	}
	else if ((code==DEF_CODE_VAR)||(code==DEF_CODE_CONST))	// read a definition
	{
		if (def->index == DEF_INDEX_STATIC) {
			if (def->type == MM.Float) {
				LFLOAT f = FLOAT_FROM_VAL(def->val);
				if (bufferAddChar(c->bytecode, OPfloat)) return NULL;
				if (bufferAddInt(c->bytecode, *(LINT*)&f)) return NULL;
			}
			else {
				if (bufferAddChar(c->bytecode, OPint)) return NULL;
				if (bufferAddInt(c->bytecode, INT_FROM_VAL(def->val))) return NULL;
			}
			return def->type;
		}
		else {
			LINT global;
			if (funMakerNeedGlobal(c->fmk, (LB*)def, &global)) return NULL;
			if (bc_byte_or_int(c, global, OPrglobb, OPrglob)) return NULL;
			return compileGetPoint(c, def->type);
		}
	}
	return compileError(c,"impossible def '%s')\n",compileToken(c));
}

