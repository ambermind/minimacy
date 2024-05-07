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
#ifndef _SYSTEM_
#define _SYSTEM_

#define FUN_NATIVE_LENGTH 2
#define FUN_NATIVE_NAME 0
#define FUN_NATIVE_POINTER 1

#define FUN_USER_LENGTH 4
#define FUN_USER_NAME 0
#define FUN_USER_BC 1
#define FUN_USER_GLOBALS 2
#define FUN_USER_PKG 3

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

	LB* name;	// useless except for debug
	LINT code;	// one of the DEF_CODE_* values above ; positive value means function and gives the number of arguments
	LINT index;
	LW val;
	int valType;
	Type* type;
	LINT proto;
	LINT tagged;
	LB* instances;
	LINT dI;	// derived index
	LINT dCI;	// derived child index
	Def* parent;
	Pkg* pkg;

	Parser* parser;
	LINT parserIndex;
	int public;
	Def* next;
};

#define PKG_FROM_IMPORT 0
#define PKG_FROM_NOTHING 1

#define PKG_STAGE_COMPILING 0
#define PKG_STAGE_READY 1

struct Pkg
{
	LB header;
	FORGET forget;
	MARK mark;

	LINT uid;
	LINT stage;
	LB* name;
	Def* first;
	Def* start;
	HashSlots* defs;
	LB* importList;
	Pkg* listNext;
	int forPrompt;	// indicates whether full package listing should be displayed after compiling
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

File* _fileCreate(Thread* th, void* file);

Def* defAlloc(Thread* th, LINT code,LINT index,LW val,int valType,Type* type);
void defSetParser(Def* def, Compiler* c, LINT index);
void defSet(Def* def,LW val,int valType);
char* defName(Def* def);
char* defPkgName(Def* def);

Pkg* pkgAlloc(Thread* th, LB* name, int nbits, int type);
char* pkgName(Pkg* pkg);

int pkgAddImport(Compiler* c, Pkg* pkg, Pkg* pkgImport, LB* alias);
Pkg* pkgImportByAlias(Pkg* pkg, char* alias);
Pkg* pkgImportByName(char* name);
void pkgCleanCompileError(void);

//void pkgSetStart(Pkg* pkg, LB* start);
int pkgAddDef(Thread* th,Pkg* pkg, LB* name, Def* def);
Def* pkgFirstGet(Pkg* pkg, char* name);
Def* pkgGet(Pkg* pkg, char* name, int followParent);
void pkgRemoveDef(Def* def);
int pkgHasWeak(Compiler* c, Thread* th, Pkg* pkg, int showError);
int pkgDisplay(Thread* th, int mask, Pkg* pkg);

Def* pkgAddType(Thread* th, Pkg *pkg,char* name);
int pkgAddFun(Thread* th, Pkg *pkg,char* name,NATIVE fun,Type* type);
int pkgAddOpcode(Thread* th, Pkg *pkg,char* name,LINT opcode,Type* type);
Def* pkgAddConstInt(Thread* th, Pkg* pkg, char* name, LINT value, Type* type);
Def* pkgAddConstFloat(Thread* th, Pkg* pkg, char* name, LFLOAT value, Type* type);
Def* pkgAddConstPnt(Thread* th, Pkg* pkg, char* name, LB* value, Type* type);

Def* pkgAddSum(Thread* th, Pkg *pkg,char* name);
Def* pkgAddCons(Thread* th, Pkg *pkg,char* name, Def* defType,Type* consType);
Def* pkgAddCons0(Thread* th, Pkg* pkg, char* name, Def* defType);

void systemKeywords(Thread* th);
void systemInit(Thread* th, Pkg *system);
void systemTerminate(void);
int sysSerialInit(Thread* th, Pkg* system);
#endif
