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

int compileFunctionIsPrivate(char* token)
{
	return (token[0] == '_') ? 1 : 0;
}
Def* compileGetDef(Compiler* c)
{
	int underscored = compileFunctionIsPrivate(c->parser->token);
	Pkg* pkg= pkgImportByAlias(c->pkg,c->parser->token);
	if (!pkg) return pkgGet(c->pkg,c->parser->token, !underscored);	// if underscored, search only in the current pkg
	if (underscored) return NULL; // underscored names are not searchable from other packages
	if (parserAssume(c,".")) return NULL;
	if ((!parserNext(c))||(!islabel(c->parser->token)))	return NULL;
	return pkgGet(pkg,c->parser->token,0);
}

int compileDisplay(int mask, Compiler* c)
{
	if (c->displayed) return 0;
	c->displayed++;
	if (!c->pkg->forPrompt) pkgDisplay(c->th, mask, c->pkg);
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

Type* compile(Compiler* c,LB* src,Pkg* pkg,Type* expectedType)
// expectedType is null when called for import
{
	Type* result;
	LB* importsBefore= pkg->importList;
	LINT t0;
	FunMaker f;
	LINT weakError = 0;
	Def* def0;

	memoryEnterFast();
	c->fmk=NULL;
	c->pkg = pkg;
	c->parser = NULL;
	c->parserLib = NULL;
	c->displayed=0;
	c->pastUid = MM.listPkgs->uid;
	c->nbDerivations = 0;
	c->exports = NULL;
	c->th = NULL;
	c->th = threadCreate(c->th, NULL);
	if (!c->th) return NULL;

	t0 = hwTimeMs();

//	PRINTF(LOG_USER, "> compiling in package '%s'\n",pkgName(pkg));
	if (parserFromData(c, pkgName(c->pkg), STR_START(src))) return NULL;

	pkgRemoveDef(pkgGet(c->pkg, "_init", 0));
	def0 = pkg->first;

	if (funMakerInit(c, &f, NULL, NULL, 0, NULL, NULL)) return NULL;	// funMaker for vars definitions

	result = compileStep1(c);
	if (result) result = compileStep2(c);

	if (result) result = compileStep3(c);

	if (result) result = compileStep4(c);

	if (result && compileInstanceSolver(c)) result = NULL;

//	if (0)
	if (result && pkgHasWeak(c, c->th, c->pkg, expectedType?0:1)) {
		weakError = 1;
		if (!expectedType) result = NULL;
	}
	if (result && expectedType)
	{
		Type* u = typeCopy(c->th,expectedType); if (!u) return NULL;
		if (typeUnify(c, result, u)) result = NULL;
	}
	t0 = hwTimeMs() - t0;

	if (!result)
	{
		pkg->importList = importsBefore;
		MEMORY_MARK((LB*)pkg, importsBefore);
		if (c->pkg->forPrompt) pkgCleanCompileError();
		while (pkg->first && (pkg->first != def0)) pkgRemoveDef(pkg->first);
		parserRestorechar(c);
		if (c->parser && (c->displayed<2) && !weakError) termEchoSourceLine(c->pkg, c->th, LOG_USER, c->parser->name?STR_START(c->parser->name):NULL, c->parser->src, c->parser->index0);
	}
	else
	{
		if (c->pkg == MM.system) {
			if (MainTerm.showBiosListing) compileDisplay(LOG_SYS, c);
			PRINTF(LOG_SYS, "> BIOS compiled in "LSD" ms\n", t0);	//only for bios, handled by bios for other packages
		}
		else {
			if (MainTerm.showPkgListing) compileDisplay(LOG_SYS, c);
		}
	}
	c->pkg->forPrompt = 1;
//	itemDump(c->th, LOG_USER, VAL_FROM_PNT(c->exports), 0);
	memoryLeaveFast();
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
	Compiler c;
	Type* result;

	LINT def=STACK_REF(th)-2;
	LB* src = STACK_PNT(th, 2);
	Pkg* pkg=(Pkg*)STACK_PNT(th,1);
	Type* type = (Type*)STACK_PNT(th, 0);
	MM.OM = 0;
	if (!src) goto cleanup;

	if (!pkg) pkg = MM.system;

	if (!(result=compile(&c,src,pkg,type))) goto cleanup;

	STACK_DROPN(th, 3);
	FUN_PUSH_PNT((LB*)(result));
	FUN_PUSH_PNT( PNT_FROM_VAL(pkg->start->val));

//	STACK_SKIP(th,3);
	return 0;

cleanup:
	if (th->OM) return EXEC_OM;
	while(def!=STACK_REF(th)) STACK_DROP(th);
	STACK_SET_NIL(th,0);
	FUN_PUSH_NIL;
	return 0;

}

int compilePromptAndRun(Thread* th)
{
	int k;

	if ((k= promptOnThread(th))) return k;

	interpreterExec(th,0,0);
	interpreterRun(th,0);

	STACK_DROPN(th,2);
	return 0;
}
