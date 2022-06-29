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

int _socketInfo(void* p, int* ip, int* port);

Term MainTerm;

void termInit()
{
	MainTerm.mask = LOG_ALL;

	MainTerm.userBufferize = 0;
	MainTerm.errBufferize = 0;
}
void termEnd()
{
	// may release later fields
	MainTerm.userBufferize = 0;
	MainTerm.errBufferize = 0;
}
void termSetMask(int i)
{
	MainTerm.mask=i;
}
int termGetMask()
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
void termWrite(Thread* th,int mask,char* src, LINT len)
{
	if (!(mask & MainTerm.mask)) return;
	if (mask == LOG_USER && MainTerm.userBufferize) bufferAddBin(th, MM.userBuffer, src, len);
	else if (mask == LOG_ERR && MainTerm.errBufferize) bufferAddBin(th, MM.errBuffer, src, len);
	else consoleWrite(termUserOutput(mask),src, (int)len);
}
void termPrintfv(Thread* th, int mask,char *format,va_list arglist)
{
	if (!(mask&MainTerm.mask)) return;
	if (mask==LOG_USER && MainTerm.userBufferize) bufferVPrintf(th, MM.userBuffer, format, arglist);
	else if (mask == LOG_ERR && MainTerm.errBufferize) bufferVPrintf(th, MM.errBuffer, format, arglist); 
	else consoleVPrint(termUserOutput(mask), format, arglist);
}

void termPrintf(Thread* th, int mask,char *format, ...)
{
	va_list arglist;
	va_start(arglist,format);
	termPrintfv(th, mask,format,arglist);
	va_end(arglist);
}

