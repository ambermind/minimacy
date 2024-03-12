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


// parsing (.Term/field)*
Type* compileSetPoint(Compiler* c, Type* t0, int* opstore)
{
	LINT index;
	Type* t;

	*opstore = OPstore;
	while (1)
	{
		Def* def;
		if (!parserNext(c)) return compileError(c,"expression or field name expected (found '%s')\n", compileToken(c));

		index = -1;
		if ((islabel(c->parser->token))
			&& (def = compileGetDef(c))
			&& (def->code == DEF_CODE_FIELD))
		{
			Type* field = typeInstance(c, def); if (!field) return NULL;
			if (typeUnify(c, t0, field->child[TYPEFIELD_MAIN])) return NULL;
			t0 = field->child[TYPEFIELD_FIELD];

			index = def->index;
		}
		else
		{
			Type* u;
			parserGiveback(c);

			if (!(t = compileTerm(c, 1))) return NULL;
			TYPE_PUSH_NULL(c, t0);
			TYPE_PUSH_NULL(c, t);
			u = typeCopy(c->th, MM.fun_array_u0_I_u0); if (!u) return NULL;
			if (!(t0 = typeUnifyFromStack(c, u))) return NULL;
		}
		if (!parserNext(c)) return t0;
		if (strcmp(c->parser->token, "."))
		{
			parserGiveback(c);
			if (index >= 0)
			{
				if (bcint_byte_or_int(c, index)) return NULL;
			}
			return t0;
		}
		if (index >= 0) {
			if (bc_byte_or_int(c, index, OPfetchb, OPfetch)) return NULL;
		}
		else if (bufferAddChar(c->th, c->bytecode, OPfetch)) return NULL;
	}
}

Type* compileSetDefOrLocal(Compiler* c,Type* t0,int local,LINT index,int* opstore)
{
	if (!parserNext(c)) return 0;
	if (strcmp(c->parser->token,"."))
	{
		if (bcint_byte_or_int(c,index)) return NULL;
		if (local) *opstore=OPsloci;
		else *opstore=OPsglobi;
		parserGiveback(c);
		return t0;
	}
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
		if (!(t = compileTerm(c, 0))) return NULL;
		if (parserAssume(c, ")")) return NULL;
		if (parserAssume(c, ".")) return NULL;
		if (!(t = compileSetPoint(c, t, &opstore))) return NULL;
	}
	else if (islabel(c->parser->token))
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
	if (bufferAddChar(c->th, c->bytecode,opstore)) return NULL;

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
			if (!parserNext(c)) return compileError(c,"']' expected (found '%s')\n", compileToken(c));

			if (!strcmp(c->parser->token, "]")) return MM.Boolean;
			if (!strcmp(c->parser->token, "_"))
			{
			}
			else
			{
				parserGiveback(c);
				if (!(t = compileSkipLocal(c))) return NULL;
			}
		}
	}
	if (!strcmp(c->parser->token, "_")) return MM.Boolean;
	if (islabel(c->parser->token)) {
		if (!parserNext(c)) return compileError(c,"unexpected end of file\n");
		if (!strcmp(c->parser->token, "@")) {
			if (compilerSkipTypeDef(c)) return NULL;
		}
		else parserGiveback(c);
		return MM.Boolean;
	}
	return compileError(c,"unexpected term '%s'\n", compileToken(c));
}

