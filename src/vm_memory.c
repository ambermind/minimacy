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


Memory MM;

void memoryOptimize(int nbGc)
{
	int i;
	for (i = 0; i < nbGc; i++) memoryFinalizeGC();
#ifdef USE_MEMORY_C
	bmmRebuildTree();
	if (MM.gcTrace) PRINTF(LOG_SYS, "> Larger block: %d\n", bmmMaxSize());
#endif
}
LB* memoryAlloc(LINT size,LINT type,LW dbg)
{
	LB *p=NULL;
	LINT total = BLOCK_TOTAL_MEMORY(type, size);

// simulate memory limit:
	//if (0) {
	//	const LINT MAX_SIMU_MEM = (512 * 1024);
	//	if (MM.blocs_length > MAX_SIMU_MEM) {
	//		memoryFinalizeGC();
	//		memoryFinalizeGC();
	//		if (MM.blocs_length > MAX_SIMU_MEM) {
	//			PRINTF(LOG_SYS, "> Error: malloc " LSD " returns simulated OM error\n", size);
	//			MM.OM = 1;
	//			goto cleanup;
	//		}
	//	}
	//}
	if ((MM.blocs_length > MEMORY_SAFE_SIZE) && (MM.blocs_nb > (MM.gc_nb0 << 1))) memoryOptimize(2);
//	memoryFinalizeGC(); memoryFinalizeGC();
	p=(LB*)VM_MALLOC(total);
//	PRINTF(LOG_DEV,">>>>>>malloc %d bytes -> %llx\n", (int)total, p);
	if (!p) 
	{
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
				
				PRINTF(LOG_SYS, "> Error: malloc " LSD " returns null\n", size);
				//				threadDump(LOG_SYS, th, 10);
				goto cleanup;
			}
		}
	}
//	PRINTF(LOG_DEV, "%d,"LSX","LSD" ", (int)total, p, (INT_FROM_VAL(HEADER_DBG(p)) - 1) / 2);

	HEADER_SET_SIZE_AND_TYPE(p,size,type);
	p->pkg = MM.currentPkg;
	p->data[0]=dbg;

	p->lifo = MM.lifo; MM.lifo = p;

	if (MM.fastAlloc>0)
	{
		p->nextBlock=MM.listFast;
		MM.listFast=p;
	}
	else
	{
		p->nextBlock=MM.listBlocks;
		MM.listBlocks=p;
	}
	MM.blocs_nb++;
	MM.blocs_length+=total;
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

LB* memoryAllocExt(LINT sizeofExt,LW dbg,FORGET forget,MARK mark)
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
	MM.step = 1;
	MM.listCheck = MM.listBlocks;
	MM.listBlocks = NULL;
	MM.gc_nb0 = MM.blocs_nb;
	MM.gc_length0 = MM.blocs_length;
	MM.gc_free = 0;
	MM.gc_time = 0;
	MM.gc_count = 0;

//	lockEnter(&MM.lock);
	p = MM.USEFUL; MM.USEFUL = MM.USELESS; MM.USELESS = p;

	MEMORY_MARK(MM.system);
	MEMORY_MARK(MM.scheduler);
	MEMORY_MARK(MM.tmpStack);
	MEMORY_MARK(MM.tmpBuffer);
	MEMORY_MARK(MM.args);
	MEMORY_MARK(MM.fun_u0_list_u0_list_u0);
	MEMORY_MARK(MM.fun_array_u0_I_u0);
	MEMORY_MARK(MM.funStart);
	MEMORY_MARK(MM.roots);
	MEMORY_MARK(MM.tmpRoot);
	MEMORY_MARK(MM.partitionsFS);
	p = MM.listFast;
	while (p)
	{
		if (HEADER_DBG(p) != DBG_STACK) MEMORY_MARK(p);
		p = p->nextBlock;
	}
//	lockLeave(&MM.lock);
}

