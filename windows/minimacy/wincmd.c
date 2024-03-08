#include"../../src/minimacy.h"
#include"resource.h"

extern LPCTSTR IDI_ICON;

int main(int argc, char** argv)
{
	IDI_ICON = (LPCTSTR)IDI_ICON1;
	SetConsoleOutputCP(1252);
	SetConsoleCP(1252);
	return start(argc, argv);
}