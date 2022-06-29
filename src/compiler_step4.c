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
		if ((k = bufferAddchar(c->th, c->bytecode, opbyte)))return k;
		if ((k = bufferAddchar(c->th, c->bytecode, (char)val)))return k;
	}
	else
	{
		if ((k = bufferAddchar(c->th, c->bytecode, OPint)))return k;
		if ((k = bufferAddint(c->th, c->bytecode, val)))return k;
		if ((k = bufferAddchar(c->th, c->bytecode, opint)))return k;
	}
	return 0;
}

int bcint_byte_or_int(Compiler* c, LINT val)
{
	int k;
	if ((val >= 0) && (val <= 255))
	{
		if ((k = bufferAddchar(c->th, c->bytecode, OPintb)))return k;
		if ((k = bufferAddchar(c->th, c->bytecode, (char)val)))return k;
	}
	else
	{
		if ((k = bufferAddchar(c->th, c->bytecode, OPint)))return k;
		if ((k = bufferAddint(c->th, c->bytecode, val)))return k;
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
	return bufferAddintN(c->th, c->bytecode, pin - c->bytecode->index, BC_JUMP_SIZE);
}

// compute the relative jump value
void bytecodeSetJump(Compiler* c, LINT index, LINT pin)
{
	bufferSetintN(c->bytecode, index, pin - index, BC_JUMP_SIZE);
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

Ref* compileSetStart(Compiler* c)
{
//	printf("create pkg->start for %s\n", pkgName(c->pkg));
	c->pkg->start = refAlloc(c->th, 0, REFINDEX_BC, NIL, NULL); if (!c->pkg->start) return NULL;
	c->pkg->start->proto = 1;
	c->pkg->start->pkg = c->pkg;
	c->pkg->start->name = MM.funStart;
	return c->pkg->start;
}

int funMakerInit(Compiler* c, FunMaker* f, Locals* locals, LINT level, Ref* ref, Ref* refForInstances)
{
	int k;
	if (!refForInstances)
	{
		refForInstances = compileSetStart(c);
		if (!refForInstances) return EXEC_OM;
	}
	f->th = c->th;
	f->ref = ref;
	f->refForInstances = refForInstances;
	f->locals = locals;
	f->globals = NULL;
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
	if ((k = bufferAddint(c->th, c->bytecode, 0))) return k;	// some place to store the number of args
	if ((k = bufferAddint(c->th, c->bytecode, 0))) return k;	// some place to store the number of locals
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
	if (nblocals + 1 > LOCALS_MAX_NUMBER) return (Locals*)compileError(c, "Compiler: maximum number of local variables has been reached (%d)\n", LOCALS_MAX_NUMBER);
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
	if (bufferAddchar(c->th, c->bytecode, OPret)) return NULL;	// be carefull this can change c->bytecode->buffer !!

	start = (LINT*)c->bytecode->buffer;
	start[0] = argc;	// number of arguments (only usefull to check the stack after a call)
	start[1] = c->fmk->maxlocals - argc;	// number of pure local variables to put in the stack between the arguments and the callstack

	bytecode = memoryAllocBin(c->th, bufferStart(c->bytecode), bufferSize(c->bytecode), DBG_BYTECODE);
	if (!bytecode) return NULL;
	bytecodeOptimize(bytecode);

	result = memoryAllocTable(c->th, FUN_USER_LEN, DBG_FUN);
	if (!result) return NULL;
	TABSET(result, FUN_USER_NAME, PNTTOVAL(name));
	TABSET(result, FUN_USER_BC, PNTTOVAL(bytecode));
	if (globalsExtract(c->th, c->fmk->globals, &globals)) return NULL;
	TABSET(result, FUN_USER_GLOBALS, PNTTOVAL(globals));
	TABSET(result, FUN_USER_PKG, PNTTOVAL(c->pkg));
	/*	printf("fun %s at %llx\n",name?STRSTART(name):"_",result);
		if (name&&(!strcmp(STRSTART(name),"RSAtest")))
		{
			printf("THIS ONE WILL BUG %llx size=%ld\n",result,HEADER_SIZE(result));
			runtimeCheckAddress=result;
			runtimeCheckValue=HEADER_SIZE(result);
		}
	*/
	//	itemDump(LOG_ERR,PNTTOVAL(result)); getchar();
	return result;
}
Type* compileFinalize(Compiler* c, Type* result)
{
	LB* bc;
	Ref* _init= pkgGet(c->pkg, "_init", 0);
	if (_init)
	{
		LINT global;
		if (_init->code!=0) return compileError(c, "Compiler: '_init' is reserved for the initialization of the package. It must be a function without argument.\n");
		if (funMakerNeedGlobal(c->fmk, (LB*)_init, &global)) return NULL;
		if (bc_byte_or_int(c, global, OPrglobb, OPrglob)) return NULL;
		if (bc_byte_or_int(c, _init->code, OPexecb, OPexec)) return NULL;
		if (typeUnify(c, result, _init->type->child[0])) return NULL;
//		pkgRemoveRef(_init);
	}
	bc = bytecodeFinalize(c, 0, c->pkg->start->name);
	if (!bc) return NULL;
	c->pkg->start->val = PNTTOVAL(bc);
	c->pkg->start->proto = 0;
//	pkgSetStart(c->pkg, bc);	// function start is the initialisation function.

// HERE YOU CAN get the full pkg description by uncommenting the following line

//	itemDump(LOG_ERR,PNTTOVAL(c->pkg->refs)); getchar();
	return result;

}

Type* compileFun4(Compiler *c, Ref* ref)
{
	FunMaker Fmk;
	LINT argc=0;
	LINT i;
	Type* type;
	Type* resultType;
	Type* result;
	LB* bc;
	LINT global;

//	PRINTF(LOG_USER,"----------HANDLE FUN %s\n",refName(ref));

	if (funMakerInit(c,&Fmk,NULL,0,ref,ref)) return NULL;
	argc = ref->type->nb - 1;
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
		STACKPUSH_OM(c->th, PNTTOVAL(argType), NULL);
		i++;
	}
	parserGiveback(c);
	if (parserAssume(c, "=")) return NULL;

	// prepare the type structure of the function (fun arg0 arg1 ... argn-1 result)
	resultType = typeAllocUndef(c->th); if (!resultType) return NULL;
	TYPEPUSH_NULL(c, resultType);	// push the type of the result
	type = typeAllocFromStack(c->th, NULL, TYPECODE_FUN, argc + 1); if (!type) return NULL;

	if (typeUnify(c, type, ref->type)) return NULL;

	c->fmk->resultType=resultType;
	// now we are ready...

	if (!(result=compileProgram(c))) return NULL;
	if (parserAssume(c,";;")) return NULL;
	
	// unify the result
	if (typeUnify(c,resultType,result)) return NULL;

	bc=bytecodeFinalize(c, argc, ref->name);
	if (!bc) return NULL;
	ref->val=PNTTOVAL(bc);

	funMakerRelease(c);

	if (funMakerAddGlobal(c->fmk, (LB*)ref, &global)) return NULL;
	if (bc_byte_or_int(c,global,OPconstb,OPconst)) return NULL;

	ref->proto = 0;
	return type;
}
Type* compileVarOrConst4(Compiler* c, LINT code, Ref* ref)
{
	LINT global;
	if (!parserNext(c)) return compileError(c, "Compiler: ';;' expected (found %s)\n", compileToken(c));

	if (!strcmp(c->parser->token, "="))
	{
		Type* result;
		LINT global;
		if (funMakerNeedGlobal(c->fmk, (LB*)ref, &global)) return NULL;
		if (bcint_byte_or_int(c, global)) return NULL;
		if (!(result = compileProgram(c))) return NULL;
		if (typeUnify(c, ref->type, result)) return NULL;
		if (bufferAddchar(c->th, c->bytecode, OPsglobi)) return NULL;	// 1:index, 0:value
		if (bufferAddchar(c->th, c->bytecode, OPdrop)) return NULL;
	}
	else
	{
		parserGiveback(c);
	}
	if (parserAssume(c, ";;")) return NULL;

	if (funMakerAddGlobal(c->fmk, (LB*)ref, &global)) return NULL;
	if (bc_byte_or_int(c, global, OPconstb, OPconst)) return NULL;

	ref->proto = 0;
	return ref->type;
}
Type* compileRecRefs4(Compiler* c, Ref* ref)
{
	Type* result = NULL;
	if (!ref) return MM.Boolean;	// anything not null
	if (!compileRecRefs4(c,ref->next)) return NULL;

	if (ref->parser)
	{
//		printf("Compile step4 '%s.%s'\n", refPkgName(ref), refName(ref));
		if (ref->code == REFCODE_CONST || ref->code == REFCODE_VAR)
		{
			parserRestoreFromRef(c, ref);
			if (bufferAddchar(c->th, c->bytecode, OPdrop)) return NULL;
			if (!(result = compileVarOrConst4(c, ref->code, ref)))
			{
				if (ref->code == REFCODE_VAR) return compileError(c, "Compiler: error compiling var '%s'\n", refName(ref));
				return compileError(c, "Compiler: error compiling const '%s'\n", refName(ref));
			}
		}
		else if (ref->code >= 0) {
			parserRestoreFromRef(c, ref);
			if (bufferAddchar(c->th, c->bytecode, OPdrop)) return NULL;
			if (!(result = compileFun4(c, ref))) return compileErrorInFunction(c, "Compiler: error compiling function '%s.%s'\n", refPkgName(ref), refName(ref));
		}
	}
	return MM.Boolean;	// anything not null
}
Type* compileFile4(Compiler* c)
{
	Type* result = NULL;

	if (bufferAddchar(c->th, c->bytecode, OPnil)) return NULL;
	if (!compileRecRefs4(c, c->pkg->first)) return NULL;
	if (!result) result = typeAllocUndef(c->th);
	if (!result) return NULL;
	result=compileFinalize(c, result);
	funMakerRelease(c);
	return result;
}

Type* compileImports4(Compiler* c)
{
	Pkg* pkg;
	LB* p;
	for (p = c->pkg->importList; p; p = VALTOPNT(TABGET(p, LIST_NXT)))
	{
		LB* i = VALTOPNT(TABGET(p, LIST_VAL));
		pkg = (Pkg*)VALTOPNT(TABGET(i, IMPORT_PKG));
		if (pkg->stage == PKG_STAGE_3)
		{
			LINT global;
			Pkg* pkgSave = c->pkg;
			FunMaker f;

			c->pkg = pkg;

			pkg->stage = PKG_STAGE_READY;

			if (funMakerInit(c, &f, NULL, 0, NULL, NULL)) return NULL;	// funMaker for vars definitions

			if (!compileImports4(c)) return NULL;

			c->parser = NULL;
			if (!compileFile4(c)) return NULL;
			
			c->pkg = pkgSave;
		
			if (c->fmk)
			{
				if (funMakerAddGlobal(c->fmk, (LB*)VALTOPNT(pkg->start->val), &global)) return NULL;
				if (bc_byte_or_int(c, global, OPconstb, OPconst)) return NULL;
				if (bc_byte_or_int(c, 0, OPexecb, OPexec)) return NULL;
				if (bufferAddchar(c->th, c->bytecode, OPdrop)) return NULL;
			}
		}
	}
	return MM.I;
}
Type* compileStep4(Compiler* c)
{
	FunMaker f;
	if (funMakerInit(c, &f, NULL, 0, NULL, NULL)) return NULL;	// funMaker for vars definitions

	if (!compileImports4(c)) return NULL;

	c->parser = NULL;
	return compileFile4(c);
}
