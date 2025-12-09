// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

int compileFunctionIsPrivate(char* token)
{
	return (token[0] == '_') ? 1 : 0;
}
Def* compileGetDef(Compiler* c)
{
	Def* d;
	Pkg* pkg= pkgImportByAlias(c->pkg,c->parser->token);
	if (!pkg) {
		return pkgGet(c->pkg, c->parser->token, !compileFunctionIsPrivate(c->parser->token));	// if underscored, search only in the current pkg
	}
	if (parserAssume(c,".")) return NULL;
	if ((!parserNext(c))||(!isLabel(c->parser->token)))	return NULL;
	if (compileFunctionIsPrivate(c->parser->token)) return NULL;
	d=pkgGet(pkg,c->parser->token,0);
	if (d && d->public != DEF_HIDDEN) return d;
	return NULL;
}

int compileDisplay(int mask, Compiler* c)
{
	if (c->displayed) return 0;
	c->displayed++;
	if (!c->pkg->forPrompt) pkgDisplay(mask, c->pkg);
	return 0;
}

char* compileToken(Compiler *c)
{
	if (c->parser->token) return c->parser->token;
	return "EOF";
}
Type* compileError(Compiler* c, char *format, ...)
{
	va_list arglist;
	if (c->displayed) return NULL;
	if (compileDisplay(LOG_USER,c)) return NULL;
	PRINTF(LOG_USER,"\n");
	PRINTF(LOG_USER,"> Compiler error: ");
	va_start(arglist, format);
	termPrintfv(LOG_USER, format, arglist);
	va_end(arglist);
	return NULL;
}
Type* compileErrorInFunction(Compiler* c, char* format, ...)
{
	va_list arglist;
	if (MM.OM) return NULL;
	if (compileDisplay(LOG_USER, c)) return NULL;
	PRINTF(LOG_USER, "\n");
	if (!c->pkg->forPrompt)	//we do not print this error when the error is in a prompt, because it would display: Error compiling function '_init'
	{
		PRINTF(LOG_USER,"> Compiler error: ");
		va_start(arglist, format);
		termPrintfv(LOG_USER, format, arglist);
		va_end(arglist);
	}
	return NULL;
}

void compilerMark(LB* user)
{
	Compiler* c = (Compiler*)user;
	MARK_OR_MOVE(c->pkg);
	MARK_OR_MOVE(c->fmk);
	MARK_OR_MOVE(c->bytecode);
	MARK_OR_MOVE(c->firstBytecodeBuffer);
	MARK_OR_MOVE(c->parser);
	MARK_OR_MOVE(c->mainParser);
	MARK_OR_MOVE(c->exports);
	MARK_OR_MOVE(c->def0);
}