char* MMTAB="                                ";
void _itemDump(Thread* th, int mask, int skipHeader, LW v, int rec, char* tab);
void _globalsPrint(Thread* th, int mask, LB* g, int rec,char* tab)
{
	LINT j;
	if (!g) return;
	PRINTF(th, mask, "%s links:\n", tab);
	for (j = 0; j < TABLEN(g); j++)
	{
		LW w = TABGET(g, j);
		LB* q = VALTOPNT(w);
		PRINTF(th, mask, "  %s " LSD ":\n", tab, j);
		if ((ISVALPNT(w)) && (HEADER_DBG(q) == DBG_REF))
		{
			Ref* ref = (Ref*)q;
			PRINTF(th, mask, "  %s %s.%s: ", tab - 2, refPkgName(ref), refName(ref));
			typePrint(th, mask, ref->type);
			PRINTF(th, mask, "\n");
		}
		else if ((ISVALPNT(w)) && (HEADER_DBG(q) == DBG_FUN))
		{
			PRINTF(th, mask, "%s lambda:\n", tab-4);
			_itemDump(th, mask, 0, TABGET(q, FUN_USER_BC), rec - 1, tab - 6);
			_globalsPrint(th, mask, VALTOPNT(TABGET(q, FUN_USER_GLOBALS)), rec - 1, tab - 6);
		}
		else _itemDump(th, mask, 0, w, rec > 1 ? 2 : rec - 1, tab - 4);
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
		PRINTF(th, mask, format, line);
		for (i = line; i < line + 16; i++) if (i < len) PRINTF(th, mask, "%02x ", 255 & (src[i]));
		else PRINTF(th, mask, "   ");
		for (i = line; i < line + 16; i++) if (i < len) PRINTF(th, mask, "%c", src[i] < 32 ? '.' : src[i]);
		PRINTF(th, mask, "\n");
	}
}
void _itemDump(Thread* th, int mask, int skipHeader, LW v,int rec,char* tab)
{
	if (rec<0)
	{
		PRINTF(th, mask,"%s ...\n",tab);
		return;
	}
	if (!tab) tab=MMTAB+strlen(MMTAB);
	else if (strlen(tab)>(strlen(MMTAB)-4)) tab+=4;

	if (v==NIL) PRINTF(th, mask,"%s nil\n",tab);
	else if (v == MM.falseRef)
	{
		if (!skipHeader) PRINTF(th, mask, "%s Bool: ", tab);
		PRINTF(th, mask, "false\n");
	}
	else if (v == MM.trueRef)
	{
		if (!skipHeader) PRINTF(th, mask, "%s Bool: ", tab);
		PRINTF(th, mask, "true\n");
	}
	else if (ISVALPNT(v))
	{
		LB* p=VALTOPNT(v);
		LW dbg=HEADER_DBG(p);
		if (ISVALPNT(dbg))
		{
			Ref* refType=(Ref*)VALTOPNT(dbg);
			// we do not handle REFCODE_SUM because there is no way to get such a value here
			if (refType->code==REFCODE_STRUCT)
			{
				Ref* ref = refType;
				if (!skipHeader) PRINTF(th, mask,"%s struct %s\n",tab, STRSTART(refType->name));
				else PRINTF(th, mask, "\n");
				while (ref)
				{
					Ref* next = (Ref*)VALTOPNT(ref->val);
					while (next)
					{
//						PRINTF(th, mask, "  %s %s %lld/%lld:\n", tab, STRSTART(next->name),next->index,TABLEN(p));
						PRINTF(th, mask, "  %s %s:\n", tab, STRSTART(next->name));
						_itemDump(th, mask, 0, TABGET(p, next->index), rec - 1, tab - 4);
						next = (Ref*)VALTOPNT(next->val);;
					}
					ref = ref->parent;
				}
			}
			else if ((refType->code==REFCODE_CONS)|| (refType->code == REFCODE_CONS0))
			{
				LINT j;
				Ref* main=refType->parent;

				if (refType->code == REFCODE_CONS)
				{
					if (!skipHeader) PRINTF(th, mask, "%s sum %s ", tab, STRSTART(main->name));
					PRINTF(th, mask, "constructor %s\n", STRSTART(refType->name));
					for (j = 1; j < TABLEN(p); j++) {
						PRINTF(th, mask, "  %s " LSD " :\n", tab, j - 1);
						_itemDump(th, mask, 0, TABGET(p, j), rec - 1, tab - 4);
					}
				}
				else
				{
					if (!skipHeader) PRINTF(th, mask, "%s sum %s ", tab, STRSTART(main->name));
					PRINTF(th, mask, "const %s\n", STRSTART(refType->name));
				}
			}
		}
		else if (dbg==DBG_HASHSET)
		{
			Hashmap* h=(Hashmap*)p;
			LINT index;
			LB* table=h->table;
			LINT first=1;
//				_itemDump(th, mask, 0,PNTTOVAL(h->table),rec-1,tab-4);
//				if (0)
			LINT end = h->nb - TERM_LIMIT_ENUM;
			LINT i = 0;
			if (!skipHeader) PRINTF(th, mask, "%s hashmap ", tab);
			PRINTF(th, mask, "#" LSD " {\n", h->nb);
			for(index=0;index<TABLEN(table);index++)
			{
				LB* next=VALTOPNT(TABGET(table,index));
				while(next)
				{
					if (i< TERM_LIMIT_ENUM || i>end)
					{
						if (!first) PRINTF(th, mask, "  %s ---\n", tab);
						first = 0;
						_itemDump(th, mask, 0, TABGET(next, HASH_LIST_KEY), rec - 1, tab - 4);
						PRINTF(th, mask, "  %s ->\n", tab);
						_itemDump(th, mask, 0, TABGET(next, HASH_LIST_VAL), rec - 1, tab - 4);
					}
					else if (i== TERM_LIMIT_ENUM) PRINTF(th, mask, "  %s ...skipping %d elements...\n", tab, end - TERM_LIMIT_ENUM);
					i++;
					next=VALTOPNT(TABGET(next,HASH_LIST_NEXT));
				}
			}
			PRINTF(th, mask,"%s}\n",tab);
		}
		else if (dbg==DBG_BYTECODE)
		{
			if (!skipHeader) PRINTF(th, mask,"%s bytecode:\n",tab);
			bytecodePrint(th, mask,p);
		}
		else if (dbg==DBG_S)
		{
			if (strToSource(th, MM.tmpBuffer, STRSTART(p), STRLEN(p))) return;
			if (!skipHeader) PRINTF(th, mask,"%s Str: ",tab);
			PRINTF(th, mask,"%s\n",bufferStart(MM.tmpBuffer));
		}
		else if (dbg==DBG_B)
		{
			int i;
			bignum a=(bignum)p;
			if (!skipHeader) PRINTF(th, mask,"%s BigNum: (%d)",tab,bignumLen(a));
			if (bignumSign(a)) PRINTF(th, mask,"-");
			for(i=bignumLen(a)-1;i>=0;i--) PRINTF(th, mask,"%08x ",bignumGet(a,i));
			PRINTF(th, mask,"\n");
		}
		else if (dbg==DBG_TYPE)
		{
			Type* t=(Type*)p;
			if (!skipHeader) PRINTF(th, mask,"%s Type: ",tab);
			typePrint(th, mask,t);
			PRINTF(th, mask,"\n");
		}
		else if (dbg==DBG_REF)	// to be completed
		{
			Ref* r=(Ref*)p;
			char* name=(r->name==NULL)?"?":STRSTART(r->name);
			if (!skipHeader) PRINTF(th, mask,"%s Reference ",tab);
			PRINTF(th, mask,"%s %s\n",refCodeName(r->code), name);
			if (r->parser) PRINTF(th, mask, "%s File: %s at "LSD"\n", tab-4,r->parser->name, r->parserIndex);
//			PRINTF(th, mask,"%s %s (#" LSD " /" LSD ")\n",refCodeName(r->code), name, r->code,r->index);
			_itemDump(th, mask, 0,TYPETOVAL(r->type),rec-1,tab-4);
			if ((r->code!=REFCODE_CONS0)&& (r->code != REFCODE_CONS) && (r->code != REFCODE_SUM) && 
				(r->code != REFCODE_STRUCT) && (r->code != REFCODE_FIELD) && (rec>0))
				_itemDump(th, mask, 0,r->val,rec-1,tab-4);
		}
		else if (dbg == DBG_LAMBDA)
		{
			LINT j;
			LB* f = VALTOPNT(TABGET(p,0));
			LB* fName = VALTOPNT(TABGET(f, FUN_USER_NAME));
			Pkg* pkg = (Pkg*)VALTOPNT(TABGET(f, FUN_USER_PKG));
			PRINTF(th, mask, "%s lambda function defined in function: %s.%s\n", tab, pkgName(pkg),STRSTART(fName));
			_itemDump(th, mask, 0, TABGET(f, FUN_USER_BC), rec - 1, tab - 2);
			_globalsPrint(th, mask,VALTOPNT(TABGET(f, FUN_USER_GLOBALS)),rec-1,tab-2);
			if (TABLEN(p) > 1)
			{
				PRINTF(th, mask, "%s curried locals:\n", tab-2);
				for (j = 1; j < TABLEN(p); j++)
				{
					PRINTF(th, mask, "  %s " LSD ":\n", tab, j-1);
					_itemDump(th, mask, 0, TABGET(p, j), rec - 1, tab - 4);
				}
			}
		}
		else if (dbg == DBG_FUN)
		{
			char* name = VALTOPNT(TABGET(p, FUN_USER_NAME)) ? STRSTART(VALTOPNT(TABGET(p, FUN_USER_NAME))) : "[NO NAME]";
			if (!skipHeader) PRINTF(th, mask, "%s fun ", tab);
			PRINTF(th, mask, "%s\n", name);
			if (rec > 1)
			{
				_itemDump(th, mask, 0, TABGET(p, FUN_USER_BC), rec - 1, tab - 2);
				_globalsPrint(th, mask, VALTOPNT(TABGET(p, FUN_USER_GLOBALS)), rec-1, tab - 2);
			}
		}
		else if (dbg==DBG_PKG)
		{
			Pkg* pkg=(Pkg*)p;
			if (!skipHeader) PRINTF(th, mask, "%s Pkg ", tab);
			PRINTF(th, mask, "%s\n", pkgName(pkg));
			pkgDisplay(th, mask,pkg);
		}
		else if (dbg== DBG_SOCKET)
		{
			int port,ip;
			_socketInfo((void*)p,&ip,&port);
			if (!skipHeader) PRINTF(th, mask, "%s Socket ", tab);
			PRINTF(th, mask, "%x:%d\n", ip,port);
		}
		else if (dbg==DBG_LIST)
		{
			LINT end = - TERM_LIMIT_ENUM;
			LINT i = 0;
			LB* q = p;
			while (q)
			{
				end++;
				q = VALTOPNT(TABGET(q, LIST_NXT));
			}
			if (!skipHeader) PRINTF(th, mask,"%s list:\n",tab);
			else PRINTF(th, mask, "\n");
			while(p)
			{
				if (i< TERM_LIMIT_ENUM || i>end)
				{
//					PRINTF(th, mask,"%s   ->\n",tab);
					_itemDump(th, mask, 0,TABGET(p,LIST_VAL),rec-1,tab-2);
				}
				else if (i == TERM_LIMIT_ENUM) PRINTF(th, mask, "  %s ...skipping %d elements...\n", tab, end - TERM_LIMIT_ENUM);
				i++;
				p = VALTOPNT(TABGET(p, LIST_NXT));
			}
		}
		else if (dbg == DBG_FIFO)
		{
			LINT end = VALTOINT(TABGET(p, FIFO_COUNT)) - TERM_LIMIT_ENUM;
			LINT i = 0;

			if (!skipHeader) PRINTF(th, mask, "%s fifo ", tab);
			PRINTF(th, mask, "#" LSD "\n", VALTOINT(TABGET(p,FIFO_COUNT)));
			p = VALTOPNT(TABGET(p, FIFO_START));
			while (p)
			{
				if (i< TERM_LIMIT_ENUM || i>end)
				{
//					PRINTF(th, mask, "%s   ->\n", tab);
					_itemDump(th, mask, 0, TABGET(p, LIST_VAL), rec - 1, tab - 2);
				}
				else if (i == TERM_LIMIT_ENUM) PRINTF(th, mask, "  %s ...skipping %d elements...\n", tab, end - TERM_LIMIT_ENUM);
				i++;
				p = VALTOPNT(TABGET(p, LIST_NXT));
			}
		}
		else if (dbg==DBG_THREAD)
		{
			Thread* th=(Thread*)p;
			PRINTF(th, mask,"%s thread " LSD " callstack=" LSD " pc=" LSD " pp=" LSD "\n",tab,th->uid,th->callstack,th->pc,th->pp);
/*			for(i=0;i<=th->pp;i++)
			{
				PRINTF(th, mask,"%s-- " LSD ":\n",tab,th->pp-i);
				_itemDump(th, mask, 0,TABGET(th->stack,i),rec-1,tab-4);
			}
*/
            //			_itemDump(th, mask, 0,PNTTOVAL((LB*)th->next),rec,tab);
		}
		else if (dbg == DBG_MEMCOUNT)
		{
			Mem* m = (Mem*)p;
			PRINTF(th, mask, "%s MEM " LSD "/" LSD" %s (%s)\n", tab, m->bytes, m->maxBytes, m->name?STRSTART(m->name):"no name", m->header.mem?"child":"root");
		}
		else if (HEADER_TYPE(p)==TYPE_TAB)
		{
			LINT j,t;
			char* name=(dbg==DBG_ARRAY)?"array":"tuple";
			LINT end = TABLEN(p) - TERM_LIMIT_ENUM;

			t=TABLEN(p);
			if (!skipHeader) PRINTF(th, mask, "%s  %s ", tab,name);
			PRINTF(th, mask,"#" LSD " %c\n",TABLEN(p), (dbg == DBG_ARRAY) ? '{':'[');
			if (rec) for(j=0;j<t;j++) {
				if (j< TERM_LIMIT_ENUM || j>end)
				{
					PRINTF(th, mask, "  %s " LSD ":\n", tab, j);
					_itemDump(th, mask, 0, TABGET(p, j), rec - 1, tab - 4);
				}
				else if (j == TERM_LIMIT_ENUM) PRINTF(th, mask, "  %s ...skipping %d elements...\n", tab, end - TERM_LIMIT_ENUM);
			}
			else PRINTF(th, mask," ...\n");
			PRINTF(th, mask,"%s%c\n",tab, (dbg == DBG_ARRAY) ? '}' : ']');
		}
		else
		{
			LINT len=BINLEN(p);
			LINT lenDump = len;
			char* src=(char*)BINSTART(p);

			if (!skipHeader) PRINTF(th, mask,"%s ",tab);
			PRINTF(th, mask,"binary #" LSD " (0x" LSX ")\n",len,len);
			if (lenDump >= TERM_LIMIT_BIN) lenDump = TERM_LIMIT_BIN;
			_hexDump(th, mask, src, lenDump,0);
			if (lenDump<len) PRINTF(th, mask, "[hiding "LSD" (0x"LSX") bytes]\n", len-lenDump, len - lenDump);
		}
	}
	else if (ISVALFLOAT(v))
	{
		if (!skipHeader) PRINTF(th, mask, "%s Float: ", tab);
		PRINTF(th, mask, "%g\n", VALTOFLOAT(v));
	}
	else
	{
		if (!skipHeader) PRINTF(th, mask, "%s Int: ", tab);
		PRINTF(th, mask, LSD " (0x" LSX ")\n",VALTOINT(v), VALTOINT(v));
	}
}

