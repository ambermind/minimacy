// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"


Memory MM;

void memoryOptimize(int nbGc)
{
	int i;
	for (i = 0; i < nbGc; i++) memoryFinalizeGC();
#ifdef USE_MEMORY_C
	bmmRebuildTree();
	if (MM.gcTrace) PRINTF(LOG_SYS, "> GC: Larger block: %d\n", BmmMaxSize);
#endif
}

int memoryTry(LINT size)
{
#ifdef USE_MEMORY_C
	LINT total = BLOCK_TOTAL_MEMORY(TYPE_BINARY, size) + sizeof(BMM);
	if (total <= BmmMaxSize) return 1;
	memoryOptimize(1);
	if (total <= BmmMaxSize) return 1;
	memoryOptimize(1);
	if (total <= BmmMaxSize) return 1;
	bmmMayday();
	if (total <= BmmMaxSize) {
		bmmRebuildTree();
		return 1;
	}
	bmmRebuildTree();
	return 0;
#else
	LINT total = BLOCK_TOTAL_MEMORY(TYPE_BINARY, size);
	LB* p = (LB*)VM_MALLOC(total);
	if (!p) {
		memoryOptimize(1);
		p = (LB*)VM_MALLOC(total);
		if (!p) {
			memoryOptimize(1);
			p = (LB*)VM_MALLOC(total);
		}
	}
	if (!p) return 0;
	VM_FREE((void*)p);
	return 1;
#endif
}


LB* memoryAlloc(LINT size,LINT type,LW dbg)
{
	LB *p=NULL;
	LINT total = BLOCK_TOTAL_MEMORY(type, size);

	//if ((*runtimeCheckAddress)==runtimeCheckValue) {
	//	PRINTF(LOG_SYS,"address changed");
	//}

//	if ((MM.blocs_length > MEMORY_SAFE_SIZE) && (MM.blocs_nb > (MM.gc_nb0 << 1))) memoryOptimize(1);

	p=(LB*)VM_MALLOC(total);
//	PRINTF(LOG_DEV,">>>>>>malloc %d bytes -> %llx\n", (int)total, p);
	if (!p) 
	{
//		PRINTF(LOG_DEV, ">>>>>>malloc %d bytes\n", (int)total);
		memoryOptimize(1);
		p = (LB*)VM_MALLOC(total);
		if (!p) {
			memoryOptimize(1);
			p = (LB*)VM_MALLOC(total);
#ifdef USE_MEMORY_C
			if (!p) {
				bmmMayday();
				p = (LB*)VM_MALLOC(total);
//				if (!p) bmmDump();
			}
#endif
			if (!p) {
				MM.OM = 1;
				//				PRINTF(LOG_DEV,"x");
				
				PRINTF(LOG_SYS, "> GC: malloc " LSD " returns null\n", size);
				//				threadDump(LOG_SYS, th, 10);
				goto cleanup;
			}
		}
	}
//	PRINTF(LOG_DEV, "%d,"LSX","LSD" ", (int)total, p, (INT_FROM_VAL(HEADER_DBG(p)) - 1) / 2);

	HEADER_SET_SIZE_AND_TYPE(p,size,type);
	p->pkg = MM.currentPkg;
	p->data[0]=dbg;

	if (MM.gcStage == GC_STAGE_SWEEP) p->listMark = MM.USEFUL;
	else {
		p->listMark = MM.listMark; MM.listMark = p;
	}

	if (MM.safeAlloc>0)
	{
		p->nextBlock=MM.listSafe;
		MM.listSafe=p;
	}
	else
	{
		p->nextBlock=MM.listBlocks;
		MM.listBlocks=p;
	}
	MM.blocs_nb++;
	MM.blocs_length+=total;

	MM.gc_period_counter--;
	if (MM.gc_period_counter <= 0) {
		LINT newPeriod;
		float per, tau;
		tau = GC_PERIOD_COUNT;
		tau /= MM.gc_period_time;

		if (tau) per = 0.2929f / tau;	// 0.2929= 1-sqrt(1-0.5) : keep f around 0.5*N0 (use 0.5 to keep f arount 0.75*N0)
		else per = 100;
		newPeriod = (LINT)per;
		if (newPeriod > 1000) newPeriod = 1000;
		else if (newPeriod < 10) newPeriod = 10;
		if (newPeriod != MM.gc_period) {
			if (newPeriod > MM.gc_period) newPeriod = newPeriod - ((newPeriod - MM.gc_period) >> 1);
//			if (MM.gcTrace && ((newPeriod< MM.gc_period-1) || (newPeriod > MM.gc_period +1))) PRINTF(LOG_SYS,"> GC: update period "LSD"->"LSD"\n",MM.gc_period,newPeriod);
			MM.gc_period = newPeriod;
		}
		MM.gc_period_counter = GC_PERIOD_COUNT;
		MM.gc_period_time = 0;
	}
cleanup:
	return p;
}

