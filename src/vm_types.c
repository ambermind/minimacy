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

TypeLabel* typeLabelAdd(Thread* th, LW type, LW category, TypeLabel* next)
{
	TypeLabel* l;
	TypeLabel* i=next;
	memoryEnterFast(th);
	l = (TypeLabel*)memoryAllocExt(th, sizeof(TypeLabel), DBG_BIN, NULL, NULL);	if (!l) return NULL;
	while (i && i->category != category) i = i->next;

	l->num = i ? i->num + 1 : 1;
	l->type = type;
	l->category = category;
	l->next = next;
	memoryLeaveFast(th);
	return l;
}
LINT typeLabelGet(TypeLabel* h, LW type)
{
	while (h)
	{
		if (h->type == type) return h->num;
		h = h->next;
	}
	return 0;
}


/*
	LINT code;	// one of the TYPECODE_* values above
	Type* actual;	// NULL for TYPECODE_PRIMARY
	Type* copy;		// NULL for TYPECODE_PRIMARY
	LB* name; // only for TYPECODE_PRIMARY
	LINT nb;
	Type* child[1];
*/
void typeMark(LB* user)
{
	LINT i;
	Type* type=(Type*)user;
	MEMORYMARK((LB*)type,(LB*)type->actual);
	MEMORYMARK((LB*)type,(LB*)type->copy);
	MEMORYMARK((LB*)type,(LB*)type->ref);
	for(i=0;i<type->nb;i++) MEMORYMARK((LB*)type,(LB*)type->child[i]);
}

Type* typeAllocEmpty(Thread* th, LINT code,Ref* ref,LINT nb)
{
	LINT i;
	Type* t=(Type*)memoryAllocExt(th, sizeof(Type)+(nb-1)*LWLEN,DBG_TYPE,NULL,typeMark); if (!t) return NULL;
//	printf("alloc type " LSD " " LSD "\n",code,nb);
	t->code=code;
	t->ref=ref;
	t->actual=NULL;
	t->copy=NULL;
	t->nb=nb;
	for(i=0;i<t->nb;i++) t->child[i]=NULL;
	return t;
}
Type* typeAllocRec(Thread* th, LINT rec)
{
	Type* t = typeAllocEmpty(th, TYPECODE_REC, NULL, 0); if (!t) return NULL;
	t->actual = (Type*)rec;
	return t;
}
Type* typeAlloc(Thread* th, LINT code, Ref* ref,LINT nb,...)
{
	Type* t = typeAllocEmpty(th,code, ref, nb); if (!t) return NULL;
	if (nb)
	{
		LINT i;
		va_list arglist;
		va_start(arglist,nb);
		for(i=0;i<nb;i++) t->child[i]=va_arg(arglist,Type*);
		va_end(arglist);
	}
	return t;
}

Type* typeAllocFromStack(Thread* th, Ref* ref, LINT code, LINT nb)
{
	LINT i;
	Type* t = typeAllocEmpty(th, code, ref, nb); if (!t) return NULL;
	for(i=nb-1;i>=0;i--)
	{
		LB* p=VALTOPNT(STACKPULL(th));
		t->child[i]=(Type*)p;
		MEMORYMARK((LB*)t, p);
	}
	return t;
}
Type* typeAllocWeak(Thread* th)
{
	return typeAlloc(th, TYPECODE_WEAK, NULL, 0);
}
Type* typeAllocUndef(Thread* th)
{
	return typeAlloc(th, TYPECODE_UNDEF, NULL, 0);
}

Type* typeDerivate(Thread* th, Type* p,int weak)
{
	Type* d;

	while (p->actual) p = p->actual;
	d = typeAlloc(th, weak?TYPECODE_WEAK:TYPECODE_UNDEF, NULL, TYPENB_DERIVATIVE,p); if (!d) return d;
	return d;
}
Type* typeUnderivate(Compiler* c,Type* p)
{
	while (p->actual) p = p->actual;
	if (p->code == TYPECODE_UNDEF)
	{
		if (p->nb == TYPENB_DERIVATIVE)
		{
			typeUnify(c, p, p->child[0]);
			while (p->actual) p = p->actual;
			return p;
//			p->actual=p->child[0];
//			return p->child[0];
		}
	}
	return NULL;
}

