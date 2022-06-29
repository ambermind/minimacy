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

#ifdef DBG_MEM_CHECK
LB* MemoryCheck=NULL;

void memoryAlert(LB* from, LB* to)
{
	printf(">>------- " LSX " dbg=" LSD" REFERENCED BY " LSX " dbg=" LSD"\n", (LINT)to, VALTOINT(HEADER_DBG(to)), (LINT)from,VALTOINT(HEADER_DBG(from)));
/*	if (from == to)
	{
		printf("Same!!!\n");
		printf("continue\n");

	}
*/	itemDump(-1, PNTTOVAL(from));
}

void memoryCheck(LB* p)
{
	printf("\n\n\n>> ------ CHECK REFERENCES TO " LSX "\n", (LINT)p);
	MemoryCheck = p;
}

#endif
int memoryChainTest(Mem* mem, LINT total)
{
	while (mem)
	{
		if (mem->maxBytes &&((mem->bytes + total) > mem->maxBytes)) return 0;
		mem = mem->header.mem;
	}
	return 1;
}
LINT memoryTest(Mem* mem, LINT total)
{
	if (total < 0) return 0;
	total += sizeof(LB);
	if (!memoryChainTest(mem, total))
	{
		memoryFullGC();
		if (!memoryChainTest(mem, total))
		{
			memoryFullGC();
			if (!memoryChainTest(mem, total)) return 0;
		}
	}
	return 1;
}