Type* compile(LB* src,Pkg* pkg,int fromImport, int* displayed)
// src is in some stack, there is no way it could be gc-ed
{
	Compiler* c;
	Type* result;
	LB* importsBefore= pkg->importList;
	LINT t0;
	LINT weakError = 0;
	if (displayed) *displayed = 0;
	c = (Compiler*)memoryAllocNative(sizeof(Compiler), DBG_BIN, NULL, compilerMark);	if (!c) return NULL;

	c->fmk=NULL;
	c->pkg = pkg;
	c->bytecode = NULL;
	c->firstBytecodeBuffer = NULL;
	c->parser = NULL;
	c->mainParser = NULL;
	c->displayed=0;
	c->nbDerivations = 0;
	c->exports = NULL;
	pkg->stage = PKG_STAGE_COMPILING;

	t0 = hwTimeMs();
	TMP_PUSH((LB*)c, NULL);	// putting the compiler in the stack will make it safe and most of the compilation structures as well
	
//	PRINTF(LOG_USER, "> compiling in package '%s'\n",pkgName(pkg));
	if (parserFromData(c, pkgName(c->pkg), src)) return NULL;
	c->mainParser = c->parser;	
	pkgRemoveDef(pkgGet(c->pkg, "_init", 0));
	c->def0 = pkg->first;

	if (funMakerInit(c, NULL, NULL, 0, NULL, NULL)) return NULL;	// funMaker for vars definitions

	result = compileStep1(c);

	if (result) result = compileStep2(c);

	if (result) result = compileStep3(c);

	c->firstBytecodeBuffer = bufferCreateWithSize(512); if (!c->firstBytecodeBuffer) return NULL;

	if (result) result = compileStep4(c);

	memoryEnterSafe();
	if (result && compileInstanceSolver(c)) result = NULL;

//	if (0)
	if (result && pkgHasWeak(c, c->pkg, fromImport)) {
		weakError = 1;
		if (fromImport) result = NULL;
	}
	t0 = hwTimeMs() - t0;
	if (!result)
	{
		pkg->importList = importsBefore;
		BLOCK_MARK(importsBefore);
		if (c->pkg->forPrompt) pkgCleanCompileError();
		while (pkg->first && (pkg->first != c->def0)) pkgRemoveDef(pkg->first);
		parserRestorechar(c);
		if (c->parser && (c->displayed<2) && (!weakError) && (!MM.OM)) termEchoSourceLine(c->pkg, LOG_USER, c->parser->name?STR_START(c->parser->name):NULL, c->parser->src, c->parser->index0);
	}
	else
	{
#ifdef FORGET_PARSER
		Def* d;
		for (d = pkg->first; d; d = d->next) d->parser = NULL;
#endif
		if (c->pkg == MM.system) {
			if (MainTerm.showBiosListing) compileDisplay(LOG_SYS, c);
			PRINTF(LOG_SYS, "> BIOS compiled in "LSD" ms\n", t0);	//only for bios, handled by bios for other packages
		}
		else {
			if (MainTerm.showPkgListing) compileDisplay(LOG_SYS, c);
		}
	}
	if (displayed) *displayed = c->displayed;
	c->pkg->forPrompt = 1;
//	itemDump(LOG_USER, VAL_FROM_PNT(c->exports), 0);
	TMP_PULL();	// pull compiler
	pkg->stage = PKG_STAGE_READY;
	memoryLeaveSafe();
	return result;
}

// input :
// 2: src : the source code to compile
// 1: pkg : the package in which the compiling will happen
// 0: type: should be initialized to undef (typeAllocUndef)
// output :
// 1: the type of the compiling 
// 0: the 0000 function, ready to call with OPexec
int promptOnThread(Thread* th)
{
	Type* result;
	Pkg* pkgSave;

	LINT def=STACK_REF(th);
	LB* src = STACK_PNT(th, 1);
	Pkg* pkg=(Pkg*)STACK_PNT(th,0);
	MM.OM = 0;
	if (!src) goto cleanup;

	src = memoryAllocStr(STR_START(src), -1); if (!src) goto cleanup;	// we copy the string because the tokenizer will alter it 
	STACK_SET_PNT(th, 1, src);

	if (!pkg) pkg = MM.system;
	pkgSave = MM.currentPkg;
	MM.currentPkg = pkg;
	result = compile(src, pkg, 0, NULL);
	MM.currentPkg = pkgSave;
	if (!result) goto cleanup;

	STACK_SET_PNT(th,1,(LB*)(result));
	STACK_SET_PNT(th,0,PNT_FROM_VAL(pkg->start->val));
	return 0;

cleanup:
	if (MM.OM) return EXEC_OM;
	while(def!=STACK_REF(th)) STACK_DROP(th);
	STACK_SET_NIL(th, 1);
	STACK_SET_NIL(th, 0);
	return 0;

}

int compilePromptAndRun(Thread* th)
{
	int k;

	if ((k= promptOnThread(th))) return k;

	interpreterExec(th,0,0);
	interpreterRun(th,0);	// may move thread th
	th = MM.scheduler;	// hack: as for today compilePromptAndRun is only called on MM.scheduler
	STACK_DROPN(th,2);
	return 0;
}