Type* _compilerParseType(Compiler* c,int mono, int depth, Locals** labels, LINT* withRec)
{
	int n;
	Locals* lb;
	Type* t;

	if (!parserNext(c)) return compileError(c, "Compiler: uncomplete type reaches EOF\n");
	lb = localsGet(*labels, 0, c->parser->token);
//	printf("search %s -> %llx\n", c->parser->token,lb);
	if (lb)
	{
		if (!lb->type) return compileError(c, "Compiler: illegal use of label '%s'\n",STRSTART(lb->name));
		if (lb->type->nb == TYPENB_DERIVATIVE)
		{
			if ((!parserNext(c))|| (strcmp(c->parser->token, "("))) return compileError(c, "Compiler: missing '(' for derivative types, found '%s'\n", c->parser->token);
			if (!(t = _compilerParseType(c, mono, depth + 1, labels, withRec))) return NULL;
			if (t->code != TYPECODE_PRIMARY) return compileError(c, "Compiler: only primary types may be derivated\n");
			if (t->nb!=lb->type->nb) return compileError(c, "Compiler: derivative type has a wrong number of parameters (%d instead of %d)\n",t->nb,lb->type->nb);
			if (parserAssume(c, ")")) return NULL;
		}
		return lb->type;
	}
	if (!strcmp(c->parser->token, "_")) return typeAllocWeak(c->th);
	if (((c->parser->token[0] == 'w')|| (c->parser->token[0] == 'a')) && (isdecimal(c->parser->token + 1)))
	{
		Locals* label = localsCreate(c->th, c->parser->token, 0, NULL, *labels); if (!(labels)) return NULL;
		int weak = (c->parser->token[0] == 'w') ? 1 : 0;
		if (mono && !weak) return compileError(c, "Compiler: polymorphism (%s) is not accepted here\n", c->parser->token);
		*labels = label;
		if ((parserNext(c)) && (!strcmp(c->parser->token, "(")))
		{
			
			if (!(t = _compilerParseType(c, mono, depth + 1, labels, withRec))) return NULL;
			if (t->code != TYPECODE_PRIMARY) return compileError(c, "Compiler: only primary types may be derivated\n");
			if (parserAssume(c, ")")) return NULL;
			label->type = typeDerivate(c->th, t, weak);
			return label->type ;
		}
		else parserGiveback(c);
		label->type = weak?typeAllocWeak(c->th): typeAllocUndef(c->th);
		return label->type;
	}
	else if ((c->parser->token[0] == 'r') && (isdecimal(c->parser->token + 1)))
	{
		LINT i = ls_atoi(c->parser->token + 1,1);
		if ((i < 0) || (i >= depth)) return compileError(c, "Compiler: recursivity out of range %d [0 %d[\n", i, depth);
		*withRec = 1;
		return typeAllocRec(c->th,i);
	}
	if (!strcmp(c->parser->token, "array"))
	{
		Type* nt = typeAllocEmpty(c->th,TYPECODE_ARRAY, NULL, 1); if (!nt) return NULL;

		if (!(t = _compilerParseType(c, mono, depth + 1, labels, withRec))) return NULL;
		nt->child[0] = t;
		return nt;
	}
	if (!strcmp(c->parser->token, "list"))
	{
		Type* nt = typeAllocEmpty(c->th, TYPECODE_LIST, NULL, 1); if (!nt) return NULL;

		if (!(t = _compilerParseType(c, mono, depth + 1, labels, withRec))) return NULL;
		nt->child[0] = t;
		return nt;
	}
	if (!strcmp(c->parser->token, "fifo"))
	{
		Type* nt = typeAllocEmpty(c->th, TYPECODE_FIFO, NULL, 1); if (!nt) return NULL;

		if (!(t = _compilerParseType(c, mono, depth + 1, labels, withRec))) return NULL;
		nt->child[0] = t;
		return nt;
	}
	if (!strcmp(c->parser->token, "hashmap"))
	{
		Type* nt = typeAllocEmpty(c->th, TYPECODE_MAP, NULL, 2); if (!nt) return NULL;

		if (!(t = _compilerParseType(c, mono, depth + 1, labels, withRec))) return NULL;
		nt->child[0] = t;
		if (parserAssume(c, "->")) return NULL;
		if (!(t = _compilerParseType(c, mono, depth + 1, labels, withRec))) return NULL;
		nt->child[1] = t;
		return nt;
	}
	if (!strcmp(c->parser->token, "["))
	{
		n = 0;
		while (1)
		{
			if (!parserNext(c)) return compileError(c, "Compiler: uncomplete type reaches EOF\n");
			if (!strcmp(c->parser->token, "]"))
			{
				return typeAllocFromStack(c->th, NULL, TYPECODE_TUPLE, n);
			}
			parserGiveback(c);
			if (!(t = _compilerParseType(c, mono, depth + 1, labels, withRec))) return NULL;
			TYPEPUSH_NULL(c, t);
			n++;
		}
	}
	else if (!strcmp(c->parser->token, "fun"))
	{
		n = 0;
		while (1)
		{
			if (!parserNext(c)) return compileError(c, "Compiler: uncomplete type reaches EOF\n");
			if (!strcmp(c->parser->token, "->"))
			{
				if (!(t = _compilerParseType(c, mono, depth + 1, labels, withRec))) return NULL;
				TYPEPUSH_NULL(c, t);
				n++;
				return typeAllocFromStack(c->th, NULL, TYPECODE_FUN, n);
			}
			parserGiveback(c);
			if (!(t = _compilerParseType(c, mono, depth + 1, labels, withRec))) return NULL;
			TYPEPUSH_NULL(c, t);
			n++;
		}
	}
	if (islabel(c->parser->token))
	{
		Ref* ref;
		Type* nt;
		LINT n = 0;
		LINT code= TYPECODE_PRIMARY;
		
		ref = compileGetRef(c);
		if (!ref) return compileError(c, "Compiler: unknown label '%s'\n", compileToken(c));
		if ((ref->code != REFCODE_TYPE) && (ref->code != REFCODE_SUM) && (ref->code != REFCODE_STRUCT)) return compileError(c, "Compiler: '%s' is not a type\n", compileToken(c));

		if (parserNext(c))
		{
			if (!strcmp(c->parser->token, "("))
			{
				while (1)
				{
					Type* t;
					if (!(t = _compilerParseType(c, mono, depth + 1, labels, withRec))) return NULL;
					TYPEPUSH_NULL(c, t);
					n++;

					if (!parserNext(c)) return compileError(c, "Compiler: type or ')' expected (found '%s')\n", compileToken(c));

					if (!strcmp(c->parser->token, ")")) break;
					parserGiveback(c);
				}
			}
			else parserGiveback(c);
		}
		nt = typeAllocFromStack(c->th, ref, code, n); if (!nt) return NULL;

		t = ref->type;
		if (t->nb!=nt->nb) return compileError(c, "Compiler: wrong number of parameters for type '%s' %lld/%lld\n", refName(ref),t->nb,nt->nb);
		if ((!t->nb)&& (code == TYPECODE_PRIMARY)) return t;
		return nt;
	}
	return compileError(c, "Compiler: unknown token %s\n", c->parser->token);
}