void memoryCleanListThreads(void)
{
	Thread** previous = &MM.listThreads;
	while (*previous)
	{
		Thread* th = *previous;
		if (th->header.lifo == MM.USELESS)
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
		if (p->header.lifo == MM.USELESS)
		{
			*previous = p->listNext;
			p->listNext = NULL;	// useless yet clean
		}
		else previous = &p->listNext;
	}
}

void memoryGC(LINT period)
{
	MM.gc_time+=period;
//	PRINTF(LOG_DEV,LSD,MM.step);
	if (MM.step==1)
	{
//		PRINTF(LOG_DEV,"$");
		if (MM.lifo)
		{
			LB* p=MM.lifo; MM.lifo=p->lifo;
			p->lifo=MM.USEFUL;

			MEMORY_MARK(p->pkg);
			if ((HEADER_TYPE(p)==TYPE_ARRAY)&&(HEADER_DBG(p)!= DBG_STACK))	// stack content is marked by threadMark
			{
				LINT i,l;
				l=ARRAY_LENGTH(p);
				for(i=0;i<l;i++) if (ARRAY_IS_PNT(p,i)) MEMORY_MARK(ARRAY_PNT(p,i));
			}
			else if (HEADER_TYPE(p)==TYPE_NATIVE)
			{
				void** q = (void**)&p->data[1];	// complicated because of the 32/64 bits compatibility
				MARK mark = (MARK)q[NATIVE_MARK];
				if (mark) (*mark)(p);
			}

		}
		else
		{
			memoryCleanListThreads();
			memoryCleanPkgs();
//			threadDumpLoop("go step 2");
			MM.step=2;
		}
	}
	else
	{
//		PRINTF(LOG_DEV,".");
//		PRINTF(LOG_DEV,"GC2 %llx.", MM.listCheck);
		if (MM.listCheck)
		{
			LB* p=MM.listCheck;
			MM.listCheck=MM.listCheck->nextBlock;
			if (p->lifo!=MM.USELESS)
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
			float f;

//			threadDumpLoop("end of GC");

			MM.gc_period=MM.gc_time/(MM.gc_free+1);
//			MM.gc_period = 1000;
			if (MM.gc_period>1000) MM.gc_period=1000;
			if (MM.gc_period<10) MM.gc_period=10;
			MM.gc_count++;
			MM.gc_totalCount++;

			f=(float)MM.gc_free;
			if (MM.gc_nb0) f/=MM.gc_nb0;
	//		else f=0;
			f*=100;
			if (MM.gcTrace)
			{
				if (MM.blocs_length >= 1024 * 1024 * 10)
					PRINTF(LOG_SYS, "> GC:"LSD" (" LSD ") free " LSD " of " LSD " ->" LSD " (" LSD "%c) " LSD "M\n", MM.gc_totalCount, MM.gc_time, MM.gc_free, MM.gc_nb0, MM.gc_period, (LINT)f, '%', MM.blocs_length >> 20);
				else
					PRINTF(LOG_SYS, "> GC:"LSD" (" LSD ") free " LSD " of " LSD " ->" LSD " (" LSD "%c) " LSD "k\n", MM.gc_totalCount, MM.gc_time, MM.gc_free, MM.gc_nb0, MM.gc_period, (LINT)f, '%', MM.blocs_length >> 10);
			}
			memoryGCinit();
		}
	}
}

void memoryFinalizeGC(void)
{
//	if (MM.gcTrace) PRINTF(LOG_DEV,"> memoryFinalizeGC\n");

	if (MM.step==0) memoryGCinit();
	if (MM.step==1) while(MM.step==1) memoryGC(1);
	while(MM.step==2) memoryGC(1);
}

