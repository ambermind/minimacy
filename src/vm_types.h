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
#ifndef _TYPES_
#define _TYPES_

#define TYPECODE_PRIMARY 0	// 0 child for non-parametric
#define TYPECODE_WEAK 1	// 0 child
#define TYPECODE_UNDEF 2	// 0 child
#define TYPECODE_FUN 3	// 2 childs
#define TYPECODE_LIST 4	// 1 child
#define TYPECODE_TUPLE 5	// n sons
#define TYPECODE_ARRAY 6	// 1 child
#define TYPECODE_HASHMAP 8	// 2 childs
#define TYPECODE_FIELD 9	// 2 childs: type struct, type field
#define TYPECODE_FIFO 10	// 1 child
#define TYPECODE_REC 11	// 0 child	// temporary only
#define TYPECODE_HASHSET 12	// 1 child

#define TYPENB_DERIVATIVE 1
struct Type
{
	LB header;
	FORGET forget;
	MARK mark;

	LINT code;	// one of the TYPECODE_* values above
	Type* actual;	// NULL for TYPECODE_PRIMARY
	Type* copy;		// NULL for TYPECODE_PRIMARY
	Def* def;
	LINT nb;
	Type* child[1];
};

extern Type* TypeTest;

#define TYPEFIELD_MAIN 0
#define TYPEFIELD_FIELD 1

#define TYPE_PUSH_NULL(c,t) STACK_PUSH_PNT_ERR((c)->th,(LB*)t,NULL)

typedef struct TypeLabel TypeLabel;

struct TypeLabel
{
	LB header;
	FORGET forget;
	MARK mark;

	LB* type;
	LINT num;
	LINT category;
	TypeLabel* next;
};

Type* typeAllocEmpty(Thread* th, LINT code,Def* def,LINT nb);
Type* typeAlloc(Thread* th, LINT code, Def* def,LINT nb,...);
Type* typeAllocWeak(Thread* th);
Type* typeAllocUndef(Thread* th);
Type* typeDerivate(Thread* th, Type* parent);
Type* typeUnderivate(Compiler* c, Type* p);
Type* typeAllocFromStack(Thread* th, Def* def, LINT code, LINT nb);
int compilerSkipTypeDef(Compiler* c);
Type* compilerParseTypeDef(Compiler* c, int mono, Locals** labels);

int typeBuffer(Thread* th, Buffer* tmp, Type* type);
int typePrint(Thread* th, int mask,Type* type);

void typesInit(Thread* th, Pkg* system);

int typePrimaryIsChild(Def* child, Def* parent);
Type* typeCopy(Thread* th, Type* p);
int typeHasWeak(Type* p);

#endif