Type* _compilerHandleRec(Compiler* c, Type* p)
{
	if (p->code == TYPECODE_REC)
	{
		LINT rec = (LINT)p->actual;
		p->actual = VALTOTYPE(STACKGET(c->th, rec));
	}
	else if (p->nb>0)
	{
		LINT i;
		TYPEPUSH_NULL(c, p);
		for (i = 0; i < p->nb; i++) if (!_compilerHandleRec(c, p->child[i])) return NULL;
		STACKDROP(c->th);
	}
	return p;
}
Type* compilerParseTypeDef(Compiler* c, int mono, Locals** labels)
{
	LINT withRec = 0;
	Type *t=_compilerParseType(c, mono, 0, labels, &withRec);
	if (withRec) _compilerHandleRec(c,t);
	return t;
}
int compilerSkipTypeDef(Compiler* c)
{
	if (!parserNext(c)) { compileError(c, "Compiler: uncomplete type reaches EOF\n"); return -1; }
	if (!strcmp(c->parser->token, "array")) return compilerSkipTypeDef(c);
	if (!strcmp(c->parser->token, "list")) return compilerSkipTypeDef(c);
	if (!strcmp(c->parser->token, "fifo")) return compilerSkipTypeDef(c);
	if (!strcmp(c->parser->token, "hashmap"))
	{
		if (compilerSkipTypeDef(c)) return -1;
		if (!parserNext(c)) { compileError(c, "Compiler: uncomplete type reaches EOF\n"); return -1; }
		if (strcmp(c->parser->token, "->")) { compileError(c, "Compiler: -> expected, found '%s'\n",compileToken(c)); return -1; }
		return compilerSkipTypeDef(c);
	}
	if (!strcmp(c->parser->token, "["))
	{
		while (1)
		{
			if (!parserNext(c)) { compileError(c, "Compiler: uncomplete type reaches EOF\n"); return -1; }
			if (!strcmp(c->parser->token, "]")) return 0;
			parserGiveback(c);
			if (compilerSkipTypeDef(c)) return -1;
		}
	}
	if (!strcmp(c->parser->token, "fun"))
	{
		while (1)
		{
			if (!parserNext(c)) { compileError(c, "Compiler: uncomplete type reaches EOF\n"); return -1; }
			if (!strcmp(c->parser->token, "->")) return compilerSkipTypeDef(c);
			parserGiveback(c);
			if (compilerSkipTypeDef(c)) return -1;
		}
	}
	if (islabel(c->parser->token))
	{
		if (!parserNext(c)) return 0;

		if (strcmp(c->parser->token, "("))
		{
			parserGiveback(c);
			return 0;
		}
		while (1)
		{
			if (compilerSkipTypeDef(c)) return -1;
			if (!parserNext(c)) { compileError(c, "Compiler: uncomplete type reaches EOF\n"); return -1; }
			if (!strcmp(c->parser->token, ")")) return 0;
			parserGiveback(c);
		}
	}
	compileError(c, "Compiler: unknown token %s\n", c->parser->token);
	return -1;
}

