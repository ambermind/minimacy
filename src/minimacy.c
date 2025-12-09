// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

int boot(void)
{
	Thread* th = MM.scheduler;
	LB* src = fsReadPackage(BOOT_FILE, NULL, 0);
//	LB* src = fsReadPackage("listDefs", NULL, 0);
	if (!src)
	{
		PRINTF(LOG_USER, "\n> Error: boot file '%s"SUFFIX_CODE"' not found\n", BOOT_FILE);
		fsReadPackage(BOOT_FILE, NULL, 1);
#ifdef USE_FS_ANSI
		ansiHelpBiosFinder();
#endif
		return -1;
	}
	FUN_PUSH_PNT(src);
	FUN_PUSH_NIL;
	return 0;
}

int start(int argc, const char** argv)
{
	int i;
	int standalone = 1;
	int help = 0;
#ifdef ON_WINDOWS
	int directQuit = 0;
#endif	
//	char* args[]={"minimacy.exe","../../programs/demo/demo.fun.pacman.mcy"};	argc=2;argv=args;
//	for (i = 0; i < argc; i++) PRINTF(LOG_DEV,"arg %d: '%s'\n", i, argv[i]);
	termInit();

	for (i = 0; i < argc; i++)
	{
		const char* arg = argv[i];
		if ((!strcmp(arg, "-h"))|| (!strcmp(arg, "-?")) || (!strcmp(arg, "--help")))
		{
			termSetMask(LOG_ALL);
			help = 1;
		}
		if ((!strcmp(arg, "-q")) || (!strcmp(arg, "--quiet"))) {
			termShowBiosListing(0);
			termShowPkgListing(0);
			termSetMask(LOG_ALL);
		}
		if ((!strcmp(arg, "-s")) || (!strcmp(arg, "--silent"))) {
			termShowBiosListing(0);
			termShowPkgListing(0);
			termSetMask(LOG_USER);
		}
		if ((!strcmp(arg, "-v")) || (!strcmp(arg, "--verbose"))) {
			termShowBiosListing(1);
			termShowPkgListing(1);
			termSetMask(LOG_ALL);
		}
#ifdef ON_WINDOWS
		if (!strcmp(arg, "-d")) directQuit = 1;
#endif	
		if ((strlen(arg) >= strlen(SUFFIX_CODE)) && !strcmp(arg + strlen(arg) - strlen(SUFFIX_CODE), SUFFIX_CODE)) standalone = 0;
	}

	PRINTF(LOG_SYS,"\n> Minimacy - Sylvain Huet - 2020-25 - "VERSION_MINIMACY"/"DEVICE_MODE"\n");
	PRINTF(LOG_SYS,"> ----\n");

//	if (!standalone) termSetMask(LOG_USER);	// disable LOG_SYS

	if ((LWLEN==8)&&(sizeof(LINT)==8)&&(sizeof(LFLOAT)==8)) PRINTF(LOG_SYS,"> %d bits\n",64);
	else if ((LWLEN==4)&&(sizeof(LINT)==4)&&(sizeof(LFLOAT)==4) && (sizeof(double) == 8)) PRINTF(LOG_SYS,"> %d bits\n", 32);
	else
	{
		PRINTF(LOG_USER,"> Error: wrong system [P:%d I:%d F:%d D:%d]\n",LWLEN,sizeof(LINT),sizeof(LFLOAT),sizeof(double));
		return -1;
	}

	if (help)
	{
		PRINTF(LOG_USER, "\nUsage: minimacy [-h|-?|--help] [-v|--verbose] [-q|--quiet] [-s|--silent] [-c|--compile] [*.mcy] [args]\n\n");
		PRINTF(LOG_USER, "Options:\n");
		PRINTF(LOG_USER, "  -h|-?|--help: display this help message\n");
		PRINTF(LOG_USER, "  -v|--verbose: print everything including bios listing\n");
		PRINTF(LOG_USER, "  -q|--quiet: do not print compilation listing\n");
		PRINTF(LOG_USER, "  -s|--silent: do not print compilation listing and system messages\n");
		PRINTF(LOG_USER, "  -compile: compile and stop, this will be useful for any IDE extension\n");
		PRINTF(LOG_USER, "  [*.mcy]: a mcy file to start with (default is programs/boot.mcy)\n");
		PRINTF(LOG_USER, "  [args]: args following a mcy file are reserved to this mcy program\n");
#ifdef USE_FS_ANSI
		ansiHelpBiosFinder();
#endif
		goto cleanup;
	}
#ifdef USE_MEMORY_C
//	bmmSetTotalSize(270* 1024);
	cMallocInit();
#endif
	if (hwInit()) goto cleanup;
	do
	{
		memoryInit(argc,argv);
		if (fsMount(argc?argv[0]:NULL, standalone)) goto cleanup;
		if (boot())
		{
			memoryEnd();
			goto cleanup;
		}
		compilePromptAndRun(MM.scheduler);
		if (MM.gcTrace) PRINTF(LOG_SYS, "> end of scheduler\n");
		romdiskReleaseUserDisk();
//		PRINTF(LOG_SYS, "\n--\n\n");
		fsRelease();
	} while (memoryEnd());
cleanup:
	termEnd();

	PRINTF(LOG_SYS, "> end of line\n");
#ifdef DBG_MEM
	MainTerm.mask = LOG_ALL;
	PRINTF(LOG_SYS, "> -------\n");
	PRINTF(LOG_SYS, "> leaks:\n");
	_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDOUT );
	_CrtDumpMemoryLeaks();
#endif
#ifdef ON_WINDOWS
	if (!directQuit) getchar();
#endif
	return 0;
}
#ifdef GROUP_UNIX
int MTargc=0;
const char** MTargv=NULL;
MTHREAD_START startThread(void* custom)
{
	start(MTargc,(const char**)MTargv);
	return MTHREAD_RETURN;
}
int startInThread(int argc, const char** argv)
{
//    PRINTF(LOG_DEV,"sigpipe main thread\n");
	signal(SIGPIPE,SIG_IGN);
    MTargc=argc;
    MTargv=argv;
	hwThreadCreate(startThread, NULL);
	return 0;
}
#endif
#if defined ON_UNIX || defined ON_MACOS_CMDLINE || defined ON_RPIOS
int main(int argc, const char** argv)
{
	signal(SIGPIPE,SIG_IGN);

	return start(argc,argv);
}
#endif

#ifdef ON_UNIX_BM
int main(int argc, const char** argv)
{
		return start(argc,argv);
}
#endif

