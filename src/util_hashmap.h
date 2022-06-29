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

struct Hashmap
{
	LB header;
	FORGET forget;
	MARK mark;
//	LINT type;
	LINT nb;
	LINT nbits;
	LB* table;
};

Hashmap* hashmapCreate(Thread* th, LINT nbits);
LINT hashmapComputeIndex(LINT nbits, LW key);
LINT hashmapComputeString(LINT nbits, char* key, LINT len);
LINT hashmapBitSize(Hashmap* h);
LW hashmapSlotGet(Hashmap* h, LINT index);
int hashmapAdd(Thread* th, Hashmap* hashmap,LW key,LW value);
int hashmapAddAlways(Thread* th, Hashmap* h, LW key, LW value);
LW hashmapGet(Hashmap* hashmap,LW key);
LW hashmapDictGet(Hashmap* h,char* key);
LW hashmapDictGetOpti(Hashmap* h,char* key,LINT len,LINT index);


#endif