int _typePrintRec(Thread* th,Buffer* tmp, TypeLabel **h,Type* p,int depth)
{
	LINT i;
	LW typew;
	TypeLabel* labels = *h;


	if (!p) return 0;
	while(p->actual) p=p->actual;
	typew=TYPETOVAL(p);
	STACKPUSH_OM(MM.tmpStack,typew,EXEC_OM);

	for(i=0;i<depth;i++) if (typew==STACKGET(MM.tmpStack,i+1))
	{
		bufferPrintf(th, tmp,"r%d",i);
		STACKPULL(MM.tmpStack);
		return 0;
	}
	if (p->code==TYPECODE_PRIMARY)
	{
		bufferAddStr(th, tmp, refName(p->ref));
		if (p->nb)
		{
			bufferAddStr(th, tmp,"(");
			if (depth<100) for(i=0;i<p->nb;i++)
			{
				if (i) bufferAddStr(th, tmp, " ");
				_typePrintRec(th, tmp,h,p->child[i],depth+1);
			}
			else bufferAddStr(th, tmp, "...");
			bufferAddStr(th, tmp,")");
		}
	}
	else if (p->code==TYPECODE_FUN)
	{
//		if (depth) bufferAddStr(th, tmp, "(");
		bufferAddStr(th, tmp,"fun ");
		for(i=0;i<p->nb-1;i++)
		{
			_typePrintRec(th, tmp,h,p->child[i],depth+1);
			bufferAddStr(th, tmp, " ");
		}
		bufferAddStr(th, tmp,"-> ");
		_typePrintRec(th, tmp,h,p->child[p->nb-1],depth+1);
//		if (depth) bufferAddStr(th, tmp, ")");
	}
	else if (p->code==TYPECODE_LIST)
	{
//		if (depth) bufferAddStr(th, tmp, "(");
		bufferAddStr(th, tmp,"list ");
		_typePrintRec(th, tmp,h,p->child[0],depth+1);
//		if (depth) bufferAddStr(th, tmp, ")");
	}
	else if (p->code==TYPECODE_MAP)
	{
//		if (depth) bufferAddStr(th, tmp, "(");
		bufferAddStr(th, tmp,"hashmap ");
		_typePrintRec(th, tmp,h,p->child[0],depth+1);
		bufferAddStr(th, tmp," -> ");
		_typePrintRec(th, tmp,h,p->child[1],depth+1);
//		if (depth) bufferAddStr(th, tmp, ")");
	}
	else if (p->code==TYPECODE_FIELD)
	{
//		bufferAddStr(th, tmp,"field ");
		_typePrintRec(th, tmp,h,p->child[0],depth+1);
		bufferAddStr(th, tmp," -> ");
		_typePrintRec(th, tmp,h,p->child[1],depth+1);
	}
	else if (p->code==TYPECODE_ARRAY)
	{
//		if (depth) bufferAddStr(th, tmp, "(");
		bufferAddStr(th, tmp,"array ");
		_typePrintRec(th, tmp,h,p->child[0],depth+1);
//		if (depth) bufferAddStr(th, tmp, ")");
	}
	else if (p->code == TYPECODE_FIFO)
	{
//		if (depth) bufferAddStr(th, tmp, "(");
		bufferAddStr(th, tmp, "fifo ");
		_typePrintRec(th, tmp, h, p->child[0], depth + 1);
//		if (depth) bufferAddStr(th, tmp, ")");
	}
	else if (p->code==TYPECODE_TUPLE)
	{
		bufferAddStr(th, tmp,"[");
		for(i=0;i<p->nb;i++)
		{
			if (i) bufferAddStr(th, tmp," ");
			_typePrintRec(th, tmp,h,p->child[i],depth+1);
		}
		bufferAddStr(th, tmp,"]");
	}
	else if ((p->code==TYPECODE_UNDEF)||(p->code==TYPECODE_WEAK))
	{
		LINT num;
		if ((p->nb == TYPENB_DERIVATIVE) && (p->child[0]->ref->dCI == 0))
		{
			_typePrintRec(th, tmp, h, p->child[0], depth + 1);
		}
		else
		{
			num = typeLabelGet(labels, typew);
			if (!num)
			{
				*h = typeLabelAdd(th, typew, INTTOVAL(p->code), labels); if (!(*h)) return EXEC_OM;
				num = (*h)->num;
			}
//			printf("\ntypeLabelGet %llx: %d -> %d\n", typew, p->code, num);
			bufferPrintf(th, tmp, "%c"LSD, (p->code == TYPECODE_UNDEF) ? 'a' : 'w', num);
			if (p->nb == TYPENB_DERIVATIVE)	// derivative struct
			{
				bufferAddStr(th, tmp, "(");
				_typePrintRec(th, tmp, h, p->child[0], depth + 1);
				bufferAddStr(th, tmp, ")");
			}
		}
	}
	STACKPULL(MM.tmpStack);
	return 0;
}

