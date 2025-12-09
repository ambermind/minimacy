// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

// switch 8 / 32|64 bits
int bc_byte_or_int(Compiler* c, LINT val, char opbyte, char opint)
{
	int k;
	if ((val >= 0) && (val <= 255))
	{
		if ((k = bufferAddChar(c->bytecode, opbyte)))return k;
		if ((k = bufferAddChar(c->bytecode, (char)val)))return k;
	}
	else
	{
		if ((k = bufferAddChar(c->bytecode, OPint)))return k;
		if ((k = bufferAddInt(c->bytecode, val)))return k;
		if ((k = bufferAddChar(c->bytecode, opint)))return k;
	}
	return 0;
}

int bcint_byte_or_int(Compiler* c, LINT val)
{
	int k;
	if ((val >= 0) && (val <= 255))
	{
		if ((k = bufferAddChar(c->bytecode, OPintb)))return k;
		if ((k = bufferAddChar(c->bytecode, (char)val)))return k;
	}
	else
	{
		if ((k = bufferAddChar(c->bytecode, OPint)))return k;
		if ((k = bufferAddInt(c->bytecode, val)))return k;
	}
	return 0;
}
int bc_opcode(Compiler* c, LINT opcode)
{
	int k;
	if (opcode & 0x8000) {
		if ((k = bufferAddChar(c->bytecode, (char)(opcode >> 8)))) return k;
	}
	if ((k = bufferAddChar(c->bytecode, (char)opcode))) return k;
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
	return bufferAddIntN(c->bytecode, pin - c->bytecode->index, BC_JUMP_SIZE);
}

int bytecodeAddJumpList(Compiler* c, LINT next)
{
	return bufferAddIntN(c->bytecode, next, BC_JUMP_SIZE);
}

// compute the relative jump value
void bytecodeSetJump(Compiler* c, LINT index, LINT pin)
{
	bufferSetIntN(c->bytecode, index, pin - index, BC_JUMP_SIZE);
}

void bytecodeSetJumpList(Compiler* c, LINT index, LINT pin)
{
	while (index) {
		LINT next = bufferGetIntN(c->bytecode, index, BC_JUMP_SIZE);
		bytecodeSetJump(c, index, pin);
		index = next;
	}
}

// make room for later relative jump, return the position where to later store the jump value
LINT bytecodeAddEmptyJump(Compiler* c)
{
	LINT index = bufferSize(c->bytecode);
	if (bufferAddZero(c->bytecode, BC_JUMP_SIZE)) return -1;
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
	c->pkg->start = defAlloc(0, DEF_INDEX_BC, NIL, VAL_TYPE_PNT, NULL); if (!c->pkg->start) return NULL;
	c->pkg->start->proto = 1;
	c->pkg->start->header.pkg = c->pkg;
	c->pkg->start->name = MM.funStart;
	return c->pkg->start;
}

void funMakerMark(LB* user)
{
	FunMaker* f = (FunMaker*)user;
	MARK_OR_MOVE(f->def);
	MARK_OR_MOVE(f->defForInstances);
	MARK_OR_MOVE(f->locals);
	MARK_OR_MOVE(f->globals);
	MARK_OR_MOVE(f->typeLabels);
	MARK_OR_MOVE(f->bc);
	MARK_OR_MOVE(f->resultType);
	MARK_OR_MOVE(f->loopType);
	MARK_OR_MOVE(f->parent);
}

