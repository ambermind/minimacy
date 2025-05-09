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

#define BMM_SIZE(p) ((LINT)((HEADER_TYPE(p)==TYPE_FREE)?(HEADER_SIZE(p)):BLOCK_TOTAL_MEMORY(HEADER_TYPE(p), HEADER_SIZE(p))))

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
BMM* BmmRoot;
BMM* BmmReserve = NULL;
int BmmWrongPointer;

char* bmmAllocForEver(LINT size)
{
	size= (((size)+ LWLEN_MASK)&~LWLEN_MASK);
	BmmTotalSize-=size;
	return &MemoryStrip[BmmTotalSize];
}

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
	nodeToAdd->left = node->left;
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
void bmmCheckOrMoveMM()
{
	MARK_OR_MOVE(MM.system);
	MARK_OR_MOVE(MM.scheduler);
	MARK_OR_MOVE(MM.tmpStack);
	MARK_OR_MOVE(MM.tmpBuffer);
	MARK_OR_MOVE(MM.args);
	MARK_OR_MOVE(MM.fun_u0_list_u0_list_u0);
	MARK_OR_MOVE(MM.fun_array_u0_I_u0);
	MARK_OR_MOVE(MM.funStart);
	MARK_OR_MOVE(MM.roots);
	MARK_OR_MOVE(MM.popOblivions);
	MARK_OR_MOVE(MM.listOblivions);
	MARK_OR_MOVE(MM.tmpRoot);
	MARK_OR_MOVE(MM.partitionsFS);
	romdiskMark(NULL);

	MARK_OR_MOVE(MM.listMark);
	MARK_OR_MOVE(MM.listBlocks);
	MARK_OR_MOVE(MM.listCheck);
	MARK_OR_MOVE(MM.listSafe);

	MARK_OR_MOVE(MM.listThreads);
	MARK_OR_MOVE(MM.listPkgs);
	MARK_OR_MOVE(MM.currentPkg);

	MARK_OR_MOVE(MM._true);
	MARK_OR_MOVE(MM._false);
	MARK_OR_MOVE(MM.loopMark);
	MARK_OR_MOVE(MM.abortMark);

	MARK_OR_MOVE(MM.ansiVolume);
	MARK_OR_MOVE(MM.romdiskVolume);

	MARK_OR_MOVE(MM.Int);
	MARK_OR_MOVE(MM.Float);
	MARK_OR_MOVE(MM.Str);
	MARK_OR_MOVE(MM.Bytes);
	MARK_OR_MOVE(MM.Boolean);
	MARK_OR_MOVE(MM.BigNum);
	MARK_OR_MOVE(MM.Package);
	MARK_OR_MOVE(MM.Type);

	MARK_OR_MOVE(FM.READ_ONLY);
	MARK_OR_MOVE(FM.REWRITE);
	MARK_OR_MOVE(FM.READ_WRITE);
	MARK_OR_MOVE(FM.APPEND);
}
LINT bmmCompact(void)
{
	LINT index, indexAfterMove;
	HashSlots* hashslots = NULL;

	if (MM.gcTrace) PRINTF(LOG_SYS, "> GC: compacting memory...\n");
	while (MM.safeAlloc) memoryLeaveSafe();
	memoryFinalizeGC();
	memoryFinalizeGC();

	if (BmmReserve) bmmFree(BmmReserve);
	BmmReserve = NULL;

	workerWaitUntilAllInactive();

	lockEnter(&MM.lock);
	while (MM.listMark) {
		LB* p = MM.listMark->listMark;
		MM.listMark->listMark = MM.USELESS;
		MM.listMark = p;
	}

	indexAfterMove = index = 0;
	while (index < BmmTotalSize)
	{
		LB* node = (LB*)&MemoryStrip[index];
		LINT size= BMM_SIZE((BMM*)node);
		index += size;
		if (HEADER_TYPE(node) == TYPE_FREE) continue;
		node->listMark=(LB*)&MemoryStrip[indexAfterMove];
		indexAfterMove += size;
	}

	MM.blockOperation=MEMORY_MOVE;
	index = 0;
	while (index < BmmTotalSize)
	{
		LB* node = (LB*)&MemoryStrip[index];
		LINT size = BMM_SIZE((BMM*)node);
		int type = HEADER_TYPE(node);
		index += size;

		if (type == TYPE_FREE) continue;
		MARK_OR_MOVE(node->nextBlock);
		MARK_OR_MOVE(node->pkg);

		if ((type == TYPE_ARRAY) && (HEADER_DBG(node) != DBG_STACK)) {
			LINT i, l;
			l = ARRAY_LENGTH(node);
			for (i = 0; i < l; i++) if (ARRAY_IS_PNT(node, i)) {
				LB* pnt=ARRAY_PNT(node, i);
				MARK_OR_MOVE(pnt);
				ARRAY_GET(node, i)=VAL_FROM_PNT(pnt);
			}
		}
		else if (type == TYPE_NATIVE)
		{
			void** q = (void**)&node->data[1];	// complicated because of the 32/64 bits compatibility
			MARK mark = (MARK)q[NATIVE_MARK];
			if (mark) (*mark)(node);
		}
		if (DBG_IS_PNT(node->data[0])) node->data[0] = VAL_FROM_PNT(PNT_FROM_VAL(node->data[0])->listMark);
	}
	bmmCheckOrMoveMM();
	MM.blockOperation = MEMORY_MARK;

	index = 0;
	while (index < BmmTotalSize)
	{
		LB* newNode;
		LB* node = (LB*)&MemoryStrip[index];
		LINT size = BMM_SIZE((BMM*)node);
		index += size;
		if (HEADER_TYPE(node) == TYPE_FREE) continue;
		newNode = node->listMark;
		if (node != newNode) {
			LINT* src = (LINT*)node;
			LINT* dst = (LINT*)newNode;
			size >>= LSHIFT;
			while (size--) *(dst++) = *(src++);
		}
		if ((HEADER_DBG(newNode) == DBG_HASHSET) || (HEADER_DBG(newNode) == DBG_HASHMAP)) {
			HashSlots* h = (HashSlots*)newNode;
			h->save = (LB*)hashslots;
			hashslots = h;
		}
		newNode->listMark = MM.USEFUL;
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
//	memoryCheck(1); 
	while (hashslots) {
		HashSlots* next = (HashSlots * )hashslots->save;
		hashSlotsRecompute(hashslots);
		hashslots = next;
	}
	return BmmMaxSize;
}
int checkPointer(LB* p)
{
	LINT index = ((LINT)p) - ((LINT)MemoryStrip);
	if (!p) return 1;
	if ((index < 0) || ((index+4*((LINT)sizeof(LB*))) >= BmmTotalSize) ) {
		PRINTF(LOG_USER, "> Invalid pointer "LSX" (index "LSD")\n", (LINT)p,index);
		goto fault;
	}
	index = 0;
	while (index < BmmTotalSize)
	{
		LB* node = (LB*)&MemoryStrip[index];
		LINT size = BMM_SIZE((BMM*)node);
		if (node == p) return 1;
		if (size <= 0) {
			PRINTF(LOG_USER, "> Invalid block "LSX" (index "LSD") size=%d\n", (LINT)node, index, size);
			goto fault;
		}
		index += size;
	}
	index = ((LINT)p) - ((LINT)MemoryStrip);
	PRINTF(LOG_USER, "> Invalid pointer "LSX" (index "LSD")\n", (LINT)p, index);
fault:
	BmmWrongPointer++;
	PRINTF(LOG_USER, "> you may put a breakpoint here\n");
	return 0;
}
int memoryCheckStrip(void)
{
	LINT index = 0;
	while (index < BmmTotalSize)
	{
		LB* node = (LB*)&MemoryStrip[index];
		LINT size = BMM_SIZE((BMM*)node);
		if (size <= 0) {
			PRINTF(LOG_USER, "> Invalid block "LSX" (index "LSD") size=%d\n", (LINT)node, index, size);
			return 0;
		}
		index += size;
	}
	return 1;
}

int memoryCheckTree(BMM* node,LINT minSize, LINT maxSize)
{
	LINT size;
	BMM* nxt;
	if (!node) return 1;
	if (!checkPointer((LB*)node)) return 0;
	size = BMM_SIZE((BMM*)node);
	if (size <= minSize) {
		PRINTF(LOG_USER, "> Invalid tree block "LSX" wrong size %d is not in ]%d, %d[\n", (LINT)node, minSize, maxSize);
		return 0;
	}
	if ((size >= maxSize) && (maxSize!=-1)) {
		PRINTF(LOG_USER, "> Invalid tree block "LSX" wrong size %d is not in ]%d, %d[\n", (LINT)node, minSize, maxSize);
		return 0;
	}
	for (nxt = node->next; nxt; nxt = nxt->next) if (size != BMM_SIZE(nxt)) {
		PRINTF(LOG_USER, "> Invalid tree block "LSX" wrong size %d (instead of %d)\n", (LINT)nxt, BMM_SIZE(nxt), size);
		return 0;
	}
	if (!memoryCheckTree(node->left,minSize,size)) return 0;
	return (memoryCheckTree(node->right,size,maxSize));
}
int memoryFindTree(BMM* node, BMM* find)
{
	LINT size;
	LINT sizeFind = BMM_SIZE((BMM*)find);
	if (!node) return 0;
	size = BMM_SIZE((BMM*)node);
	if (sizeFind < size) return memoryFindTree(node->left, find);
	if (sizeFind > size) return memoryFindTree(node->right, find);
	while (node) {
		if (node == find) return 1;
		node = node->next;
	}
	return 0;
}
int memoryCheckMM(void)
{
	BmmWrongPointer = 0;
	MM.blockOperation = MEMORY_VALIDATE;
	bmmCheckOrMoveMM();
	MM.blockOperation = MEMORY_MARK;
	if (BmmWrongPointer) {
		PRINTF(LOG_USER, "> %d Invalid pointer(s) in MM\n", BmmWrongPointer);
		return 0;
	}
	return 1;
}
int memoryCheckBlocks(void)
{
	LINT index = 0;
	while (index < BmmTotalSize)
	{
		LB* node = (LB*)&MemoryStrip[index];
		LINT size = BMM_SIZE((BMM*)node);
		LINT type = HEADER_TYPE(node);
		if (type == TYPE_FREE) {
			if (!memoryFindTree(BmmRoot, (BMM*)node)) {
				PRINTF(LOG_USER, "> Free block "LSX" (index "LSD") is not in the tree\n", (LINT)node, index);
			}
		}
		else if ((MM.gcStage != GC_STAGE_SWEEP) || (node->listMark != MM.USELESS)) {
			BmmWrongPointer = 0;
			MM.blockOperation = MEMORY_VALIDATE;
			if (type == TYPE_ARRAY) {
				if (HEADER_DBG(node) != DBG_STACK)	// stack content is checked by threadMark
				{
					LINT i, l;
					l = ARRAY_LENGTH(node);
					for (i = 0; i < l; i++) if (ARRAY_IS_PNT(node, i)) {
						LB* pnt=ARRAY_PNT(node, i);
						MARK_OR_MOVE(pnt);
					}
				}
			}
			else if (type == TYPE_NATIVE) {
				void** q = (void**)&node->data[1];	// complicated because of the 32/64 bits compatibility
				MARK mark = (MARK)q[NATIVE_MARK];
				if (mark) (*mark)(node);
			}
			MM.blockOperation = MEMORY_MARK;
			if (BmmWrongPointer) {
				PRINTF(LOG_USER, "> %d Invalid pointer(s) in block "LSX" (index "LSD") type=%d debug=%d size=%d\n", BmmWrongPointer, (LINT)node, index, type, HEADER_DBG(node), size);
				_myHexDump((char*)node, (int)size, (LINT)node);
			}
		}
		index += size;
	}
	return 1;
}
int memoryCheck(int debug)
{
	if (debug) PRINTF(LOG_USER, "> memoryCheck\n> -----------\n");

	if (debug) PRINTF(LOG_USER, "> checking strip\n");
	if (!memoryCheckStrip()) return 0;

	if (debug) PRINTF(LOG_USER, "> checking tree\n");
	if (!memoryCheckTree(BmmRoot,-1,-1)) return 0;

	if (debug) PRINTF(LOG_USER, "> checking MM pointers\n");
	if (!memoryCheckMM()) return 0;

	if (debug) PRINTF(LOG_USER, "> checking blocks\n");
	if (!memoryCheckBlocks()) return 0;

	if (debug) PRINTF(LOG_USER, "> memoryCheck done.\n");
	return 1;
}
#endif