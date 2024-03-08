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
struct BytecodeOps {
	int op;
	int argc;
	char* str;
};

struct BytecodeOps BytecodeDef[OPCODE_NB] = {
{OPadd, 0, "add"},
{OPatomic, 1, "atomic"},
{OPbreak, 0, "break"},
{OPconst, 0, "const"},
{OPconstb, 1, "const.b"},
{OPdrop, 0, "drop"},
{OPdup, 0, "dup"},
{OPelse, BC_JUMP_SIZE, "else"},
{OPeq, 0, "eq"},
{OPfalse, 0, "false"},
{OPfetch, 0, "fetch"},
{OPfetchb, 1, "fetch.b"},
{OPfinal, 0, "final"},
{OPfirst, 0, "first"},
{OPfloat, LWLEN, "float"},
{OPge, 0, "ge"},
{OPgoto, BC_JUMP_SIZE, "goto"},
{OPgt, 0, "gt"},
{OPhd, 0, "head"},
{OPint, LWLEN, "int"},
{OPintb, 1, "int.b"},
{OPle, 0, "le"},
{OPlef, 0, "lef"},
{OPlt, 0, "lt"},
{OPmark, BC_JUMP_SIZE, "mark"},
{OPmklist, 0, "mklist"},
{OPmod, 0, "mod"},
{OPmul, 0, "mul"},
{OPne, 0, "ne"},
{OPneg, 0, "neg"},
{OPnil, 0, "nil"},
{OPnon, 0, "non"},
{OPnop, 0, "nop"},
{OPnot, 0, "not"},
{OPor, 0, "or"},
{OPret, 0, "ret"},
{OPrglob, 0, "rglob"},
{OPrglobb, 1, "rglob.b"},
{OPrloc, 0, "rloc"},
{OPrlocb, 1, "rloc.b"},
{OPsglobi, 0, "sglobi"},
{OPshl, 0, "shl"},
{OPshr, 0, "shr"},
{OPskip, 0, "skip"},
{OPskipb, 1, "skip.b"},
{OPsloc, 0, "sloc"},
{OPslocb, 1, "sloc.b"},
{OPsloci, 0, "sloci"},
{OPstore, 0, "store"},
{OPstruct, 0, "struct"},
{OPsub, 0, "sub"},
{OPsum, 0, "sum"},
{OPswap, 0, "swap"},
{OPtfc, 0, "tfc"},
{OPtfcb, 1, "tfc.b"},
{OPthrow, 0, "throw"},
{OPtl, 0, "tail"},
{OPtrue, 0, "true"},
{OPtry, BC_JUMP_SIZE, "try"},
{OPunmark, 0, "unmark"},
{OPupdt, 0, "updt"},
{OPupdtb, 1, "updt.b"},
{OPabs, 0, "abs"},
{OPabsf, 0, "absf"},
{OPacos, 0, "acos"},
{OPaddf, 0, "addf"},
{OPand, 0, "and"},
{OPasin, 0, "asin"},
{OPatan, 0, "atan"},
{OPatan2, 0, "atan2"},
{OPcast, 0, "cast"},
{OPcastb, 1, "cast.b"},
{OPceil, 0, "ceil"},
{OPcos, 0, "cos"},
{OPcosh, 0, "cosh"},
{OPdftab, 0, "dftab"},
{OPdftabb, 1, "dftab.b"},
{OPdiv, 0, "div"},
{OPdivf, 0, "divf"},
{OPdump, 0, "dump"},
{OPeor, 0, "eor"},
{OPexec, 0, "exec"},
{OPexecb, 1, "exec.b"},
{OPexp, 0, "exp"},
{OPfloor, 0, "floor"},
{OPgef, 0, "gef"},
{OPgtf, 0, "gtf"},
{OPholdon, 0, "holdon"},
{OPln, 0, "ln"},
{OPlog, 0, "log"},
{OPltf, 0, "ltf"},
{OPmax, 0, "max"},
{OPmaxf, 0, "maxf"},
{OPmin, 0, "min"},
{OPminf, 0, "minf"},
{OPmktab, 0, "mktab"},
{OPmktabb, 1, "mktab.b"},
{OPmodf, 0, "modf"},
{OPmulf, 0, "mulf"},
{OPnegf, 0, "negf"},
{OPpowint, 0, "powint"},
{OPpow, 0, "pow"},
{OPprompt, 0, "prompt"},
{OPround, 0, "round"},
{OPsin, 0, "sin"},
{OPsinh, 0, "sinh"},
{OPsqr, 0, "sqr"},
{OPsqrt, 0, "sqrt"},
{OPsubf, 0, "subf"},
{OPtan, 0, "tan"},
{OPtanh, 0, "tanh"},
{OPtroff, 0, "troff"},
{OPtron, 0, "tron"},
{OPlambda, 0, "lambda"},
{OPlambdab, 1, "lambda.b"},
{OPpick, 0, "pick"},
{OPpickb, 1, "pick.b"},
{OPdumpd, 0, "dumpd"},
{OPformat, 0, "format"},
{OPformatb, 1, "format.b"},
{OPdfarray, 0, "dfarray"},
{OPdfarrayb, 1, "dfarray.b"},
{OPtablen, 0, "tablen"},
{OPisnan, 0, "isnan"},
{OPisinf, 0, "isinf"},
};
int BytecodeArgc[OPCODE_NB];
char* BytecodeStr[OPCODE_NB];