Type* compileSkipLocal(Compiler* c)
{
	Type* t;
//	PRINTF(LOG_DEV,"------compileSkipLocal %d again=%d\n", c->parser->index, c->parser->again);

	t= compileSkipLocalsTerm(c); if (!t) return NULL;

	if (parserNext(c) && !strcmp(c->parser->token, ":")) return compileSkipLocal(c);
	parserGiveback(c);

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
			if (!parserNext(c)) return compileError(c,"']' expected (found '%s')\n", compileToken(c));

			if (!strcmp(c->parser->token, "]"))
			{
				if (bufferAddChar(c->th, c->bytecode, OPdrop)) return NULL;
				return typeAllocFromStack(c->th, NULL, TYPECODE_TUPLE, n);
			}
			else if (!strcmp(c->parser->token, "_"))
			{
				Type* u = typeAllocUndef(c->th); if (!u) return NULL;
				TYPE_PUSH_NULL(c, u);
				n++;
			}
			else
			{
				parserGiveback(c);
				if (bufferAddChar(c->th, c->bytecode, OPdup)) return NULL;
				if (bc_byte_or_int(c, n, OPfetchb, OPfetch)) return NULL;
				if (!(t = compileLocals(c, simple))) return NULL;
				TYPE_PUSH_NULL(c, t);
				n++;
			}
		}
	}
	if (!strcmp(c->parser->token, "_"))
	{
		if (bufferAddChar(c->th, c->bytecode, OPdrop)) return NULL;
		return typeAllocUndef(c->th);
	}
	if (islabel(c->parser->token))
	{
		Locals* l = funMakerAddLocal(c, c->parser->token); if (!l) return NULL;
		if (!parserNext(c)) return compileError(c,"unexpected end of file\n");
		if (!strcmp(c->parser->token, "@")) {
			Type* t = compilerParseTypeDef(c, 0, &c->fmk->typeLabels);
			if (!t) return NULL;
			if (typeUnify(c, l->type, t)) return NULL;
		}
		else parserGiveback(c);

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
	if (bufferAddChar(c->th, c->bytecode, OPnop)) return NULL;
	t = compileLocalsTerm(c, simple); if (!t) return NULL;

	if (parserNext(c) && !strcmp(c->parser->token, ":"))
	{
		Type* tlist = typeAlloc(c->th, TYPECODE_LIST, NULL, 1, t); if (!tlist) return NULL;
		if (simple) *simple = 0;
		if ((bytecodePin(c) == firstOpcode + 2) && (bufferGetChar(c->bytecode, firstOpcode + 1) == OPdrop))
			bufferDelete(c->bytecode, firstOpcode, 2);	// remove OPnop, OPdrop
		else
			bufferSetChar(c->bytecode, firstOpcode, OPfirst);
		if (bufferAddChar(c->th, c->bytecode, OPfetchb)) return NULL;
		if (bufferAddChar(c->th, c->bytecode, 1)) return NULL; 
		
		t = compileLocals(c, simple); if (!t) return NULL;
		if (typeUnify(c, tlist, t)) return NULL;
		return tlist;
	}
	bufferDelete(c->bytecode, firstOpcode, 1);	// remove the OPnop

	parserGiveback(c);
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
	if (parserAssume(c,"->")) return NULL;

	locals=c->fmk->locals;	// we save the list of locals

	if (!(t=compileLocals(c,NULL))) return NULL;
	if (typeUnify(c,tsrc,t)) return NULL;

	if (parserAssume(c,"in")) return NULL;

	if (!(t=compileExpression(c))) return NULL;

	c->fmk->locals=locals;	// we restore the list of locals
	return t;
}


// parsing (.Term/champ)* en lecture
Type* compileGetPoint(Compiler* c,Type* t0)
{
	Type* t;
	while(1)
	{
		Def* def;
		if (!parserNext(c)) return 0;
		if (strcmp(c->parser->token,"."))
		{
			parserGiveback(c);
			return t0;
		}
		if (!parserNext(c)) return compileError(c,"expression or field name expected (found '%s')\n",compileToken(c));

		if ((islabel(c->parser->token))
			&&(def=compileGetDef(c))
			&&(def->code==DEF_CODE_FIELD))
		{
			Type* field = typeInstance(c, def); if (!field) return NULL;

			if (bc_byte_or_int(c,def->index,OPfetchb,OPfetch)) return NULL;
			if (typeUnify(c,t0,field->child[TYPEFIELD_MAIN])) return NULL;
			t0=field->child[TYPEFIELD_FIELD];
		}
		else
		{
			Type* u;
			parserGiveback(c);

			if (!(t=compileTerm(c,1))) return NULL;
			TYPE_PUSH_NULL(c,t0);
			TYPE_PUSH_NULL(c,t);
			u = typeCopy(c->th,MM.fun_array_u0_I_u0); if (!u) return NULL;
			if (!(t0=typeUnifyFromStack(c,u))) return NULL;
			if (bufferAddChar(c->th, c->bytecode,OPfetch)) return NULL;
		}
	}
}

