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

int boot(void)
{
	Thread* th = MM.scheduler;
	LB* src = fsReadPackage(th, BOOT_FILE, NULL, 0);
	if (!src)
	{
		PRINTF(LOG_USER, "\nboot file '%s"SUFFIX_CODE"' not found\n", BOOT_FILE);
		fsReadPackage(th, BOOT_FILE, NULL, 1);
#ifdef USE_FS_ANSI
		ansiHelpBiosFinder();
#endif
		return -1;
	}
	FUN_PUSH_PNT(src);
	FUN_PUSH_NIL;
	FUN_PUSH_NIL;
	return 0;
}

int start(int argc, char** argv)
{
	int i;
	int standalone = 1;
	int help = 0;
	
//	char* args[]={"minimacy.exe","-h"};	argc=2;argv=args;
//	for (i = 0; i < argc; i++) PRINTF(LOG_DEV,"arg %d: '%s'\n", i, argv[i]);
	termInit();
	if (argc && (strlen(argv[argc - 1]) >= strlen(SUFFIX_CODE))&& !strcmp(argv[argc-1] + strlen(argv[argc-1]) - strlen(SUFFIX_CODE), SUFFIX_CODE)) standalone = 0;

	for (i = 0; i < argc; i++)
	{
		char* arg = argv[i];
		if ((!strcmp(arg, "-h"))|| (!strcmp(arg, "-?")) || (!strcmp(arg, "-help")))
		{
			termSetMask(LOG_ALL);
			help = 1;
		}
		if ((!strcmp(arg, "-q")) || (!strcmp(arg,"-quiet"))) termSetMask(LOG_USER);
		if ((!strcmp(arg, "-v")) || (!strcmp(arg,"-verbose"))) termSetMask(LOG_ALL);
	}

	PRINTF(LOG_SYS,"\n> Minimacy - Sylvain Huet - 2020-24 - "VERSION_MINIMACY"/"DEVICE_MODE"\n");
	PRINTF(LOG_SYS,"> ----\n");

//	if (!standalone) termSetMask(LOG_USER);	// disable LOG_SYS

	if ((LWLEN==8)&&(sizeof(LINT)==8)&&(sizeof(LFLOAT)==8)) PRINTF(LOG_SYS,"> 64 bits\n");
	else if ((LWLEN==4)&&(sizeof(LINT)==4)&&(sizeof(LFLOAT)==4) && (sizeof(double) == 8)) PRINTF(LOG_SYS,"> 32 bits\n");
	else
	{
		PRINTF(LOG_USER,"> wrong system [P:%d I:%d F:%d D:%d]\n",LWLEN,sizeof(LINT),sizeof(LFLOAT),sizeof(double));
		return -1;
	}

	if (help)
	{
		PRINTF(LOG_USER, "\nUsage: minimacy [-h|-?|-help] [-dir path] [-v|-verbose] [-q|-quiet] [-compile] [*.mcy]\n\n");
		PRINTF(LOG_USER, "Options:\n");
		PRINTF(LOG_USER, "  -h|-?|-help: display this help message\n");
		PRINTF(LOG_USER, "  -q|-quiet: disable the stderr output\n");
		PRINTF(LOG_USER, "  -v|-verbose: enable the stderr output\n");
		PRINTF(LOG_USER, "  -compile: compile and stop, this will be useful for any IDE extension\n");
		PRINTF(LOG_USER, "  [*.mcy]: when the last argument is a *.mcy file:\n");
		PRINTF(LOG_USER, "    - the stderr output is disabled until further notice (-v option or programmatically)\n");
		PRINTF(LOG_USER, "    - the *.mcy file is launched instead of the system_path/pkg/boot.mcy file\n\n");
#ifdef USE_FS_ANSI
		ansiHelpBiosFinder();
#endif
		goto cleanup;
	}
#ifdef USE_MEMORY_C
	PRINTF(LOG_SYS,"> Allocated memory: %lld bytes\n",MEMORY_C_LENGTH);
	cMallocInit();
#endif
	fsInit();
	bytecodeInit();
	if (hwInit()) goto cleanup;
	do
	{
		memoryInit(argc,argv);
		if (fsMount(NULL, argc, argv, standalone)) goto cleanup;
		if (boot())
		{
			memoryEnd();
			goto cleanup;
		}
		compilePromptAndRun(MM.scheduler);
		romdiskReleaseUserDisk();
//		PRINTF(LOG_SYS, "\n--\n\n");
	} while (memoryEnd());
	fsRelease();
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
	getchar();
#endif
	return 0;
}
int MTargc=0;
char** MTargv=NULL;
MTHREAD_START startThread(void* custom)
{
	start(MTargc,MTargv);
	return MTHREAD_RETURN;
}
#ifdef GROUP_UNIX
int startInThread(int argc, char** argv)
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
int main(int argc, char** argv)
{
	return start(argc,argv);
}
#endif

