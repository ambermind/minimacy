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

void hashmapMark(LB* user)
{
	Hashmap* h=(Hashmap*)user;
	MEMORYMARK(user,h->table);
}

Hashmap* hashmapCreate(Thread* th, LINT nbits)
{
	Hashmap* h;
	LINT one=1;
	LINT size;

	if (nbits==0) nbits=HASH_DEFAULT;
	size = one << nbits;
	memoryEnterFast(th);
	h=(Hashmap*)memoryAllocExt(th, sizeof(Hashmap),DBG_HASHSET,NULL,hashmapMark); if (!h) return NULL;
	h->nb=0;
	h->nbits=nbits;
	h->table = NULL;	// required because the following memoryAlloc may fire a GC
	h->table= memoryAllocTable(th, size,DBG_ARRAY);
	if (!h->table) return NULL;
	//	printf("hashmap %llx nbits %lld -> %llx #%lld \n", h, nbits, h->table,size);
	memoryLeaveFast(th);
	return h;
}

LINT hashmapBitSize(Hashmap* h)
{
	if (!h) return 0;
	return h->nbits;
}
LW hashmapSlotGet(Hashmap* h, LINT index)
{
	if (!h) return NIL;
	if ((index < 0) || (index >= (LINT)(1 << h->nbits))) return NIL;
	return TABGET(h->table, index);
}

LINT hashmapComputeBin(LINT nbits, LW key)
{
	LINT msk=(1<<nbits)-1;
	LINT value=(LINT)key; 
	LINT nbMaxBits=LWLEN*8;
	LINT result=0;
	while(nbMaxBits>0)
	{
		result+=value&msk;
		result=(result&msk)+(result>>nbits);
		value>>=nbits;
		nbMaxBits-=nbits;
	}
	return result&msk;
}
LINT hashmapComputeString(LINT nbits, char* key, LINT len)
{
	LINT msk=(1<<nbits)-1;

	LINT i;
	LINT result=0;
	for(i=0;i<len;i++)
	{
		result=(result<<1)+((*(key++))&255);
		result=(result&msk)+(result>>nbits);
	}
	return result&msk;
}
LINT hashmapComputeBignum(LINT nbits, bignum key)
{
	LINT msk = (1 << nbits) - 1;

	LINT i;
	LINT result = key->sign;
	LINT len = key->len * sizeof(uint);
	char* p = (char*)key->data;
	for (i = 0; i < len; i++)
	{
		result = (result << 1) + ((*(p++)) & 255);
		result = (result & msk) + (result >> nbits);
	}
	return result & msk;
}
LINT hashmapComputeIndex(LINT nbits, LW key)
{
	if (ISVALPNT(key))
	{
		LB* p = VALTOPNT(key);
		LW dbg = HEADER_DBG(p);
		if (dbg == DBG_S) return hashmapComputeString(nbits, STRSTART(p), STRLEN(p));
		if (dbg == DBG_B) return hashmapComputeBignum(nbits, (bignum)p);
	}
	return hashmapComputeBin(nbits, key);
}

int hashmapAdd(Thread* th, Hashmap* h, LW key, LW value)
{
	if ((!h)||(key==NIL)) goto cleanup;
	{
		LB* p;
		LINT index= hashmapComputeIndex(h->nbits,key);
		LB* table=h->table;
		LB* next=VALTOPNT(TABGET(table,index));
		if (value==NIL)
		{
			LB* last=NULL;
			while(next)
			{
				if (lwEquals(TABGET(next,HASH_LIST_KEY),key))
				{
					if (last) TABSET(last,HASH_LIST_NEXT,TABGET(next,HASH_LIST_NEXT))
					else TABSET(table,index,TABGET(next,HASH_LIST_NEXT));
					h->nb--;
					goto cleanup;
				}
				last=next;
				next=VALTOPNT(TABGET(next,HASH_LIST_NEXT));
			}
			goto cleanup;
		}
		while(next)
		{
			if (lwEquals(TABGET(next,HASH_LIST_KEY),key))
			{
				TABSET(next,HASH_LIST_VAL,value);
				goto cleanup;
			}
			next=VALTOPNT(TABGET(next,HASH_LIST_NEXT));
		}
		p= memoryAllocTable(th, HASH_LIST_NB, DBG_HASH_LIST_LIST);
		if (!p) return EXEC_OM;
		TABSET(p, HASH_LIST_KEY,key);
		TABSET(p, HASH_LIST_VAL, value);
		TABSET(p, HASH_LIST_NEXT,TABGET(table,index));
		TABSET(table,index,PNTTOVAL(p));
		h->nb++;
	}
cleanup:
	return 0;
}

int hashmapAddAlways(Thread* th, Hashmap* h, LW key, LW value)
{
	LB* p=NULL;
	if (h)
	{
		LINT index= hashmapComputeIndex(h->nbits, key);
		p = memoryAllocTable(th, HASH_LIST_NB, DBG_HASH_LIST_LIST);
		if (!p) return EXEC_OM;
		TABSET(p, HASH_LIST_KEY, key);
		TABSET(p, HASH_LIST_VAL, value);
		TABSET(p, HASH_LIST_NEXT, TABGET(h->table, index));
		TABSET(h->table, index, PNTTOVAL(p));
		h->nb++;
	}
	return 0;
}

LW hashmapGet(Hashmap* h,LW key)
{
	if ((!h)||(key==NIL)) return NIL;
	{
		LINT index= hashmapComputeIndex(h->nbits, key);
		LB* table=h->table;
		LB* next=VALTOPNT(TABGET(table,index));
		while(next)
		{
			if (lwEquals(TABGET(next,HASH_LIST_KEY),key)) return TABGET(next,HASH_LIST_VAL);
			next=VALTOPNT(TABGET(next,HASH_LIST_NEXT));
		}
	}
	return NIL;
}

LW hashmapDictGet(Hashmap* h,char* key)
{
	if ((h)&&(key))
	{
		LINT len=strlen(key);
		LINT index=hashmapComputeString(h->nbits,key,len);
		LB* next=VALTOPNT(TABGET(h->table,index));
		while(next)
		{
			LB* p=VALTOPNT(TABGET(next,HASH_LIST_KEY));
			if ((STRLEN(p)==len)&&(!memcmp(STRSTART(p),key,len))) return TABGET(next,HASH_LIST_VAL);
			next=VALTOPNT(TABGET(next,HASH_LIST_NEXT));
		}
	}
	return NIL;
}

LW hashmapDictGetOpti(Hashmap* h,char* key,LINT len,LINT index)
{
	LB* next=VALTOPNT(TABGET(h->table,index));
	while(next)
	{
		LB* p=VALTOPNT(TABGET(next,HASH_LIST_KEY));
		if ((STRLEN(p)==len)&&(!memcmp(STRSTART(p),key,len))) return TABGET(next,HASH_LIST_VAL);
		next=VALTOPNT(TABGET(next,HASH_LIST_NEXT));
	}
	return NIL;
}

int hashmapprintKeys(Hashmap* h)
{
	LINT index;
	LB* table = h->table;
	for (index = 0; index < TABLEN(table); index++)
	{
		LB* next = VALTOPNT(TABGET(table, index));
		while (next)
		{
			printf("]%d] %s\n",(int)index,STRSTART(VALTOPNT(TABGET(next, HASH_LIST_KEY))));
			next = VALTOPNT(TABGET(next, HASH_LIST_NEXT));
		}
	}
	return 0;
}