Type* compileUpCast(Compiler* c, Def* to)
{
	Type* t;
	Type* derivate;
	Type* u;
	if (!(t = compileExpression(c))) return NULL;
	u = typeCopy(c->th, to->type); if (!u) return NULL;
	if (!(derivate = typeDerivate(c->th, u, 0))) return NULL;
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
	u = typeCopy(c->th, from->type); if (!u) return NULL;
	if (typeUnify(c, t, u)) return NULL;
	u = typeCopy(c->th, to->type); if (!u) return NULL;
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
/*	// this block would allow to force the type of an expression
*	// imagined for core.db.inMemory to force the type of [db][Table]Id 
*	// eventually not used because this could be achieved by (_[db][Table]< row)
	if (!parserNext(c)) return compileError(c,"unexpected end of file\n");
	if (strcmp(c->parser->token, "<")) {
		Type* t;
		Type* u;
		parserGiveback(c);

		if (!(t = compileExpression(c))) return NULL;
		u = typeCopy(c->th, to->type); if (!u) return NULL;
		if (typeUnify(c, t, u)) return NULL;
		return u;
	}
*/
	if (parserAssume(c, "<")) return NULL;

	if (!parserNext(c)) return compileError(c,"unexpected end of file\n");
	if (islabel(c->parser->token))
	{
		Def*  from = compileGetDef(c);
		if ((from) && (from->code == DEF_CODE_STRUCT)) return compileDownCast(c, from, to);
	}
	parserGiveback(c);
	return compileUpCast(c, to);
}
Type* compileDef(Compiler* c,int noPoint)
{
	LINT code;
	Type* t;
	Def* def;
	Locals* lb=localsGet(c->fmk->locals,c->fmk->level,c->parser->token);
//	PRINTF(LOG_DEV,"token=%s\n", c->parser->token);
	if (lb)
	{
		if (bc_byte_or_int(c,lb->index,OPrlocb,OPrloc)) return NULL;
		if (noPoint) return lb->type;
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
		Type* q;
		LINT global;
		
		if (def->index!=DEF_INDEX_OPCODE)
		{
			if (funMakerNeedGlobal(c->fmk, (LB*)def, &global)) return NULL;
			if (bc_byte_or_int(c,global,OPrglobb,OPrglob)) return NULL;	// or store def->val and use OPconst ? => no, because def->val may change during compilation (if prototype)
		}
		for(i=0;i<code;i++)
		{
			if (!(t=compileExpression(c))) return NULL;
			TYPE_PUSH_NULL(c,t);
		}

		if (code) {
			if (!parserNext(c)) return compileError(c,"unexpected end of file\n");
			if (!parserIsFinal(c)) return compileError(c,"too many argument(s) for function '%s' (should be "LSD")\n", defName(def),code);
			parserGiveback(c);		
		}
		if (def->index != DEF_INDEX_OPCODE) {
			if (bc_byte_or_int(c, code, OPexecb, OPexec)) return NULL;
		}
		else if (bufferAddChar(c->th, c->bytecode,(char)(ARRAY_INT(PNT_FROM_VAL(def->val),FUN_NATIVE_POINTER)))) return NULL;

		q=(def!=c->fmk->def)?typeInstance(c,def):def->type; if (!q) return NULL;
//		PRINTF(LOG_DEV,"def=%s\n", defName(def));
		return typeUnifyFromStack(c,q);
	}
	else if ((code==DEF_CODE_VAR)||(code==DEF_CODE_CONST))	// read a definition
	{
		LINT global;
		if (funMakerNeedGlobal(c->fmk, (LB*)def, &global)) return NULL;
		if (bc_byte_or_int(c,global,OPrglobb,OPrglob)) return NULL;
		if (noPoint) return def->type;
		return compileGetPoint(c,def->type);
	}
	return compileError(c,"impossible def '%s')\n",compileToken(c));
}

