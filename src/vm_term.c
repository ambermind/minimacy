// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

Term MainTerm;

void termInit(void)
{
	MainTerm.mask = LOG_ALL;
	MainTerm.showBiosListing = 0;
#ifdef HIDE_COMPILER_LISTING
	MainTerm.showPkgListing = 0;
#else
	MainTerm.showPkgListing = 1;
#endif
}
void termEnd(void)
{
	// called once before terminating
}
void termSetMask(int i)
{
	MainTerm.mask=i;
}
int termGetMask(void)
{
	return MainTerm.mask;
}
int termCheckMask(int mask)
{
	return mask & MainTerm.mask;
}
void termShowBiosListing(int val) { MainTerm.showBiosListing = val; }
void termShowPkgListing(int val) { MainTerm.showPkgListing = val; }
int termUserOutput(int mask)
{
	return (mask == LOG_USER) ? 1 : 0;
}
void termWrite(int mask,char* src, LINT len)
{
	if (!(mask & MainTerm.mask)) return;
	consoleWrite(termUserOutput(mask),src, (int)len);
}
void termPrintfv(int mask,char *format,va_list arglist)
{
	if (!(mask&MainTerm.mask)) return;
	consoleVPrint(termUserOutput(mask), format, arglist);
}

void termPrintf(int mask,char *format, ...)
{
	va_list arglist;
	va_start(arglist,format);
	termPrintfv(mask,format,arglist);
	va_end(arglist);
}

