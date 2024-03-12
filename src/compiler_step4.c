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

// switch 8 / 32|64 bits
int bc_byte_or_int(Compiler* c, LINT val, char opbyte, char opint)
{
	int k;
	if ((val >= 0) && (val <= 255))
	{
		if ((k = bufferAddChar(c->th, c->bytecode, opbyte)))return k;
		if ((k = bufferAddChar(c->th, c->bytecode, (char)val)))return k;
	}
	else
	{
		if ((k = bufferAddChar(c->th, c->bytecode, OPint)))return k;
		if ((k = bufferAddInt(c->th, c->bytecode, val)))return k;
		if ((k = bufferAddChar(c->th, c->bytecode, opint)))return k;
	}
	return 0;
}

int bcint_byte_or_int(Compiler* c, LINT val)
{
	int k;
	if ((val >= 0) && (val <= 255))
	{
		if ((k = bufferAddChar(c->th, c->bytecode, OPintb)))return k;
		if ((k = bufferAddChar(c->th, c->bytecode, (char)val)))return k;
	}
	else
	{
		if ((k = bufferAddChar(c->th, c->bytecode, OPint)))return k;
		if ((k = bufferAddInt(c->th, c->bytecode, val)))return k;
	}
	return 0;
}

// get the current position for later jump
LINT bytecodePin(Compiler* c)
{
	return bufferSize(c->bytecode);
}

// add and compute the relative jump value
int bytecodeAddJump(Compiler* c, LINT pin)
{
	return bufferAddIntN(c->th, c->bytecode, pin - c->bytecode->index, BC_JUMP_SIZE);
}

// compute the relative jump value
void bytecodeSetJump(Compiler* c, LINT index, LINT pin)
{
	bufferSetIntN(c->bytecode, index, pin - index, BC_JUMP_SIZE);
}

// make room for later relative jump, return the position where to later store the jump value
LINT bytecodeAddEmptyJump(Compiler* c)
{
	LINT index = bufferSize(c->bytecode);
	if (bufferAddZero(c->th, c->bytecode, BC_JUMP_SIZE)) return -1;
	return index;
}

LINT bytecodeGetJump(char* pc)
{
	int j = BC_JUMP_SIZE - 1;
	LINT result = (pc[j] & 0x80) ? -1 : 0;

	while (j >= 0) result = (result << 8) + (255 & pc[j--]);
	return result;

}

Def* compileSetStart(Compiler* c)
{
//	PRINTF(LOG_DEV,"create pkg->start for %s\n", pkgName(c->pkg));
	c->pkg->start = defAlloc(c->th, 0, DEF_INDEX_BC, NIL, VAL_TYPE_PNT, NULL); if (!c->pkg->start) return NULL;
	c->pkg->start->proto = 1;
	c->pkg->start->pkg = c->pkg;
	c->pkg->start->name = MM.funStart;
	return c->pkg->start;
}

