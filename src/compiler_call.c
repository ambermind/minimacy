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


Type* compilePointer(Compiler* c)
{
	Def* def;
	if (!parserNext(c)) return compileError(c,"function name expected (found '%s')\n",compileToken(c));

	def=compileGetDef(c);
	if (def && def->code >= 0 && def->index == DEF_INDEX_STATIC) def = systemGetNative((char*)def->name);
	if (def && def->code>=0)
	{
		LINT global;
		if (funMakerNeedGlobal(c->fmk, (LB*)def,&global)) return NULL;
		if (bc_byte_or_int(c,global,OPrglobb,OPrglob)) return NULL;
		if (def==c->fmk->def) return def->type;	// pointer for the currently compiled function
//		return def->type;
		return typeInstance(c,def);
	}
	return compileError(c,"function not found or hidden ('%s')\n",compileToken(c));
}

Type* compileDefHide(Compiler* c)
{
	LINT global;
	Def* def;

	if (bufferAddChar(c->bytecode, OPnil)) return NULL;
	while (1) {
		if (!parserNext(c)) return compileError(c, "definition name expected (found '%s')\n", compileToken(c));
		if (!islabel(c->parser->token)) {
			parserGiveback(c);
			return MM.Boolean;
		}
		if (bufferAddChar(c->bytecode, OPdrop)) return NULL;
		def = compileGetDef(c);
		if (def && def->code >= 0 && def->index == DEF_INDEX_STATIC) def = systemMakeNative(0x7fff & INT_FROM_VAL(def->val));
		if (!def) return compileError(c, "definition not found or hidden ('%s')\n", compileToken(c));

		if (funMakerNeedGlobal(c->fmk, (LB*)def, &global)) return NULL;

		if (bufferAddChar(c->bytecode, OPint)) return NULL;
		if (bufferAddInt(c->bytecode, global)) return NULL;
		if (bufferAddChar(c->bytecode, OPhide)) return NULL;
	}
}

Type* compileCall(Compiler* c)
{
	Type* t;
	Type* funType;
	Type* callType;
	Type* result;
	LINT argc=0;
	
	if (!(funType=compileExpression(c))) return NULL;	// lire la fonction

// TODO : ne pas faire la copie si c'est la fonction en cours de definition ? mais comment le savoir lors de la compilation ???
//	funType=typeCopy(funType);
	while((parserNext(c))&&(!parserIsFinal(c)))
	{
		parserGiveback(c);
		if (!(t=compileExpression(c))) return NULL;
		TYPE_PUSH_NULL(t);
		argc++;
	}
	parserGiveback(c);
	result = typeAllocUndef(); if (!result) return NULL;
	TYPE_PUSH_NULL(result);	// push the type of the result
	callType = typeAllocFromStack(NULL, TYPECODE_FUN, argc + 1); if (!callType) return NULL;

	if (typeUnify(c,funType,callType)) return NULL;
	if (bc_byte_or_int(c, argc, OPtfcb, OPtfc)) return NULL;
	return result;
}

int compileBindRec(Compiler* c, Locals* locals, LINT level)
{
	int k;
	if (!locals) return 0;
	if ((k=compileBindRec(c, locals->next, level))) return k;
//	PRINTF(LOG_DEV,"check %s %lld > %lld\n", STR_START(locals->name),locals->level,level);
	if (locals->level > level)
	{
		locals->level=level;
		return bc_byte_or_int(c, locals->index, OPrlocb, OPrloc);
	}
	return bufferAddChar(c->bytecode,OPnil);
}
Type* compileLambda(Compiler* c)
{
	LB* fun;
	LINT global;
	Type* type;
	Type* resultType;
	Type* result;
	Locals* locals0=c->fmk->locals;
	LINT argc0=localsNb(locals0);
	LINT i;
	LINT argc= argc0;
	int indexBc = parserIndex(c);

	if (funMakerInit(c,c->fmk->locals,c->fmk->typeLabels,c->fmk->level+1,NULL,c->fmk->defForInstances)) return NULL;
	while (parserNext(c) && strcmp(c->parser->token, "="))
	{
		parserGiveback(c);
		if (!compileSkipLocal(c)) return NULL;
		if (!funMakerAddLocal(c, NULL)) return NULL;
		argc++;
	}
	parserJump(c, indexBc);
	i = argc0;
	while (parserNext(c) && strcmp(c->parser->token, "="))
	{
		int simple = 1;
		Type* argType;
		LINT firstOpcode = bytecodePin(c);
		Locals* localsBefore = c->fmk->locals;
		if (bc_byte_or_int(c, i, OPrlocb, OPrloc)) return NULL;
		parserGiveback(c);
		argType = compileLocals(c, &simple); if (!argType) return NULL;
		if (simple)
		{
			if (localsBefore != c->fmk->locals)	// check we are not in the underscore case
			{
				Locals* l = localsByIndex(c->fmk->locals, i); if (!l) return NULL;
				l->name = c->fmk->locals->name;
				l->type = argType;
				c->fmk->locals = c->fmk->locals->next;
			}
			bufferCut(c->bytecode, firstOpcode);
		}
		TYPE_PUSH_NULL((LB*)(argType));
		i++;
	}
	parserGiveback(c);
	if (parserAssume(c, "=")) return NULL;

	// prepare the type structure of the function (fun arg0 arg1 ... argn-1 result)
	resultType=typeAllocUndef(); if (!resultType) return NULL;
	TYPE_PUSH_NULL(resultType);	// push the type of the result

	type=typeAllocFromStack(NULL, TYPECODE_FUN, argc+1-argc0); if (!type) return NULL;

	c->fmk->resultType=resultType;
	// now we are ready...

	if (!(result=compileProgram(c))) return NULL;
	
	// unify the result
	if (typeUnify(c,resultType,result)) return NULL;

//	fun=bytecodeFinalize(c,argc,NULL);
	fun=bytecodeFinalize(c,argc,c->fmk->parent->defForInstances->name);
	if (!fun) return NULL;

	funMakerRelease(c);

	if (funMakerNeedGlobal(c->fmk,fun, &global)) return NULL;
	if (bc_byte_or_int(c,global,OPconstb,OPconst)) return NULL;
	if (argc)
	{
//		PRINTF(LOG_DEV,"###########lambda in %s\n",defName(c->fmk->def));
		if (compileBindRec(c,locals0,c->fmk->level)) return NULL;
		if (bc_byte_or_int(c,argc0+1,OPlambdab, OPlambda)) return NULL;
	}
	return type;
}

Type* compileFormat(Compiler* c, Type* returnedType)
{
	Type* t;
	Type* format;
	LINT argc=0;
	
	if (!(format=compileExpression(c))) return NULL;	// lire le format
	if (typeUnify(c, format, MM.Str)) return NULL;

	while((parserNext(c))&&(!parserIsFinal(c)))
	{
		parserGiveback(c);
		if (!(t=compileExpression(c))) return NULL;
		argc++;
	}
	parserGiveback(c);
	if (bc_byte_or_int(c, argc, OPformatb, OPformat)) return NULL;
	return returnedType;
}
