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
	HashSlots* h=(HashSlots*)user;
	MEMORYMARK(user,h->table);
}

HashSlots* hashSlotsCreate(Thread* th, LINT nbits, LW type)
{
	HashSlots* h;
	LINT one=1;
	LINT size;

	if (nbits==0) nbits=HASH_DEFAULT;
	size = one << nbits;
	memoryEnterFast();
	h=(HashSlots*)memoryAllocExt(th, sizeof(HashSlots),type,NULL,hashmapMark); if (!h) return NULL;
	h->nb=0;
	h->nbits=nbits;
	h->table = NULL;	// required because the following memoryAlloc may fire a GC
	h->table= memoryAllocTable(th, size,DBG_ARRAY);
	if (!h->table) return NULL;
	//	PRINTF(LOG_DEV,"hashmap %llx nbits %lld -> %llx #%lld \n", h, nbits, h->table,size);
	memoryLeaveFast();
	return h;
}

LINT hashSlotsBitSize(HashSlots* h)
{
	if (!h) return 0;
	return h->nbits;
}
LB* hashSlotsGet(HashSlots* h, LINT index)
{
	if (!h) return NULL;
	if ((index < 0) || (index >= (LINT)(1 << h->nbits))) return NULL;
	return TABPNT(h->table, index);
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
LINT hashSlotsComputeString(LINT nbits, char* key, LINT len)
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
LINT hashmapComputeIndex(LINT nbits, LW key, int type)
{
	if (type==VAL_TYPE_PNT)
	{
		LB* p = VALTOPNT(key);
		LW dbg = HEADER_DBG(p);
		if (dbg == DBG_S) return hashSlotsComputeString(nbits, STRSTART(p), STRLEN(p));
		if (dbg == DBG_B) return hashmapComputeBignum(nbits, (bignum)p);
	}
	return hashmapComputeBin(nbits, key);
}

int hashmapAdd(Thread* th, int i, HashSlots* h, LW key, int type)
{
	LB* p;
	LINT index= hashmapComputeIndex(h->nbits,key, type);
	LB* table=h->table;
	LB* next=TABPNT(table,index);
	if ((!th)||STACKISNIL(th,i))
	{
		LB* last=NULL;
		while(next)
		{
			if (lwEquals(TABGET(next,HASH_LIST_KEY),TABTYPE(next,HASH_LIST_KEY),key,type))
			{
				if (last) TABSETPNT(last,HASH_LIST_NEXT,TABPNT(next,HASH_LIST_NEXT))
				else TABSETPNT(table,index,TABPNT(next,HASH_LIST_NEXT));
				h->nb--;
				return 0;
			}
			last=next;
			next=TABPNT(next,HASH_LIST_NEXT);
		}
		return 0;
	}
	while(next)
	{
		if (lwEquals(TABGET(next,HASH_LIST_KEY),TABTYPE(next,HASH_LIST_KEY),key,type))
		{
			STACKSTORE(next,HASH_LIST_VAL,th,i);
			return 0;
		}
		next=TABPNT(next,HASH_LIST_NEXT);
	}
	p= memoryAllocTable(th, HASH_LIST_NB, DBG_HASH_LIST_LIST);
	if (!p) return EXEC_OM;
	TABSETTYPE(p, HASH_LIST_KEY,key,type);
	STACKSTORE(p, HASH_LIST_VAL, th, i);
	TABSETPNT(p, HASH_LIST_NEXT,TABPNT(table,index));
	TABSETPNT(table,index,p);
	h->nb++;
	return 0;
}


void hashmapGet(Thread* th, int i, HashSlots* h,LW key, int type)
{
	LINT index= hashmapComputeIndex(h->nbits, key, type);
	LB* table=h->table;
	LB* next=TABPNT(table,index);
	while(next)
	{
		if (lwEquals(TABGET(next,HASH_LIST_KEY),TABTYPE(next,HASH_LIST_KEY),key,type)) {
			STACKLOAD(th,i,next,HASH_LIST_VAL);
			return;
		}
		next=TABPNT(next,HASH_LIST_NEXT);
	}
	STACKSETNIL(th,i);
}


int hashmapDictAdd(Thread* th, HashSlots* h, LB* key, LB* value)
{
	LB* p=NULL;
	if (h && key)
	{
		LINT index= hashmapComputeIndex(h->nbits, PNTTOVAL(key), VAL_TYPE_PNT);
		p = memoryAllocTable(th, HASH_LIST_NB, DBG_HASH_LIST_LIST);
		if (!p) return EXEC_OM;
		TABSETPNT(p, HASH_LIST_KEY, key);
		TABSETPNT(p, HASH_LIST_VAL, value);
		TABSETPNT(p, HASH_LIST_NEXT, TABPNT(h->table, index));
		TABSETPNT(h->table, index, p);
		h->nb++;
	}
	return 0;
}

LB* hashmapDictGetOpti(HashSlots* h,char* key,LINT len,LINT index)
{
	LB* next=(TABPNT(h->table,index));
	while(next)
	{
		LB* p=(TABPNT(next,HASH_LIST_KEY));
		if ((STRLEN(p)==len)&&(!memcmp(STRSTART(p),key,len))) return TABPNT(next,HASH_LIST_VAL);
		next=(TABPNT(next,HASH_LIST_NEXT));
	}
	return NULL;
}

LB* hashmapDictGet(HashSlots* h,char* key)
{
	if ((h)&&(key))
	{
		LINT len=strlen(key);
		LINT index=hashSlotsComputeString(h->nbits,key,len);
		return hashmapDictGetOpti(h, key, len, index);
	}
	return NULL;
}

int hashsetContains(HashSlots* h, LW key, int type)
{
	LINT index = hashmapComputeIndex(h->nbits, key, type);
	LB* table = h->table;
	LB* next = TABPNT(table, index);
	while (next)
	{
		if (lwEquals(TABGET(next, LIST_VAL), TABTYPE(next, LIST_VAL), key, type)) return 1;
		next = TABPNT(next, LIST_NXT);
	}
	return 0;
}

int hashsetRemove(HashSlots* h, LW key, int type)
{
	LINT index = hashmapComputeIndex(h->nbits, key, type);
	LB* table = h->table;
	LB* next = TABPNT(table, index);
	LB* last = NULL;
	while (next)
	{
		if (lwEquals(TABGET(next, LIST_VAL), TABTYPE(next, LIST_VAL), key, type))
		{
			if (last) TABSETPNT(last, LIST_NXT, TABPNT(next, LIST_NXT))
			else TABSETPNT(table, index, TABPNT(next, LIST_NXT));
			h->nb--;
			return 1;
		}
		last = next;
		next = TABPNT(next, LIST_NXT);
	}
	return 0;
}

int hashsetAdd(Thread* th, HashSlots* h, LW key, int type)
{
	LB* p;
	LINT index = hashmapComputeIndex(h->nbits, key, type);
	LB* table = h->table;
	LB* next = (TABPNT(table, index));
	if (!th) return 0;
	while (next)
	{
		if (lwEquals(TABGET(next, LIST_VAL), TABTYPE(next, LIST_VAL), key, type)) return 0;
		next = TABPNT(next, LIST_NXT);
	}
	p = memoryAllocTable(th, LIST_LENGTH, DBG_LIST);
	if (!p) return EXEC_OM;
	TABSETTYPE(p, LIST_VAL, key, type);
	TABSETPNT(p, LIST_NXT, TABPNT(table, index));
	TABSETPNT(table, index, p);
	h->nb++;
	return 0;
}