int typeBuffer(Thread *th,Buffer* tmp, Type* type)
{
	int k;
	TypeLabel* h = NULL;
	stackReset(MM.tmpStack);
	memoryEnterFast(th);
	if ((k = _typePrintRec(th, tmp, &h, type, 0))) return k;
	memoryLeaveFast(th);
	return 0;
}
int typePrint(Thread* th, int mask, Type* type)
{
	int k;
	if (!termCheckMask(mask)) return 0;
	bufferReinit(MM.tmpBuffer);
	if ((k = typeBuffer(th, MM.tmpBuffer, type))) return k;
	termWrite(th, mask, bufferStart(MM.tmpBuffer), bufferSize(MM.tmpBuffer));
	return 0;
}


// initialize types required by the compiler
void typesInit(Thread* th, Pkg* system)
{
	Ref* Exception;
	Ref* Boolean;
	Type* u0, * list_u0, * array_u0;

	MM.I = pkgAddType(th,system, "Int")->type;
	MM.F = pkgAddType(th,system, "Float")->type;
	MM.S = pkgAddType(th,system, "Str")->type;
	MM.Bytes = pkgAddType(th,system, "Bytes")->type;
	MM.BigNum = pkgAddType(th,system, "BigNum")->type;
	MM.Bitmap = pkgAddType(th,system, "Bitmap")->type;
	MM.Buffer = pkgAddType(th,system, "Buffer")->type;

	Boolean = pkgAddSum(th,system, "Bool");
	MM.Boolean = Boolean->type;
	MM.trueRef = PNTTOVAL(pkgAddCons0(th,system, "true", Boolean));
	MM.falseRef = PNTTOVAL(pkgAddCons0(th,system, "false", Boolean));

	MM.Pkg = pkgAddType(th,system, "Package")->type;
	MM.Thread = pkgAddType(th,system, "_Thread")->type;
	Exception = pkgAddSum(th,system, "Exception");
	MM.Exception = Exception->type;
	pkgAddCons0(th,system, "anyException", Exception);
	MM.MemoryException =PNTTOVAL(pkgAddCons0(th,system, "memoryException", Exception));

	u0 = typeAllocUndef(th);
	list_u0 = typeAlloc(th,TYPECODE_LIST, NULL, 1, u0);
	array_u0 = typeAlloc(th,TYPECODE_ARRAY, NULL, 1, u0);

	MM.fun_u0_list_u0_list_u0 = typeAlloc(th, TYPECODE_FUN, NULL, 3, u0, list_u0, list_u0);
	MM.fun_array_u0_I_u0 = typeAlloc(th, TYPECODE_FUN, NULL, 3, array_u0, MM.I, u0);
}



