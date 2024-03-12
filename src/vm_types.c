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

TypeLabel* typeLabelAdd(Thread* th, LB* type, LINT category, TypeLabel* next)
{
	TypeLabel* l;
	TypeLabel* i=next;
	memoryEnterFast();
	l = (TypeLabel*)memoryAllocExt(th, sizeof(TypeLabel), DBG_BIN, NULL, NULL);	if (!l) return NULL;

	// this is to find the next available index a0, a1, a2, ...
	// category is for weak or any
	// the first label of the given category in this list is the last added
	while (i && i->category != category) i = i->next;

	l->num = i ? i->num + 1 : 1;
	l->type = type;
	l->category = category;
	l->next = next;
	memoryLeaveFast();
	return l;
}
LINT typeLabelGet(TypeLabel* h, LB* type)
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
	MEMORY_MARK((LB*)type,(LB*)type->actual);
	MEMORY_MARK((LB*)type,(LB*)type->copy);
	MEMORY_MARK((LB*)type,(LB*)type->def);
	for(i=0;i<type->nb;i++) MEMORY_MARK((LB*)type,(LB*)type->child[i]);
}

Type* typeAllocEmpty(Thread* th, LINT code,Def* def,LINT nb)
{
	LINT i;
	Type* t=(Type*)memoryAllocExt(th, sizeof(Type)+(nb-1)*sizeof(Type*),DBG_TYPE,NULL,typeMark); if (!t) return NULL;
//	PRINTF(LOG_DEV,"alloc type " LSD " " LSD "\n",code,nb);
	t->code=code;
	t->def=def;
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
Type* typeAlloc(Thread* th, LINT code, Def* def,LINT nb,...)
{
	Type* t = typeAllocEmpty(th,code, def, nb); if (!t) return NULL;
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

Type* typeAllocFromStack(Thread* th, Def* def, LINT code, LINT nb)
{
	LINT i;
	Type* t = typeAllocEmpty(th, code, def, nb); if (!t) return NULL;
	for(i=nb-1;i>=0;i--)
	{
		LB* p=STACK_PULL_PNT(th);
		t->child[i]=(Type*)p;
		MEMORY_MARK((LB*)t, p);
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

	if (!parserNext(c)) return compileError(c,"uncomplete type reaches EOF\n");
	lb = localsGet(*labels, 0, c->parser->token);
//	PRINTF(LOG_DEV,"search %s -> %llx\n", c->parser->token,lb);
	if (lb)
	{
		if (!lb->type) return compileError(c,"illegal use of label '%s'\n",STR_START(lb->name));
		if (lb->type->nb == TYPENB_DERIVATIVE)
		{
			if ((!parserNext(c))|| (strcmp(c->parser->token, "{"))) return compileError(c,"missing '{' for derivative types, found '%s'\n", c->parser->token);
			if (!(t = _compilerParseType(c, mono, depth + 1, labels, withRec))) return NULL;
			if (t->code != TYPECODE_PRIMARY) return compileError(c,"only primary types may be derivated\n");
			if (t->nb!=lb->type->nb) return compileError(c,"derivative type has a wrong number of parameters (%d instead of %d)\n",t->nb,lb->type->nb);
			if (parserAssume(c, "}")) return NULL;
		}
		return lb->type;
	}
	if (!strcmp(c->parser->token, "_")) return typeAllocWeak(c->th);
	if (((c->parser->token[0] == 'w')|| (c->parser->token[0] == 'a')) && (isdecimal(c->parser->token + 1)))
	{
		Locals* label = localsCreate(c->th, c->parser->token, 0, NULL, *labels); if (!(labels)) return NULL;
		int weak = (c->parser->token[0] == 'w') ? 1 : 0;
		if (mono && !weak) return compileError(c,"polymorphism (%s) is not accepted here\n", c->parser->token);
		*labels = label;
		if ((parserNext(c)) && (!strcmp(c->parser->token, "{")))
		{
			
			if (!(t = _compilerParseType(c, mono, depth + 1, labels, withRec))) return NULL;
			if (t->code != TYPECODE_PRIMARY) return compileError(c,"only primary types may be derivated\n");
			if (parserAssume(c, "}")) return NULL;
			label->type = typeDerivate(c->th, t, weak);
			return label->type ;
		}
		else parserGiveback(c);
		label->type = weak?typeAllocWeak(c->th): typeAllocUndef(c->th);
		return label->type;
	}
	if ((c->parser->token[0] == 'r') && (isdecimal(c->parser->token + 1)))
	{
		LINT i = ls_atoi(c->parser->token + 1,1);
		if ((i < 0) || (i >= depth)) return compileError(c,"recursivity out of range %d [0 %d[\n", i, depth);
		*withRec = 1;
		return typeAllocRec(c->th,i);
	}
	if (!strcmp(c->parser->token, "array"))
	{
		Type* nt = typeAllocEmpty(c->th,TYPECODE_ARRAY, NULL, 1); if (!nt) return NULL;
		if (!depth) return compileError(c,"types starting with 'array' must be enclosed in parentheses\n");

		if (!(t = _compilerParseType(c, mono, depth + 1, labels, withRec))) return NULL;
		nt->child[0] = t;
		return nt;
	}
	if (!strcmp(c->parser->token, "list"))
	{
		Type* nt = typeAllocEmpty(c->th, TYPECODE_LIST, NULL, 1); if (!nt) return NULL;
		if (!depth) return compileError(c,"types starting with 'list' must be enclosed in parentheses\n");
		if (!(t = _compilerParseType(c, mono, depth + 1, labels, withRec))) return NULL;
		nt->child[0] = t;
		return nt;
	}
	if (!strcmp(c->parser->token, "fifo"))
	{
		Type* nt = typeAllocEmpty(c->th, TYPECODE_FIFO, NULL, 1); if (!nt) return NULL;
		if (!depth) return compileError(c,"types starting with 'fifo' must be enclosed in parentheses\n");

		if (!(t = _compilerParseType(c, mono, depth + 1, labels, withRec))) return NULL;
		nt->child[0] = t;
		return nt;
	}
	if (!strcmp(c->parser->token, "hashmap"))
	{
		Type* nt = typeAllocEmpty(c->th, TYPECODE_HASHMAP, NULL, 2); if (!nt) return NULL;
		if (!depth) return compileError(c,"types starting with 'hashmap' must be enclosed in parentheses\n");

		if (!(t = _compilerParseType(c, mono, depth + 1, labels, withRec))) return NULL;
		nt->child[0] = t;
		if (parserAssume(c, "->")) return NULL;
		if (!(t = _compilerParseType(c, mono, depth + 1, labels, withRec))) return NULL;
		nt->child[1] = t;
		return nt;
	}
	if (!strcmp(c->parser->token, "hashset"))
	{
		Type* nt = typeAllocEmpty(c->th, TYPECODE_HASHSET, NULL, 1); if (!nt) return NULL;
		if (!depth) return compileError(c,"types starting with 'hashset' must be enclosed in parentheses\n");

		if (!(t = _compilerParseType(c, mono, depth + 1, labels, withRec))) return NULL;
		nt->child[0] = t;
		return nt;
	}
	if (!strcmp(c->parser->token, "fun"))
	{
		if (!depth) return compileError(c,"types starting with 'fun' must be enclosed in parentheses\n");

		n = 0;
		while (1)
		{
			if (!parserNext(c)) return compileError(c,"uncomplete type reaches EOF\n");
			if (!strcmp(c->parser->token, "->"))
			{
				if (!(t = _compilerParseType(c, mono, depth + 1, labels, withRec))) return NULL;
				TYPE_PUSH_NULL(c, t);
				n++;
				return typeAllocFromStack(c->th, NULL, TYPECODE_FUN, n);
			}
			parserGiveback(c);
			if (!(t = _compilerParseType(c, mono, depth + 1, labels, withRec))) return NULL;
			TYPE_PUSH_NULL(c, t);
			n++;
		}
	}
	if (!strcmp(c->parser->token, "["))
	{
		n = 0;
		while (1)
		{
			if (!parserNext(c)) return compileError(c,"uncomplete type reaches EOF\n");
			if (!strcmp(c->parser->token, "]"))
			{
				return typeAllocFromStack(c->th, NULL, TYPECODE_TUPLE, n);
			}
			parserGiveback(c);
			if (!(t = _compilerParseType(c, mono, depth + 1, labels, withRec))) return NULL;
			TYPE_PUSH_NULL(c, t);
			n++;
		}
	}
	if (islabel(c->parser->token))
	{
		Def* def;
		Type* nt;
		LINT n = 0;
		LINT code= TYPECODE_PRIMARY;
		
		def = compileGetDef(c);
		if (!def) return compileError(c,"unknown label '%s'\n", compileToken(c));
		if ((def->code != DEF_CODE_TYPE) && (def->code != DEF_CODE_SUM) && (def->code != DEF_CODE_STRUCT)) return compileError(c,"'%s' is not a type\n", compileToken(c));

		if (parserNext(c))
		{
			if (!strcmp(c->parser->token, "{"))
			{
				while (1)
				{
					Type* t;
					if (!(t = _compilerParseType(c, mono, depth + 1, labels, withRec))) return NULL;
					TYPE_PUSH_NULL(c, t);
					n++;

					if (!parserNext(c)) return compileError(c,"type or '}' expected (found '%s')\n", compileToken(c));

					if (!strcmp(c->parser->token, "}")) break;
					parserGiveback(c);
				}
			}
			else parserGiveback(c);
		}
		nt = typeAllocFromStack(c->th, def, code, n); if (!nt) return NULL;

		t = def->type;
		if (t->nb!=nt->nb) return compileError(c,"wrong number of parameters for type '%s' %lld/%lld\n", defName(def),t->nb,nt->nb);
		if ((!t->nb)&& (code == TYPECODE_PRIMARY)) return t;
		return nt;
	}
	if (!strcmp(c->parser->token, "("))
	{
		if (!(t = _compilerParseType(c, mono, depth + 1, labels, withRec))) return NULL;
		if (parserNext(c) && !strcmp(c->parser->token, ")")) return t;
		return compileError(c,"unbalanced '('\n");
	}

	return compileError(c,"unknown token %s\n", c->parser->token);
}

Type* _compilerHandleRec(Compiler* c, Type* p)
{
	if (p->code == TYPECODE_REC)
	{
		LINT rec = (LINT)p->actual;
		p->actual = (Type*)STACK_PNT(c->th, rec);
	}
	else if (p->nb>0)
	{
		LINT i;
		TYPE_PUSH_NULL(c, p);
		for (i = 0; i < p->nb; i++) if (!_compilerHandleRec(c, p->child[i])) return NULL;
		STACK_DROP(c->th);
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
int _compilerSkipTypeDef(Compiler* c, int depth)
{
	char* token;
	if (!parserNext(c)) { compileError(c,"uncomplete type reaches EOF\n"); return -1; }
	token = c->parser->token;
	if ((!strcmp(token, "array")) ||
		(!strcmp(token, "list")) ||
		(!strcmp(token, "fifo")) ||
		(!strcmp(token, "hashset"))) {
		if (!depth) {
			compileError(c,"types starting with '%s' must be enclosed in parentheses\n", token); return -1;
		}
		return _compilerSkipTypeDef(c, depth + 1);
	}

	if (!strcmp(token, "hashmap"))
	{
		if (!depth) {
			compileError(c,"types starting with 'hashmap' must be enclosed in parentheses\n");
			return -1;
		}
		if (_compilerSkipTypeDef(c, depth+1)) return -1;
		if (!parserNext(c)) { compileError(c,"uncomplete type reaches EOF\n"); return -1; }
		if (strcmp(c->parser->token, "->")) { compileError(c,"-> expected, found '%s'\n",compileToken(c)); return -1; }
		return _compilerSkipTypeDef(c, depth+1);
	}
	if (!strcmp(token, "fun"))
	{
		if (!depth) {
			compileError(c,"types starting with 'fun' must be enclosed in parentheses\n");
			return -1;
		}
		while (1)
		{
			if (!parserNext(c)) { compileError(c,"uncomplete type reaches EOF\n"); return -1; }
			if (!strcmp(c->parser->token, "->")) return _compilerSkipTypeDef(c, depth + 1);
			parserGiveback(c);
			if (_compilerSkipTypeDef(c, depth + 1)) return -1;
		}
	}
	if (!strcmp(token, "["))
	{
		while (1)
		{
			if (!parserNext(c)) { compileError(c,"uncomplete type reaches EOF\n"); return -1; }
			if (!strcmp(c->parser->token, "]")) return 0;
			parserGiveback(c);
			if (_compilerSkipTypeDef(c, depth+1)) return -1;
		}
	}
	if (islabel(token))
	{
		if (!parserNext(c)) return 0;

		if (strcmp(c->parser->token, "{"))
		{
			parserGiveback(c);
			return 0;
		}
		while (1)
		{
			if (_compilerSkipTypeDef(c, depth+1)) return -1;
			if (!parserNext(c)) { compileError(c,"uncomplete type reaches EOF\n"); return -1; }
			if (!strcmp(c->parser->token, "}")) return 0;
			parserGiveback(c);
		}
	}
	if (!strcmp(token, "("))
	{
		if (_compilerSkipTypeDef(c, depth+1)) return -1;
		if (parserNext(c)&& !strcmp(c->parser->token, ")")) return 0;
		compileError(c,"unbalanced '('\n");
		return -1;
	}
	compileError(c,"unknown token %s\n", token);
	return -1;
}
int compilerSkipTypeDef(Compiler* c)
{
	return _compilerSkipTypeDef(c, 0);
}

int _typePrintRec(Thread* th,Buffer* tmp, TypeLabel **h,Type* p,int depth)
{
	LINT i;
	LB* type;
	TypeLabel* labels = *h;


	if (!p) return 0;
	while(p->actual) p=p->actual;
	type=(LB*)p;
	STACK_PUSH_PNT_ERR(MM.tmpStack,type,EXEC_OM);

	for(i=0;i<depth;i++) if (type==STACK_PNT(MM.tmpStack,i+1))
	{
		bufferPrintf(th, tmp,"r%d",i);
		STACK_DROP(MM.tmpStack);
		return 0;
	}
	if (p->code==TYPECODE_PRIMARY)
	{
		bufferAddStr(th, tmp, defName(p->def));
		if (p->nb)
		{
			bufferAddStr(th, tmp,"{");
			if (depth<100) for(i=0;i<p->nb;i++)
			{
				if (i) bufferAddStr(th, tmp, " ");
				_typePrintRec(th, tmp,h,p->child[i],depth+1);
			}
			else bufferAddStr(th, tmp, "...");
			bufferAddStr(th, tmp,"}");
		}
	}
	else if (p->code==TYPECODE_FUN)
	{
		if (depth) bufferAddStr(th, tmp, "(");
		bufferAddStr(th, tmp,"fun ");
		for(i=0;i<p->nb-1;i++)
		{
			_typePrintRec(th, tmp,h,p->child[i],depth+1);
			bufferAddStr(th, tmp, " ");
		}
		bufferAddStr(th, tmp,"-> ");
		_typePrintRec(th, tmp,h,p->child[p->nb-1],depth+1);
		if (depth) bufferAddStr(th, tmp, ")");
	}
	else if (p->code==TYPECODE_LIST)
	{
//		if (depth) bufferAddStr(th, tmp, "(");
		bufferAddStr(th, tmp,"list ");
		_typePrintRec(th, tmp,h,p->child[0],depth+1);
//		if (depth) bufferAddStr(th, tmp, ")");
	}
	else if (p->code==TYPECODE_HASHMAP)
	{
		if (depth) bufferAddStr(th, tmp, "(");
		bufferAddStr(th, tmp,"hashmap ");
		_typePrintRec(th, tmp,h,p->child[0],depth+1);
		bufferAddStr(th, tmp," -> ");
		_typePrintRec(th, tmp,h,p->child[1],depth+1);
		if (depth) bufferAddStr(th, tmp, ")");
	}
	else if (p->code==TYPECODE_HASHSET)
	{
		bufferAddStr(th, tmp,"hashset ");
		_typePrintRec(th, tmp,h,p->child[0],depth+1);
	}
	else if (p->code == TYPECODE_FIELD)
	{
		//		bufferAddStr(th, tmp,"field ");
		_typePrintRec(th, tmp, h, p->child[0], depth + 1);
		bufferAddStr(th, tmp, " -> ");
		_typePrintRec(th, tmp, h, p->child[1], depth + 1);
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
		if ((p->nb == TYPENB_DERIVATIVE) && (p->child[0]->def->dCI == 0))
		{
			_typePrintRec(th, tmp, h, p->child[0], depth + 1);
		}
		else
		{
			num = typeLabelGet(labels, type);
			if (!num)
			{
				*h = typeLabelAdd(th, type, p->code, labels); if (!(*h)) return EXEC_OM;
				num = (*h)->num;
			}
//			PRINTF(LOG_DEV,"\ntypeLabelGet %llx: %d -> %d\n", typew, p->code, num);
			bufferPrintf(th, tmp, "%c"LSD, (p->code == TYPECODE_UNDEF) ? 'a' : 'w', num);
			if (p->nb == TYPENB_DERIVATIVE)	// derivative struct
			{
				bufferAddStr(th, tmp, "{");
				_typePrintRec(th, tmp, h, p->child[0], depth + 1);
				bufferAddStr(th, tmp, "}");
			}
		}
	}
	STACK_DROP(MM.tmpStack);
	return 0;
}

int typeBuffer(Thread *th,Buffer* tmp, Type* type)
{
	int k;
	TypeLabel* h = NULL;
	stackReset(MM.tmpStack);
	memoryEnterFast();
	if ((k = _typePrintRec(th, tmp, &h, type, 0))) return k;
	memoryLeaveFast();
	return 0;
}
int typePrint(Thread* th, int mask, Type* type)
{
	int k;
	if (!termCheckMask(mask)) return 0;
	bufferReinit(MM.tmpBuffer);
	if ((k = typeBuffer(th, MM.tmpBuffer, type))) return k;
	termWrite(mask, bufferStart(MM.tmpBuffer), bufferSize(MM.tmpBuffer));
	return 0;
}


// initialize types required by the compiler
void typesInit(Thread* th, Pkg* system)
{
	Def* Exception;
	Def* Boolean;
	Type* u0, * list_u0, * array_u0;

	MM.Int = pkgAddType(th,system, "Int")->type;
	MM.Float = pkgAddType(th,system, "Float")->type;
	MM.Str = pkgAddType(th,system, "Str")->type;
	MM.Bytes = pkgAddType(th,system, "Bytes")->type;
	MM.BigNum = pkgAddType(th,system, "BigNum")->type;
	MM.Bitmap = pkgAddType(th,system, "Bitmap")->type;
	MM.Buffer = pkgAddType(th,system, "Buffer")->type;

	Boolean = pkgAddSum(th,system, "Bool");
	MM.Boolean = Boolean->type;
	MM._true = (LB*)pkgAddCons0(th,system, "true", Boolean);
	MM._false =(LB*)pkgAddCons0(th,system, "false", Boolean);

	MM.Package = pkgAddType(th,system, "Package")->type;
	MM.Thread = pkgAddType(th,system, "_Thread")->type;
	Exception = pkgAddSum(th,system, "Exception");
	MM.Exception = Exception->type;
	pkgAddCons0(th,system, "anyException", Exception);
	MM.MemoryException =VAL_FROM_PNT((LB*)pkgAddCons0(th,system, "memoryException", Exception));

	u0 = typeAllocUndef(th);
	list_u0 = typeAlloc(th,TYPECODE_LIST, NULL, 1, u0);
	array_u0 = typeAlloc(th,TYPECODE_ARRAY, NULL, 1, u0);

	MM.fun_u0_list_u0_list_u0 = typeAlloc(th, TYPECODE_FUN, NULL, 3, u0, list_u0, list_u0);
	MM.fun_array_u0_I_u0 = typeAlloc(th, TYPECODE_FUN, NULL, 3, array_u0, MM.Int, u0);
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
	t=typeAllocEmpty(th, copyCode,p->def,p->nb); if (!t) return NULL;	// copy everything else
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
		if ((p->nb != TYPENB_DERIVATIVE) || (p->child[0]->def->dCI > 0))
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

int typePrimaryIsChild(Def* child, Def* parent)
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
					if (typePrimaryIsChild(parentT->def, parentS->def)) return typeLink(s,t, parentS, parentT );
					if (typePrimaryIsChild(parentS->def, parentT->def)) return typeLink(t,s, parentT, parentS);
					return COMPILER_ERR_TYPE;
				}
				return typeLink(t, s,NULL,NULL);
			}
			if (nt == TYPECODE_PRIMARY)
			{
				if (typePrimaryIsChild(t->def, parentS->def)) return typeLink(s, t,parentS, t);
			}
			return COMPILER_ERR_TYPE;
		}
		return typeLink(s, t,NULL,NULL);
	}
	if (ns != nt) return COMPILER_ERR_TYPE;
	if (ns == TYPECODE_PRIMARY)
	{
		if (s->def != t->def) return COMPILER_ERR_TYPE;
	}
	if (s->nb != t->nb) return COMPILER_ERR_TYPE;

	return typeLink(t, s,t,s);
}

int typeUnify(Compiler* c,Type* x,Type* y)
{
	int err=0;
	if ((!x) || (!y)) return COMPILER_ERR_TYPE;
//	PRINTF(LOG_DEV,  "unify:"); typePrint(-1, x); PRINTF(LOG_DEV,"  with:"); typePrint(-1, y); PRINTF(LOG_DEV,"\n");
	if (!(err = typeRecUnify(x, y)))
	{
//		PRINTF(LOG_DEV,"---> "); typePrint(-1, x); PRINTF(LOG_DEV,"\n");
		return 0;
	}
	compileError(c,"'");
	typePrint(c->th, LOG_USER,x);
	PRINTF(LOG_USER,"' does not match with '");
	typePrint(c->th, LOG_USER,y);
	PRINTF(LOG_USER,"'\n");

	if (c->fmk)
	{
		Globals* globals = c->fmk->globals;
		Locals* lb = c->fmk->locals;
		if (lb) PRINTF(LOG_USER, "Provisional infered types:\n");
		while (lb)
		{
			if (lb->name)
			{
				PRINTF(LOG_USER, "   local %s: ",STR_START(lb->name));
				typePrint(c->th, LOG_USER, lb->type);
				PRINTF(LOG_USER, "\n");
			}
			lb = lb->next;
		}
		while (globals)
		{
			if (globals->data && HEADER_DBG(globals->data)== DBG_DEF)
			{
				Def* def = (Def*)globals->data;
				if (def->name)
				{
					PRINTF(LOG_USER, "   global %s: ", STR_START(def->name));
					typePrint(c->th, LOG_USER, def->type);
					PRINTF(LOG_USER, "\n");
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
	while(i>=0) if (typeUnify(c,(Type*)STACK_PULL_PNT(c->th),fun->child[i--])) return NULL;	// check and pull args
	return fun->child[fun->nb-1];	// return unified function result
}
