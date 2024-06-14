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
int gcTRON = 0;
extern Mem* MemCheck;

int memoryChainTest(Mem* mem, LINT total)
{
	while (mem)
	{
		if (mem->maxBytes &&((mem->bytes + total) > mem->maxBytes)) return 0;
		mem = mem->header.mem;
	}
	return 1;
}

LB* memoryAlloc(Thread* th, LINT size,LINT type,LW dbg)
{
	LB *p=NULL;
	Mem* mem = th?th->memDelegate:MM.mem;
	LINT total = sizeof(LB) + (((size+ LWLEN - 1)>>LSHIFT)<<LSHIFT);
#ifdef USE_ALL_BITS
	if (type == TYPE_ARRAY) total += (((size >> LSHIFT) + LWLEN - 1) >> LSHIFT) << LSHIFT;
#endif
	if (mem)
	{
		if (!memoryChainTest(mem, total))
		{
//			PRINTF(LOG_DEV, "Force gc for thread " LSD /*" current0=" LSX*/ "\n", th->uid);// , mem);
			memoryFullGC();
			if (!memoryChainTest(mem, total))
			{
				memoryFullGC();
				if (!memoryChainTest(mem, total))
				{
					PRINTF(LOG_SYS, "> Error: Out of memory\n");
					MM.OM = 1;
					if (th) th->OM=1;
					goto cleanup;	// no more memory for this thread
				}
			}
		}
		else if ((MM.blocs_length > 16*1024 * 1024) && (MM.blocs_nb > (MM.gc_nb0 << 1)))
		{
//			PRINTF(LOG_DEV, "==========>force gc\n");
			memoryFullGC();
			memoryFullGC();
		}
	}
	p=(LB*)VM_MALLOC(total);
//	PRINTF(LOG_DEV,">>>>>>malloc %d bytes -> %llx\n", (int)total, p);
	if (!p) 
	{
//		PRINTF(LOG_DEV,"x");
		PRINTF(LOG_SYS,"> Error: malloc " LSD " returns null\n",size);
//		threadDump(LOG_SYS, th, 10);
		goto cleanup;
	}
	HEADER_SET_SIZE_AND_TYPE(p,size,type);
	p->mem = mem;
	p->data[0]=dbg;

/*	if ((!th) && (dbg == DBG_MEMCOUNT))
	{
		MemoryCheck = p;
//		PRINTF(LOG_DEV,"ALLOC " LSX " / " LSD " (type " LSD ")(size " LSD ")(total " LSD ")\n", p, INT_FROM_VAL(HEADER_DBG(p)), type, size, total);
	}
	if (!th) PRINTF(LOG_DEV,"ALLOC " LSX " / " LSD " (type " LSD ")(size " LSD ")(total " LSD ")\n", p, INT_FROM_VAL(HEADER_DBG(p)), type, size, total);
	if ((total==72)&& (p->mem == MemoryCheck)) PRINTF(LOG_DEV,"MemoryCheck (%lld) %llx +=%lld\n", INT_FROM_VAL(dbg), p, 72);
*/	p->lifo = MM.USEFUL;

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

	if (mem) mem->count++;
//	if (MemCheck && mem == MemCheck) PRINTF(LOG_DEV,"memCountIncrement "LSX" from "LSX" count:%lld\n", mem, p, mem->count);
	while(mem)
	{
		mem->bytes += total;
		mem = mem->header.mem;
	}
cleanup:
	return p;
}

