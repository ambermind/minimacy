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
Ref* compileGetRef(Compiler* c)
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
	Pkg* p = MM.listPkgs;

	if (c->displayed) return 0;
	c->displayed=1;
	if (c->pkg0 == MM.system) pkgDisplay(c->th, mask,MM.system);	// special case, because it is the only time we dump the package in which a prompt command occurs
	while ((p != MM.system) && ((p->uid - c->pastUid) >0))
	{
		int k;
		if ((k=pkgDisplay(c->th, mask, p))) return k;
		p = p->listNext;
	}
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
	if (compileDisplay(LOG_USER,c)) return NULL;
	PRINTF(c->th, LOG_USER,"\n");
	va_start(arglist, format);
	termPrintfv(c->th, LOG_USER, format, arglist);
	va_end(arglist);
	return NULL;
}
Type* compileErrorInFunction(Compiler* c, char* format, ...)
{
	va_list arglist;
	if (compileDisplay(LOG_USER, c)) return NULL;
	PRINTF(c->th, LOG_USER, "\n");
	if (STRLEN(c->parser->name))	//little hack, we do not print this error when the error is in a prompt, because it would display: Error compiling function '_init'
	{
		va_start(arglist, format);
		termPrintfv(c->th, LOG_USER, format, arglist);
		va_end(arglist);
	}
	return NULL;
}
Type* compile(Compiler* c,LB* psrc,Pkg* pkg,Type* expectedType)
{
	Type* result;
	char* src = STRSTART(psrc);
	LB* importsBefore= pkg->importList;
	LINT t0;

	c->fmk=NULL;
	c->pkg = c->pkg0=pkg;
	c->parser = NULL;
	c->parserLib = NULL;
	c->displayed=0;
	c->pastUid = MM.listPkgs->uid;
	c->nbDerivations = 0;
	c->th = NULL;
	c->th = threadCreate(c->th, NULL);
	if (!c->th) return NULL;
	memoryEnterFast(c->th);

	t0 = hwTimeMs();

//	PRINTF(c->th, LOG_ERR, "> compiling in package '%s'\n",pkgName(pkg));
	if (parserFromData(c, c->pkg==MM.system? BOOT_FILE:"",src)) return NULL;
//	if (parserFromData(c, "",src)) return NULL;

	pkgRemoveRef(pkgGet(c->pkg, "_init", 0));

	result = compileStep1(c);

	if (result) result = compileStep2(c);

	if (result) result = compileStep3(c);

	if (result) result = compileStep4(c);

	if (result && compileInstanceSolver(c)) result = NULL;

	if (result && expectedType)
	{
		Type* u = typeCopy(c->th,expectedType); if (!u) return NULL;
		if (typeUnify(c, result, u)) result = NULL;
	}
	t0 = hwTimeMs() - t0;

	if (!result)
	{
		pkg->importList = importsBefore;
		MEMORYMARK((LB*)pkg, importsBefore);
		
		while (MM.listPkgs->stage!=PKG_STAGE_READY) MM.listPkgs = MM.listPkgs->listNext;
		MEMORYMARK((LB*)MM.system, (LB*)MM.listPkgs);

		parserRestorechar(c);
		if (c->parser) termEchoSourceLine(c->th, LOG_USER, c->parser->name?STRSTART(c->parser->name):NULL, c->parser->src, c->parser->index0);
	}
	else
	{
		Pkg* p;
		compileDisplay(LOG_ERR, c);
		if (c->pkg0==MM.system) PRINTF(c->th, LOG_ERR, "> compiled in "LSD" ms\n\n", t0);	//only for bios, handled by bios for other packages
		if (c->pkg0->uid == 0) pkgHasWeak(c->th, c->pkg0);
		for(p = MM.listPkgs; p->uid>c->pastUid ; p=p->listNext) pkgHasWeak(c->th, p);
	}

	memoryLeaveFast(c->th);
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

	LINT ref=STACKREF(th)-2;
	LB* src = VALTOPNT(STACKGET(th, 2));
	Pkg* pkg=(Pkg*)VALTOPNT(STACKGET(th,1));
	Type* type = (Type*)VALTOPNT(STACKGET(th, 0));
	MM.OM = 0;
	if (!src) goto cleanup;

	if (!pkg) pkg = MM.system;

	if (!(result=compile(&c,src,pkg,type))) goto cleanup;


	STACKDROPN(th, 3);
	STACKPUSH_OM(th, PNTTOVAL(result), EXEC_OM);
	STACKPUSH_OM(th, pkg->start->val,EXEC_OM);

//	STACKSKIP(th,3);
	return 0;

cleanup:
	if (MM.OM) return EXEC_OM;
	while(ref!=STACKREF(th)) STACKPULL(th);
	STACKSETNIL(th,0);
	STACKPUSH_OM(th, NIL, EXEC_OM);
	return 0;

}

int compilePromptAndRun(Thread* th)
{
	int k;

	if ((k= promptOnThread(th))) return k;

	interpreterExec(th,0,0);
	interpreterRun(th,0);

//	PRINTF(th, LOG_ERR,"->");
//	itemDump(LOG_ERR,STACKGET(th,0));
	STACKDROP(th);
	STACKDROP(th);
	return 0;
}
