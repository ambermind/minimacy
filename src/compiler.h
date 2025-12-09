// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#ifndef _COMPILER_
#define _COMPILER_

typedef struct FunMaker FunMaker;
typedef struct Parser Parser;

struct FunMaker
{
	LB header;
	FORGET forget;
	MARK mark;

	FunMaker* parent;
	Def* def;
	Def* defForInstances;
	Locals* locals;
	Globals* globals;
	Locals* typeLabels;
	LINT maxlocals;
	LINT level;
	Buffer* bc;
	Type* resultType;

	Type* loopType;
	LINT breakList;
	LINT continueList;
	LINT forceModulo;
	LINT forceMu;
	int forceNumbers;
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
	int mayGetBackToParent;
};

struct Compiler
{
	LB header;
	FORGET forget;
	MARK mark;

	Pkg* pkg;	// current package
	FunMaker* fmk;	// current function maker

	Buffer* bytecode;	// only for optimisation, it is a copy of fmk->bc
	Buffer* firstBytecodeBuffer;	// to be used and reused for functions (not for lambda)
	int displayed;

	int nbDerivations;
	Parser* parser;	// current parser
	Parser* mainParser;	// main parser (only main parser may include files)
	LB* exports;
	Def* def0;
};

#define COMPILER_ERR_SN -1
#define COMPILER_ERR_TYPE -2

#define FUNMAKER_HASHSET_NBITS 4
#define BC_JUMP_SIZE 3
#define LOCALS_MAX_NUMBER 255

#define FORCE_NUMBER_NONE 0
#define FORCE_NUMBER_FLOAT 1
#define FORCE_NUMBER_BIGNUM 2
#define FORCE_NUMBER_MOD 3
#define FORCE_NUMBER_MODOPTI 4

#define BC_ARGS(bc) (((LINT*)STR_START(bc))[0])
#define BC_LOCALS(bc) (((LINT*)STR_START(bc))[1])
#define BC_OFFSET (LWLEN*2)
#define BC_START(bc) (STR_START(bc)+BC_OFFSET)

#define INSTANCE_LENGTH 5
#define INSTANCE_DEF 0
#define INSTANCE_TYPE 1
#define INSTANCE_PARSER 2
#define INSTANCE_POSITION 3
#define INSTANCE_NEXT 4

#define FUN_START_NAME "0000"

LB* exportLabelList(Compiler* c, char* name);
int exportLabelListIsSingle(Compiler* c, char* name);

int funMakerInit(Compiler *c,Locals* locals, Locals* typeLabels, LINT level, Def* def, Def* defForInstances);
void funMakerRelease(Compiler *c);
int funMakerIsForVar(Compiler *c);
Locals* funMakerAddLocal(Compiler* c,char* name);
int funMakerAddGlobal(FunMaker* f, LB* data, LINT* index);
int funMakerNeedGlobal(FunMaker *f,LB *data, LINT* index);

int bc_byte_or_int(Compiler* c,LINT val,char opbyte,char opint);
int bcint_byte_or_int(Compiler* c,LINT val);
int bc_opcode(Compiler* c, LINT opcode);

LINT bytecodePin(Compiler *c);
int bytecodeAddJump(Compiler *c, LINT pin);
void bytecodeSetJump(Compiler *c, LINT index, LINT pin);
int bytecodeAddJumpList(Compiler* c, LINT next);
void bytecodeSetJumpList(Compiler* c, LINT index, LINT pin);
LINT bytecodeAddEmptyJump(Compiler *c);
LINT bytecodeGetJump(char *pc);

LB* bytecodeFinalize(Compiler *c, LINT argc,LB* name);

Type* compileSkipLocal(Compiler* c);
Type* compileLocals(Compiler* c, int* simple);

Type* compileInclude(Compiler* c);
Def* compileParseDef(Compiler* c, char* what);
Type* compileStep1(Compiler* c);
Type* compileStep2(Compiler *c);
Type* compileStep3(Compiler *c);
Type* compileStep4(Compiler *c);
Type* compileProgram(Compiler* c);
Type* compileExpression(Compiler* c);
Type* compileArithm(Compiler* c);
Type* compileTerm(Compiler* c);

Type* compileFor(Compiler* c);
Type* compileWhile(Compiler* c);
Type* compileRepeat(Compiler* c);
Type* compileBreak(Compiler* c);
Type* compileContinue(Compiler* c);

Type* compileGetPoint(Compiler* c, Type* t0);
Type* compileDef(Compiler* c);
Type* compileLet(Compiler* c);
Type* compileLt(Compiler* c);
Type* compileSet(Compiler* c);

Type* compilePointer(Compiler* c);
Type* compileDefHide(Compiler* c);
Type* compileCall(Compiler* c);
Type* compileLambda(Compiler* c);
Type* compileFormat(Compiler* c, Type* returnedType);

Type* compileStructure1(Compiler* c, Def* defType);
Type* compileStructure2(Compiler* c, Def* defType, Locals* labels);
Type* compileStructure3(Compiler* c, Def* structDef, int rec);
Type* compileFields(Compiler* c, Def* p);
Type* compileEmptyStruct(Compiler* c, Def* def);

Type* compileDefCons1(Compiler* c);
Type* compileDefCons2(Compiler* c, Def* defType, Locals* labels);

Type* compileCons(Compiler* c,Def* def);
Type* compileCons0(Compiler* c, Def* def);

Type* compileMatchChoice(Compiler* c,Type* tval,Type* tresult, LINT unifyResults);
Type* compileMatch(Compiler* c, LINT unifyResults);

Type* compileTry(Compiler* c);
Type* compileAbort(Compiler* c);

int typeUnify(Compiler* c,Type* x,Type* y);
Type* typeUnifyFromStack(Compiler* c,Type* fun);

Type* typeInstance(Compiler* c, Def* def);
LINT compileInstanceSolver(Compiler* c);
Type* compile(LB* psrc, Pkg* pkg, int fromImport, int* displayed);

int compileDisplay(int mask,Compiler* c);
char* compileToken(Compiler *c);
Type* compileError(Compiler* c, char *format, ...);
Type* compileErrorInFunction(Compiler* c, char* format, ...);
int compileFunctionIsPrivate(char* token);
Def* compileGetDef(Compiler* c);

int promptOnThread(Thread* th);
int compilePromptAndRun(Thread* th);

#endif