int funMakerInit(Compiler* c, Locals* locals, Locals* typeLabels, LINT level, Def* d, Def* defForInstances)
{
	int k;
	FunMaker* f= (FunMaker*)memoryAllocNative(sizeof(FunMaker), DBG_BIN, NULL, funMakerMark);	if (!f) return EXEC_OM;
	// for the vars definition function, d and defForInstances are NULL
	// for a lambda function, d is NULL and defForInstances is the containing function definition
	// for a regular function, d and defForInstances are equal

	f->def = d;
	f->defForInstances = NULL;
	f->locals = locals;
	f->globals = NULL;
	f->typeLabels = typeLabels;
	f->maxlocals = localsNb(locals);
	f->level = level;
	f->resultType = NULL;
	f->loopType = NULL;
	f->breakList = 0;
	f->parent = c->fmk;
	c->fmk = f;
	f->forceNumbers = FORCE_NUMBER_NONE;

	if (!defForInstances)
	{
		defForInstances = compileSetStart(c);
		if (!defForInstances) return EXEC_OM;
	}
	f->defForInstances = defForInstances;
	if (d) {
		f->bc = c->firstBytecodeBuffer;
		bufferReinit(f->bc);
	}
	else {
		f->bc = bufferCreateWithSize(512); if (!f->bc) return EXEC_OM;
	}

	c->bytecode = f->bc;
	BLOCK_MARK(c->bytecode);
	if ((k = bufferAddInt(c->bytecode, 0))) return k;	// some place to store the number of args
	if ((k = bufferAddInt(c->bytecode, 0))) return k;	// some place to store the number of locals
	return 0;
}
void funMakerRelease(Compiler* c)
{
	c->fmk = c->fmk->parent;
	c->bytecode = c->fmk?c->fmk->bc:NULL;
	BLOCK_MARK(c->fmk);
	BLOCK_MARK(c->bytecode);
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
	t = typeAllocUndef(); if (!t) return NULL;
	f->locals = localsCreate(name, f->level, t, f->locals); if (!f->locals) return NULL;
	nblocals++;
	if (f->maxlocals < nblocals) f->maxlocals = nblocals;
	return f->locals;
}
int funMakerAddGlobal(FunMaker* f, LB* data, LINT* index)
{
	f->globals = globalsCreate(data, f->globals);
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
	if (bufferAddChar(c->bytecode, OPret)) return NULL;	// be carefull this can change c->bytecode->buffer !!

//	PRINTF(LOG_DEV,"fun %s args=%lld locals=%lld\n", name ? STR_START(name) : "_", argc, c->fmk->maxlocals);

	start = (LINT*)c->bytecode->buffer;
	start[0] = argc;	// number of arguments (only usefull to check the stack after a call)
	start[1] = c->fmk->maxlocals - argc;	// number of pure local variables to put in the stack between the arguments and the callstack

	bytecode = memoryAllocBin(bufferStart(c->bytecode), bufferSize(c->bytecode), DBG_BYTECODE);
	if (!bytecode) return NULL;
	bytecodeOptimize(bytecode);
//	if (name == c->pkg->start->name) {

//		bytecodePrint(LOG_DEV, bytecode);
//		exit(0);
//	}
	result = memoryAllocArray(FUN_USER_LENGTH, DBG_FUN);
	if (!result) return NULL;
	ARRAY_SET_PNT(result, FUN_USER_NAME, name);
	ARRAY_SET_PNT(result, FUN_USER_BC, bytecode);
	if (globalsExtract(c->fmk->globals, &globals)) return NULL;
	ARRAY_SET_PNT(result, FUN_USER_GLOBALS, (globals));
	result->pkg = c->pkg;
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

Type* compileFun4(Compiler *c, Def* d)
{
	LINT argc=0;
	LINT i;
	Type* type;
	Type* resultType;
	Type* result;
	LB* bc;
	Type* definedType = NULL;

//	PRINTF(LOG_USER,"----------HANDLE FUN %s\n",defName(d));
	if (funMakerInit(c,NULL,NULL,0,d,d)) return NULL;
	if (!parserNext(c)) return compileError(c,"unexpected end of file\n");
	if (!strcmp(c->parser->token, ":")) {
		definedType = compilerParseTypeDef(c, 0, &c->fmk->typeLabels);
		if (!definedType) return NULL;
	}
	else parserRewind(c);
	argc = d->type->nb - 1;
//	PRINTF(LOG_DEV,"argc=%d\n", argc);
	for (i = 0; i < argc; i++) if (!funMakerAddLocal(c,NULL)) return NULL;
	i = 0;
	if (parserAssume(c, "(")) return NULL;

	while (1)
	{
		int simple = 1;
		Type* argType;
		LINT firstOpcode = bytecodePin(c);
		Locals* localsBefore = c->fmk->locals;
		if (!parserNext(c)) return compileError(c, "locals or ')' expected (found '%s')\n", compileToken(c));
		if (!strcmp(c->parser->token, ")")) break;

		if (bc_byte_or_int(c, i, OPrlocb, OPrloc)) return NULL;
		parserRewind(c);
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
		TYPE_PUSH_NULL((LB*)(argType));
		i++;
		if ((!parserNext(c)) || (strcmp(c->parser->token, ",") && strcmp(c->parser->token, ")")))
			return compileError(c, "',' or ')' expected (found '%s')\n", compileToken(c));
		if (!strcmp(c->parser->token, ")")) break;
	}
	if (parserAssume(c, "=")) return NULL;
	// prepare the type structure of the function (fun arg0 arg1 ... argn-1 result)
	resultType = typeAllocUndef(); if (!resultType) return NULL;
	TYPE_PUSH_NULL(resultType);	// push the type of the result
	type = typeAllocFromStack(NULL, TYPECODE_FUN, argc + 1); if (!type) return NULL;

	if (typeUnify(c, type, d->type)) return NULL;

	c->fmk->resultType=resultType;
	// now we are ready...

	if (!(result=compileProgram(c))) return NULL;
	if (parserAssume(c,";;")) return NULL;
	// unify the result
	if (typeUnify(c,resultType,result)) return NULL;

	bc=bytecodeFinalize(c, argc, d->name);
	if (!bc) return NULL;
	defSet(d,VAL_FROM_PNT(bc),VAL_TYPE_PNT);

	funMakerRelease(c);

	if (definedType && typeUnify(c, result, definedType)) return NULL;
	d->type = typeSimplify(d->type);
	d->proto = 0;
	return type;
}
Type* compileVarOrConst4(Compiler* c, LINT code, Def* d)
{
	if (!parserNext(c)) return compileError(c,"unexpected end of file\n");

	if (!strcmp(c->parser->token, ":")) {
		Type* t = compilerParseTypeDef(c, 1, &c->fmk->typeLabels);
		if (!t) return NULL;
		if (typeUnify(c, d->type, t)) return NULL;
		if (!parserNext(c)) return compileError(c,"unexpected end of file\n");
	}
	if (!strcmp(c->parser->token, "="))
	{
		Type* result;
		LINT global;
		if (funMakerNeedGlobal(c->fmk, (LB*)d, &global)) return NULL;
		if (bcint_byte_or_int(c, global)) return NULL;
		if (!(result = compileProgram(c))) return NULL;
		if (typeUnify(c, d->type, result)) return NULL;
		if (bufferAddChar(c->bytecode, OPsglobi)) return NULL;	// 1:index, 0:value
		if (bufferAddChar(c->bytecode, OPdrop)) return NULL;
	}
	else
	{
		parserRewind(c);
	}
	if (parserAssume(c, ";;")) return NULL;

	d->type = typeSimplify(d->type);
	d->proto = 0;
	return d->type;
}
Type* compileDefs4(Compiler* c)
{
	Type* result = NULL;
	Def* d;
	for (d = c->pkg->first; d; d = d->next) {
//		PRINTF(LOG_DEV,"Compile step4 '%s.%s'\n", defPkgName(d), defName(d));
		if (d->parser && d->proto)
		{
			memoryEnterSafe();
			if ((d->code == DEF_CODE_CONST || d->code == DEF_CODE_VAR) && (d->index < 0))
			{
				parserRestoreFromDef(c, d);
				if (!(result = compileVarOrConst4(c, d->code, d)))
				{
					if (d->code == DEF_CODE_VAR) return compileError(c, "error compiling var '%s'\n", defName(d));
					return compileError(c, "error compiling const '%s'\n", defName(d));
				}
				parserReset(c);
			}
			else if (d->code >= 0) {
				parserRestoreFromDef(c, d);
				if (!(result = compileFun4(c, d))) return compileErrorInFunction(c, "error compiling function '%s'\n", defName(d));
				parserReset(c);
			}
			memoryLeaveSafe();
		}
#ifdef FORGET_PARSER
		d->parser = NULL;
#endif
	}
	return MM.Boolean;	// anything not null
}

Type* compileStep4(Compiler* c)
{
	Type* result = NULL;

	if (bufferAddChar(c->bytecode, OPnil)) return NULL;
	defReverse(c->pkg);
	result = compileDefs4(c);
	defReverse(c->pkg);
	if (!result) return NULL;
	memoryEnterSafe();
	result = typeAllocUndef(); if (!result) return NULL;
	result=compileFinalize(c, result);
	funMakerRelease(c);
	memoryLeaveSafe();
	return result;
}
