// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
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

	short code;	// one of the TYPECODE_* values above
	short nb;
	Type* actual;	// NULL for TYPECODE_PRIMARY
	Type* copy;		// NULL for TYPECODE_PRIMARY
	Def* def;
	Type* child[0];
};

extern Type* TypeTest;

#define TYPEFIELD_MAIN 0
#define TYPEFIELD_FIELD 1

#define TYPE_PUSH_NULL(t) STACK_PUSH_PNT_ERR(MM.tmpStack,(LB*)t,NULL)

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

Type* typeAllocEmpty(LINT code,Def* def,LINT nb);
Type* typeAlloc(LINT code, Def* def,LINT nb,...);
Type* typeAllocWeak(void);
Type* typeAllocUndef(void);
Type* typeDerivate(Type* parent);
Type* typeUnderivate(Compiler* c, Type* p);
Type* typeAllocFromStack(Def* def, LINT code, LINT nb);
int compilerSkipTypeDef(Compiler* c);
Type* compilerParseTypeDef(Compiler* c, int mono, Locals** labels);
Type* typeParseStatic(const char* typeStr);

int typeBuffer(Buffer* tmp, Type* type);
int typePrint(int mask,Type* type);

void typesInit(Pkg* system);

int typePrimaryIsChild(Def* child, Def* parent);
Type* typeCopy(Type* p);
int typeHasWeak(Type* p);
Type* typeSimplify(Type* p);


#endif