LB* memoryAllocArray(LINT size, LW dbg)
{
	LB* p=memoryAlloc(size<< LSHIFT, TYPE_ARRAY, dbg);
	if (!p) return NULL;
#ifdef USE_ALL_BITS
	memset(BIN_START(p), 0, size + (size << LSHIFT));
#else
	{
		LINT i;
		for (i = 0; i < size; i++) ARRAY_SET_NIL(p, i);
	}
#endif
	return p;
}

LB* memoryAllocNative(LINT sizeofExt,LW dbg,FORGET forget,MARK mark)
{
	void** q;
	LB* p=memoryAlloc(sizeofExt- sizeof(LB),TYPE_NATIVE,dbg);
	if (!p) return NULL;
	memset((void*)(&p->data[1]),0,sizeofExt-sizeof(LB));

	q =(void**) &p->data[1];
	q[0]=forget;
	q[1]=mark;
	return p;
}

LB* memoryAllocStr(char* src, LINT size)
{
	LB* p;
	if (size < 0) {
		if (!src) return NULL;
		size = strlen(src);
	}
	
	p=memoryAlloc(size+1,TYPE_BINARY,DBG_S);
	if (!p) return NULL;
	if (src) memcpy(STR_START(p),src,size);
	STR_START(p)[size]=0;
	return p;
}

LB* memoryAllocBin(char* src, LINT size,LW dbg)
{
	LB* p=memoryAlloc(size,TYPE_BINARY,dbg);
	if (!p) return NULL;
	if (src) memcpy(BIN_START(p),src,size);
	return p;
}

LB* memoryAllocFromBuffer(Buffer* b)
{
	return memoryAllocStr(bufferStart(b), bufferSize(b));
}

char* errorName(int err)
{
	if (err==COMPILER_ERR_SN) return "Syntax error";
	else if (err==COMPILER_ERR_TYPE) return "Typechecking error";
	else return "Unknown error";
}

void memoryDesalloc(LB* p)
{
	LINT size=HEADER_SIZE(p);
	LINT type = HEADER_TYPE(p);
	LINT total = BLOCK_TOTAL_MEMORY(type, size);

	if (type==TYPE_NATIVE)
	{
		void** q = (void**)&p->data[1];	// complicated because of the 32/64 bits compatibility
		FORGET forget = (FORGET)q[NATIVE_FORGET];
		if (forget && (*forget)(p)) goto cleanup;
	}
	MM.blocs_nb--;
	MM.blocs_length-=total;
	
//	PRINTF(LOG_DEV,"free %llx\n", p);
	VM_FREE((void*)p);
cleanup:
	return;
}
void memoryGCinit(void)
{
	LB* p;
	MM.gcStage = GC_STAGE_MARK;
	MM.listCheck = MM.listBlocks;
	MM.listBlocks = NULL;
	MM.gc_nb0 = MM.blocs_nb;
	MM.gc_free = 0;

	p = MM.USEFUL; MM.USEFUL = MM.USELESS; MM.USELESS = p;

	BLOCK_MARK(MM.system);
	BLOCK_MARK(MM.scheduler);
	BLOCK_MARK(MM.tmpStack);
	BLOCK_MARK(MM.tmpBuffer);
	BLOCK_MARK(MM.args);
	BLOCK_MARK(MM.fun_u0_list_u0_list_u0);
	BLOCK_MARK(MM.fun_array_u0_I_u0);
	BLOCK_MARK(MM.funStart);
	BLOCK_MARK(MM.roots);
	BLOCK_MARK(MM.popOblivions);
	BLOCK_MARK(MM.tmpRoot);
	BLOCK_MARK(MM.partitionsFS);
	romdiskMark(NULL);
	p = MM.listSafe;
	while (p)
	{
		if (HEADER_DBG(p) != DBG_STACK) BLOCK_MARK(p);
		p = p->nextBlock;
	}
}
int memoryCleanOblivions(void)
{
	int result = 0;
	Oblivion** previous = &MM.listOblivions;
	while (*previous)
	{
		Oblivion* ob = *previous;
		if (ob->header.listMark == MM.USELESS)
		{
			*previous = ob->listNext;
			if (ob->f) {
				result = 1;
				ob->header.listMark = MM.listMark;
				MM.listMark = (LB*)ob;
				ob->popNext = MM.popOblivions;
				MM.popOblivions = ob;
			}
			ob->listNext = NULL;
		}
		else previous = &ob->listNext;
	}
	return result;
}
void memoryCleanThreads(void)
{
	Thread** previous = &MM.listThreads;
	while (*previous)
	{
		Thread* th = *previous;
		if (th->header.listMark == MM.USELESS)
		{
			*previous = th->listNext;
			th->listNext = NULL;	// useless yet clean
		}
		else previous = &th->listNext;
	}
}
void memoryCleanPkgs(void)
{
	Pkg** previous = &MM.listPkgs;
	while (*previous)
	{
		Pkg* p = *previous;
		if (p->header.listMark == MM.USELESS)
		{
			*previous = p->listNext;
			p->listNext = NULL;	// useless yet clean
		}
		else previous = &p->listNext;
	}
}

