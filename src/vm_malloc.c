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
	LINT sizeAndType;
	BMM* left;
	BMM* right;
	BMM* next;
};

#define BMM_SIZE(p) ((HEADER_TYPE(p)==TYPE_FREE)?(HEADER_SIZE(p)):BLOCK_TOTAL_MEMORY(HEADER_TYPE(p), HEADER_SIZE(p)))

#define BMM_MASK (LWLEN-1)

#ifdef MEMORY_C_START
char* MemoryStrip=NULL;
#ifdef MEMORY_C_SIZE
LINT BmmTotalSize=MEMORY_C_SIZE;
#else
LINT BmmTotalSize=0;
#endif
#else
char MemoryStrip[MEMORY_C_SIZE];
LINT BmmTotalSize=MEMORY_C_SIZE;
#endif

void checkPointer(LINT p)
{
	LINT start=(LINT)MemoryStrip;
	if (!p) return;
	if (p<start || p>=start+BmmTotalSize) {
		PRINTF(LOG_DEV,">>>>>>>>>>>Invalid pointer "LSX"\n", p);
	}
}
BMM* BmmRoot;
BMM* BmmReserve = NULL;
void _bmmDump(BMM* node,int depth)
{
	int n = 0;
	int i;
	BMM* p;
//	PRINTF(LOG_SYS, "malloc> "LSX"\n",node);
	if (!node) return;
//	_myHexDump((char*)node,16,(LINT)node);
	_bmmDump(node->left, depth+1);
	p = node;
	while (p) {
		p = p->next;
		n++;
	}
	for (i = 0; i < depth; i++) PRINTF(LOG_SYS, ".. ");
	PRINTF(LOG_SYS, LSD" x%d\n", BMM_SIZE(node), n);
	_bmmDump(node->right, depth+1);
}
void bmmDump(void) {
	PRINTF(LOG_SYS, "malloc> DUMP:\n");
	_bmmDump(BmmRoot,0);
	PRINTF(LOG_SYS, "malloc> end\n");
}
LINT bmmMaxSize(void) {
	BMM* node = BmmRoot;
	if (!node) return 0;
	while (node->right) node = node->right;
	return BMM_SIZE(node);
}
LINT bmmReservedMem(void) {
	if (!BmmReserve) return 0;
	return BMM_SIZE(BmmReserve);
}
void cMallocInit(void) {
#ifdef MEMORY_C_START
	MemoryStrip=(char*)(MEMORY_C_START);
#endif
	PRINTF(LOG_SYS,"> Memory strip : "LSX"\n",MemoryStrip);
	PRINTF(LOG_SYS,"> Memory end   : "LSX"\n",MemoryStrip+BmmTotalSize);
	PRINTF(LOG_SYS,"> Memory length: "LSD" bytes\n",BmmTotalSize);
	BmmRoot = (BMM*)MemoryStrip;
	HEADER_SET_SIZE_AND_TYPE(BmmRoot, BmmTotalSize, TYPE_FREE);
	BmmRoot->left = BmmRoot->right = BmmRoot->next = NULL;
	BmmReserve = bmmMalloc(MEMORY_SAFE_SIZE);
//	bmmDump();
}
void bmmMayday(void)
{
	if (!BmmReserve) return;
	if (MM.gcTrace) PRINTF(LOG_SYS, "> Memory Mayday! recovering "LSD" bytes\n",BMM_SIZE(BmmReserve));
	bmmFree(BmmReserve);
	BmmReserve = NULL;
}


int _bmmAdd(BMM** parentLink, BMM* nodeToAdd)
{
	LINT size;
	BMM* node = *parentLink;
	if (!node) {
		nodeToAdd->left = nodeToAdd->right = nodeToAdd->next = NULL;
		*parentLink = nodeToAdd;
		return 0;
	}
	size = BMM_SIZE(nodeToAdd);
	if (size > BMM_SIZE(node)) return _bmmAdd(&node->right, nodeToAdd);
	if (size < BMM_SIZE(node)) return _bmmAdd(&node->left, nodeToAdd);
/*	if (((LINT)nodeToAdd) >((LINT)node)) {
		while (node->next && (((LINT)nodeToAdd) > ((LINT)node->next))) node = node->next;
		nodeToAdd->next = node->next;
		node->next = nodeToAdd;
		return 0;
	}
*/	nodeToAdd->left = node->left;
	nodeToAdd->right = node->right;
	node->left = node->right = NULL;
	nodeToAdd->next = node;
	*parentLink = nodeToAdd;
	return 0;
}
/*
int _bmmAdd(BMM** parentLink, BMM* nodeToAdd)
{
	LINT size;
	BMM* node = *parentLink;
	if (!node) {
		nodeToAdd->left = nodeToAdd->right = nodeToAdd->next = NULL;
		*parentLink = nodeToAdd;
		return 0;
	}
	size = BMM_SIZE(nodeToAdd);
	if (size > BMM_SIZE(node)) return _bmmAdd(&node->right, nodeToAdd);
	if (size < BMM_SIZE(node)) return _bmmAdd(&node->left, nodeToAdd);
	nodeToAdd->left = node->left;
	nodeToAdd->right = node->right;
	node->left = node->right = NULL;
	nodeToAdd->next = node;
	*parentLink = nodeToAdd;
	return 0;
}
*/
void bmmRebuildTree(void)
{
	LINT maxSize;
	LINT index = 0;
	BmmRoot = NULL;
	while (index < BmmTotalSize)
	{
		LINT index0 = index;
		BMM* node = (BMM*)&MemoryStrip[index];
		index += BMM_SIZE(node);
		if (HEADER_TYPE(node) != TYPE_FREE) continue;
		while (index < BmmTotalSize)
		{
			BMM* next = (BMM*)&MemoryStrip[index];
			if (HEADER_TYPE(next) != TYPE_FREE) break;
			index += BMM_SIZE(next);
		}
		HEADER_SET_SIZE_AND_TYPE(node,index - index0, TYPE_FREE);
		_bmmAdd(&BmmRoot, node);
	}
	maxSize = bmmMaxSize();
	if (maxSize >= MEMORY_SAFE_SIZE + sizeof(BMM)) maxSize = MEMORY_SAFE_SIZE;
	if ((!BmmReserve) || (maxSize > BMM_SIZE(BmmReserve))) {
		if (BmmReserve) bmmFree(BmmReserve);
		if (MM.gcTrace) PRINTF(LOG_SYS, "> update Mayday block %d\n", maxSize);
		BmmReserve = bmmMalloc(maxSize);
	}
}

