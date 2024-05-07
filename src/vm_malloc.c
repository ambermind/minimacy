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

#ifdef USE_MEMORY_C

typedef struct BMM BMM;

struct BMM {
	long long header;
	BMM* left;
	BMM* right;
	BMM* next;
};

#define BMM_FREE 0
#define BMM_USED 1

#define BMM_HEADER(len,type) ((len)|(type))
#define BMM_LENGTH(val) ((val)&~3)
#define BMM_TYPE(val) ((val)&3)

#ifdef MEMORY_C_START
char* MemoryStrip=NULL;
#else
char MemoryStrip[MEMORY_C_SIZE];
#endif

BMM* BmmRoot;

void cMallocInit() {
#ifdef MEMORY_C_START
	MemoryStrip=(char*)(MEMORY_C_START);
	PRINTF(LOG_SYS,"> Memory strip : %llx\n",MemoryStrip);
	PRINTF(LOG_SYS,"> Memory end   : %llx\n",MemoryStrip+MEMORY_C_SIZE);
#endif
	PRINTF(LOG_SYS,"> Memory length: %lld bytes\n",MEMORY_C_SIZE);
	BmmRoot = (BMM*)MemoryStrip;
	BmmRoot->header = BMM_HEADER(MEMORY_C_SIZE, BMM_FREE);
	BmmRoot->left = BmmRoot->right = BmmRoot->next = NULL;
}

int _bmmAdd(BMM** parentLink, BMM* nodeToAdd)
{
	long long size;
	BMM* node = *parentLink;
	if (!node) {
		nodeToAdd->left = nodeToAdd->right = nodeToAdd->next = NULL;
		*parentLink = nodeToAdd;
		return 0;
	}
	size = BMM_LENGTH(nodeToAdd->header);
	if (size > BMM_LENGTH(node->header)) return _bmmAdd(&node->right, nodeToAdd);
	if (size < BMM_LENGTH(node->header)) return _bmmAdd(&node->left, nodeToAdd);
	nodeToAdd->left = node->left;
	nodeToAdd->right = node->right;
	node->left = node->right = NULL;
	nodeToAdd->next = node;
	*parentLink = nodeToAdd;
	return 0;
}

void bmmRebuildTree()
{
	long long index = 0;
	if (MM.gcTrace) PRINTF(LOG_SYS,"> Malloc: rebuild\n");
	BmmRoot = NULL;
	while (index < MEMORY_C_SIZE)
	{
		long long index0 = index;
		BMM* node = (BMM*)&MemoryStrip[index];
		index += BMM_LENGTH(node->header);
		if (BMM_TYPE(node->header) == BMM_USED) continue;
		while (index < MEMORY_C_SIZE)
		{
			BMM* next = (BMM*)&MemoryStrip[index];
			if (BMM_TYPE(next->header) == BMM_USED) break;
			index += BMM_LENGTH(next->header);
		}
		node->header = BMM_HEADER(index - index0, BMM_FREE);
		_bmmAdd(&BmmRoot, node);
	}
}

BMM* _bmmTakeSmallest(BMM** parentLink)
{
	BMM* node = *parentLink;
	if (node->left) return _bmmTakeSmallest(&node->left);
	*parentLink = node->right;
	return node;
}
BMM* _bmmTakeAtLeast(BMM** parentLink, long long size, int exact)
{
	BMM* node = *parentLink;
	BMM* smallest;
	if (!node) return NULL;
	if (size > BMM_LENGTH(node->header)) return _bmmTakeAtLeast(&node->right, size, exact);
	if (size < BMM_LENGTH(node->header))
	{
		BMM* result = _bmmTakeAtLeast(&node->left, size, exact);
		if (result) return result;
		if (exact) return NULL;
	}
	// here we know that node.length is at least size
	if (node->next) {	// there are at least two nodes with this size
		node->next->left = node->left;
		node->next->right = node->right;
		*parentLink = node->next;
		return node;
	}
	if (!node->left) {
		*parentLink = node->right;
		return node;
	}
	if (!node->right) {
		*parentLink = node->left;
		return node;
	}
	smallest = _bmmTakeSmallest(&node->right);
	smallest->left = node->left;
	smallest->right = node->right;
	*parentLink = smallest;
	return node;
}
void* _bmmMalloc(long long size)
{
	BMM* newBlock;
	long long remainingSize;
	size = (size + 7) & (~7);

	size += sizeof(BmmRoot->header);
	BMM* result = _bmmTakeAtLeast(&BmmRoot, size, 1);	// exact
	if (result) {
		result->header = BMM_HEADER(size, BMM_USED);
		return (void*)&result->left;
	}
	result = _bmmTakeAtLeast(&BmmRoot, size + sizeof(BMM), 0);	// we need at least extra space for the smallest free block
	if (!result) return NULL;	// should recompute the binary tree
	newBlock = (BMM*)(((char*)result) + size);
	remainingSize = BMM_LENGTH(result->header) - size;
	newBlock->header = BMM_HEADER(remainingSize, BMM_FREE);
	_bmmAdd(&BmmRoot, newBlock);
	result->header = BMM_HEADER(size, BMM_USED);
	return (void*)&result->left;
}
void* bmmMalloc(long long size)
{
	void* result = _bmmMalloc(size);
	if (result) return result;
	bmmRebuildTree();
	return _bmmMalloc(size);
}
void bmmFree(void* block)
{
	char* start = (char*)block;
	BMM* node = (BMM*)(start - sizeof(BmmRoot->header));
	node->header = BMM_HEADER(BMM_LENGTH(node->header), BMM_FREE);
	node->left = node->right = node->next = NULL;
	_bmmAdd(&BmmRoot, node);
}
#endif