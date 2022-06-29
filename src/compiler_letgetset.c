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
		Ref* ref;
		if (!parserNext(c)) return compileError(c, "Compiler: expression or field name expected (found '%s')\n", compileToken(c));

		index = -1;
		if ((islabel(c->parser->token))
			&& (ref = compileGetRef(c))
			&& (ref->code == REFCODE_FIELD))
		{
			Type* field = typeInstance(c, ref); if (!field) return NULL;
			if (typeUnify(c, t0, field->child[TYPEFIELD_MAIN])) return NULL;
			t0 = field->child[TYPEFIELD_FIELD];

			index = ref->index;
		}
		else
		{
			Type* u;
			parserGiveback(c);

			if (!(t = compileTerm(c, 1))) return NULL;
			TYPEPUSH_NULL(c, t0);
			TYPEPUSH_NULL(c, t);
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
		else if (bufferAddchar(c->th, c->bytecode, OPfetch)) return NULL;
	}
}

Type* compileSetRefOrLocal(Compiler* c,Type* t0,int local,LINT index,int* opstore)
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

	if (!parserNext(c)) return compileError(c,"Compiler: reference expected (found '%s')\n",compileToken(c));
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
			if (!(t = compileSetRefOrLocal(c, lb->type, 1, lb->index, &opstore))) return NULL;
		}
		else
		{
			Ref* ref = compileGetRef(c);
			if (ref)
			{
				LINT code = ref->code;
				if (code == REFCODE_CONST) return compileError(c, "Compiler: constant '%s' cannot be modified\n", compileToken(c));

				if (code == REFCODE_VAR)	// variable
				{
					LINT global;
					if (funMakerNeedGlobal(c->fmk, (LB*)ref, &global)) return NULL;
					if (!(t = compileSetRefOrLocal(c, ref->type, 0, global, &opstore))) return NULL;
				}
			}
		}
	}
	if (opstore==-1) return compileError(c,"Compiler: variable expected (found '%s')\n",compileToken(c));

	if (parserAssume(c,"=")) return NULL;
	if (!(tsrc=compileExpression(c))) return NULL;
	if (bufferAddchar(c->th, c->bytecode,opstore)) return NULL;

	if (typeUnify(c,t,tsrc)) return NULL;
	return t;
}

// return NULL (ie error) or MM.Boolean (because we need something not null)
Type* compileSkipLocal(Compiler* c);
Type* compileSkipLocalsTerm(Compiler* c)
{
	Type* t;
	if (!parserNext(c)) return compileError(c, "Compiler: term expected (found '%s')\n", compileToken(c));
//	printf("------compileSkipLocalsTerm %s\n", compileToken(c));
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
			if (!parserNext(c)) return compileError(c, "Compiler: ']' expected (found '%s')\n", compileToken(c));

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
	if (islabel(c->parser->token)) return MM.Boolean;
	return compileError(c, "Compiler: unexpected term '%s'\n", compileToken(c));
}

Type* compileSkipLocal(Compiler* c)
{
	Type* t;
//	printf("------compileSkipLocal %d again=%d\n", c->parser->index, c->parser->again);

	t= compileSkipLocalsTerm(c); if (!t) return NULL;

	if (parserNext(c) && !strcmp(c->parser->token, ":")) return compileSkipLocal(c);
	parserGiveback(c);

	return t;
}


Type* compileLocals(Compiler* c, int* simple);
Type* compileLocalsTerm(Compiler* c, int* simple)
{
	Type* t;
	if (!parserNext(c)) return compileError(c, "Compiler: term expected (found '%s')\n", compileToken(c));
//	printf("------compileLocalsTerm %s\n", compileToken(c));

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
			if (!parserNext(c)) return compileError(c, "Compiler: ']' expected (found '%s')\n", compileToken(c));

			if (!strcmp(c->parser->token, "]"))
			{
				if (bufferAddchar(c->th, c->bytecode, OPdrop)) return NULL;
				return typeAllocFromStack(c->th, NULL, TYPECODE_TUPLE, n);
			}
			else if (!strcmp(c->parser->token, "_"))
			{
				Type* u = typeAllocUndef(c->th); if (!u) return NULL;
				TYPEPUSH_NULL(c, u);
				n++;
			}
			else
			{
				parserGiveback(c);
				if (bufferAddchar(c->th, c->bytecode, OPdup)) return NULL;
				if (bc_byte_or_int(c, n, OPfetchb, OPfetch)) return NULL;
				if (!(t = compileLocals(c, simple))) return NULL;
				TYPEPUSH_NULL(c, t);
				n++;
			}
		}
	}
	if (!strcmp(c->parser->token, "_"))
	{
		if (bufferAddchar(c->th, c->bytecode, OPdrop)) return NULL;
		return typeAllocUndef(c->th);
	}
	if (islabel(c->parser->token))
	{
		Locals* l = funMakerAddLocal(c, c->parser->token); if (!l) return NULL;

		if (bc_byte_or_int(c, l->index, OPslocb, OPsloc)) return NULL;
		return l->type;
	}
	return compileError(c, "Compiler: unexpected term '%s'\n", compileToken(c));
}

