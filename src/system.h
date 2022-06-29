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

#define FUN_NATIVE_LEN 2
#define FUN_NATIVE_NAME 0
#define FUN_NATIVE_POINTER 1

#define FUN_USER_LEN 4
#define FUN_USER_NAME 0
#define FUN_USER_BC 1
#define FUN_USER_GLOBALS 2
#define FUN_USER_PKG 3

#define REFCODE_TYPE -1	// type
#define REFCODE_VAR -2	// variable
#define REFCODE_FIELD -3	// structure field
#define REFCODE_STRUCT -4	// struct type
#define REFCODE_CONS -5	// constructor of sum type
#define REFCODE_SUM -6	// sum type
#define REFCODE_CONST -7	// constant
#define REFCODE_CONS0 -8	// constructor of sum type without argument
#define REFCODE_EXTEND -9	// temporary reference for "extend" declaration

#define REFINDEX_BC (0)
#define REFINDEX_NATIVE (-1)
#define REFINDEX_OPCODE (-2)
#define REFINDEX_VALUE (-3)	// const or var after second pass

#define REF_HIDDEN 0
#define REF_PUBLIC_DEFAULT 1
#define REF_PUBLIC_FORCED 2


// fun bytecode (code>0)  : index=REFINDEX_BC, val=bytecode (LB*), type
// fun native (code>0)  : index=REFINDEX_NATIVE, val=NATIVE pointer, type
// REFCODE_TYPE  : type
// REFCODE_VAR   : val, type
// REFCODE_FIELD : index=index of field, val=ref of next field, type=field Type Field
// REFCODE_CONS  : index=index in sum type, val=ref of next cons, type=fun _ _ _ Type
// REFCODE_STRUCT: index=number of fields, val= ref of first child, type=primary Type
// REFCODE_SUM   : index=number of constructors(may be increased dynamically), val= ref of first child, type=primary Type
// REFCODE_CONST : val, type

struct Ref
{
	LB header;
	FORGET forget;
	MARK mark;

	LB* name;	// useless except for debug
	LINT code;	// one of the REFCODE_* values above ; positive value means function and gives the number of arguments
	LINT index;
	LW val;
	Type* type;
	LINT proto;
	LINT tagged;
	LB* instances;
	LINT dI;	// derived index
	LINT dCI;	// derived child index
	Ref* parent;
	Pkg* pkg;

	Parser* parser;
	LINT parserIndex;
	int public;
	Ref* next;
};

#define PKG_STAGE_EMPTY 0
#define PKG_STAGE_1 1
#define PKG_STAGE_2 2
#define PKG_STAGE_3 3
#define PKG_STAGE_READY 4

struct Pkg
{
	LB header;
	FORGET forget;
	MARK mark;

	LINT uid;
	LINT stage;
	LB* name;
	Ref* first;
	Ref* start;
	Hashmap* refs;
	LB* importList;
	Pkg* listNext;
};

typedef struct
{
	LB header;
	FORGET forget;
	MARK mark;

	void* file;
}File;

#define PACKAGE_HASHSET_NBITS 6
#define VALTOREF(p) (Ref*)VALTOPNT(p)

#define IMPORT_LENGTH 2
#define IMPORT_ALIAS 0
#define IMPORT_PKG 1

#define PKG_FROM_IMPORT 0
#define PKG_EMPTY 1
File* _fileCreate(Thread* th, void* file);

Ref* refAlloc(Thread* th, LINT code,LINT index,LW val,Type* type);
void refSetParser(Ref* ref, Compiler* c, LINT index);
void refSet(Ref* ref,LW val);
char* refName(Ref* ref);
char* refPkgName(Ref* ref);

Pkg* pkgAlloc(Thread* th, LB* name, int nbits, int type);
char* pkgName(Pkg* pkg);

int pkgAddImport(Compiler* c, Pkg* pkg, Pkg* pkgImport, LB* alias);
Pkg* pkgImportByAlias(Pkg* pkg, char* alias);
Pkg* pkgImportByName(char* name);

//void pkgSetStart(Pkg* pkg, LB* start);
int pkgAddRef(Thread* th,Pkg* pkg, LB* name, Ref* ref);
Ref* pkgFirstGet(Pkg* pkg, char* name);
Ref* pkgGet(Pkg* pkg, char* name, int followParent);
void pkgRemoveRef(Ref* ref);
void pkgHideAll(Pkg* pkg);
int pkgHasWeak(Thread* th,Pkg* pkg);
int pkgDisplay(Thread* th, int mask, Pkg* pkg);

Ref* pkgAddType(Thread* th, Pkg *pkg,char* name);
int pkgAddFun(Thread* th, Pkg *pkg,char* name,NATIVE fun,Type* type);
int pkgAddOpcode(Thread* th, Pkg *pkg,char* name,LINT opcode,Type* type);
Ref* pkgAddConst(Thread* th, Pkg *pkg,char* name,LW value,Type* type);
Ref* pkgAddSum(Thread* th, Pkg *pkg,char* name);
Ref* pkgAddCons(Thread* th, Pkg *pkg,char* name, Ref* refType,Type* consType);
Ref* pkgAddCons0(Thread* th, Pkg* pkg, char* name, Ref* refType);

void systemKeywords(Thread* th);
void systemInit(Thread* th, Pkg *system);
void systemTerminate(void);

#endif