void bytecodeInit()
{
	int i;
	for (i = 0; i < OPCODE_NB; i++) {
		BytecodeArgc[BytecodeDef[i].op] = BytecodeDef[i].argc;
		BytecodeStr[BytecodeDef[i].op] = BytecodeDef[i].str;
	}
}
void opcodePrint(Thread* th, int msk,LINT op,char* p,LINT ind0)
{
	char* spaces="         ";
	if ((op<0)||(op>=OPCODE_NB)) PRINTF(msk,"??\n");
	else if (op==OPint)
	{
		LINT v=getLsbInt(p);
		PRINTF(msk,"%s%s " LSD "\n",BytecodeStr[op],spaces+strlen(BytecodeStr[op]),v);
		return;
	}
	else if ((op==OPgoto)||(op==OPelse)||(op==OPmark))
	{
		LINT v=ind0+bytecodeGetJump(p);
		PRINTF(msk,"%s%s >" LSD "\n",BytecodeStr[op],spaces+strlen(BytecodeStr[op]),v);
		return;
	}
	else if (op==OPfloat)
	{
		LINT v=getLsbInt(p);
		PRINTF(msk,"%s%s %g\n",BytecodeStr[op],spaces+strlen(BytecodeStr[op]),*(LFLOAT*)&v);
		return;
	}
	else if (BytecodeArgc[op]==1)
	{
		LINT v=p[0]&255;
		PRINTF(msk,"%s%s " LSD "\n",BytecodeStr[op],spaces+strlen(BytecodeStr[op]),v);
		return;
	}
	else PRINTF(msk,"%s%s\n",BytecodeStr[op],spaces+strlen(BytecodeStr[op]));
}

void bytecodePrint(Thread* th, int msk,LB* bytecode)
{
	LINT ind=0;
	LINT len=BINLEN(bytecode);
	char* p=BINSTART(bytecode);

	p+=BC_OFFSET; len-=BC_OFFSET;
	PRINTF(msk,"| args=" LSD " locals=" LSD "\n",BC_ARGS(bytecode),BC_LOCALS(bytecode));
	while(ind<len)
	{
		LINT op=255&(*(p++));
		PRINTF(msk,"| %4d   ",ind++);
//		PRINTF(msk,"| %02d.%4d   ",op,ind++);
		opcodePrint(th, msk,op,p,ind);
//		if (op == OPpickb) PRINTF(LOG_DEV,"found OPpickb %d\n", BytecodeArgc[op]);
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

//	PRINTF(LOG_DEV,"bytecodeOptimize before\n");
//	bytecodePrint(LOG_SYS,bytecode);

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