int funMakerInit(Compiler* c, FunMaker* f, Locals* locals, Locals* typeLabels, LINT level, Def* def, Def* defForInstances)
{
	int k;
	if (!defForInstances)
	{
		defForInstances = compileSetStart(c);
		if (!defForInstances) return EXEC_OM;
	}
	f->th = c->th;
	f->def = def;
	f->defForInstances = defForInstances;
	f->locals = locals;
	f->globals = NULL;
	f->typeLabels = typeLabels;
	f->maxlocals = localsNb(locals);
	f->level = level;
	f->bc = bufferCreate(c->th); if (!f->bc) return EXEC_OM;
	f->resultType = NULL;
	f->breakType = NULL;
	f->breakUse = 0;

	f->forceNumbers = FORCE_NUMBER_NONE;
	f->parent = c->fmk;
	c->fmk = f;
	if (f) c->bytecode = f->bc;
	if ((k = bufferAddInt(c->th, c->bytecode, 0))) return k;	// some place to store the number of args
	if ((k = bufferAddInt(c->th, c->bytecode, 0))) return k;	// some place to store the number of locals
	return 0;
}
void funMakerRelease(Compiler* c)
{
	c->fmk = c->fmk->parent;
	c->bytecode = c->fmk?c->fmk->bc:NULL;
}
int funMakerIsForVar(Compiler* c)
{
	if (c->fmk->parent) return 0;
	return 1;
}
Locals* funMakerAddLocal(Compiler* c, char* name)
{
	Type* t;
	FunMaker* f = c->fmk;
	LINT nblocals = localsNb(f->locals);

	if (nblocals + 1 > LOCALS_MAX_NUMBER) return (Locals*)compileError(c,"maximum number of local variables has been reached (%d)\n", LOCALS_MAX_NUMBER);
	t = typeAllocUndef(c->th); if (!t) return NULL;
	f->locals = localsCreate(c->th, name, f->level, t, f->locals); if (!f->locals) return NULL;
	nblocals++;
	if (f->maxlocals < nblocals) f->maxlocals = nblocals;
	return f->locals;
}
int funMakerAddGlobal(FunMaker* f, LB* data, LINT* index)
{
	f->globals = globalsCreate(f->th, data, f->globals);
	if (!f->globals) return EXEC_OM;
	*index = f->globals->index;
	return 0;
}
int funMakerNeedGlobal(FunMaker* f, LB* data, LINT* index)
{
	LINT i = globalsGet(f->globals, data);
	if (i >= 0) {
		*index = i;
		return 0;
	}
	return funMakerAddGlobal(f, data, index);
}
LB* bytecodeFinalize(Compiler* c, LINT argc, LB* name)
{
	LB* bytecode;
	LB* result;
	LINT* start;
	LB* globals;
	if (bufferAddChar(c->th, c->bytecode, OPret)) return NULL;	// be carefull this can change c->bytecode->buffer !!

//	PRINTF(LOG_DEV,"fun %s args=%lld locals=%lld\n", name ? STR_START(name) : "_", argc, c->fmk->maxlocals);

	start = (LINT*)c->bytecode->buffer;
	start[0] = argc;	// number of arguments (only usefull to check the stack after a call)
	start[1] = c->fmk->maxlocals - argc;	// number of pure local variables to put in the stack between the arguments and the callstack

	bytecode = memoryAllocBin(c->th, bufferStart(c->bytecode), bufferSize(c->bytecode), DBG_BYTECODE);
	if (!bytecode) return NULL;
	bytecodeOptimize(bytecode);

	result = memoryAllocArray(c->th, FUN_USER_LENGTH, DBG_FUN);
	if (!result) return NULL;
	ARRAY_SET_PNT(result, FUN_USER_NAME, name);
	ARRAY_SET_PNT(result, FUN_USER_BC, bytecode);
	if (globalsExtract(c->th, c->fmk->globals, &globals)) return NULL;
	ARRAY_SET_PNT(result, FUN_USER_GLOBALS, (globals));
	ARRAY_SET_PNT(result, FUN_USER_PKG, (LB*)(c->pkg));
	return result;
}
Type* compileFinalize(Compiler* c, Type* result)
{
	LB* bc;
	Def* _init= pkgGet(c->pkg, "_init", 0);
	if (_init)
	{
		LINT global;
		if (_init->code!=0) return compileError(c,"'_init' is reserved for the initialization of the package. It must be a function without argument.\n");
		if (funMakerNeedGlobal(c->fmk, (LB*)_init, &global)) return NULL;
		if (bc_byte_or_int(c, global, OPrglobb, OPrglob)) return NULL;
		if (bc_byte_or_int(c, _init->code, OPexecb, OPexec)) return NULL;
		if (typeUnify(c, result, _init->type->child[0])) return NULL;
		if (bc_byte_or_int(c, 1, OPskipb, OPskip)) return NULL;
//		pkgRemoveDef(_init);
	}
	bc = bytecodeFinalize(c, 0, c->pkg->start->name);
	if (!bc) return NULL;
	defSet(c->pkg->start,VAL_FROM_PNT(bc),VAL_TYPE_PNT);
	c->pkg->start->proto = 0;
//	pkgSetStart(c->pkg, bc);	// function start is the initialisation function.

// HERE YOU CAN get the full pkg description by uncommenting the following line

//	itemDump(LOG_SYS,VAL_FROM_PNT(c->pkg->defs),VAL_TYPE_PNT); getchar();
	return result;

}

