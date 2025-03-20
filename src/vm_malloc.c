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

#define BMM_SIZE(p) ((HEADER_TYPE(p)==TYPE_FREE)?(HEADER_SIZE(p)):BLOCK_TOTAL_MEMORY(HEADER_TYPE(p), HEADER_SIZE(p)))

#define BMM_MASK (LWLEN-1)

#ifdef MEMORY_C_START
char* MemoryStrip=(char*)(MEMORY_C_START);
#ifdef MEMORY_C_SIZE
LINT BmmTotalSize=MEMORY_C_SIZE;
#else
LINT BmmTotalSize=0;
#endif
#else
char _MemoryStrip[MEMORY_C_SIZE];
char* MemoryStrip=_MemoryStrip;
LINT BmmTotalSize=MEMORY_C_SIZE;
#endif
LINT BmmTotalFree = 0;
LINT BmmMaxSize=0;

char* bmmAllocForEver(LINT size)
{
	size= (((size)+ LWLEN_MASK)&~LWLEN_MASK);
	BmmTotalSize-=size;
	return &MemoryStrip[BmmTotalSize];
}
int checkPointer(LINT p)
{
	LINT start=(LINT)MemoryStrip;
	if (!p) return 1;
	if (p<start || p>=start+BmmTotalSize) {
		PRINTF(LOG_DEV,">>>>>>>>>>>Invalid pointer "LSX"\n", p);
		return 0;
	}
	return 1;
}
LINT relativePointer(LB* p)
{
	return (LINT)(((char*)p) - MemoryStrip);
}
int checkListPointer(char* title, LB* p)
{
	LB* pLast;

	PRINTF(LOG_DEV, ">>>>>>>>>>>Check %s pointers from "LSX"\n", title, (LINT)p);
	pLast = p;
	while (p) {
		if (!checkPointer((LINT)p)) {
			PRINTF(LOG_DEV, "error in "LSX"\n", relativePointer(pLast));
			PRINTF(LOG_DEV, "stopped\n");
			return 0;
		}
//		if (MM.gcTrace > 1) printf(LSX":%d(%d). ", relativePointer(p), (INT_FROM_VAL(HEADER_DBG(p)) - 1) / 2, HEADER_SIZE(p));
		pLast = p;
		p = p->nextBlock;
	}
	return 1;
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
void bmmComputeMaxSize(void) {
	BMM* node = BmmRoot;
	BmmMaxSize = 0;
	if (!node) return;
	while (node->right) node = node->right;
	BmmMaxSize= BMM_SIZE(node);
//	if (MM.gcTrace) PRINTF(LOG_SYS, "> GC: Larger block: %d\n", BmmMaxSize);
}
LINT bmmReservedMem(void) {
	if (!BmmReserve) return 0;
	return BMM_SIZE(BmmReserve);
}
void cMallocInit(void) {
	PRINTF(LOG_SYS,"> Memory strip : "LSX"\n",MemoryStrip);
	PRINTF(LOG_SYS,"> Memory end   : "LSX"\n",MemoryStrip+BmmTotalSize);
	PRINTF(LOG_SYS,"> Memory length: "LSD" bytes\n",BmmTotalSize);
	BmmRoot = (BMM*)MemoryStrip;
	HEADER_SET_SIZE_AND_TYPE(BmmRoot, BmmTotalSize, TYPE_FREE);
	BmmRoot->left = BmmRoot->right = BmmRoot->next = NULL;
	BmmTotalFree = BmmMaxSize= BmmTotalSize;
	BmmReserve = bmmMalloc(MEMORY_SAFE_SIZE);
//	bmmDump();
}
void bmmMayday(void)
{
	if (!BmmReserve) return;
	if (MM.gcTrace) PRINTF(LOG_SYS, "> GC: Memory Mayday! use reserve of "LSD" bytes\n",BMM_SIZE(BmmReserve));
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
/*	// insert sort
	if (((LINT)nodeToAdd) >((LINT)node)) {
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

void bmmRebuildTree(void)
{
	LINT maxSize;
	LINT index = 0;
	BmmTotalFree = 0;
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
		BmmTotalFree += index - index0;

	}
	bmmComputeMaxSize();
	maxSize = BmmMaxSize;
	if (maxSize >= MEMORY_SAFE_SIZE + sizeof(BMM)) maxSize = MEMORY_SAFE_SIZE;
	if ((!BmmReserve) || (maxSize > BMM_SIZE(BmmReserve))) {
		if (BmmReserve) bmmFree(BmmReserve);
		if (MM.gcTrace) PRINTF(LOG_SYS, "> GC: update Mayday block %d\n", maxSize);
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
//	printf("alloc %lld. ",size);
	result = _bmmTakeAtLeast(&BmmRoot, size, 1);	// exact
	if (result) {
		HEADER_SET_SIZE_AND_TYPE(result, size - sizeof(LB), TYPE_BINARY);
		BmmTotalFree -= size;
		bmmComputeMaxSize();
		return result;
	}
	result = _bmmTakeAtLeast(&BmmRoot, size + sizeof(BMM), 0);	// we need at least extra space for the smallest free block
	if (!result) return NULL;	// should recompute the binary tree
	newBlock = (BMM*)(((char*)result) + size);
	remainingSize = BMM_SIZE(result) - size;
	HEADER_SET_SIZE_AND_TYPE(newBlock, remainingSize, TYPE_FREE);

	_bmmAdd(&BmmRoot, newBlock);
	HEADER_SET_SIZE_AND_TYPE(result, size - sizeof(LB), TYPE_BINARY);
	BmmTotalFree -= size;
	bmmComputeMaxSize();
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
			*parentLink = node;
		}
		else if (!nodeToRemove->left) *parentLink = nodeToRemove->right;
		else if (!nodeToRemove->right) *parentLink = nodeToRemove->left;
		else {
			*parentLink = nodeToRemove->right;
			node = nodeToRemove->right;
			while (node->left) node = node->left;
			node->left = nodeToRemove->left;
		}
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
	BmmTotalFree += BMM_SIZE(node);

	while (index < BmmTotalSize) {
		BMM* next = (BMM*)(&MemoryStrip[index]);
		if (HEADER_TYPE(next) != TYPE_FREE) break;
		index += BMM_SIZE(next);
		_bmmRemove(&BmmRoot, next);
	}
//	printf("free %lld. ", index - index0);

	HEADER_SET_SIZE_AND_TYPE(node, index-index0, TYPE_FREE);
	node->left = node->right = node->next = NULL;
	_bmmAdd(&BmmRoot, node);
	bmmComputeMaxSize();
}

void bmmSetTotalSize(LINT size)
{
	BmmTotalSize=size;
}

LINT bmmCompact(void)
{
	LINT index, indexAfterMove;
	if (MM.gcTrace) PRINTF(LOG_SYS, "> GC: compacting memory...\n");
	while (memoryGetFast()) memoryLeaveFast();
	memoryFinalizeGC();
	memoryFinalizeGC();

	if (BmmReserve) bmmFree(BmmReserve);
	BmmReserve = NULL;

	workerWaitUntilAllInactive();

	lockEnter(&MM.lock);
	while (MM.lifo) {
		LB* p = MM.lifo->lifo;
		MM.lifo->lifo = MM.USELESS;
		MM.lifo = p;
	}

	indexAfterMove = index = 0;
	while (index < BmmTotalSize)
	{
		LB* node = (LB*)&MemoryStrip[index];
		LINT size= BMM_SIZE((BMM*)node);
		index += size;
		if (HEADER_TYPE(node) == TYPE_FREE) continue;
		node->lifo=(LB*)&MemoryStrip[indexAfterMove];
		indexAfterMove += size;
	}

	// updating pointers
	MM.updating = 1;
	index = 0;
	while (index < BmmTotalSize)
	{
		LB* node = (LB*)&MemoryStrip[index];
		LINT size = BMM_SIZE((BMM*)node);
		int type = HEADER_TYPE(node);
		index += size;

		if (type == TYPE_FREE) continue;
		MEMORY_MARK(node->nextBlock);
		MEMORY_MARK(node->pkg);

		if ((type == TYPE_ARRAY) && (HEADER_DBG(node) != DBG_STACK)) {
			LINT i, l;
			l = ARRAY_LENGTH(node);
			for (i = 0; i < l; i++) if (ARRAY_IS_PNT(node, i)) {
				LB* pnt=ARRAY_PNT(node, i);
				MEMORY_MARK(pnt);
				ARRAY_GET(node, i)=VAL_FROM_PNT(pnt);
			}
		}
		else if (type == TYPE_NATIVE)
		{
			void** q = (void**)&node->data[1];	// complicated because of the 32/64 bits compatibility
			MARK mark = (MARK)q[NATIVE_MARK];
			if (mark) (*mark)(node);
		}
		if (DBG_IS_PNT(node->data[0])) node->data[0] = VAL_FROM_PNT(PNT_FROM_VAL(node->data[0])->lifo);
	}

	MEMORY_MARK(MM.system);
	MEMORY_MARK(MM.scheduler);
	MEMORY_MARK(MM.tmpStack);
	MEMORY_MARK(MM.tmpBuffer);
	MEMORY_MARK(MM.args);
	MEMORY_MARK(MM.fun_u0_list_u0_list_u0);
	MEMORY_MARK(MM.fun_array_u0_I_u0);
	MEMORY_MARK(MM.funStart);
	MEMORY_MARK(MM.roots);
	MEMORY_MARK(MM.popOblivions);
	MEMORY_MARK(MM.listOblivions);
	MEMORY_MARK(MM.tmpRoot);
	MEMORY_MARK(MM.partitionsFS);
	romdiskMark(NULL);

	MEMORY_MARK(MM.listBlocks);
	MEMORY_MARK(MM.listCheck);
	MEMORY_MARK(MM.listFast);

	MEMORY_MARK(MM.listThreads);
	MEMORY_MARK(MM.listPkgs);
	MEMORY_MARK(MM.currentPkg);

	MEMORY_MARK(MM._true);
	MEMORY_MARK(MM._false);

	MEMORY_MARK(MM.ansiVolume);
	MEMORY_MARK(MM.romdiskVolume);

	MEMORY_MARK(MM.Int);
	MEMORY_MARK(MM.Float);
	MEMORY_MARK(MM.Str);
	MEMORY_MARK(MM.Bytes);
	MEMORY_MARK(MM.Boolean);
	MEMORY_MARK(MM.BigNum);
	MEMORY_MARK(MM.Package);
	MEMORY_MARK(MM.Type);

	MEMORY_MARK(FM.READ_ONLY);
	MEMORY_MARK(FM.REWRITE);
	MEMORY_MARK(FM.READ_WRITE);
	MEMORY_MARK(FM.APPEND);
	MM.updating = 0;

	index = 0;
	while (index < BmmTotalSize)
	{
		LB* newNode;
		LB* node = (LB*)&MemoryStrip[index];
		LINT size = BMM_SIZE((BMM*)node);
		index += size;
		if (HEADER_TYPE(node) == TYPE_FREE) continue;
		newNode = node->lifo;
		if (node != newNode) {
			LINT* src = (LINT*)node;
			LINT* dst = (LINT*)newNode;
			size >>= LSHIFT;
			while (size--) *(dst++) = *(src++);
		}
		newNode->lifo = _USELESS;
	}
	BmmRoot =(BMM*)&MemoryStrip[indexAfterMove];
	HEADER_SET_SIZE_AND_TYPE(BmmRoot, BmmTotalSize- indexAfterMove, TYPE_FREE);
	BmmRoot->left = BmmRoot->right = BmmRoot->next = NULL;
	BmmTotalFree = BmmMaxSize = BmmTotalSize - indexAfterMove;
	lockLeave(&MM.lock);

	if (MM.gcTrace) PRINTF(LOG_SYS, "> GC: defragmented %d free bytes in one single block\n", BmmMaxSize);
	BmmReserve = bmmMalloc(MEMORY_SAFE_SIZE);
	if (MM.gcTrace && BmmReserve) PRINTF(LOG_SYS, "> GC: restored a reserve of %d bytes\n", MEMORY_SAFE_SIZE);
	bmmComputeMaxSize();
	if (MM.gcTrace) PRINTF(LOG_SYS, "> GC: Larger block: %d\n", BmmMaxSize);
	return BmmMaxSize;
}
#endif