BMM* _bmmTakeSmallest(BMM** parentLink)
{
	BMM* node = *parentLink;
	if (node->left) return _bmmTakeSmallest(&node->left);
	*parentLink = node->right;
	return node;
}
BMM* _bmmTakeAtLeast(BMM** parentLink, LINT size, int exact)
{
	BMM* node = *parentLink;
	BMM* smallest;
	if (!node) return NULL;
	if (size > BMM_SIZE(node)) return _bmmTakeAtLeast(&node->right, size, exact);
	if (size < BMM_SIZE(node))
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
void* bmmMalloc(LINT size)
{
	BMM* newBlock;
	BMM* result;
	LINT remainingSize;
	if (size < sizeof(LB)) size = sizeof(LB);
	size = (size + BMM_MASK) & (~BMM_MASK);

	result = _bmmTakeAtLeast(&BmmRoot, size, 1);	// exact
	if (result) {
		HEADER_SET_SIZE_AND_TYPE(result, size - sizeof(LB), TYPE_BINARY);
		return result;
	}
	result = _bmmTakeAtLeast(&BmmRoot, size + sizeof(BMM), 0);	// we need at least extra space for the smallest free block
	if (!result) return NULL;	// should recompute the binary tree
	newBlock = (BMM*)(((char*)result) + size);
	remainingSize = BMM_SIZE(result) - size;
	HEADER_SET_SIZE_AND_TYPE(newBlock, remainingSize, TYPE_FREE);

	_bmmAdd(&BmmRoot, newBlock);
	HEADER_SET_SIZE_AND_TYPE(result, size - sizeof(LB), TYPE_BINARY);
	return (void*)result;
}

int _bmmRemove(BMM** parentLink, BMM* nodeToRemove)
{
	LINT size;
	BMM* node = *parentLink;
	if (!node) return 0;
	if (node == nodeToRemove) {
		node = node->next;
		if (node) {
			node->left = nodeToRemove->left;
			node->right = nodeToRemove->right;
		}
		*parentLink = node;
		return 0;
	}
	size = BMM_SIZE(nodeToRemove);
	if (size > BMM_SIZE(node)) return _bmmRemove(&node->right, nodeToRemove);
	if (size < BMM_SIZE(node)) return _bmmRemove(&node->left, nodeToRemove);
	while (node->next) {
		if (node->next == nodeToRemove) {
			node->next = nodeToRemove->next;
			return 0;
		}
		node = node->next;
	}
	return 0;
}
void bmmFree(void* block)
{
	LINT index, index0;
	BMM* node = (BMM*)block;
	index0 = ((char*)node) - MemoryStrip;
	index= index0 + BMM_SIZE(node);
//	PRINTF(LOG_DEV,"free %x.",(LINT)block);
	while (index < BmmTotalSize) {
		BMM* next = (BMM*)(&MemoryStrip[index]);
		if (HEADER_TYPE(next) != TYPE_FREE) break;
		index += BMM_SIZE(next);
//		printf("f.");
//	PRINTF(LOG_DEV,"remove %x.",(LINT)next);
		_bmmRemove(&BmmRoot, next);
	}
	HEADER_SET_SIZE_AND_TYPE(node, index-index0, TYPE_FREE);
	node->left = node->right = node->next = NULL;
	_bmmAdd(&BmmRoot, node);
}

void* bmmRegularMalloc(LINT size)
{
	char* result;
	if (size < sizeof(LB)) size = sizeof(LB);
	result = bmmMalloc(size+ sizeof(LINT));
	if (!result) return NULL;
	return (void*)(result + sizeof(LINT));
}
void bmmRegularFree(void* block)
{
	char* node = (char*)block;
	bmmFree((void*)(node - sizeof(LINT)));
}

void bmmSetTotalSize(LINT size)
{
	BmmTotalSize=size;
}
LINT bmmTotalSize()
{
	return BmmTotalSize;
}
#endif