Type* compileLocals(Compiler* c, int* simple)
{
	Type* t;
	LINT firstOpcode = bytecodePin(c);
//	printf("------compileLocals %d again=%d\n", c->parser->index, c->parser->again);
	if (bufferAddchar(c->th, c->bytecode, OPnop)) return NULL;
	t = compileLocalsTerm(c, simple); if (!t) return NULL;

	if (parserNext(c) && !strcmp(c->parser->token, ":"))
	{
		Type* tlist = typeAlloc(c->th, TYPECODE_LIST, NULL, 1, t); if (!tlist) return NULL;
		if (simple) *simple = 0;
		if ((bytecodePin(c) == firstOpcode + 2) && (bufferGetchar(c->bytecode, firstOpcode + 1) == OPdrop))
			bufferDelete(c->bytecode, firstOpcode, 2);	// remove OPnop, OPdrop
		else
			bufferSetchar(c->bytecode, firstOpcode, OPfirst);
		if (bufferAddchar(c->th, c->bytecode, OPfetchb)) return NULL;
		if (bufferAddchar(c->th, c->bytecode, 1)) return NULL; 
		
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
		Ref* ref;
		if (!parserNext(c)) return 0;
		if (strcmp(c->parser->token,"."))
		{
			parserGiveback(c);
			return t0;
		}
		if (!parserNext(c)) return compileError(c,"Compiler: expression or field name expected (found '%s')\n",compileToken(c));

		if ((islabel(c->parser->token))
			&&(ref=compileGetRef(c))
			&&(ref->code==REFCODE_FIELD))
		{
			Type* field = typeInstance(c, ref); if (!field) return NULL;

			if (bc_byte_or_int(c,ref->index,OPfetchb,OPfetch)) return NULL;
			if (typeUnify(c,t0,field->child[TYPEFIELD_MAIN])) return NULL;
			t0=field->child[TYPEFIELD_FIELD];
		}
		else
		{
			Type* u;
			parserGiveback(c);

			if (!(t=compileTerm(c,1))) return NULL;
			TYPEPUSH_NULL(c,t0);
			TYPEPUSH_NULL(c,t);
			u = typeCopy(c->th,MM.fun_array_u0_I_u0); if (!u) return NULL;
			if (!(t0=typeUnifyFromStack(c,u))) return NULL;
			if (bufferAddchar(c->th, c->bytecode,OPfetch)) return NULL;
		}
	}
}

Type* compileCast(Compiler* c, Ref* to)
{
	int i;
	Ref* from;
	Type* t;
	Type* u;
	if (parserAssume(c, "<")) return NULL;
	if ((!parserNext(c)) || (!islabel(c->parser->token)))
		return compileError(c, "Compiler: type name expected (found '%s')\n", compileToken(c));
	from = compileGetRef(c);
	if ((!from)||(from->code!= REFCODE_STRUCT)) return compileError(c, "Compiler: struct type expected (found '%s')\n", compileToken(c));
	if (typePrimaryIsChild(from, to))
	{
		if (!(t = compileExpression(c))) return NULL;
		u = typeCopy(c->th,from->type); if (!u) return NULL;
		if (typeUnify(c, t, u)) return NULL;
		u = typeCopy(c->th,to->type); if (!u) return NULL;
		while (t->actual) t = t->actual;
		for(i=0;i<u->nb;i++) if (typeUnify(c, t->child[i], u->child[i])) return NULL;
		return u;
	}
	if (typePrimaryIsChild(to,from))
	{
		// we need the following condition, else there are typechecking traps
		if (from->type->nb!=to->type->nb) return compileError(c, "Compiler: no cast between %s and %s (different number of parameters)\n", refName(from), refName(to));
		if (!(t = compileExpression(c))) return NULL;
		u = typeCopy(c->th,from->type); if (!u) return NULL;
		if (typeUnify(c, t, u)) return NULL;
		u = typeCopy(c->th,to->type); if (!u) return NULL;
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
	return compileError(c, "Compiler: no cast between %s and %s\n", refName(from),refName(to));
}
Type* compileRef(Compiler* c,int noPoint)
{
	LINT code;
	Type* t;
	Ref* ref;
	Locals* lb=localsGet(c->fmk->locals,c->fmk->level,c->parser->token);
//	printf("token=%s\n", c->parser->token);
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
		return MM.Pkg;
	}
	ref=compileGetRef(c);
	if (!ref) return compileError(c,"Compiler: unknown label '%s'\n",compileToken(c));

	code=ref->code;
	if (code == REFCODE_STRUCT) return compileCast(c, ref);
	if (code == REFCODE_CONS) return compileCons(c,ref);
	if (code == REFCODE_CONS0) return compileCons0(c, ref);
	if (code>=0)
	{
		int i;
		Type* q;
		LINT global;
		
		if (ref->index!=REFINDEX_OPCODE)
		{
			if (funMakerNeedGlobal(c->fmk, (LB*)ref, &global)) return NULL;
			if (bc_byte_or_int(c,global,OPrglobb,OPrglob)) return NULL;	// or store ref->val and use OPconst ? => no, because ref->val may change during compilation (if prototype)
		}
		for(i=0;i<code;i++)
		{
			if (!(t=compileExpression(c))) return NULL;
			TYPEPUSH_NULL(c,t);
		}
		if (ref->index != REFINDEX_OPCODE) {
			if (bc_byte_or_int(c, code, OPexecb, OPexec)) return NULL;
		}
		else if (bufferAddchar(c->th, c->bytecode,(char)VALTOINT(TABGET(VALTOPNT(ref->val),FUN_NATIVE_POINTER)))) return NULL;

		q=(ref!=c->fmk->ref)?typeInstance(c,ref):ref->type; if (!q) return NULL;
//		printf("ref=%s\n", refName(ref));
		return typeUnifyFromStack(c,q);
	}
	else if ((code==REFCODE_VAR)||(code==REFCODE_CONST))	// read a reference
	{
		LINT global;
		if (funMakerNeedGlobal(c->fmk, (LB*)ref, &global)) return NULL;
		if (bc_byte_or_int(c,global,OPrglobb,OPrglob)) return NULL;
		if (noPoint) return ref->type;
		return compileGetPoint(c,ref->type);
	}
	return compileError(c,"Compiler: impossible ref '%s')\n",compileToken(c));
}