// type graph copy
int typeRecNeedCopy(Type* p)
{
	int i;

	if (!p) return 0;
	while(p->actual) p=p->actual;
	if (p->copy) return 0;
	if (p->code==TYPECODE_UNDEF) return 1;
	p->copy=p;
	for(i=0;i<p->nb;i++) if (typeRecNeedCopy(p->child[i])) return 1;
	return 0;
}
Type* typeRecCopy(Thread* th, Type* p)
{
	int i;
	Type* t;
	LINT copyCode;

	if (!p) return p;
	while(p->actual) p=p->actual;
	if (p->copy) return p->copy;
	
	if ((p->code==TYPECODE_PRIMARY)&&(p->nb==0)) return p;	// primary type, non parametric, no copy
	if (p->code==TYPECODE_WEAK) return p;	// weak type, do not copy

	copyCode = p->code;
//	if (p->code == TYPECODE_UNDEF) copyCode = TYPECODE_WEAK;
	t=typeAllocEmpty(th, copyCode,p->ref,p->nb); if (!t) return NULL;	// copy everything else
	p->copy=t;

	for (i = 0; i < p->nb; i++) {
		Type* r = typeRecCopy(th, p->child[i]); if (!r) return NULL;
		t->child[i] = r;
	}
	return t;
}
void typeRecReset(Type* p)
{
	int i;

	if (!p) return;
	while(p->actual) p=p->actual;
	if (!p->copy) return;
	p->copy=NULL;
	for(i=0;i<p->nb;i++) typeRecReset(p->child[i]);
}

Type* typeCopy(Thread* th, Type* p)
{
	if (!p) return NULL;
	if (typeRecNeedCopy(p))
	{
		Type* t;
		typeRecReset(p);
		t=typeRecCopy(th, p);
		typeRecReset(p);
		return t;
	}
	typeRecReset(p);
	return p;
}

void typeRecHasWeak(Type* p, int* flag)
{
	int i;

	if (!p) return;
	while (p->actual) p = p->actual;
	if (p->copy) return;

	if (p->code == TYPECODE_WEAK)
	{
		if ((p->nb != TYPENB_DERIVATIVE) || (p->child[0]->ref->dCI > 0))
		{
			*flag = 1;
			return;
		}
	}

	if (!p->nb) return;

	p->copy = p;
	for (i = 0; i < p->nb; i++) typeRecHasWeak(p->child[i], flag);
}

int typeHasWeak(Type* p)
{
	int flag = 0;
	typeRecHasWeak(p, &flag);
	typeRecReset(p);
	return flag;
}

// undef -> weak

void typeRecGoWeak(Type* p)
{
	int i;

	if (!p) return;
	while(p->actual) p=p->actual;
	if (p->copy) return;
	
	if (p->code==TYPECODE_UNDEF) p->code=TYPECODE_WEAK;
	if (!p->nb) return;

	p->copy=p;
	for(i=0;i<p->nb;i++) typeRecGoWeak(p->child[i]);
}

void typeGoWeak(Type* p)
{
	typeRecGoWeak(p);
	typeRecReset(p);
}

int typePrimaryIsChild(Ref* child, Ref* parent)
{
	while(child)
	{
		if (child == parent) return 1;
		child = child->parent;
	}
	return 0;
}

int typeRecUnify(Type* s, Type* t);

