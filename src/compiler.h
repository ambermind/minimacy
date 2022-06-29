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
#ifndef _COMPILER_
#define _COMPILER_

typedef struct FunMaker FunMaker;
typedef struct Parser Parser;

struct FunMaker
{
	Thread* th;
	Ref* ref;
	Ref* refForInstances;
	Locals* locals;
	Globals* globals;
	LINT maxlocals;
	LINT level;
	Buffer* bc;
	Type* resultType;

	Type* breakType;
	int breakUse;
	int forceNumbers;
	LINT forceModulo;
	LINT forceMu;

	FunMaker *parent;
};

struct Parser
{
	LB header;
	FORGET forget;
	MARK mark;

	LB* name;
	LB* block;
	char* src;
	int index;

	char savedchar;
	int indexsavedchar;

	char* token;

	int again;
	int index0;
	Parser* parent;
	Parser* nextLib;
};

struct Compiler
{
	LINT pastUid;	// counter of last package before compilation
	Pkg* pkg0;	// initial compilation package
	Thread* th;	// we need a stack for some compiling operations
	Pkg* pkg;	// current parser
	FunMaker* fmk;	// current function maker

	Buffer* bytecode;	// only for optimisation, it is a copy of fmk->bc

	int displayed;

	int nbDerivations;
	Parser* parser;	// current parser
	Parser* parserLib;	// library of parsers
};

#define FUNMAKER_HASHSET_NBITS 4
#define BC_JUMP_SIZE 3
#define LOCALS_MAX_NUMBER 255

#define FORCE_NUMBER_NONE 0
#define FORCE_NUMBER_FLOAT 1
#define FORCE_NUMBER_BIGNUM 2
#define FORCE_NUMBER_MOD 3
#define FORCE_NUMBER_MODOPTI 4

#define BC_ARGS(bc) (((LINT*)STRSTART(bc))[0])
#define BC_LOCALS(bc) (((LINT*)STRSTART(bc))[1])
#define BC_OFFSET (LWLEN*2)
#define BC_START(bc) (STRSTART(bc)+BC_OFFSET)

#define INSTANCE_LEN 5
#define INSTANCE_REF 0
#define INSTANCE_TYPE 1
#define INSTANCE_POSITION 2
#define INSTANCE_PARSER 3
#define INSTANCE_NEXT 4

#define FUN_START_NAME "0000"

int funMakerInit(Compiler *c,FunMaker *f,Locals* locals,LINT level, Ref* ref, Ref* refForInstances);
void funMakerRelease(Compiler *c);
int funMakerIsForVar(Compiler *c);
Locals* funMakerAddLocal(Compiler* c,char* name);
int funMakerAddGlobal(FunMaker* f, LB* data, LINT* index);
int funMakerNeedGlobal(FunMaker *f,LB *data, LINT* index);

int bc_byte_or_int(Compiler* c,LINT val,char opbyte,char opint);
int bcint_byte_or_int(Compiler* c,LINT val);

LINT bytecodePin(Compiler *c);
int bytecodeAddJump(Compiler *c, LINT pin);
void bytecodeSetJump(Compiler *c, LINT index, LINT pin);
LINT bytecodeAddEmptyJump(Compiler *c);
LINT bytecodeGetJump(char *pc);

LB* bytecodeFinalize(Compiler *c, LINT argc,LB* name);

Type* compileSkipLocal(Compiler* c);
Type* compileLocals(Compiler* c, int* simple);

Type* compileInclude(Compiler* c);
Ref* compileParseRef(Compiler* c, char* what);
Type* compileStep1(Compiler* c);
Type* compileStep2(Compiler *c);
Type* compileStep3(Compiler *c);
Type* compileStep4(Compiler *c);
Type* compileProgram(Compiler* c);
Type* compileExpression(Compiler* c);
Type* compileArithm(Compiler* c);
Type* compileTerm(Compiler* c,int noPoint);

Type* compileFor(Compiler* c);
Type* compileWhile(Compiler* c);
Type* compileBreak(Compiler* c);

Type* compileGetPoint(Compiler* c, Type* t0);
Type* compileRef(Compiler* c,int noPoint);
Type* compileLet(Compiler* c);
Type* compileSet(Compiler* c);

Type* compilePointer(Compiler* c);
Type* compileCall(Compiler* c);
Type* compileLambda(Compiler* c);
Type* compileFormat(Compiler* c);

Type* compileStructure1(Compiler* c, Ref* refType);
Type* compileStructure2(Compiler* c, Ref* refType, Locals* labels);
Type* compileStructure3(Compiler* c, Ref* structRef, int rec);
Type* compileFields(Compiler* c, Ref* p);
Type* compileEmptyStruct(Compiler* c, Ref* ref);

Type* compileDefCons1(Compiler* c);
Type* compileDefCons2(Compiler* c, Ref* refType, Locals* labels);

Type* compileCons(Compiler* c,Ref* ref);
Type* compileCons0(Compiler* c, Ref* ref);

Type* compileMatchChoice(Compiler* c,Type* tval,Type* tresult,LINT* end,int trycatch);
Type* compileMatch(Compiler* c);

Type* compileTry(Compiler* c);
Type* compileThrow(Compiler* c);

int typeUnify(Compiler* c,Type* x,Type* y);
Type* typeUnifyFromStack(Compiler* c,Type* fun);

Type* typeInstance(Compiler* c, Ref* ref);
LINT compileInstanceSolver(Compiler* c);
Type* compile(Compiler* c, LB* psrc, Pkg* pkg, Type* expectedType);

int compileDisplay(int mask,Compiler* c);
char* compileToken(Compiler *c);
Type* compileError(Compiler* c, char *format, ...);
Type* compileErrorInFunction(Compiler* c, char* format, ...);
int compileFunctionIsPrivate(char* token);
Ref* compileGetRef(Compiler* c);

int promptOnThread(Thread* th);
int compilePromptAndRun(Thread* th);

#endif
