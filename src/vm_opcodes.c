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
{OPabs, 0, "abs"},
{OPabsf, 0, "absf"},
{OPacos, 0, "acos"},
{OPadd, 0, "add"},
{OPaddf, 0, "addf"},
{OPand, 0, "and"},
{OParraylen, 0, "arraylen"},
{OPasin, 0, "asin"},
{OPatan, 0, "atan"},
{OPatan2, 0, "atan2"},
{OPatomic, 1, "atomic"},
{OPbreak, 0, "break"},
{OPcast, 0, "cast"},
{OPcastb, 1, "cast.b"},
{OPceil, 0, "ceil"},
{OPconst, 0, "const"},
{OPconstb, 1, "const.b"},
{OPcos, 0, "cos"},
{OPcosh, 0, "cosh"},
{OPdfarray, 0, "dfarray"},
{OPdfarrayb, 1, "dfarray.b"},
{OPdftup, 0, "dftup"},
{OPdftupb, 1, "dftup.b"},
{OPdiv, 0, "div"},
{OPdivf, 0, "divf"},
{OPdrop, 0, "drop"},
{OPdump, 0, "dump"},
{OPdumpd, 0, "dumpd"},
{OPdup, 0, "dup"},
{OPelse, BC_JUMP_SIZE, "else"},
{OPeor, 0, "eor"},
{OPeq, 0, "eq"},
{OPexec, 0, "exec"},
{OPexecb, 1, "exec.b"},
{OPexp, 0, "exp"},
{OPfalse, 0, "false"},
{OPfetch, 0, "fetch"},
{OPfetchb, 1, "fetch.b"},
{OPfinal, 0, "final"},
{OPfirst, 0, "first"},
{OPfloat, LWLEN, "float"},
{OPfloor, 0, "floor"},
{OPformat, 0, "format"},
{OPformatb, 1, "format.b"},
{OPge, 0, "ge"},
{OPgef, 0, "gef"},
{OPgoto, BC_JUMP_SIZE, "goto"},
{OPgt, 0, "gt"},
{OPgtf, 0, "gtf"},
{OPhd, 0, "head"},
{OPholdon, 0, "holdon"},
{OPint, LWLEN, "int"},
{OPintb, 1, "int.b"},
{OPisinf, 0, "isinf"},
{OPisnan, 0, "isnan"},
{OPlambda, 0, "lambda"},
{OPlambdab, 1, "lambda.b"},
{OPle, 0, "le"},
{OPlef, 0, "lef"},
{OPln, 0, "ln"},
{OPlog, 0, "log"},
{OPlt, 0, "lt"},
{OPltf, 0, "ltf"},
{OPmark, BC_JUMP_SIZE, "mark"},
{OPmax, 0, "max"},
{OPmaxf, 0, "maxf"},
{OPmin, 0, "min"},
{OPminf, 0, "minf"},
{OPmklist, 0, "mklist"},
{OPmod, 0, "mod"},
{OPmodf, 0, "modf"},
{OPmul, 0, "mul"},
{OPmulf, 0, "mulf"},
{OPne, 0, "ne"},
{OPneg, 0, "neg"},
{OPnegf, 0, "negf"},
{OPnil, 0, "nil"},
{OPnon, 0, "non"},
{OPnop, 0, "nop"},
{OPnot, 0, "not"},
{OPor, 0, "or"},
{OPpick, 0, "pick"},
{OPpickb, 1, "pick.b"},
{OPpow, 0, "pow"},
{OPpowint, 0, "powint"},
{OPprompt, 0, "prompt"},
{OPret, 0, "ret"},
{OPrglob, 0, "rglob"},
{OPrglobb, 1, "rglob.b"},
{OPrloc, 0, "rloc"},
{OPrlocb, 1, "rloc.b"},
{OPround, 0, "round"},
{OPsglobi, 0, "sglobi"},
{OPshl, 0, "shl"},
{OPshr, 0, "shr"},
{OPsin, 0, "sin"},
{OPsinh, 0, "sinh"},
{OPskip, 0, "skip"},
{OPskipb, 1, "skip.b"},
{OPsloc, 0, "sloc"},
{OPslocb, 1, "sloc.b"},
{OPsloci, 0, "sloci"},
{OPsqr, 0, "sqr"},
{OPsqrt, 0, "sqrt"},
{OPstore, 0, "store"},
{OPstruct, 0, "struct"},
{OPsub, 0, "sub"},
{OPsubf, 0, "subf"},
{OPsum, 0, "sum"},
{OPswap, 0, "swap"},
{OPtan, 0, "tan"},
{OPtanh, 0, "tanh"},
{OPtfc, 0, "tfc"},
{OPtfcb, 1, "tfc.b"},
{OPthrow, 0, "throw"},
{OPtl, 0, "tail"},
{OPtroff, 0, "troff"},
{OPtron, 0, "tron"},
{OPtrue, 0, "true"},
{OPtry, BC_JUMP_SIZE, "try"},
{OPunmark, 0, "unmark"},
{OPupdt, 0, "updt"},
{OPupdtb, 1, "updt.b"},
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
	LINT len=BIN_LENGTH(bytecode);
	char* p=BIN_START(bytecode);

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
	LINT len=STR_LENGTH(bytecode);
	char* p=STR_START(bytecode);

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