Type* compileFun4(Compiler *c, Def* def)
{
	FunMaker Fmk;
	LINT argc=0;
	LINT i;
	Type* type;
	Type* resultType;
	Type* result;
	LB* bc;
	LINT global;
	Type* definedType = NULL;

//	PRINTF(LOG_USER,"----------HANDLE FUN %s\n",defName(def));

	if (funMakerInit(c,&Fmk,NULL,NULL,0,def,def)) return NULL;
	if (!parserNext(c)) return compileError(c,"unexpected end of file\n");
	if (!strcmp(c->parser->token, "@")) {
		definedType = compilerParseTypeDef(c, 0, &c->fmk->typeLabels);
		if (!definedType) return NULL;
	}
	else parserGiveback(c);

	argc = def->type->nb - 1;
//	PRINTF(LOG_DEV,"argc=%d\n", argc);
	for (i = 0; i < argc; i++) if (!funMakerAddLocal(c,NULL)) return NULL;
	i = 0;
	while (parserNext(c) && strcmp(c->parser->token, "="))
	{
		int simple = 1;
		Type* argType;
		LINT firstOpcode = bytecodePin(c);
		Locals* localsBefore = c->fmk->locals;
		if (bc_byte_or_int(c, i, OPrlocb, OPrloc)) return NULL;
		parserGiveback(c);
		argType = compileLocals(c,&simple); if (!argType) return NULL;
//		PRINTF(LOG_DEV,"simple %d=%d\n",i, simple);
		if (simple)
		{
			if (localsBefore != c->fmk->locals)	// check we are not in the underscore case
			{
				Locals* l = localsByIndex(c->fmk->locals, i); if (!l) return NULL;
				l->name = c->fmk->locals->name;
				l->type = argType;
				c->fmk->locals = c->fmk->locals->next;
				c->fmk->maxlocals--;
			}
			bufferCut(c->bytecode, firstOpcode);
		}
		STACK_PUSH_PNT_ERR(c->th, (LB*)(argType), NULL);
		i++;
	}
	parserGiveback(c);
	if (parserAssume(c, "=")) return NULL;

	// prepare the type structure of the function (fun arg0 arg1 ... argn-1 result)
	resultType = typeAllocUndef(c->th); if (!resultType) return NULL;
	TYPE_PUSH_NULL(c, resultType);	// push the type of the result
	type = typeAllocFromStack(c->th, NULL, TYPECODE_FUN, argc + 1); if (!type) return NULL;

	if (typeUnify(c, type, def->type)) return NULL;

	c->fmk->resultType=resultType;
	// now we are ready...

	if (!(result=compileProgram(c))) return NULL;
	if (parserAssume(c,";;")) return NULL;
	
	// unify the result
	if (typeUnify(c,resultType,result)) return NULL;

	bc=bytecodeFinalize(c, argc, def->name);
	if (!bc) return NULL;
	defSet(def,VAL_FROM_PNT(bc),VAL_TYPE_PNT);

	funMakerRelease(c);

	if (funMakerAddGlobal(c->fmk, (LB*)def, &global)) return NULL;
	if (bc_byte_or_int(c,global,OPconstb,OPconst)) return NULL;
	if (definedType && typeUnify(c, result, definedType)) return NULL;

	def->proto = 0;
	return type;
}
Type* compileVarOrConst4(Compiler* c, LINT code, Def* def)
{
	LINT global;
	if (!parserNext(c)) return compileError(c,"unexpected end of file\n");

	if (!strcmp(c->parser->token, "@")) {
		Type* t = compilerParseTypeDef(c, 1, &c->fmk->typeLabels);
		if (!t) return NULL;
		if (typeUnify(c, def->type, t)) return NULL;
		if (!parserNext(c)) return compileError(c,"unexpected end of file\n");
	}
	if (!strcmp(c->parser->token, "="))
	{
		Type* result;
		LINT global;
		if (funMakerNeedGlobal(c->fmk, (LB*)def, &global)) return NULL;
		if (bcint_byte_or_int(c, global)) return NULL;
		if (!(result = compileProgram(c))) return NULL;
		if (typeUnify(c, def->type, result)) return NULL;
		if (bufferAddChar(c->th, c->bytecode, OPsglobi)) return NULL;	// 1:index, 0:value
		if (bufferAddChar(c->th, c->bytecode, OPdrop)) return NULL;
	}
	else
	{
		parserGiveback(c);
	}
	if (parserAssume(c, ";;")) return NULL;

	if (funMakerAddGlobal(c->fmk, (LB*)def, &global)) return NULL;
	if (bc_byte_or_int(c, global, OPconstb, OPconst)) return NULL;

	def->proto = 0;
	return def->type;
}
Type* compileRecDefs4(Compiler* c, Def* def)
{
	Type* result = NULL;
	if (!def) return MM.Boolean;	// anything not null
	if (!compileRecDefs4(c,def->next)) return NULL;

	if (def->parser && def->proto)
	{
//		PRINTF(LOG_DEV,"Compile step4 '%s.%s'\n", defPkgName(def), defName(def));
		if ((def->code == DEF_CODE_CONST || def->code == DEF_CODE_VAR)&&(def->index<0))
		{
			parserRestoreFromDef(c, def);
			if (bufferAddChar(c->th, c->bytecode, OPdrop)) return NULL;
			if (!(result = compileVarOrConst4(c, def->code, def)))
			{
				if (def->code == DEF_CODE_VAR) return compileError(c,"error compiling var '%s'\n", defName(def));
				return compileError(c,"error compiling const '%s'\n", defName(def));
			}
			parserReset(c);
		}
		else if (def->code >= 0) {
			parserRestoreFromDef(c, def);
			if (bufferAddChar(c->th, c->bytecode, OPdrop)) return NULL;
			if (!(result = compileFun4(c, def))) return compileErrorInFunction(c, "error compiling function '%s'\n", defName(def));
			parserReset(c);
		}
	}
	return MM.Boolean;	// anything not null
}

Type* compileStep4(Compiler* c)
{
	Type* result = NULL;

	if (bufferAddChar(c->th, c->bytecode, OPnil)) return NULL;
	if (!compileRecDefs4(c, c->pkg->first)) return NULL;
	if (!result) result = typeAllocUndef(c->th);
	if (!result) return NULL;
	result=compileFinalize(c, result);
	funMakerRelease(c);
	return result;
}