void memoryGC(LINT period)
{
	MM.gc_period_time += period;
//	PRINTF(LOG_DEV,LSD,MM.gcStage);
	if (MM.gcStage== GC_STAGE_MARK)
	{
//		PRINTF(LOG_DEV,"$");
		if (MM.listMark)
		{
			LB* p=MM.listMark; MM.listMark=p->listMark;
			p->listMark=MM.USEFUL;

			BLOCK_MARK(p->pkg);
			if ((HEADER_TYPE(p)==TYPE_ARRAY)&&(HEADER_DBG(p)!= DBG_STACK))	// stack content is marked by threadMark
			{
				LINT i,l;
				l=ARRAY_LENGTH(p);
				for(i=0;i<l;i++) if (ARRAY_IS_PNT(p,i)) BLOCK_MARK(ARRAY_PNT(p,i));
			}
			else if (HEADER_TYPE(p)==TYPE_NATIVE)
			{
				void** q = (void**)&p->data[1];	// complicated because of the 32/64 bits compatibility
				MARK mark = (MARK)q[NATIVE_MARK];
				if (mark) (*mark)(p);
			}

		}
		else if (!memoryCleanOblivions())
		{
			memoryCleanThreads();
			memoryCleanPkgs();
//			threadDumpLoop("go step 2");
			MM.gcStage= GC_STAGE_SWEEP;
		}
	}
	else if (MM.gcStage== GC_STAGE_SWEEP)
	{
//		PRINTF(LOG_DEV,".");
//		PRINTF(LOG_DEV,"GC2 %llx.", MM.listCheck);
		if (MM.listCheck)
		{
			LB* p=MM.listCheck;
			MM.listCheck=MM.listCheck->nextBlock;
			if (p->listMark!=MM.USELESS)
			{
				p->nextBlock=MM.listBlocks;
				MM.listBlocks=p;
			}
			else
			{
				MM.gc_free++;
/*				if (runtimeCheckAddress==p) {
					PRINTF(LOG_DEV,"WRONG desalloc %llx\n",p);
					itemHeader(LOG_SYS,p);
				}
*/
//				if (HEADER_DBG(p) == DBG_IMPORTS) PRINTF(LOG_DEV,"Release DBG_IMPORTS %llx\n", p);
				memoryDesalloc(p);
			}
		}
		else
		{
			MM.gc_count++;
			if (MM.gcTrace)
			{
				int useMB = (MM.blocs_length >= 1024 * 1024 * 10);
				PRINTF(LOG_SYS, "> GC: #"LSD" freed " LSD " of " LSD " blocks, still using "LSD "%s\n", 
					MM.gc_count, MM.gc_free, MM.gc_nb0, MM.blocs_length >> (useMB?20:10),useMB?"M":"k");
			}
			MM.gcStage = GC_STAGE_INIT;
		}
	}
	else memoryGCinit();
}

