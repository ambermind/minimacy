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
#ifndef _HASHSET_
#define _HASHSET_

#define HASH_DEFAULT 4


#define HASH_TYPE_DICT 0
#define HASH_TYPE_MAP 1

#define HASH_LIST_NB 3
#define HASH_LIST_KEY 0
#define HASH_LIST_VAL 1
#define HASH_LIST_NEXT 2

struct HashSlots
{
	LB header;
	FORGET forget;
	MARK mark;
//	LINT type;
	LINT nb;
	LINT nbits;
	LB* table;
	LB* save;	// used only in GC compact
};

HashSlots* hashSlotsCreate(LINT nbits, LW type);
LINT hashSlotsComputeString(LINT nbits, char* key, LINT len);
LINT hashSlotsBitSize(HashSlots* h);
LB* hashSlotsGet(HashSlots* h, LINT index);

int hashmapAdd(Thread* th, int i, HashSlots* hashmap,LW key, int type);
void hashmapGet(Thread* th, int i, HashSlots* h,LW key, int type);
int hashmapDictAdd(HashSlots* h, LB* key, LB* value);
LB* hashmapDictGet(HashSlots* h,char* key);
LB* hashmapDictGetOpti(HashSlots* h,char* key,LINT len,LINT index);

int hashsetContains(HashSlots* h, LW key, int type);
int hashsetRemove(HashSlots* h, LW key, int type);
int hashsetAdd(HashSlots* h, LW key, int type);

void hashSlotsRecompute(HashSlots* h);

#endif