LB* memoryAllocArray(Thread* th, LINT size, LW dbg)
{
	LB* p=memoryAlloc(th, size<< LSHIFT, TYPE_ARRAY, dbg);
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

LB* memoryAllocExt(Thread* th, LINT sizeofExt,LW dbg,FORGET forget,MARK mark)
{
	void** q;
	LB* p=memoryAlloc(th,sizeofExt- sizeof(LB),TYPE_NATIVE,dbg);
	if (!p) return NULL;
	memset((void*)(&p->data[1]),0,sizeofExt-sizeof(LB));

	q =(void**) &p->data[1];
	q[0]=forget;
	q[1]=mark;
	return p;
}

LB* memoryAllocStr(Thread* th, char* src, LINT size)
{
	LB* p;
	if (size < 0) {
		if (!src) return NULL;
		size = strlen(src);
	}
	
	p=memoryAlloc(th,size+1,TYPE_BINARY,DBG_S);
	if (!p) return NULL;
	if (src) memcpy(STR_START(p),src,size);
	STR_START(p)[size]=0;
	return p;
}

LB* memoryAllocBin(Thread* th, char* src, LINT size,LW dbg)
{
	LB* p=memoryAlloc(th,size,TYPE_BINARY,dbg);
	if (!p) return NULL;
	if (src) memcpy(BIN_START(p),src,size);
	return p;
}

LB* memoryAllocFromBuffer(Thread* th, Buffer* b)
{
	return memoryAllocStr(th,bufferStart(b), bufferSize(b));
}

char* errorName(int err)
{
	if (err==COMPILER_ERR_SN) return "Syntax error";
	else if (err==COMPILER_ERR_TYPE) return "Typechecking error";
	else return "Unknown error";
}

void memoryTake(Mem* m, LB* p)
{
	if ((!p) || (p->mem == m)) return;
	else
	{
		LINT size = HEADER_SIZE(p);
		LINT total = sizeof(LB) + (((size + LWLEN - 1) >> LSHIFT) << LSHIFT);
		Mem* mem = p->mem;

		while (mem)
		{
			mem->bytes -= total;
			mem = mem->header.mem;
		}
//		if (MemCheck && p->mem == MemCheck) PRINTF(LOG_DEV,"memCountDecrement from memoryTake "LSX" by "LSX"\n", p, m);
		memCountDecrement(p->mem);

		p->mem = m;
		if (m) m->count++;
//		if (MemCheck && m == MemCheck) PRINTF(LOG_DEV,"memCountIncrement "LSX" from "LSX" count:%lld\n", m, p, m->count);
		while (m)
		{
			m->bytes += total;
			m = m->header.mem;
		}
	}
}

void memoryDesalloc(LB* p)
{
	LINT size=HEADER_SIZE(p);
	LINT type = HEADER_TYPE(p);
	LINT total = sizeof(LB) + (((size + LWLEN - 1) >> LSHIFT) << LSHIFT);
	Mem* mem = p->mem;
#ifdef USE_ALL_BITS
	if (type == TYPE_ARRAY) total += (((size >> LSHIFT) + LWLEN - 1) >> LSHIFT) << LSHIFT;
#endif
	//	if (MemoryCheck == p) PRINTF(LOG_DEV,"try desalloc MemoryCheck\n");
//	if ((total == 72) && (p->mem == MemoryCheck)) PRINTF(LOG_DEV,"MemoryCheck (%lld) %llx -=%lld\n", INT_FROM_VAL(HEADER_DBG(p)), p, 72);
	while (mem)
	{
//		if (MemCheck && mem == MemCheck) PRINTF(LOG_DEV,"memBytesDecrement from memoryDesalloc "LSX" by "LSX"\n", mem, p);
		mem->bytes -= total;
		mem = mem->header.mem;
	}

//	if (MemCheck && p->mem == MemCheck) PRINTF(LOG_DEV,"memCountDecrement from memoryDesalloc "LSX" by "LSX"\n", p->mem, p);

// following line : if we let the Mem block decrement its parent, it might cause an issue
// ex: during a GC, B -> M1 -> M2 have to be desalloc, and the GC proceeds in following order : M2, M1, B
// M2 : decrement, but still referenced by M1
// M1 : decrement M2 therefore free it
// B  : fill try to substract it total to M1->bytes and M2->bytes
// --> solution:
// decrement the parent of a Mem block only when we actually free the block : see recursion in memCountDecrement(...)
	if (HEADER_DBG(p) != DBG_MEMCOUNT) memCountDecrement(p->mem);

	if (type==TYPE_NATIVE)
	{
		void** q = (void**)&p->data[1];	// complicated because of the 32/64 bits compatibility
		FORGET forget = (FORGET)q[NATIVE_FORGET];
		if (forget && (*forget)(p)) goto cleanup;
	}
	//	if (MemoryCheck == p) PRINTF(LOG_DEV,"ready to desalloc MemoryCheck\n");
	MM.blocs_nb--;
	MM.blocs_length-=total;
	
//	PRINTF(LOG_DEV,"free %llx\n", p);
	VM_FREE((void*)p);
cleanup:
	return;
//	PRINTF(LOG_DEV,"DONE\n");
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

	MEMORY_MARK(NULL, (LB*)MM.system);
	MEMORY_MARK(NULL, (LB*)MM.scheduler);
	MEMORY_MARK(NULL, (LB*)MM.tmpStack);
	MEMORY_MARK(NULL, (LB*)MM.tmpBuffer);
	MEMORY_MARK(NULL, MM.args);
	MEMORY_MARK(NULL, (LB*)MM.fun_u0_list_u0_list_u0);
	MEMORY_MARK(NULL, (LB*)MM.fun_array_u0_I_u0);
	MEMORY_MARK(NULL, MM.funStart);
	MEMORY_MARK(NULL, MM.roots);
	MEMORY_MARK(NULL, MM.partitionsFS);
	p = MM.listFast;
	while (p)
	{
		if (HEADER_DBG(p) != DBG_STACK) MEMORY_MARK(NULL, p);
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
void memoryCleanListMems(void)
{
	Mem** previous = &MM.listMems;
	while (*previous)
	{
		Mem* m = *previous;
		if (m->header.lifo == MM.USELESS)
		{
			*previous = m->listNext;
			m->listNext = NULL;	// useless yet clean
		}
		else previous = &m->listNext;
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
/*	if (gcTRON) PRINTF(LOG_DEV,LSD, gcTRON++);
	if (gcTRON == 4681) {
		PRINTF(LOG_DEV,"now!\n");
	}
	*/
	if (MM.step==1)
	{
//		PRINTF(LOG_DEV,"$");
		if (MM.lifo)
		{
			LB* p=MM.lifo; MM.lifo=p->lifo;
			p->lifo=MM.USEFUL;

			MEMORY_MARK((LB*)p,(LB*)p->mem);
			if (HEADER_TYPE(p)==TYPE_ARRAY)
			{
				LINT i,l;
				l=ARRAY_LENGTH(p);
				for(i=0;i<l;i++) if (ARRAY_IS_PNT(p,i)) MEMORY_MARK(p, ARRAY_PNT(p,i));
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
			memoryCleanListMems();
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

void memoryFullGC(void)
{
//	if (gcTRON) PRINTF(LOG_DEV,"memoryFullGC\n");

	if (MM.step==0) memoryGCinit();
	if (MM.step==1) while(MM.step==1) memoryGC(1);
	while(MM.step==2) memoryGC(1);
}

LB* memoryPrintBuffer(Thread* th, Buffer* buffer)
{
	LB* p;
	if (!bufferSize(buffer)) return NULL;
	p = memoryAllocFromBuffer(th, buffer); if (!p) return NULL;
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
void memoryInit(int argc, char** argv)
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


	MM.mem = NULL;
	MM.listThreads = NULL;
	MM.listMems = NULL;
	MM.listPkgs = NULL;

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

	MM._true = MM._false = NULL;

	MM.ansiVolume= NULL;
	MM.uefiVolume= NULL;
	MM.romdiskVolume = NULL;
	MM.partitionsFS = NULL;

	MM.args = NULL;

	MM.reboot = 0;
	MM.OM = 0;

	lockCreate(&MM.lock);
	MM.mem = memCreate(NULL, NULL, 1024 * 1024 * 32, NULL); 
	MM.scheduler=threadCreate(NULL, MM.mem);
	memoryEnterFast();
	biosName = memoryAllocStr(MM.scheduler, BOOT_FILE, -1);
	MM.mem->name = biosName;
	MM.listThreads = MM.scheduler;
	
	MM.tmpBuffer = bufferCreate(MM.scheduler);
	MM.tmpStack= threadCreate(MM.scheduler, NULL);
	MM.system=pkgAlloc(MM.scheduler, biosName,8, PKG_FROM_IMPORT);

	for (i = 1; i < argc; i++) STACK_PUSH_STR_ERR(MM.tmpStack, argv[i], strlen(argv[i]),/*NOTHING*/);
	STACK_PUSH_NIL_ERR(MM.tmpStack,/*NOTHING*/);
	for (i = 1; i < argc; i++) STACK_PUSH_FILLED_ARRAY_ERR(MM.tmpStack, 2, DBG_LIST,/*NOTHING*/);
	MM.args = (STACK_PULL_PNT(MM.tmpStack));

	typesInit(MM.scheduler, MM.system);
	systemInit(MM.scheduler, MM.system);

	MM.funStart= memoryAllocStr(MM.scheduler,FUN_START_NAME, -1);

	memoryLeaveFast();
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
	MM.mem = NULL;
	MM.fun_u0_list_u0_list_u0 = NULL;
	MM.fun_array_u0_I_u0 = NULL;
	MM.funStart = NULL;
	MM.roots = NULL;
	MM.partitionsFS = NULL;
	while(memoryGetFast()>0) memoryLeaveFast();
	memoryFullGC();
	memoryFullGC();
//	memoryFullGC();
//	PRINTF(LOG_DEV,"MemoryCheck %lld %lld\n", ((Mem*)MemoryCheck)->bytes, ((Mem*)MemoryCheck)->parentMem);
	if (MM.gcTrace) PRINTF(LOG_SYS, "> " LSD " blocks, " LSD " bytes, fast=" LSD "\n",MM.blocs_nb,MM.blocs_length,MM.fastAlloc);
	systemTerminate();
	lockDelete(&MM.lock);

	return MM.reboot;
}