void memoryFinalizeGC(void)
{
	if (MM.gcTrace) PRINTF(LOG_DEV,"> GC: waiting for GC ending\n");
//	memoryCheck(1);
	if (MM.gcStage== GC_STAGE_INIT) memoryGCinit();
	if (MM.gcStage== GC_STAGE_MARK) while(MM.gcStage== GC_STAGE_MARK) memoryGC(0);
	while(MM.gcStage== GC_STAGE_SWEEP) memoryGC(0);
}

void memoryRecount(void)
{
	int i;
	LB* p;
	Pkg* pkg;
	LINT slots[32];
	for (i = 0; i < 32; i++) slots[i] = 0;

	memoryFinalizeGC();
//	if (MM.gcTrace) PRINTF(LOG_SYS, "> number of bios definitions=%d\n", MM.system->defs->nb);

	for (pkg = MM.listPkgs; pkg; pkg = pkg->listNext) pkg->memory = 0;

	for(i=0;i<2;i++) {
		p = i?MM.listBlocks:MM.listSafe;
		while (p) {
			LINT total = BLOCK_TOTAL_MEMORY(HEADER_TYPE(p), HEADER_SIZE(p));
			if (DBG_IS_PNT(HEADER_DBG(p))) slots[31] += total;
			else slots[(INT_FROM_VAL(HEADER_DBG(p)) - 1) / 2] += total;
//			if (total >= 500) {
//				printf("recount "LSD" ("LSD")\n", total, (INT_FROM_VAL(HEADER_DBG(p))-1)/2);
//				if (HEADER_DBG(p) == DBG_BIN) _hexDump(LOG_DEV, BIN_START(p), BIN_LENGTH(p),0);
//			}
			pkg = p->pkg ? p->pkg : MM.system;
			pkg->memory += total;
			p = p->nextBlock;
		}
	}
//	if (MM.gcTrace) {
//		for (i = 0; i < 32; i++) if (slots[i]) PRINTF(LOG_SYS,"> slot %2d: "LSD"\n",i,slots[i]);
//	}
}
LB* memoryPrintBuffer(Thread* th, Buffer* buffer)
{
	LB* p;
	if (!bufferSize(buffer)) return NULL;
	p = memoryAllocFromBuffer(buffer); if (!p) return NULL;
	bufferReinit(buffer);
	return p;
}
int memoryIsMainThread(void)
{
	if (MM.mainThread == hwThreadId()) return 1;
	return 0;
}
int memoryAddRoot(LB* root)
{
	STACK_PUSH_PNT_ERR(MM.tmpStack, root, EXEC_OM);
	STACK_PUSH_PNT_ERR(MM.tmpStack, MM.roots, EXEC_OM);
	STACK_PUSH_FILLED_ARRAY_ERR(MM.tmpStack, 2, DBG_LIST,EXEC_OM);
	MM.roots = STACK_PULL_PNT(MM.tmpStack);
	BLOCK_MARK(MM.roots);
	return 0;
}
void memorySetTmpRoot(LB* p)
{
	MM.tmpRoot = p;
	BLOCK_MARK(MM.tmpRoot);
}
void memoryInit(int argc, const char** argv)
{
	int i;
	LB* biosName;
	ThreadCounter = 0;
	lockCreate(&MM.lock);
	MM.mainThread= hwThreadId();
	MM.USEFUL=_USEFUL;
	MM.USELESS=_USELESS;

	MM.safeAlloc=0;
	MM.gcStage= GC_STAGE_INIT;
	MM.listMark=MM.listCheck=MM.listBlocks=MM.listSafe=NULL;
	MM.blockOperation = MEMORY_MARK;

	MM.listThreads = NULL;
	MM.listOblivions = NULL;
	MM.listPkgs = NULL;
	MM.currentPkg = NULL;

	MM.gcTrace = 0;
	MM.gc_count = 0;
	MM.blocs_nb=0;
	MM.blocs_length=0;
	MM.gc_period= GC_PERIOD_START;
	MM.gc_period_counter= GC_PERIOD_COUNT;
	MM.gc_period_time=0;

	MM.tmpBuffer = NULL;
	MM.scheduler = NULL;
	MM.tmpStack = NULL;
	MM.system = NULL;
	MM.roots = NULL;
	MM.popOblivions = NULL;
	MM.tmpRoot = NULL;

	MM._true = MM._false = NULL;

	MM.ansiVolume= NULL;
	MM.romdiskVolume = NULL;
	MM.partitionsFS = NULL;

	MM.loopMark=NULL;
	MM.abortMark=NULL;


	MM.args = NULL;

	MM.reboot = 0;
	MM.OM = 0;


	memoryEnterSafe();
	biosName = memoryAllocStr(BOOT_FILE, -1);
	MM.system = pkgAlloc(biosName, 0, PKG_FROM_IMPORT);
	MM.currentPkg = MM.system;
	MM.scheduler = threadCreate(THREAD_STACK_LENGTH0);

	MM.listThreads = MM.scheduler;
	
	MM.tmpBuffer = bufferCreate();
	MM.tmpStack= threadCreate(32);

	for (i = 1; i < argc; i++) STACK_PUSH_STR_ERR(MM.tmpStack, (char*)argv[i], strlen(argv[i]),/*NOTHING*/);
	STACK_PUSH_NIL_ERR(MM.tmpStack,/*NOTHING*/);
	for (i = 1; i < argc; i++) STACK_PUSH_FILLED_ARRAY_ERR(MM.tmpStack, 2, DBG_LIST,/*NOTHING*/);
	MM.args = (STACK_PULL_PNT(MM.tmpStack));

	MM.funStart= memoryAllocStr(FUN_START_NAME, -1);
	memoryLeaveSafe();

	systemInit(MM.system);

//	memoryFinalizeGC();
//	memoryRecount();
	if (MM.gcTrace) PRINTF(LOG_SYS, "> GC: Memory after init : " LSD " bytes\n", MM.blocs_length);
//	exit(0);
}
void memoryEnterSafe(void)	// after this, newly allocated blocks cannot be GCized
{
	MM.safeAlloc++;
}
void memoryLeaveSafe(void)
{
	LB* p = MM.listSafe;
	MM.safeAlloc--;
	if (MM.safeAlloc>0) return;
	while (p) {
		LB* next = p->nextBlock;
		if (p->listMark == MM.USEFUL || p->listMark == MM.USELESS) {
			p->listMark = MM.listMark;
			MM.listMark = p;
		}
		p->nextBlock = MM.listBlocks;
		MM.listBlocks = p;
		p = next;
	}
	MM.listSafe = NULL;
}
int memoryEnd(void)
{
/*	LB* p;
	for (p = MM.listBlocks; p; p = p->nextBlock) if (p == MemoryCheck) PRINTF(LOG_DEV,"in listBlocks\n");
	for (p = MM.listCheck; p; p = p->nextBlock) if (p == MemoryCheck) PRINTF(LOG_DEV,"in listCheck\n");
	for (p = MM.listSafe; p; p = p->nextBlock) if (p == MemoryCheck) PRINTF(LOG_DEV,"in listSafe\n");
*/	MM.scheduler=NULL;
	MM.tmpStack=NULL;
	MM.system=NULL;
	MM.tmpBuffer = NULL;
	MM.args = NULL;
	MM.fun_u0_list_u0_list_u0 = NULL;
	MM.fun_array_u0_I_u0 = NULL;
	MM.funStart = NULL;
	MM.roots = NULL;
	MM.popOblivions = NULL;
	MM.tmpRoot = NULL;
	MM.partitionsFS = NULL;
	while(MM.safeAlloc>0) memoryLeaveSafe();
	memoryFinalizeGC();
	memoryFinalizeGC();
//	PRINTF(LOG_DEV,"MemoryCheck %lld %lld\n", ((Mem*)MemoryCheck)->bytes, ((Mem*)MemoryCheck)->parentMem);
	if (MM.gcTrace) PRINTF(LOG_SYS, "> " LSD " blocks, " LSD " bytes, fast=" LSD "\n",MM.blocs_nb,MM.blocs_length,MM.safeAlloc);
	systemTerminate();
	lockDelete(&MM.lock);
	return MM.reboot;
}

