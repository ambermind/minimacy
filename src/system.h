// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#ifndef _SYSTEM_
#define _SYSTEM_

#define NATIVE_FUN 0
#define NATIVE_OPCODE 1
#define NATIVE_INT 2
#define NATIVE_FLOAT 3

#define NATIVE_DEF_BITS 11
#define NATIVE_DEF_LENGTH (1<<NATIVE_DEF_BITS)

typedef struct {
	const int code;
	const char* name;
	const void* value;
	const char* type;
}Native;
extern Native* NativeDefs[NATIVE_DEF_LENGTH];
extern unsigned char NativeDefsArgc[NATIVE_DEF_LENGTH];

#define FUN_NATIVE_LENGTH 2
#define FUN_NATIVE_NAME 0
#define FUN_NATIVE_OPCODE 1

#define FUN_USER_LENGTH 3
#define FUN_USER_NAME 0
#define FUN_USER_BC 1
#define FUN_USER_GLOBALS 2

#define DEF_CODE_TYPE -1	// type
#define DEF_CODE_VAR -2	// variable
#define DEF_CODE_FIELD -3	// structure field
#define DEF_CODE_STRUCT -4	// struct type
#define DEF_CODE_CONS -5	// constructor of sum type
#define DEF_CODE_SUM -6	// sum type
#define DEF_CODE_CONST -7	// constant
#define DEF_CODE_CONS0 -8	// constructor of sum type without argument
#define DEF_CODE_EXTEND -9	// temporary definition for "extend" declaration
#define DEF_CODE_TYPECHECK -10	// typechecking declaration

#define DEF_INDEX_BC (0)
#define DEF_INDEX_NATIVE (-1)
#define DEF_INDEX_OPCODE (-2)
#define DEF_INDEX_VALUE (-3)	// const or var after second pass
#define DEF_INDEX_STATIC (-4)	// made temporarily from static definitions

#define DEF_HIDDEN 0
#define DEF_PUBLIC 1


// fun bytecode (code>0)  : index=DEF_INDEX_BC, val=bytecode (LB*), type
// fun native (code>0)  : index=DEF_INDEX_NATIVE, val=NATIVE pointer, type
// DEF_CODE_TYPE  : type
// DEF_CODE_VAR   : val, type
// DEF_CODE_FIELD : index=index of field, val=def of next field, type=field Type Field
// DEF_CODE_CONS  : index=index in sum type, val=def of next cons, type=fun _ _ _ Type
// DEF_CODE_STRUCT: index=number of fields, val= def of first child, type=primary Type
// DEF_CODE_SUM   : index=number of constructors(may be increased dynamically), val= def of first child, type=primary Type
// DEF_CODE_CONST : val, type

struct Def
{
	LB header;
	FORGET forget;
	MARK mark;

	Def* next;
	LB* name;	// useless except for debug
	LINT code;	// one of the DEF_CODE_* values above ; positive value means function and gives the number of arguments
	LINT index;
	Type* type;
	LB* instances;
	Parser* parser;
	LW val;
	Def* parent;

	int parserIndex;

	short dI;	// derived index
	short dCI;	// derived child index

	char valType;
	char proto;
	char tagged;
	char public;
};

#define PKG_FROM_IMPORT 0
#define PKG_FROM_NOTHING 1

#define PKG_STAGE_TO_COMPILE 0
#define PKG_STAGE_COMPILING 1
#define PKG_STAGE_READY 2

struct Pkg
{
	LB header;
	FORGET forget;
	MARK mark;

	LINT memory;
	LB* name;
	Def* first;
	Def* start;
	HashSlots* defs;
	LB* importList;
	Pkg* listNext;
	char stage;
	char forPrompt;	// indicates whether full package listing should be displayed after compiling
};

typedef struct
{
	LB header;
	FORGET forget;
	MARK mark;

	void* file;
}File;

#define PACKAGE_HASHMAP_NBITS 6

#define IMPORT_LENGTH 2
#define IMPORT_ALIAS 0
#define IMPORT_PKG 1

#define NATIVE_DEF(defs) nativeDefInsert(defs,sizeof(defs)/sizeof(Native))
LINT nativeDefInsert(const Native* n, int nb);
Def* systemMakeNative(LINT i);
Def* systemGetNative(char* name);
LINT nativeOpcode(char* name, int argc);
int systemMakeAllNatives(void);

Def* defAlloc(LINT code,LINT index,LW val,int valType,Type* type);
void defSetParser(Def* def, Compiler* c, LINT index);
void defSet(Def* def,LW val,int valType);
char* defName(Def* def);
char* defPkgName(Def* def);
void defReverse(Pkg* p);

Pkg* pkgAlloc(LB* name, int nbits, int type);
char* pkgName(Pkg* pkg);

int pkgAddImport(Compiler* c, Pkg* pkg, LB* alias);
Pkg* pkgImportByAlias(Pkg* pkg, char* alias);
Pkg* pkgImportByName(char* name);
void pkgCleanCompileError(void);

//void pkgSetStart(Pkg* pkg, LB* start);
int pkgAddDef(Pkg* pkg, LB* name, Def* def);
Def* pkgFirstGet(Pkg* pkg, char* name);
Def* pkgGet(Pkg* pkg, char* name, int followParent);
void pkgRemoveDef(Def* def);
int pkgHasWeak(Compiler* c, Pkg* pkg, int showError);
int pkgDisplay(int mask, Pkg* pkg);

Def* pkgAddType(Pkg *pkg,char* name);
Def* pkgAddOpcodeStr(Pkg* pkg, char* name, LINT opcode, char* typeStr);
Def* pkgAddConstIntStr(Pkg* pkg, char* name, LINT value, char* typeStr);
Def* pkgAddConstFloatStr(Pkg* pkg, char* name, LFLOAT value, char* typeStr);

Def* pkgAddConstInt(Pkg* pkg, char* name, LINT value, Type* type);
Def* pkgAddConstFloat(Pkg* pkg, char* name, LFLOAT value, Type* type);
Def* pkgAddConstPnt(Pkg* pkg, char* name, LB* value, Type* type);

Def* pkgAddSum(Pkg *pkg,char* name);
Def* pkgAddCons(Pkg *pkg,char* name, Def* defType,Type* consType);
Def* pkgAddCons0(Pkg* pkg, char* name, Def* defType);

void systemInit(Pkg *system);
void systemTerminate(void);

int hostOnlyFunctionsInit(Pkg* system);

#endif