char* MMTAB="                                ";
void _itemDump(int mask, int skipHeader, LW v, int type, int rec, char* tab);
void _globalsPrint(int mask, LB* g, int rec,char* tab)
{
	LINT j;
	if (!g) return;
	PRINTF(mask, "%s links:\n", tab);
	for (j = 0; j < ARRAY_LENGTH(g); j++)
	{
		LB* q = ARRAY_PNT(g, j);
		PRINTF(mask, "  %s " LSD ":\n", tab, j);
		if (ARRAY_IS_PNT(g,j) && (HEADER_DBG(q) == DBG_DEF))
		{
			Def* def = (Def*)q;
			PRINTF(mask, "  %s %s.%s: ", tab - 2, defPkgName(def), defName(def));
			typePrint(mask, def->type);
			PRINTF(mask, "\n");
		}
		else if (ARRAY_IS_PNT(g,j) && (HEADER_DBG(q) == DBG_FUN))
		{
			PRINTF(mask, "%s lambda:\n", tab-4);
			_itemDump(mask, 0, ARRAY_GET(q, FUN_USER_BC), ARRAY_TYPE(q, FUN_USER_BC), rec - 1, tab - 6);
			_globalsPrint(mask, ARRAY_PNT(q, FUN_USER_GLOBALS), rec - 1, tab - 6);
		}
		else _itemDump(mask, 0, ARRAY_GET(g, j), ARRAY_TYPE(g, j), rec > 1 ? 2 : rec - 1, tab - 4);
	}

}
void _hexDump(int mask, char* src, LINT len, LINT offsetDisplay)
{
	LINT line;
	LINT tmp = len + offsetDisplay;
	char* format = "%04x ";
	if (tmp > 0x1000000) format = "%08x ";
	else if (tmp>0x10000) format = "%06x ";

	for (line = 0; line < len; line += 16)
	{
		LINT i;
		PRINTF(mask, format, line);
		for (i = line; i < line + 16; i++) if (i < len) PRINTF(mask, "%02x ", 255 & (src[i]));
		else PRINTF(mask, "   ");
		for (i = line; i < line + 16; i++) if (i < len) PRINTF(mask, "%c", src[i] < 32 ? '.' : src[i]);
		PRINTF(mask, "\n");
	}
}
void _itemDump(int mask, int skipHeader, LW v,int type,int rec,char* tab)
{
	LW dbg;
	LB* p;
	if (rec<0)
	{
		PRINTF(mask,"%s ...\n",tab);
		return;
	}
	if (!tab) tab=MMTAB+strlen(MMTAB);
	else if (strlen(tab)>(strlen(MMTAB)-4)) tab+=4;

	if (type==VAL_TYPE_FLOAT)
	{
		if (!skipHeader) PRINTF(mask, "%s Float: ", tab);
		PRINTF(mask, "%g\n", FLOAT_FROM_VAL(v));
		return;
	}
	if (type == VAL_TYPE_INT)
	{
		if (!skipHeader) PRINTF(mask, "%s Int: ", tab);
		PRINTF(mask, LSD " (0x%x)\n", INT_FROM_VAL(v), (int)INT_FROM_VAL(v));
		return;
	}

	p = PNT_FROM_VAL(v);
	if (!p) {
		PRINTF(mask, "%s nil\n", tab);
		return;
	}
	if (p == MM._false)
	{
		if (!skipHeader) PRINTF(mask, "%s Bool: ", tab);
		PRINTF(mask, "false\n");
		return;
	}
	if (p == MM._true)
	{
		if (!skipHeader) PRINTF(mask, "%s Bool: ", tab);
		PRINTF(mask, "true\n");
		return;
	}
	dbg=HEADER_DBG(p);
	if (DBG_IS_PNT(dbg))
	{
		Def* defType=(Def*)PNT_FROM_VAL(dbg);
		// we do not handle DEF_CODE_SUM because there is no way to get such a value here
		if (defType->code==DEF_CODE_STRUCT)
		{
			Def* def = defType;
			if (!skipHeader) PRINTF(mask,"%s struct %s\n",tab, STR_START(defType->name));
			else PRINTF(mask, "\n");
			while (def)
			{
				Def* next = (Def*)PNT_FROM_VAL(def->val);
				while (next)
				{
//						PRINTF(mask, "  %s %s %lld/%lld:\n", tab, STR_START(next->name),next->index,ARRAY_LENGTH(p));
					PRINTF(mask, "  %s %s:\n", tab, STR_START(next->name));
					_itemDump(mask, 0, ARRAY_GET(p, next->index), ARRAY_TYPE(p, next->index), rec - 1, tab - 4);
					next = (Def*)PNT_FROM_VAL(next->val);;
				}
				def = def->parent;
			}
		}
		else if ((defType->code==DEF_CODE_CONS)|| (defType->code == DEF_CODE_CONS0))
		{
			LINT j;
			Def* main=defType->parent;

			if (defType->code == DEF_CODE_CONS)
			{
				if (!skipHeader) PRINTF(mask, "%s sum %s ", tab, STR_START(main->name));
				PRINTF(mask, "constructor %s\n", STR_START(defType->name));
				for (j = 1; j < ARRAY_LENGTH(p); j++) {
					PRINTF(mask, "  %s " LSD " :\n", tab, j - 1);
					_itemDump(mask, 0, ARRAY_GET(p, j), ARRAY_TYPE(p, j), rec - 1, tab - 4);
				}
			}
			else
			{
				if (!skipHeader) PRINTF(mask, "%s sum %s ", tab, STR_START(main->name));
				PRINTF(mask, "const %s\n", STR_START(defType->name));
			}
		}
	}
	else if (dbg==DBG_HASHMAP)
	{
		HashSlots* h=(HashSlots*)p;
		LINT index;
		LB* table=h->table;
		LINT first=1;
		LINT end = h->nb - TERM_LIMIT_ENUM;
		LINT i = 0;
		if (!skipHeader) PRINTF(mask, "%s hashmap ", tab);
		PRINTF(mask, "#" LSD " {\n", h->nb);
		for(index=0;index<ARRAY_LENGTH(table);index++)
		{
			LB* next=(ARRAY_PNT(table,index));
			while(next)
			{
				if (i< TERM_LIMIT_ENUM || i>end)
				{
					if (!first) PRINTF(mask, "  %s ---\n", tab);
					first = 0;
					_itemDump(mask, 0, ARRAY_GET(next, HASH_LIST_KEY), ARRAY_TYPE(next, HASH_LIST_KEY), rec - 1, tab - 4);
					PRINTF(mask, "  %s ->\n", tab);
					_itemDump(mask, 0, ARRAY_GET(next, HASH_LIST_VAL), ARRAY_TYPE(next, HASH_LIST_VAL), rec - 1, tab - 4);
				}
				else if (i== TERM_LIMIT_ENUM) PRINTF(mask, "  %s ...skipping %d elements...\n", tab, end - TERM_LIMIT_ENUM);
				i++;
				next=(ARRAY_PNT(next,HASH_LIST_NEXT));
			}
		}
		PRINTF(mask,"%s}\n",tab);
	}
	else if (dbg == DBG_HASHSET)
	{
		HashSlots* h = (HashSlots*)p;
		LINT index;
		LB* table = h->table;
		LINT first = 1;
		LINT end = h->nb - TERM_LIMIT_ENUM;
		LINT i = 0;
		if (!skipHeader) PRINTF(mask, "%s hashset ", tab);
		PRINTF(mask, "#" LSD " {\n", h->nb);
		for (index = 0; index < ARRAY_LENGTH(table); index++)
		{
			LB* next = (ARRAY_PNT(table, index));
			while (next)
			{
				if (i< TERM_LIMIT_ENUM || i>end)
				{
					if (!first) PRINTF(mask, "  %s ---\n", tab);
					first = 0;
					_itemDump(mask, 0, ARRAY_GET(next, LIST_VAL), ARRAY_TYPE(next, LIST_VAL), rec - 1, tab - 4);
				}
				else if (i == TERM_LIMIT_ENUM) PRINTF(mask, "  %s ...skipping %d elements...\n", tab, end - TERM_LIMIT_ENUM);
				i++;
				next = (ARRAY_PNT(next, LIST_NXT));
			}
		}
		PRINTF(mask, "%s}\n", tab);
	}
	else if (dbg==DBG_BYTECODE)
	{
		if (!skipHeader) PRINTF(mask,"%s bytecode:\n",tab);
		bytecodePrint(mask,p);
	}
	else if (dbg==DBG_S)
	{
		if (sourceFromStr(MM.tmpBuffer, STR_START(p), STR_LENGTH(p))) return;
		if (!skipHeader) PRINTF(mask,"%s Str: ",tab);
		PRINTF(mask,"%s\n",bufferStart(MM.tmpBuffer));
	}
	else if (dbg==DBG_B)
	{
		int i;
		bignum a=(bignum)p;
		if (!skipHeader) PRINTF(mask,"%s BigNum: (%d)",tab,bignumLen(a));
		if (bignumSign(a)) PRINTF(mask,"-");
		for(i=bignumLen(a)-1;i>=0;i--) PRINTF(mask,"%08x ",bignumGet(a,i));
		PRINTF(mask,"\n");
	}
	else if (dbg==DBG_TYPE)
	{
		Type* t=(Type*)p;
		if (!skipHeader) PRINTF(mask,"%s Type: ",tab);
		typePrint(mask,t);
		PRINTF(mask,"\n");
	}
	else if (dbg==DBG_DEF)	// to be completed
	{
		Def* d=(Def*)p;
		char* name=(d->name==NULL)?"?":STR_START(d->name);
		if (!skipHeader) PRINTF(mask,"%s Definition ",tab);
		PRINTF(mask,"%s %s\n",defCodeName(d->code), name);
//		if (d->parser&& d->parser->name) PRINTF(mask, "%s File: %s at "LSD"\n", tab-4,STR_START(d->parser->name), d->parserIndex);
//			PRINTF(mask,"%s %s (#" LSD " /" LSD ")\n",defCodeName(d->code), name, d->code,d->index);
		_itemDump(mask, 0,VAL_FROM_PNT((LB*)d->type),VAL_TYPE_PNT, rec-1,tab-4);

		if ((d->code!=DEF_CODE_CONS0)&& (d->code != DEF_CODE_CONS) && (d->code != DEF_CODE_SUM) && 
			(d->code != DEF_CODE_STRUCT) && (d->code != DEF_CODE_FIELD) && (rec>0))
				_itemDump(mask, 0,d->val, d->valType, rec-1,tab-4);
	}
	else if (dbg == DBG_LAMBDA)
	{
		LINT j;
		LB* f = (ARRAY_PNT(p,0));
		LB* fName = (ARRAY_PNT(f, FUN_USER_NAME));
		Pkg* pkg = f->pkg;
		PRINTF(mask, "%s lambda function defined in function: %s.%s\n", tab, pkgName(pkg),STR_START(fName));
		_itemDump(mask, 0, ARRAY_GET(f, FUN_USER_BC), ARRAY_TYPE(f, FUN_USER_BC), rec - 1, tab - 2);
		_globalsPrint(mask,ARRAY_PNT(f, FUN_USER_GLOBALS),rec-1,tab-2);
		if (ARRAY_LENGTH(p) > 1)
		{
			PRINTF(mask, "%s curried locals:\n", tab-2);
			for (j = 1; j < ARRAY_LENGTH(p); j++)
			{
				PRINTF(mask, "  %s " LSD ":\n", tab, j-1);
				_itemDump(mask, 0, ARRAY_GET(p, j), ARRAY_TYPE(p, j), rec - 1, tab - 4);
			}
		}
	}
	else if (dbg == DBG_FUN)
	{
		char* name = (ARRAY_PNT(p, FUN_USER_NAME)) ? STR_START((ARRAY_PNT(p, FUN_USER_NAME))) : "[NO NAME]";
		if (!skipHeader) PRINTF(mask, "%s fun ", tab);
		PRINTF(mask, "%s\n", name);
		if (rec > 1)
		{
			_itemDump(mask, 0, ARRAY_GET(p, FUN_USER_BC), ARRAY_TYPE(p, FUN_USER_BC), rec - 1, tab - 2);
			_globalsPrint(mask, (ARRAY_PNT(p, FUN_USER_GLOBALS)), rec-1, tab - 2);
		}
	}
	else if (dbg==DBG_PKG)
	{
		Pkg* pkg=(Pkg*)p;
		if (!skipHeader) PRINTF(mask, "%s Pkg ", tab);
		PRINTF(mask, "%s\n", pkgName(pkg));
		pkgDisplay(mask,pkg);
	}
	else if (dbg== DBG_SOCKET)
	{
		if (!skipHeader) PRINTF(mask, "%s ", tab);
		PRINTF(mask, "Socket "LSX"\n",(LINT)p);
	}
	else if (dbg==DBG_LIST)
	{
		LINT end = - TERM_LIMIT_ENUM;
		LINT i = 0;
		LB* q = p;
		while (q)
		{
			end++;
			q = (ARRAY_PNT(q, LIST_NXT));
		}
		if (!skipHeader) PRINTF(mask,"%s list:\n",tab);
		else PRINTF(mask, "\n");
		while(p)
		{
			if (i< TERM_LIMIT_ENUM || i>end)
			{
//					PRINTF(mask,"%s   ->\n",tab);
				_itemDump(mask, 0,ARRAY_GET(p,LIST_VAL), ARRAY_TYPE(p,LIST_VAL),rec-1,tab-2);
			}
			else if (i == TERM_LIMIT_ENUM) PRINTF(mask, "  %s ...skipping %d elements...\n", tab, end - TERM_LIMIT_ENUM);
			i++;
			p = (ARRAY_PNT(p, LIST_NXT));
		}
	}
	else if (dbg == DBG_FIFO)
	{
		LINT end = (ARRAY_INT(p, FIFO_COUNT)) - TERM_LIMIT_ENUM;
		LINT i = 0;

		if (!skipHeader) PRINTF(mask, "%s fifo ", tab);
		PRINTF(mask, "#" LSD "\n", (ARRAY_INT(p,FIFO_COUNT)));
		p = (ARRAY_PNT(p, FIFO_START));
		while (p)
		{
			if (i< TERM_LIMIT_ENUM || i>end)
			{
//					PRINTF(mask, "%s   ->\n", tab);
				_itemDump(mask, 0, ARRAY_GET(p, LIST_VAL), ARRAY_TYPE(p,LIST_VAL), rec - 1, tab - 2);
			}
			else if (i == TERM_LIMIT_ENUM) PRINTF(mask, "  %s ...skipping %d elements...\n", tab, end - TERM_LIMIT_ENUM);
			i++;
			p = (ARRAY_PNT(p, LIST_NXT));
		}
	}
	else if (dbg==DBG_THREAD)
	{
		Thread* th=(Thread*)p;
		PRINTF(mask,"%s thread " LSD " callstack=" LSD " pc=" LSD " sp=" LSD "\n",tab,th->uid,th->callstack,th->pc,th->sp);
/*			for(i=0;i<=th->sp;i++)
		{
			PRINTF(mask,"%s-- " LSD ":\n",tab,th->sp-i);
			_itemDump(mask, 0,ARRAY_GET(th->stack,i),rec-1,tab-4);
		}
*/
        //			_itemDump(mask, 0,VAL_FROM_PNT((LB*)th->next),rec,tab);
	}
	else if (HEADER_TYPE(p)==TYPE_ARRAY)
	{
		LINT j,t;
		char* name=(dbg==DBG_ARRAY)?"array":"tuple";
		LINT end = ARRAY_LENGTH(p) - TERM_LIMIT_ENUM;

		t=ARRAY_LENGTH(p);
		if (!skipHeader) PRINTF(mask, "%s  %s ", tab,name);
		PRINTF(mask,"#" LSD " %c\n",ARRAY_LENGTH(p), (dbg == DBG_ARRAY) ? '{':'[');
		if (rec) for(j=0;j<t;j++) {
			if (j< TERM_LIMIT_ENUM || j>end)
			{
				PRINTF(mask, "  %s " LSD ":\n", tab, j);
				_itemDump(mask, 0, ARRAY_GET(p, j), ARRAY_TYPE(p, j), rec - 1, tab - 4);
			}
			else if (j == TERM_LIMIT_ENUM) PRINTF(mask, "  %s ...skipping %d elements...\n", tab, end - TERM_LIMIT_ENUM);
		}
		else PRINTF(mask," ...\n");
		PRINTF(mask,"%s%c\n",tab, (dbg == DBG_ARRAY) ? '}' : ']');
	}
	else
	{
		LINT len=BIN_LENGTH(p);
		LINT lenDump = len;
		char* src=(char*)BIN_START(p);

		if (!skipHeader) PRINTF(mask,"%s ",tab);
		PRINTF(mask,"binary #" LSD " (0x" LSX ")\n",len,len);
		if (lenDump >= TERM_LIMIT_BIN) lenDump = TERM_LIMIT_BIN;
		_hexDump(mask, src, lenDump,0);
		if (lenDump<len) PRINTF(mask, "[hiding "LSD" (0x"LSX") bytes]\n", len-lenDump, len - lenDump);
	}
}