void itemDump(Thread* th, int mask, LW v)
{
	if (!(mask & MainTerm.mask)) return;
	_itemDump(th, mask, 0, v, 5, NULL);
}
void itemDumpDirect(Thread* th, int mask, LW v)
{
	if (!(mask & MainTerm.mask)) return;
	_itemDump(th, mask, 1, v, 5, NULL);
}

void threadDump(int mask,Thread* th,LINT n)
{
	LINT i;
	if (n>th->pp+1) n=th->pp+1;
	PRINTF(th, mask,"\nThread " LSD "\n",th->uid);
	PRINTF(th, mask, "\nStack: " LSD "/" LSD "\n", n, th->pp);
	for(i=n-1;i>=0;i--)
	{
		PRINTF(th, mask,">%3d: ",i);
		itemDump(th, mask,STACKGET(th,i));
	}
}

void itemEcho(Thread* th, int mask, LW v,int ln)
{
	Buffer* b = MM.tmpBuffer;
	if (!(mask & MainTerm.mask)) return;
	bufferReinit(b);
	if (bufferItem(th, b, v, NULL, 0, 1)) return;
	if (ln) bufferAddchar(th, b,10);
	termWrite(th, mask, bufferStart(b), bufferSize(b));
}

void itemHeader(Thread* th, int mask,LB* p)
{
	PRINTF(th, mask, "header: " LSX " " LSX " " LSX "(%s) at " LSX "\n", p->sizetype, p->nextBlock, p->lifo, (p->lifo==MM.USELESS)?"useless":"useful", p);
}

void termEchoSourceLine(Thread* th, int mask,char* srcName, char* src, int index)
{
	int line = 1;
	int lineStart = 0;
	int i;
	int len = (int)strlen(src);
	int isPrompt = (!srcName) || (!strlen(srcName));
	if (index > len) index = len;

	for (i = 0; i < index; i++) if (src[i] == 10) {
		lineStart = i + 1;
		line++;
	}
	if (!isPrompt) PRINTF(th, mask, "\n> pkg %s:", srcName);
	if (!isPrompt) PRINTF(th, mask, "\n> line %d:", line);
	PRINTF(th, mask, "\n> ");
	for (i = lineStart; i < len; i++)
		if (src[i] == 10) break;
		else PRINTF(th, mask, "%c", src[i]);
	PRINTF(th, mask, "\n> ");
	for (i = lineStart; i < index; i++) PRINTF(th, mask, "%c", (src[i] == 9) ? 9 : 32);
	PRINTF(th, mask, "^\n\n");
}
