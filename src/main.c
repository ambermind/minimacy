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
	LB* src = fileReadPackage(th, BOOT_FILE, SUFFIX_CODE, NULL);
	if (!src)
	{
		PRINTF(th, LOG_USER, "boot file '%s' not found\n", BOOT_FILE);
		return -1;
	}
	STACKPUSH(th, PNTTOVAL(src));
	STACKPUSH(th, NIL);
	STACKPUSH(th, NIL);
	return 0;
}
extern int BignumCounter;
extern int BigCounter;

int start(int argc, char** argv)
{
	int i;
	int standalone = 1;
	int help = 0;
	
//	char* args[]={"minimacy.exe","-h"};	argc=2;argv=args;

	termInit();
	if (argc && !strcmp(argv[argc-1] + strlen(argv[argc-1]) - strlen(SUFFIX_CODE), SUFFIX_CODE)) standalone = 0;

	for (i = 0; i < argc; i++)
	{
		char* arg = argv[i];
		if ((!strcmp(arg, "-h"))|| (!strcmp(arg, "-?")) || (!strcmp(arg, "-help")))
		{
			termSetMask(LOG_ALL);
			help = 1;
		}
		if (!strcmp(arg, "-dir")) i++;
		if ((!strcmp(arg, "-q")) || (!strcmp(arg,"-quiet"))) termSetMask(LOG_USER);
		if ((!strcmp(arg, "-v")) || (!strcmp(arg,"-verbose"))) termSetMask(LOG_ALL);
	}

	PRINTF(NULL, LOG_ERR,"\nMinimacy - Sylvain Huet - 2020-22 - "VERSION_MINIMACY"/"DEVICE_MODE"\n");
	PRINTF(NULL, LOG_ERR,"----\n");

	if (!standalone) termSetMask(LOG_USER);	// disable LOG_ERR

	if ((LWLEN==8)&&(sizeof(LINT)==8)&&(sizeof(LFLOAT)==8)) PRINTF(NULL, LOG_ERR,"64 bits\n");
	else if ((LWLEN==4)&&(sizeof(LINT)==4)&&(sizeof(LFLOAT)==4) && (sizeof(double) == 8)) PRINTF(NULL, LOG_ERR,"32 bits\n");
	else
	{
		PRINTF(NULL, LOG_USER,"wrong system [P:%d I:%d F:%d D:%d]\n",LWLEN,sizeof(LINT),sizeof(LFLOAT),sizeof(double));
		return -1;
	}

	if (help)
	{
		PRINTF(NULL, LOG_USER, "\nUsage: minimacy [-h|-?|-help] [-dir path] [-v|-verbose] [-q|-quiet] [-compile] [*.mcy]\n\n");
		PRINTF(NULL, LOG_USER, "Options:\n");
		PRINTF(NULL, LOG_USER, "  -h|-?|-help: display this help message\n");
		PRINTF(NULL, LOG_USER, "  -dir path: specify the minimacy folder, such that the bios is in path/rom/bios/\n");
		PRINTF(NULL, LOG_USER, "  -q|-quiet: disable the stderr output\n");
		PRINTF(NULL, LOG_USER, "  -v|-verbose: enable the stderr output\n");
		PRINTF(NULL, LOG_USER, "  -compile: compile and stop, this will be useful for any IDE extension\n");
		PRINTF(NULL, LOG_USER, "  [*.mcy]: when the last argument is a *.mcy file:\n");
		PRINTF(NULL, LOG_USER, "    - the stderr output is disabled until further notice (-v option or programmatically)\n");
		PRINTF(NULL, LOG_USER, "    - the *.mcy file is launched instead of the system_path/pkg/boot.mcy file\n\n");
		hwHelpBiosFinder(NULL);
		goto cleanup;
	}
	if (hwInit(NULL, argc, argv, standalone)) goto cleanup;

	do
	{
		memoryInit(argc,argv);
		if (boot())
		{
			memoryEnd();
			goto cleanup;
		}
		compilePromptAndRun(MM.scheduler);
//		PRINTF(MM.scheduler, LOG_ERR, "\n--\n\n");
	} while (memoryEnd());

cleanup:
	termEnd();
//	printf("BignumCounter=%d\n", BignumCounter);
//	printf("BigCounter   =%d\n", BigCounter);

#ifdef DBG_MEM
	MainTerm.mask = LOG_ALL;
	PRINTF(NULL, LOG_ERR, "-------\n");
	PRINTF(NULL, LOG_ERR, "leaks:\n");
	_CrtSetReportMode( _CRT_WARN, _CRTDBG_MODE_FILE );
	_CrtSetReportFile( _CRT_WARN, _CRTDBG_FILE_STDOUT );
	_CrtDumpMemoryLeaks();
	PRINTF(NULL, LOG_ERR, "end of line\n");
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
int startInThread(int argc, char** argv)
{
//    printf("sigpipe main thread\n");
#ifdef ON_UNIX
	signal(SIGPIPE,SIG_IGN);
#endif
    MTargc=argc;
    MTargv=argv;
	hwThreadCreate(startThread, NULL);
	return 0;
}
#ifdef USE_COCOA
#else
#ifdef USE_IOS
#else
int main(int argc, char** argv)
{
	return start(argc,argv);
}
#endif
#endif

