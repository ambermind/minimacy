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
	MEMORY_MARK(user,h->table);
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
	h->table= memoryAllocArray(th, size,DBG_ARRAY);
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
	return ARRAY_PNT(h->table, index);
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
		LB* p = PNT_FROM_VAL(key);
		LW dbg = HEADER_DBG(p);
		if (dbg == DBG_S) return hashSlotsComputeString(nbits, STR_START(p), STR_LENGTH(p));
		if (dbg == DBG_B) return hashmapComputeBignum(nbits, (bignum)p);
	}
	return hashmapComputeBin(nbits, key);
}

int hashmapAdd(Thread* th, int i, HashSlots* h, LW key, int type)
{
	LB* p;
	LINT index= hashmapComputeIndex(h->nbits,key, type);
	LB* table=h->table;
	LB* next=ARRAY_PNT(table,index);
	if ((!th)||STACK_IS_NIL(th,i))
	{
		LB* last=NULL;
		while(next)
		{
			if (lwEquals(ARRAY_GET(next,HASH_LIST_KEY),ARRAY_TYPE(next,HASH_LIST_KEY),key,type))
			{
				if (last) ARRAY_SET_PNT(last,HASH_LIST_NEXT,ARRAY_PNT(next,HASH_LIST_NEXT))
				else ARRAY_SET_PNT(table,index,ARRAY_PNT(next,HASH_LIST_NEXT));
				h->nb--;
				return 0;
			}
			last=next;
			next=ARRAY_PNT(next,HASH_LIST_NEXT);
		}
		return 0;
	}
	while(next)
	{
		if (lwEquals(ARRAY_GET(next,HASH_LIST_KEY),ARRAY_TYPE(next,HASH_LIST_KEY),key,type))
		{
			STACK_STORE(next,HASH_LIST_VAL,th,i);
			return 0;
		}
		next=ARRAY_PNT(next,HASH_LIST_NEXT);
	}
	p= memoryAllocArray(th, HASH_LIST_NB, DBG_HASH_LIST_LIST);
	if (!p) return EXEC_OM;
	ARRAY_SET_TYPE(p, HASH_LIST_KEY,key,type);
	STACK_STORE(p, HASH_LIST_VAL, th, i);
	ARRAY_SET_PNT(p, HASH_LIST_NEXT,ARRAY_PNT(table,index));
	ARRAY_SET_PNT(table,index,p);
	h->nb++;
	return 0;
}


void hashmapGet(Thread* th, int i, HashSlots* h,LW key, int type)
{
	LINT index= hashmapComputeIndex(h->nbits, key, type);
	LB* table=h->table;
	LB* next=ARRAY_PNT(table,index);
	while(next)
	{
		if (lwEquals(ARRAY_GET(next,HASH_LIST_KEY),ARRAY_TYPE(next,HASH_LIST_KEY),key,type)) {
			STACK_LOAD(th,i,next,HASH_LIST_VAL);
			return;
		}
		next=ARRAY_PNT(next,HASH_LIST_NEXT);
	}
	STACK_SET_NIL(th,i);
}


int hashmapDictAdd(Thread* th, HashSlots* h, LB* key, LB* value)
{
	LB* p=NULL;
	if (h && key)
	{
		LINT index= hashmapComputeIndex(h->nbits, VAL_FROM_PNT(key), VAL_TYPE_PNT);
		p = memoryAllocArray(th, HASH_LIST_NB, DBG_HASH_LIST_LIST);
		if (!p) return EXEC_OM;
		ARRAY_SET_PNT(p, HASH_LIST_KEY, key);
		ARRAY_SET_PNT(p, HASH_LIST_VAL, value);
		ARRAY_SET_PNT(p, HASH_LIST_NEXT, ARRAY_PNT(h->table, index));
		ARRAY_SET_PNT(h->table, index, p);
		h->nb++;
	}
	return 0;
}

LB* hashmapDictGetOpti(HashSlots* h,char* key,LINT len,LINT index)
{
	LB* next=(ARRAY_PNT(h->table,index));
	while(next)
	{
		LB* p=(ARRAY_PNT(next,HASH_LIST_KEY));
		if ((STR_LENGTH(p)==len)&&(!memcmp(STR_START(p),key,len))) return ARRAY_PNT(next,HASH_LIST_VAL);
		next=(ARRAY_PNT(next,HASH_LIST_NEXT));
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
	LB* next = ARRAY_PNT(table, index);
	while (next)
	{
		if (lwEquals(ARRAY_GET(next, LIST_VAL), ARRAY_TYPE(next, LIST_VAL), key, type)) return 1;
		next = ARRAY_PNT(next, LIST_NXT);
	}
	return 0;
}

int hashsetRemove(HashSlots* h, LW key, int type)
{
	LINT index = hashmapComputeIndex(h->nbits, key, type);
	LB* table = h->table;
	LB* next = ARRAY_PNT(table, index);
	LB* last = NULL;
	while (next)
	{
		if (lwEquals(ARRAY_GET(next, LIST_VAL), ARRAY_TYPE(next, LIST_VAL), key, type))
		{
			if (last) ARRAY_SET_PNT(last, LIST_NXT, ARRAY_PNT(next, LIST_NXT))
			else ARRAY_SET_PNT(table, index, ARRAY_PNT(next, LIST_NXT));
			h->nb--;
			return 1;
		}
		last = next;
		next = ARRAY_PNT(next, LIST_NXT);
	}
	return 0;
}

int hashsetAdd(Thread* th, HashSlots* h, LW key, int type)
{
	LB* p;
	LINT index = hashmapComputeIndex(h->nbits, key, type);
	LB* table = h->table;
	LB* next = (ARRAY_PNT(table, index));
	if (!th) return 0;
	while (next)
	{
		if (lwEquals(ARRAY_GET(next, LIST_VAL), ARRAY_TYPE(next, LIST_VAL), key, type)) return 0;
		next = ARRAY_PNT(next, LIST_NXT);
	}
	p = memoryAllocArray(th, LIST_LENGTH, DBG_LIST);
	if (!p) return EXEC_OM;
	ARRAY_SET_TYPE(p, LIST_VAL, key, type);
	ARRAY_SET_PNT(p, LIST_NXT, ARRAY_PNT(table, index));
	ARRAY_SET_PNT(table, index, p);
	h->nb++;
	return 0;
}
