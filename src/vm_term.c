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

Term MainTerm;

void termInit(void)
{
	MainTerm.mask = LOG_ALL;
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
void _itemDump(Thread* th, int mask, int skipHeader, LW v, int type, int rec, char* tab);
void _globalsPrint(Thread* th, int mask, LB* g, int rec,char* tab)
{
	LINT j;
	if (!g) return;
	PRINTF(mask, "%s links:\n", tab);
	for (j = 0; j < TABLEN(g); j++)
	{
		LB* q = TABPNT(g, j);
		PRINTF(mask, "  %s " LSD ":\n", tab, j);
		if (TABISPNT(g,j) && (HEADER_DBG(q) == DBG_DEF))
		{
			Def* def = (Def*)q;
			PRINTF(mask, "  %s %s.%s: ", tab - 2, defPkgName(def), defName(def));
			typePrint(th, mask, def->type);
			PRINTF(mask, "\n");
		}
		else if (TABISPNT(g,j) && (HEADER_DBG(q) == DBG_FUN))
		{
			PRINTF(mask, "%s lambda:\n", tab-4);
			_itemDump(th, mask, 0, TABGET(q, FUN_USER_BC), TABTYPE(q, FUN_USER_BC), rec - 1, tab - 6);
			_globalsPrint(th, mask, TABPNT(q, FUN_USER_GLOBALS), rec - 1, tab - 6);
		}
		else _itemDump(th, mask, 0, TABGET(g, j), TABTYPE(g, j), rec > 1 ? 2 : rec - 1, tab - 4);
	}

}
void _hexDump(Thread* th,int mask, char* src, LINT len, LINT offsetDisplay)
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
void _itemDump(Thread* th, int mask, int skipHeader, LW v,int type,int rec,char* tab)
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
		PRINTF(mask, "%g\n", VALTOFLOAT(v));
		return;
	}
	if (type == VAL_TYPE_INT)
	{
		if (!skipHeader) PRINTF(mask, "%s Int: ", tab);
		PRINTF(mask, LSD " (0x" LSX ")\n", VALTOINT(v), VALTOINT(v));
		return;
	}

	p = VALTOPNT(v);
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
	if (DBGISPNT(dbg))
	{
		Def* defType=(Def*)VALTOPNT(dbg);
		// we do not handle DEF_CODE_SUM because there is no way to get such a value here
		if (defType->code==DEF_CODE_STRUCT)
		{
			Def* def = defType;
			if (!skipHeader) PRINTF(mask,"%s struct %s\n",tab, STRSTART(defType->name));
			else PRINTF(mask, "\n");
			while (def)
			{
				Def* next = (Def*)VALTOPNT(def->val);
				while (next)
				{
//						PRINTF(mask, "  %s %s %lld/%lld:\n", tab, STRSTART(next->name),next->index,TABLEN(p));
					PRINTF(mask, "  %s %s:\n", tab, STRSTART(next->name));
					_itemDump(th, mask, 0, TABGET(p, next->index), TABTYPE(p, next->index), rec - 1, tab - 4);
					next = (Def*)VALTOPNT(next->val);;
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
				if (!skipHeader) PRINTF(mask, "%s sum %s ", tab, STRSTART(main->name));
				PRINTF(mask, "constructor %s\n", STRSTART(defType->name));
				for (j = 1; j < TABLEN(p); j++) {
					PRINTF(mask, "  %s " LSD " :\n", tab, j - 1);
					_itemDump(th, mask, 0, TABGET(p, j), TABTYPE(p, j), rec - 1, tab - 4);
				}
			}
			else
			{
				if (!skipHeader) PRINTF(mask, "%s sum %s ", tab, STRSTART(main->name));
				PRINTF(mask, "const %s\n", STRSTART(defType->name));
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
		for(index=0;index<TABLEN(table);index++)
		{
			LB* next=(TABPNT(table,index));
			while(next)
			{
				if (i< TERM_LIMIT_ENUM || i>end)
				{
					if (!first) PRINTF(mask, "  %s ---\n", tab);
					first = 0;
					_itemDump(th, mask, 0, TABGET(next, HASH_LIST_KEY), TABTYPE(next, HASH_LIST_KEY), rec - 1, tab - 4);
					PRINTF(mask, "  %s ->\n", tab);
					_itemDump(th, mask, 0, TABGET(next, HASH_LIST_VAL), TABTYPE(next, HASH_LIST_VAL), rec - 1, tab - 4);
				}
				else if (i== TERM_LIMIT_ENUM) PRINTF(mask, "  %s ...skipping %d elements...\n", tab, end - TERM_LIMIT_ENUM);
				i++;
				next=(TABPNT(next,HASH_LIST_NEXT));
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
		for (index = 0; index < TABLEN(table); index++)
		{
			LB* next = (TABPNT(table, index));
			while (next)
			{
				if (i< TERM_LIMIT_ENUM || i>end)
				{
					if (!first) PRINTF(mask, "  %s ---\n", tab);
					first = 0;
					_itemDump(th, mask, 0, TABGET(next, LIST_VAL), TABTYPE(next, LIST_VAL), rec - 1, tab - 4);
				}
				else if (i == TERM_LIMIT_ENUM) PRINTF(mask, "  %s ...skipping %d elements...\n", tab, end - TERM_LIMIT_ENUM);
				i++;
				next = (TABPNT(next, LIST_NXT));
			}
		}
		PRINTF(mask, "%s}\n", tab);
	}
	else if (dbg==DBG_BYTECODE)
	{
		if (!skipHeader) PRINTF(mask,"%s bytecode:\n",tab);
		bytecodePrint(th, mask,p);
	}
	else if (dbg==DBG_S)
	{
		if (sourceFromStr(th, MM.tmpBuffer, STRSTART(p), STRLEN(p))) return;
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
		typePrint(th, mask,t);
		PRINTF(mask,"\n");
	}
	else if (dbg==DBG_DEF)	// to be completed
	{
		Def* d=(Def*)p;
		char* name=(d->name==NULL)?"?":STRSTART(d->name);
		if (!skipHeader) PRINTF(mask,"%s Definition ",tab);
		PRINTF(mask,"%s %s\n",defCodeName(d->code), name);
//		if (d->parser&& d->parser->name) PRINTF(mask, "%s File: %s at "LSD"\n", tab-4,STRSTART(d->parser->name), d->parserIndex);
//			PRINTF(mask,"%s %s (#" LSD " /" LSD ")\n",defCodeName(d->code), name, d->code,d->index);
		_itemDump(th, mask, 0,PNTTOVAL((LB*)d->type),VAL_TYPE_PNT, rec-1,tab-4);

		if ((d->code!=DEF_CODE_CONS0)&& (d->code != DEF_CODE_CONS) && (d->code != DEF_CODE_SUM) && 
			(d->code != DEF_CODE_STRUCT) && (d->code != DEF_CODE_FIELD) && (rec>0))
				_itemDump(th, mask, 0,d->val, d->valType, rec-1,tab-4);
	}
	else if (dbg == DBG_LAMBDA)
	{
		LINT j;
		LB* f = (TABPNT(p,0));
		LB* fName = (TABPNT(f, FUN_USER_NAME));
		Pkg* pkg = (Pkg*)(TABPNT(f, FUN_USER_PKG));
		PRINTF(mask, "%s lambda function defined in function: %s.%s\n", tab, pkgName(pkg),STRSTART(fName));
		_itemDump(th, mask, 0, TABGET(f, FUN_USER_BC), TABTYPE(f, FUN_USER_BC), rec - 1, tab - 2);
		_globalsPrint(th, mask,TABPNT(f, FUN_USER_GLOBALS),rec-1,tab-2);
		if (TABLEN(p) > 1)
		{
			PRINTF(mask, "%s curried locals:\n", tab-2);
			for (j = 1; j < TABLEN(p); j++)
			{
				PRINTF(mask, "  %s " LSD ":\n", tab, j-1);
				_itemDump(th, mask, 0, TABGET(p, j), TABTYPE(p, j), rec - 1, tab - 4);
			}
		}
	}
	else if (dbg == DBG_FUN)
	{
		char* name = (TABPNT(p, FUN_USER_NAME)) ? STRSTART((TABPNT(p, FUN_USER_NAME))) : "[NO NAME]";
		if (!skipHeader) PRINTF(mask, "%s fun ", tab);
		PRINTF(mask, "%s\n", name);
		if (rec > 1)
		{
			_itemDump(th, mask, 0, TABGET(p, FUN_USER_BC), TABTYPE(p, FUN_USER_BC), rec - 1, tab - 2);
			_globalsPrint(th, mask, (TABPNT(p, FUN_USER_GLOBALS)), rec-1, tab - 2);
		}
	}
	else if (dbg==DBG_PKG)
	{
		Pkg* pkg=(Pkg*)p;
		if (!skipHeader) PRINTF(mask, "%s Pkg ", tab);
		PRINTF(mask, "%s\n", pkgName(pkg));
		pkgDisplay(th, mask,pkg);
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
			q = (TABPNT(q, LIST_NXT));
		}
		if (!skipHeader) PRINTF(mask,"%s list:\n",tab);
		else PRINTF(mask, "\n");
		while(p)
		{
			if (i< TERM_LIMIT_ENUM || i>end)
			{
//					PRINTF(mask,"%s   ->\n",tab);
				_itemDump(th, mask, 0,TABGET(p,LIST_VAL), TABTYPE(p,LIST_VAL),rec-1,tab-2);
			}
			else if (i == TERM_LIMIT_ENUM) PRINTF(mask, "  %s ...skipping %d elements...\n", tab, end - TERM_LIMIT_ENUM);
			i++;
			p = (TABPNT(p, LIST_NXT));
		}
	}
	else if (dbg == DBG_FIFO)
	{
		LINT end = (TABINT(p, FIFO_COUNT)) - TERM_LIMIT_ENUM;
		LINT i = 0;

		if (!skipHeader) PRINTF(mask, "%s fifo ", tab);
		PRINTF(mask, "#" LSD "\n", (TABINT(p,FIFO_COUNT)));
		p = (TABPNT(p, FIFO_START));
		while (p)
		{
			if (i< TERM_LIMIT_ENUM || i>end)
			{
//					PRINTF(mask, "%s   ->\n", tab);
				_itemDump(th, mask, 0, TABGET(p, LIST_VAL), TABTYPE(p,LIST_VAL), rec - 1, tab - 2);
			}
			else if (i == TERM_LIMIT_ENUM) PRINTF(mask, "  %s ...skipping %d elements...\n", tab, end - TERM_LIMIT_ENUM);
			i++;
			p = (TABPNT(p, LIST_NXT));
		}
	}
	else if (dbg==DBG_THREAD)
	{
		Thread* th=(Thread*)p;
		PRINTF(mask,"%s thread " LSD " callstack=" LSD " pc=" LSD " pp=" LSD "\n",tab,th->uid,th->callstack,th->pc,th->pp);
/*			for(i=0;i<=th->pp;i++)
		{
			PRINTF(mask,"%s-- " LSD ":\n",tab,th->pp-i);
			_itemDump(th, mask, 0,TABGET(th->stack,i),rec-1,tab-4);
		}
*/
        //			_itemDump(th, mask, 0,PNTTOVAL((LB*)th->next),rec,tab);
	}
	else if (dbg == DBG_MEMCOUNT)
	{
		Mem* m = (Mem*)p;
		PRINTF(mask, "%s MEM " LSD "/" LSD" %s (%s)\n", tab, m->bytes, m->maxBytes, m->name?STRSTART(m->name):"no name", m->header.mem?"child":"root");
	}
	else if (HEADER_TYPE(p)==TYPE_TAB)
	{
		LINT j,t;
		char* name=(dbg==DBG_ARRAY)?"array":"tuple";
		LINT end = TABLEN(p) - TERM_LIMIT_ENUM;

		t=TABLEN(p);
		if (!skipHeader) PRINTF(mask, "%s  %s ", tab,name);
		PRINTF(mask,"#" LSD " %c\n",TABLEN(p), (dbg == DBG_ARRAY) ? '{':'[');
		if (rec) for(j=0;j<t;j++) {
			if (j< TERM_LIMIT_ENUM || j>end)
			{
				PRINTF(mask, "  %s " LSD ":\n", tab, j);
				_itemDump(th, mask, 0, TABGET(p, j), TABTYPE(p, j), rec - 1, tab - 4);
			}
			else if (j == TERM_LIMIT_ENUM) PRINTF(mask, "  %s ...skipping %d elements...\n", tab, end - TERM_LIMIT_ENUM);
		}
		else PRINTF(mask," ...\n");
		PRINTF(mask,"%s%c\n",tab, (dbg == DBG_ARRAY) ? '}' : ']');
	}
	else
	{
		LINT len=BINLEN(p);
		LINT lenDump = len;
		char* src=(char*)BINSTART(p);

		if (!skipHeader) PRINTF(mask,"%s ",tab);
		PRINTF(mask,"binary #" LSD " (0x" LSX ")\n",len,len);
		if (lenDump >= TERM_LIMIT_BIN) lenDump = TERM_LIMIT_BIN;
		_hexDump(th, mask, src, lenDump,0);
		if (lenDump<len) PRINTF(mask, "[hiding "LSD" (0x"LSX") bytes]\n", len-lenDump, len - lenDump);
	}
}

void itemDump(Thread* th, int mask, LW v, int type)
{
	if (!(mask & MainTerm.mask)) return;
	_itemDump(th, mask, 0, v, type, 5, NULL);
}
void itemDumpDirect(Thread* th, int mask, LW v, int type)
{
	if (!(mask & MainTerm.mask)) return;
	_itemDump(th, mask, 1, v, type, 5, NULL);
}

void threadDump(int mask,Thread* th,LINT n)
{
	LINT i;
	if (n>th->pp+1) n=th->pp+1;
	PRINTF(mask,"\nThread " LSD "\n",th->uid);
	PRINTF(mask, "\nStack: " LSD "/" LSD "\n", n, th->pp);
	for(i=n-1;i>=0;i--)
	{
		PRINTF(mask,">%3d: ",i);
		itemDump(th, mask,STACKGET(th,i),STACKTYPE(th,i));
	}
}

void itemEcho(Thread* th, int mask, LW v, int type, int ln)
{
	Buffer* b = MM.tmpBuffer;
	if (!(mask & MainTerm.mask)) return;
	bufferReinit(b);
	if (bufferItem(th, b, v, type, NULL)) return;
	if (ln) bufferAddChar(th, b,10);
	termWrite(mask, bufferStart(b), bufferSize(b));
}

void itemHeader(Thread* th, int mask,LB* p)
{
	PRINTF(mask, "header: " LSX " " LSX " " LSX "(%s) at " LSX "\n", p->sizetype, p->nextBlock, p->lifo, (p->lifo==MM.USELESS)?"useless":"useful", p);
}

void termEchoSourceLine(Pkg* pkg,Thread* th, int mask,char* srcName, char* src, int index)
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