int typeLink(Type* from, Type* to, Type* parentUnif, Type* childUnif )
{
	int i,k;
	Type* save = from->actual;
	from->actual = to;
	if (from->code == TYPECODE_WEAK) typeGoWeak(to);
	if (parentUnif)	for (i = 0; i < parentUnif->nb; i++)
	{
		if ((k = typeRecUnify(childUnif->child[i], parentUnif->child[i])))
		{
			from->actual = save;
			return k;
		}
	}
	return 0;
}
// unification
int typeRecUnify(Type* s,Type* t)
{
	Type* z;
	LINT ns, nt;

	while(s->actual) s=s->actual;
	while(t->actual) t=t->actual;
	if (s==t) return 0;	// this includes non parametric primary types
	
	if ((t->code == TYPECODE_UNDEF) || (t->code == TYPECODE_WEAK)) { z = t; t = s; s = z; }
	ns = s->code; nt = t->code;

	if ((ns == TYPECODE_UNDEF) || (ns == TYPECODE_WEAK))
	{
		if (s->nb == TYPENB_DERIVATIVE)
		{
			Type* parentS = s->child[0];
			if ((nt == TYPECODE_UNDEF) || (nt == TYPECODE_WEAK))
			{
				if (t->nb == TYPENB_DERIVATIVE)
				{
					Type* parentT = t->child[0];
					if (typePrimaryIsChild(parentT->ref, parentS->ref)) return typeLink(s,t, parentS, parentT );
					if (typePrimaryIsChild(parentS->ref, parentT->ref)) return typeLink(t,s, parentT, parentS);
					return LS_ERR_TYPE;
				}
				return typeLink(t, s,NULL,NULL);
			}
			if (nt == TYPECODE_PRIMARY)
			{
				if (typePrimaryIsChild(t->ref, parentS->ref)) return typeLink(s, t,parentS, t);
			}
			return LS_ERR_TYPE;
		}
		return typeLink(s, t,NULL,NULL);
	}
	if (ns != nt) return LS_ERR_TYPE;
	if (ns == TYPECODE_PRIMARY)
	{
		if (s->ref != t->ref) return LS_ERR_TYPE;
	}
	if (s->nb != t->nb) return LS_ERR_TYPE;

	return typeLink(t, s,t,s);
}

int typeUnify(Compiler* c,Type* x,Type* y)
{
	int err=0;
	if ((!x) || (!y)) return LS_ERR_TYPE;
//	printf(  "unify:"); typePrint(-1, x); printf("  with:"); typePrint(-1, y); printf("\n");
	if (!(err = typeRecUnify(x, y)))
	{
//		printf("---> "); typePrint(-1, x); printf("\n");
		return 0;
	}
	compileError(c, "Compiler: '");
	typePrint(c->th, LOG_USER,x);
	PRINTF(c->th, LOG_USER,"' does not match with '");
	typePrint(c->th, LOG_USER,y);
	PRINTF(c->th, LOG_USER,"'\n");

	if (c->fmk)
	{
		Globals* globals = c->fmk->globals;
		Locals* lb = c->fmk->locals;
		if (lb) PRINTF(c->th, LOG_USER, "Provisional infered types:\n");
		while (lb)
		{
			if (lb->name)
			{
				PRINTF(c->th, LOG_USER, "   local %s: ",STRSTART(lb->name));
				typePrint(c->th, LOG_USER, lb->type);
				PRINTF(c->th, LOG_USER, "\n");
			}
			lb = lb->next;
		}
		while (globals)
		{
			if (globals->data && HEADER_DBG(globals->data)== DBG_REF)
			{
				Ref* ref = (Ref*)globals->data;
				if (ref->name)
				{
					PRINTF(c->th, LOG_USER, "   global %s: ", STRSTART(ref->name));
					typePrint(c->th, LOG_USER, ref->type);
					PRINTF(c->th, LOG_USER, "\n");
				}
			}
			globals = globals->next;
		}
	}
	return err;
}


// assume that stack contains arg0, ... argn-1 and fun is an instance of/or a function with n arg and 1 result
Type* typeUnifyFromStack(Compiler* c,Type* fun)
{
	LINT i;
	i= fun->nb - 2;
	while(i>=0) if (typeUnify(c,VALTOTYPE(STACKPULL(c->th)),fun->child[i--])) return NULL;	// check and pull args
	return fun->child[fun->nb-1];	// return unified function result
}
