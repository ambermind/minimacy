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
int BytecodeArgc[OPCODE_NB]=
{
0,
1,
0,
0,
1,
0,
0,
BC_JUMP_SIZE,
0,
0,
0,
1,
0,
0,
LWLEN,
0,
BC_JUMP_SIZE,
0,
0,
LWLEN,
1,
0,
0,
0,
BC_JUMP_SIZE,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
1,
0,
1,
0,
0,
0,
0,
1,
0,
1,
0,
0,
0,
0,
0,
0,
0,
1,
0,
0,
0,
BC_JUMP_SIZE,
0,
0,
1,
0,
0,
0,
0,
0,
0,
0,
0,
0,
1,
0,
0,
0,
0,
1,
0,
0,
0,
0,
0,
1,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
1,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
0,
1,
0,
1,
0,
0,
1,
0,
1,
0,
};
char* BytecodeStr[OPCODE_NB]=
{
	"add",
"atomic",
"break",
"const",
"const.b",
"drop",
"dup",
"else",
"eq",
"false",
"fetch",
"fetch.b",
"final",
"first",
"float",
"ge",
"goto",
"gt",
"hd",
"int",
"int.b",
"le",
"lef",
"lt",
"mark",
"mklist",
"mod",
"mul",
"ne",
"neg",
"nil",
"non",
"nop",
"not",
"or",
"ret",
"rglob",
"rglob.b",
"rloc",
"rloc.b",
"sglobi",
"shl",
"shr",
"skip",
"skip.b",
"sloc",
"sloc.b",
"sloci",
"store",
"struct",
"sub",
"sum",
"swap",
"tfc",
"tfc.b",
"throw",
"tl",
"true",
"try",
"unmark",
"updt",
"updt.b",
"abs",
"absf",
"acos",
"addf",
"and",
"asin",
"atan",
"atan2",
"cast",
"cast.b",
"ceil",
"cos",
"cosh",
"dftab",
"dftab.b",
"div",
"divf",
"dump",
"eor",
"exec",
"exec.b",
"exp",
"floor",
"gef",
"gtf",
"holdon",
"ln",
"log",
"ltf",
"max",
"maxf",
"min",
"minf",
"mktab",
"mktab.b",
"modf",
"mulf",
"negf",
"powint",
"pow",
"prompt",
"round",
"sin",
"sinh",
"sqr",
"sqrt",
"subf",
"tan",
"tanh",
"troff",
"tron",
"lambda",
"lambda.b",
"pick",
"pick.b",
"dumpd",
"format",
"format.b",
"dfarray",
"dfarray.b",
"tablen",
};

void opcodePrint(Thread* th, int msk,LINT op,char* p,LINT ind0)
{
	char* spaces="         ";
	if ((op<0)||(op>=OPCODE_NB)) PRINTF(th,msk,"??\n");
	else if (op==OPint)
	{
		LINT v=getLsbInt(p);
		PRINTF(th,msk,"%s%s " LSD "\n",BytecodeStr[op],spaces+strlen(BytecodeStr[op]),v);
		return;
	}
	else if ((op==OPgoto)||(op==OPelse)||(op==OPmark))
	{
		LINT v=ind0+bytecodeGetJump(p);
		PRINTF(th,msk,"%s%s >" LSD "\n",BytecodeStr[op],spaces+strlen(BytecodeStr[op]),v);
		return;
	}
	else if (op==OPfloat)
	{
		LINT v=getLsbInt(p);
		PRINTF(th,msk,"%s%s %g\n",BytecodeStr[op],spaces+strlen(BytecodeStr[op]),*(LFLOAT*)&v);
		return;
	}
	else if (BytecodeArgc[op]==1)
	{
		LINT v=p[0]&255;
		PRINTF(th,msk,"%s%s " LSD "\n",BytecodeStr[op],spaces+strlen(BytecodeStr[op]),v);
		return;
	}
	else PRINTF(th,msk,"%s%s\n",BytecodeStr[op],spaces+strlen(BytecodeStr[op]));
}

void bytecodePrint(Thread* th, int msk,LB* bytecode)
{
	LINT ind=0;
	LINT len=BINLEN(bytecode);
	char* p=BINSTART(bytecode);

	p+=BC_OFFSET; len-=BC_OFFSET;
	PRINTF(th,msk,"| args=" LSD " locals=" LSD "\n",BC_ARGS(bytecode),BC_LOCALS(bytecode));
	while(ind<len)
	{
		LINT op=255&(*(p++));
		PRINTF(th,msk,"| %4d   ",ind++);
//		PRINTF(th,msk,"| %02d.%4d   ",op,ind++);
		opcodePrint(th, msk,op,p,ind);
//		if (op == OPpickb) printf("found OPpickb %d\n", BytecodeArgc[op]);
		p+=BytecodeArgc[op];
		ind+=BytecodeArgc[op];
	}
}

int bytecodeIsTfc(char* p)
{
	while(*p!=OPret)
	{
		if (*p==OPnop) p++;
		else if (*p==OPgoto)
		{
			p++;
			p+=bytecodeGetJump(p);
		}
		else return 0;
	}
	return 1;
}
void bytecodeOptimize(LB* bytecode)
{
	LINT ind=0;
	LINT len=STRLEN(bytecode);
	char* p=STRSTART(bytecode);

//	printf("bytecodeOptimize before\n");
//	bytecodePrint(LOG_ERR,bytecode);

	p+=BC_OFFSET; len-=BC_OFFSET;
	while(ind<len)
	{
		LINT op=255&(*p);
		if ((op==OPexec)&&(bytecodeIsTfc(p+1))) *p=OPtfc;
		if ((op==OPexecb)&&(bytecodeIsTfc(p+2))) *p=OPtfcb;
		p+=1+BytecodeArgc[op];
		ind+=1+BytecodeArgc[op];
	}

}