LB* memoryAlloc(Thread* th, LINT size,LINT type,LW dbg)
{
	LB *p=NULL;
	Mem* mem = th?th->memDelegate:MM.mem;
	LINT total = sizeof(LB) + (((size+ LWLEN - 1)>>LSHIFT)<<LSHIFT);
//	lockEnter(&MM.lock);
	if (mem)
	{
		if (!memoryChainTest(mem, total))
		{
//			PRINTF(th,LOG_ERR, "Force gc for thread " LSD /*" current0=" LSX*/ "\n", th->uid);// , mem);
			memoryFullGC();
			if (!memoryChainTest(mem, total))
			{
				memoryFullGC();
				if (!memoryChainTest(mem, total))
				{
					PRINTF(th,LOG_ERR, "Out of memory\n");
					MM.OM = 1;
					goto cleanup;	// no more memory for this thread
				}
			}
		}
		else if ((MM.blocs_length > 1024 * 1024) && (MM.blocs_nb > (MM.gc_nb0 << 1)))
		{
//			PRINTF(th,LOG_ERR, "==========>force gc\n");
			memoryFullGC();
			memoryFullGC();
		}
	}
	p=(LB*)malloc(total);
//	printf(">>>>>>malloc %d bytes -> %llX\n", (int)total, p);
	if (!p) 
	{
//		printf("x");
		PRINTF(th,LOG_ERR,"malloc " LSD " returns null\n",size);
		goto cleanup;
	}
	HEADERSETSIZETYPE(p,size,type);
	p->mem = mem;
	p->data[0]=dbg;

/*	if ((!th) && (dbg == DBG_MEMCOUNT))
	{
		MemoryCheck = p;
//		printf("ALLOC " LSX " / " LSD " (type " LSD ")(size " LSD ")(total " LSD ")\n", p, VALTOINT(HEADER_DBG(p)), type, size, total);
	}
	if (!th) printf("ALLOC " LSX " / " LSD " (type " LSD ")(size " LSD ")(total " LSD ")\n", p, VALTOINT(HEADER_DBG(p)), type, size, total);
	if ((total==72)&& (p->mem == MemoryCheck)) printf("MemoryCheck (%lld) %llx +=%lld\n", VALTOINT(dbg), p, 72);
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
//	if (MemCheck && mem == MemCheck) printf("memCountIncrement "LSX" from "LSX" count:%lld\n", mem, p, mem->count);
	while(mem)
	{
		mem->bytes += total;
		mem = mem->header.mem;
	}
cleanup:
//	lockLeave(&MM.lock);
	return p;
}

LB* memoryAllocTable(Thread* th, LINT size, LW dbg)
{
	LINT i;
	LB* p=memoryAlloc(th, size<< LSHIFT, TYPE_TAB, dbg);
	if (!p) return NULL;
	for (i = 1; i <= size; i++) p->data[i] = NIL;
	return p;
}

LB* memoryAllocExt(Thread* th, LINT sizeofExt,LW dbg,FORGET forget,MARK mark)
{
	LB* p=memoryAlloc(th,sizeofExt- sizeof(LB),TYPE_NATIVE,dbg);
	if (!p) return NULL;
	p->data[NATIVE_FORGET]=(LW)forget;
	p->data[NATIVE_MARK]=(LW)mark;
	return p;
}

LB* memoryAllocStr(Thread* th, char* src, LINT size)
{
	LB* p;
	if (size<0) size=strlen(src);
	
	p=memoryAlloc(th,size+1,TYPE_STRING,DBG_S);
	if (!p) return NULL;
	if (src) memcpy(STRSTART(p),src,size);
	STRSTART(p)[size]=0;
	return p;
}

LB* memoryAllocBin(Thread* th, char* src, LINT size,LW dbg)
{
	LB* p=memoryAlloc(th,size,TYPE_BINARY,dbg);
	if (!p) return NULL;
	if (src) memcpy(BINSTART(p),src,size);
	return p;
}

LB* memoryAllocFromBuffer(Thread* th, Buffer* b)
{
	return memoryAllocStr(th,bufferStart(b), bufferSize(b));
}

char* errorName(int err)
{
	if (err==LS_ERR_SN) return "Syntax error";
	else if (err==LS_ERR_TYPE) return "Typechecking error";
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
//		if (MemCheck && p->mem == MemCheck) printf("memCountDecrement from memoryTake "LSX" by "LSX"\n", p, m);
		memCountDecrement(p->mem);

		p->mem = m;
		if (m) m->count++;
//		if (MemCheck && m == MemCheck) printf("memCountIncrement "LSX" from "LSX" count:%lld\n", m, p, m->count);
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

//	if (MemoryCheck == p) printf("try desalloc MemoryCheck\n");
//	if ((total == 72) && (p->mem == MemoryCheck)) printf("MemoryCheck (%lld) %llx -=%lld\n", VALTOINT(HEADER_DBG(p)), p, 72);
//	lockEnter(&MM.lock);
	while (mem)
	{
//		if (MemCheck && mem == MemCheck) printf("memBytesDecrement from memoryDesalloc "LSX" by "LSX"\n", mem, p);
		mem->bytes -= total;
		mem = mem->header.mem;
	}

//	if (MemCheck && p->mem == MemCheck) printf("memCountDecrement from memoryDesalloc "LSX" by "LSX"\n", p->mem, p);

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
		FORGET x = (FORGET)p->data[NATIVE_FORGET];
		if (x && (*x)(p)) goto cleanup; 
	}
	//	if (MemoryCheck == p) printf("ready to desalloc MemoryCheck\n");
	MM.blocs_nb--;
	MM.blocs_length-=total;
	
//	PRINTF(th,LOG_ERR, "free %llx\n", p);
//	printf("free %llx\n", p);
	free((void*)p);
cleanup:
	return;
//	lockLeave(&MM.lock);
//	PRINTF(th,LOG_ERR,"DONE\n");
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

	MEMORYMARK(NULL, (LB*)MM.system);
	MEMORYMARK(NULL, (LB*)MM.scheduler);
	MEMORYMARK(NULL, (LB*)MM.tmpStack);
	MEMORYMARK(NULL, (LB*)MM.userBuffer);
	MEMORYMARK(NULL, (LB*)MM.errBuffer);
	MEMORYMARK(NULL, (LB*)MM.tmpBuffer);
	MEMORYMARK(NULL, MM.args);
	MEMORYMARK(NULL, (LB*)MM.fun_u0_list_u0_list_u0);
	MEMORYMARK(NULL, (LB*)MM.fun_array_u0_I_u0);
	MEMORYMARK(NULL, MM.funStart);
	p = MM.listFast;
	while (p)
	{
		if (HEADER_DBG(p) != DBG_STACK) MEMORYMARK(NULL, p);
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
//	printf(LSD,MM.step);
/*	if (gcTRON) printf(LSD, gcTRON++);
	if (gcTRON == 4681) {
		printf("now!\n");
	}
	*/
	if (MM.step==1)
	{
//		printf("$");
		if (MM.lifo)
		{
			LB* p=MM.lifo; MM.lifo=p->lifo;
			p->lifo=MM.USEFUL;

			MEMORYMARK((LB*)p,(LB*)p->mem);
			if (HEADER_TYPE(p)==TYPE_TAB)
			{
				LINT i,l;
				l=TABLEN(p);
				for(i=0;i<l;i++)
				{
					LW val=TABGET(p,i);
					if (ISVALPNT(val)) MEMORYMARK(p, VALTOPNT(val));
				}
			}
			else if (HEADER_TYPE(p)==TYPE_NATIVE)
			{
				MARK x=(MARK)p->data[NATIVE_MARK];
				if (x) (*x)(p);
			}

		}
		else
		{
			memoryCleanListThreads();
			memoryCleanListMems();
			memoryCleanPkgs();
//			threadDumpLoop("go step 2");
			//			PRINTF(th,LOG_ERR,"#GC: go step2\n");
			MM.step=2;
		}
	}
	else
	{
//		printf(".");
//		printf("GC2 %llX.", MM.listCheck);
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
					printf("WRONG desalloc %llx\n",p);
					itemHeader(LOG_ERR,p);
				}
*/
//				if (HEADER_DBG(p) == DBG_IMPORTS) printf("Release DBG_IMPORTS %llx\n", p);
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

			f=(float)MM.gc_free;
			if (MM.gc_nb0) f/=MM.gc_nb0;
	//		else f=0;
			f*=100;
			if (MM.gcConsole)
			{
				if (MM.blocs_length >= 1024 * 1024 * 10)
					PRINTF(MM.scheduler, LOG_ERR, "#GC: (" LSD ") free " LSD " of " LSD " ->" LSD " (" LSD "%c) " LSD "M\n", MM.gc_time, MM.gc_free, MM.gc_nb0, MM.gc_period, (LINT)f, '%', MM.blocs_length >> 20);
				else
					PRINTF(MM.scheduler, LOG_ERR, "#GC: (" LSD ") free " LSD " of " LSD " ->" LSD " (" LSD "%c) " LSD "k\n", MM.gc_time, MM.gc_free, MM.gc_nb0, MM.gc_period, (LINT)f, '%', MM.blocs_length >> 10);
			}
			memoryGCinit();
		}
	}
}