void itemDump(int mask, LW v, int type)
{
	if (!(mask & MainTerm.mask)) return;
	_itemDump(mask, 0, v, type, 5, NULL);
}
void itemDumpDirect(int mask, LW v, int type)
{
	if (!(mask & MainTerm.mask)) return;
	_itemDump(mask, 1, v, type, 5, NULL);
}

void threadDump(int mask,Thread* th,LINT n)
{
	LINT i;
	if (n>th->sp+1) n=th->sp+1;
	PRINTF(mask,"\nThread " LSD "\n",th->uid);
	PRINTF(mask, "\nStack: " LSD "/" LSD "\n", n, th->sp);
	for(i=n-1;i>=0;i--)
	{
		PRINTF(mask,">%3d: ",i);
		itemDump(mask,STACK_GET(th,i),STACK_TYPE(th,i));
	}
}

void itemEcho(int mask, LW v, int type, int ln)
{
	Buffer* b = MM.tmpBuffer;
	if (!(mask & MainTerm.mask)) return;
	bufferReinit(b);
	if (bufferItem(b, v, type, NULL)) return;
	if (ln && bufferAddChar(b, 10)) return;
	termWrite(mask, bufferStart(b), bufferSize(b));
}

void itemHeader(int mask,LB* p)
{
	PRINTF(mask, "header: " LSX " " LSX " " LSX "(%s) at " LSX "\n", p->sizeAndType, p->nextBlock, p->listMark, (p->listMark==MM.USELESS)?"useless":"useful", p);
}

void termEchoSourceLine(Pkg* pkg,int mask,char* srcName, char* src, int index)
{
	int line = 1;
	int lineStart = 0;
	int i;
	int len = (int)strlen(src);
	if (index > len) index = len;

	for (i = 0; i < index; i++) if (src[i] == 10) {
		lineStart = i + 1;
		line++;
	}
	if (!pkg->forPrompt)
	{
		PRINTF(mask, "\n> pkg %s:", srcName);
		PRINTF(mask, "\n> line %d:", line);
	}
	PRINTF(mask, "\n> ");
	for (i = lineStart; i < len; i++)
		if (src[i] == 10) break;
		else PRINTF(mask, "%c", src[i]);
	PRINTF(mask, "\n> ");
	for (i = lineStart; i < index; i++) PRINTF(mask, "%c", (src[i] == 9) ? 9 : 32);
	PRINTF(mask, "^\n\n");
}