void memoryRecount(void)
{
	int i;
	LB* p;
	Pkg* pkg;
	LINT slots[32];
	for (i = 0; i < 32; i++) slots[i] = 0;

	memoryFinalizeGC();
	if (MM.gcTrace) PRINTF(LOG_SYS, "> number of bios definitions=%d\n", MM.system->defs->nb);

	for (pkg = MM.listPkgs; pkg; pkg = pkg->listNext) pkg->memory = 0;

	for(i=0;i<2;i++) {
		p = i?MM.listCheck:MM.listFast;
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
	return 0;
}
void memorySetTmpRoot(LB* p)
{
	MM.tmpRoot = p;
}
void memoryInit(int argc, const char** argv)
{
	int i;
	LB* biosName;
	ThreadCounter = 0;

	MM.mainThread= hwThreadId();
	MM.USEFUL=_USEFUL;
	MM.USELESS=_USELESS;

	MM.fastAlloc=0;
	MM.step=0;
	MM.lifo=MM.listCheck=MM.listBlocks=MM.listFast=NULL;


	MM.listThreads = NULL;
	MM.listPkgs = NULL;
	MM.currentPkg = NULL;

	MM.gcTrace = 0;
	MM.blocs_nb=0;
	MM.blocs_length=0;
	MM.gc_period=GC_PERIOD;
	MM.gc_totalCount=0;

	MM.tmpBuffer = NULL;
	MM.scheduler = NULL;
	MM.tmpStack = NULL;
	MM.system = NULL;
	MM.roots = NULL;
	MM.tmpRoot = NULL;

	MM._true = MM._false = NULL;

	MM.ansiVolume= NULL;
	MM.uefiVolume= NULL;
	MM.romdiskVolume = NULL;
	MM.partitionsFS = NULL;

	MM.args = NULL;

	MM.reboot = 0;
	MM.OM = 0;


	lockCreate(&MM.lock);
	memoryEnterFast();
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
	memoryLeaveFast();

	systemInit(MM.system);

//	memoryFinalizeGC();
//	memoryFinalizeGC();
//	memoryRecount();
	if (MM.gcTrace) PRINTF(LOG_SYS, "> Memory after init : " LSD " bytes\n", MM.blocs_length);
//	exit(0);
}
void memoryEnterFast(void)	// after this, newly allocated blocks cannot be GCized
{
	MM.fastAlloc++;
}
LINT memoryGetFast(void)
{
	return MM.fastAlloc;
}
void memoryLeaveFast(void)
{
	LB* p=MM.listFast;

	MM.fastAlloc--;
	if (MM.fastAlloc>0) return;
	if (p)
	{
		while(p->nextBlock) p= p->nextBlock;
		p->nextBlock = MM.listBlocks;
		MM.listBlocks = MM.listFast;
		MM.listFast = NULL;
	}
}
int memoryEnd(void)
{
/*	LB* p;
	for (p = MM.listBlocks; p; p = p->nextBlock) if (p == MemoryCheck) PRINTF(LOG_DEV,"in listBlocks\n");
	for (p = MM.listCheck; p; p = p->nextBlock) if (p == MemoryCheck) PRINTF(LOG_DEV,"in listCheck\n");
	for (p = MM.listFast; p; p = p->nextBlock) if (p == MemoryCheck) PRINTF(LOG_DEV,"in listFast\n");
*/	MM.scheduler=NULL;
	MM.tmpStack=NULL;
	MM.system=NULL;
	MM.tmpBuffer = NULL;
	MM.args = NULL;
	MM.fun_u0_list_u0_list_u0 = NULL;
	MM.fun_array_u0_I_u0 = NULL;
	MM.funStart = NULL;
	MM.roots = NULL;
	MM.tmpRoot = NULL;
	MM.partitionsFS = NULL;
	while(memoryGetFast()>0) memoryLeaveFast();
	memoryFinalizeGC();
	memoryFinalizeGC();
//	memoryFinalizeGC();
//	PRINTF(LOG_DEV,"MemoryCheck %lld %lld\n", ((Mem*)MemoryCheck)->bytes, ((Mem*)MemoryCheck)->parentMem);
	if (MM.gcTrace) PRINTF(LOG_SYS, "> " LSD " blocks, " LSD " bytes, fast=" LSD "\n",MM.blocs_nb,MM.blocs_length,MM.fastAlloc);
	systemTerminate();
	lockDelete(&MM.lock);

	return MM.reboot;
}