void memoryFullGC()
{
//	if (gcTRON) printf("memoryFullGC\n");

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
void memoryUserBufferize(int val)
{
	MainTerm.userBufferize = val;
	bufferReinit(MM.userBuffer);
}
void memoryErrBufferize(int val)
{
	MainTerm.errBufferize = val;
	bufferReinit(MM.errBuffer);
}
LB* memoryErrBuffer(Thread* th)
{
	LB* p;
	if (!bufferSize(MM.errBuffer)) return NULL;
	p = memoryAllocFromBuffer(th, MM.errBuffer); if (!p) return NULL;
	bufferReinit(MM.errBuffer);
	return p;
}
int memoryIsMainThread()
{
	if (MM.mainThread == hwThreadId()) return 1;
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

	MM.blocs_nb=0;
	MM.blocs_length=0;
	MM.gc_period=GC_PERIOD;

	MM.userBuffer = NULL;
	MM.errBuffer = NULL;
	MM.tmpBuffer = NULL;
	MM.scheduler = NULL;
	MM.tmpStack = NULL;
	MM.system = NULL;

	MM.trueRef = NULL;
	MM.falseRef = NULL;

	MM.args = NULL;

	MM.reboot = 0;
	MM.OM = 0;

	lockCreate(&MM.lock);
	MainTerm.userBufferize = 0;
	MM.mem = memCreate(NULL, NULL, 1024 * 1024 * 32, NULL); 
	MM.scheduler=threadCreate(NULL, MM.mem);
	memoryEnterFast(MM.scheduler);
	biosName = memoryAllocStr(MM.scheduler, BOOT_FILE, -1);
	MM.mem->name = biosName;
	MM.listThreads = MM.scheduler;
	
	
	MM.userBuffer = bufferCreate(MM.scheduler);
	MM.errBuffer = bufferCreate(MM.scheduler);
	MM.tmpBuffer = bufferCreate(MM.scheduler);
	MM.tmpStack= threadCreate(MM.scheduler, NULL);
	MM.system=pkgAlloc(MM.scheduler, biosName,8, PKG_EMPTY);
	MM.system->stage = PKG_STAGE_READY;
	MM.listPkgs = MM.system;

	for (i = 1; i < argc; i++) stackPushStr(MM.tmpStack, argv[i], strlen(argv[i]));
	STACKPUSH(MM.tmpStack, NIL);
	for (i = 1; i < argc; i++) DEFTAB(MM.tmpStack, 2, DBG_LIST);
	MM.args = VALTOPNT(STACKPULL(MM.tmpStack));

	typesInit(MM.scheduler, MM.system);
	systemInit(MM.scheduler, MM.system);

	MM.funStart= memoryAllocStr(MM.scheduler,FUN_START_NAME, -1);

	memoryLeaveFast(MM.scheduler);
}
void memoryEnterFast(Thread* th)	// after this, newly allocated blocks cannot be GCized
{
	MM.fastAlloc++;
}
LINT memoryGetFast(Thread* th)
{
	return MM.fastAlloc;
}
void memoryLeaveFast(Thread* th) // must be called by the main thread
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
int memoryEnd()
{
/*	LB* p;
	for (p = MM.listBlocks; p; p = p->nextBlock) if (p == MemoryCheck) printf("in listBlocks\n");
	for (p = MM.listCheck; p; p = p->nextBlock) if (p == MemoryCheck) printf("in listCheck\n");
	for (p = MM.listFast; p; p = p->nextBlock) if (p == MemoryCheck) printf("in listFast\n");
*/	MM.scheduler=NULL;
	MM.tmpStack=NULL;
	MM.system=NULL;
	MM.userBuffer = NULL;
	MM.errBuffer = NULL;
	MM.tmpBuffer = NULL;
	MM.args = NULL;
	MM.mem = NULL;
	MM.gcConsole = 0;
	MM.fun_u0_list_u0_list_u0 = NULL;
	MM.fun_array_u0_I_u0 = NULL;
	MM.funStart = NULL;
	while(memoryGetFast(NULL)>0) memoryLeaveFast(NULL);
	memoryFullGC();
	memoryFullGC();
//	memoryFullGC();
//	printf("MemoryCheck %lld %lld\n", ((Mem*)MemoryCheck)->bytes, ((Mem*)MemoryCheck)->parentMem);
	if (MM.gcConsole) PRINTF(NULL,LOG_ERR, LSD " blocks, " LSD " bytes, fast=" LSD "\n",MM.blocs_nb,MM.blocs_length,MM.fastAlloc);
	systemTerminate();
	lockDelete(&MM.lock);

	return MM.reboot;
}

