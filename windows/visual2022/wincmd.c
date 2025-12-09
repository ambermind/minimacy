#include"../../src/minimacy.h"
#include"resource.h"
#ifdef ON_WINDOWS
extern LPCTSTR IDI_ICON;
#endif

void cFloatTests();

//const char* const Argv[]={"minimacy","C:\\home\\dev\\mcy\\docMaker\\usr.dok.mcy"};
int main(int argc, char** argv)
{
#ifdef ON_WINDOWS
	IDI_ICON = (LPCTSTR)IDI_ICON1;
//	SetConsoleOutputCP(1252);
	SetConsoleCP(1252);
	SetConsoleOutputCP(65001);
//	SetConsoleCP(65001);
#endif
//	return start(2, Argv);
//	cFloatTests();
	return start(argc, argv);
}