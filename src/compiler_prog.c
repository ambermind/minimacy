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

Type* compileA6(Compiler* c)
{
	Type* t;

	if (!parserNext(c)) return compileTerm(c,0);
	
	if (!strcmp(c->parser->token,"-"))
	{
		if (!parserNext(c)) return compileA6(c);
		if (isfloat(c->parser->token))
		{
			LFLOAT i=-ls_atof(c->parser->token);
			if (bufferAddchar(c->th, c->bytecode,OPfloat)) return NULL;
			if (bufferAddint(c->th, c->bytecode,*(LINT*)&i)) return NULL;
			return MM.F;
		}
		else if ((c->parser->token[0] == '0') && (c->parser->token[1] == 'd')
			&& (isdecimal(c->parser->token + 2)))
		{
			LINT i = -ls_atoi(c->parser->token + 2, 0);
			if (bcint_byte_or_int(c, i)) return NULL;
			return MM.I;
		}
		else if ((c->parser->token[0] == '0') && (c->parser->token[1] == 'x')
			&& (ishexadecimal(c->parser->token + 2)))
		{
			LINT i = -ls_htoi(c->parser->token + 2);
			if (bcint_byte_or_int(c, i)) return NULL;
			return MM.I;
		}
		else if (isdecimal(c->parser->token))
		{
			LINT i;
			if ((c->fmk->forceNumbers == FORCE_NUMBER_BIGNUM)|| (c->fmk->forceNumbers == FORCE_NUMBER_MOD) || (c->fmk->forceNumbers == FORCE_NUMBER_MODOPTI))
			{
				LINT global;
				bignum b = (bignum)bigAlloc(c->th, bignumFromDec(c->parser->token));
				if (!b) return NULL;
				bignumSignSet(b, 1 - bignumSign(b));
				if (funMakerAddGlobal(c->fmk, (LB*)b, &global)) return NULL;
				if (bc_byte_or_int(c, global, OPconstb, OPconst)) return NULL;
				return MM.BigNum;
			}
			i=-ls_atoi(c->parser->token,1);

			if (c->fmk->forceNumbers==FORCE_NUMBER_FLOAT)
			{
				LFLOAT f=(LFLOAT)i;
				if (bufferAddchar(c->th, c->bytecode,OPfloat)) return NULL;
				if (bufferAddint(c->th, c->bytecode,*(LINT*)&f)) return NULL;
				return MM.F;
			}
			if (bcint_byte_or_int(c,i)) return NULL;
			return MM.I;
		}
		parserGiveback(c);
		if (c->fmk->forceNumbers == FORCE_NUMBER_BIGNUM) 
		{
			LINT global;
			if (funMakerNeedGlobal(c->fmk, (LB*)MM.bigNeg, &global)) return NULL;
			if (bc_byte_or_int(c, global, OPrglobb, OPrglob)) return NULL;
			if (!(t = compileA6(c))) return NULL;
			if (typeUnify(c, t, MM.BigNum)) return NULL;
			if (bc_byte_or_int(c, 1, OPexecb, OPexec)) return NULL;
			return MM.BigNum;
		}
		if ((c->fmk->forceNumbers == FORCE_NUMBER_MOD) || (c->fmk->forceNumbers == FORCE_NUMBER_MODOPTI))
		{
			LINT global;
			if (funMakerNeedGlobal(c->fmk, (LB*)MM.bigNegMod, &global)) return NULL;
			if (bc_byte_or_int(c, global, OPrglobb, OPrglob)) return NULL;
			if (!(t = compileA6(c))) return NULL;
			if (typeUnify(c, t, MM.BigNum)) return NULL;
			if (bc_byte_or_int(c, c->fmk->forceModulo, OPrlocb, OPrloc)) return NULL;
			if (bc_byte_or_int(c, 2, OPexecb, OPexec)) return NULL;
			return MM.BigNum;
		}
		if (c->fmk->forceNumbers == FORCE_NUMBER_FLOAT)
		{
			if (!(t = compileA6(c))) return NULL;
			if (typeUnify(c, t, MM.F)) return NULL;
			if (bufferAddchar(c->th, c->bytecode, OPnegf)) return NULL;
			return MM.F;
		}
		if (!(t=compileA6(c))) return NULL;
		if (typeUnify(c,t,MM.I)) return NULL;
		if (bufferAddchar(c->th, c->bytecode,OPneg)) return NULL;
		return MM.I;
	}
	else if (!strcmp(c->parser->token,"~"))
	{
		if (!(t=compileA6(c))) return NULL;
		if (bufferAddchar(c->th, c->bytecode,OPnot)) return NULL;
		if (typeUnify(c,t,MM.I)) return NULL;
		return MM.I;
	}
	else if (!strcmp(c->parser->token,"-."))
	{
		if (!(t=compileA6(c))) return NULL;
		if (typeUnify(c,t,MM.F)) return NULL;
		if (bufferAddchar(c->th, c->bytecode,OPnegf)) return NULL;
		return MM.F;
	}
	else if (isfloat(c->parser->token))
	{
		LFLOAT i=ls_atof(c->parser->token);
		if (bufferAddchar(c->th, c->bytecode,OPfloat)) return NULL;
		if (bufferAddint(c->th, c->bytecode,*(LINT*)&i)) return NULL;
		return MM.F;
	}
	parserGiveback(c);
	return compileTerm(c,0);
}



Type* compileA5(Compiler* c)
{
	Type* t;
	int op;
	
	if (!(t=compileA6(c))) return NULL;
	while(1)
    {
		if (!parserNext(c)) return NULL;
		if (!strcmp(c->parser->token,"&")) op=OPand;
		else if (!strcmp(c->parser->token,"|")) op=OPor;
		else if (!strcmp(c->parser->token,"^")) op=OPeor;
		else if (!strcmp(c->parser->token,"<<")) op=OPshl;
		else if (!strcmp(c->parser->token,">>")) op=OPshr;
		else
		{
			parserGiveback(c);
			return t;
		}
		if (typeUnify(c,t,MM.I)) return NULL;
		if (!(t=compileA6(c))) return NULL;
		if (typeUnify(c,t,MM.I)) return NULL;
		if (bufferAddchar(c->th, c->bytecode,op)) return NULL;
    }
}

Type* compileA4(Compiler* c)
{
	Type* t;
	Type* typ;
	int op;
	Ref* opRef = NULL;

	if (!(t=compileA5(c))) return NULL;
	while(1)
    {
		if (!parserNext(c)) return NULL;
		if (!strcmp(c->parser->token,"*")) { op=OPmul; typ=MM.I;}
		else if (!strcmp(c->parser->token, "**")) { op = OPpowint; typ = MM.I; }
		else if (!strcmp(c->parser->token,"/")) { op=OPdiv; typ=MM.I;}
		else if (!strcmp(c->parser->token,"%")) { op=OPmod; typ=MM.I;}
		else if (!strcmp(c->parser->token,"*.")) { op=OPmulf; typ=MM.F;}
		else if (!strcmp(c->parser->token,"**.")) { op=OPpow; typ=MM.F;}
		else if (!strcmp(c->parser->token,"/.")) { op=OPdivf; typ=MM.F;}
		else if (!strcmp(c->parser->token,"%.")) { op=OPmodf; typ=MM.F;}
		else
		{
			parserGiveback(c);
			return t;
		}
		if (c->fmk->forceNumbers==FORCE_NUMBER_FLOAT)
		{
			if (op==OPmul) { op=OPmulf; typ=MM.F;}
			if (op == OPpowint) { op = OPpow; typ = MM.F; }
			if (op==OPdiv) { op=OPdivf; typ=MM.F;}
			if (op==OPmod) { op=OPmodf; typ=MM.F;}
		}
		if (c->fmk->forceNumbers == FORCE_NUMBER_BIGNUM)
		{
			if (op == OPmul) { opRef = MM.bigMul;  typ = MM.BigNum; }
			if (op == OPpowint) { opRef = MM.bigExp;  typ = MM.BigNum; }
			if (op == OPdiv) { opRef = MM.bigDiv;  typ = MM.BigNum; }
			if (op == OPmod) { opRef = MM.bigMod;  typ = MM.BigNum; }
			if (opRef) {
				LINT global;
				if (funMakerNeedGlobal(c->fmk, (LB*)opRef, &global)) return NULL;
				if (bc_byte_or_int(c, global, OPrglobb, OPrglob)) return NULL;
				if (bufferAddchar(c->th, c->bytecode, OPswap)) return NULL;
			}
		}
		if (c->fmk->forceNumbers == FORCE_NUMBER_MOD)
		{
			if (op == OPmul) { opRef = MM.bigMulMod;  typ = MM.BigNum; }
			if (op == OPpowint) { opRef = MM.bigExpMod;  typ = MM.BigNum; }
			if (op == OPdiv) { opRef = MM.bigDivMod;  typ = MM.BigNum; }
			if (opRef) {
				LINT global;
				if (funMakerNeedGlobal(c->fmk, (LB*)opRef, &global)) return NULL;
				if (bc_byte_or_int(c, global, OPrglobb, OPrglob)) return NULL;
				if (bufferAddchar(c->th, c->bytecode, OPswap)) return NULL;
			}
		}
		if (c->fmk->forceNumbers == FORCE_NUMBER_MODOPTI)
		{
			if (op == OPmul) { opRef = MM.bigMulModBarrett;  typ = MM.BigNum; }
			if (op == OPpowint) { opRef = MM.bigExpModBarrett;  typ = MM.BigNum; }
			if (op == OPdiv) { opRef = MM.bigDivModBarrett;  typ = MM.BigNum; }
			if (op == OPmod) { opRef = MM.bigModBarrett;  typ = MM.BigNum; }
			if (opRef) {
				LINT global;
				if (funMakerNeedGlobal(c->fmk, (LB*)opRef, &global)) return NULL;
				if (bc_byte_or_int(c, global, OPrglobb, OPrglob)) return NULL;
				if (bufferAddchar(c->th, c->bytecode, OPswap)) return NULL;
			}
		}
		if (typeUnify(c,t,typ)) return NULL;

		if (opRef == MM.bigModBarrett)
		{
			if (bc_byte_or_int(c, c->fmk->forceModulo, OPrlocb, OPrloc)) return NULL;
			if (bc_byte_or_int(c, c->fmk->forceMu, OPrlocb, OPrloc)) return NULL;
			if (bc_byte_or_int(c, 3, OPexecb, OPexec)) return NULL;
			continue;
		}

		if (!(t=compileA5(c))) return NULL;
		if (typeUnify(c,t,typ)) return NULL;
		if (opRef)
		{
			if (c->fmk->forceNumbers == FORCE_NUMBER_MOD)
			{
				if (bc_byte_or_int(c, c->fmk->forceModulo, OPrlocb, OPrloc)) return NULL;
				if (bc_byte_or_int(c, 3, OPexecb, OPexec)) return NULL;
			}
			else if (c->fmk->forceNumbers == FORCE_NUMBER_MODOPTI)
			{
				if (bc_byte_or_int(c, c->fmk->forceModulo, OPrlocb, OPrloc)) return NULL;
				if (bc_byte_or_int(c, c->fmk->forceMu, OPrlocb, OPrloc)) return NULL;
				if (bc_byte_or_int(c, 4, OPexecb, OPexec)) return NULL;
			}
			else if (bc_byte_or_int(c, 2, OPexecb, OPexec)) return NULL;
		}
		else if (bufferAddchar(c->th, c->bytecode, op)) return NULL;
		t=typ;
    }
}

Type* compileA3(Compiler* c)
{
	Type* t;
	Type* typ;
	int op;
	Ref* opRef = NULL;
	
	if (!(t=compileA4(c))) return NULL;
	while(1)
    {
		if (!parserNext(c)) return NULL;
		if (!strcmp(c->parser->token,"+")) { op=OPadd; typ=MM.I;}
		else if (!strcmp(c->parser->token,"-")) { op=OPsub; typ=MM.I;}
		else if (!strcmp(c->parser->token,"+.")) { op=OPaddf; typ=MM.F;}
		else if (!strcmp(c->parser->token,"-.")) { op=OPsubf; typ=MM.F;}
		else
		{
			parserGiveback(c);
			return t;
		}
		if (c->fmk->forceNumbers==FORCE_NUMBER_FLOAT)
		{
			if (op==OPadd) { op=OPaddf; typ=MM.F;}
			if (op==OPsub) { op=OPsubf; typ=MM.F;}
		}
		if (c->fmk->forceNumbers == FORCE_NUMBER_BIGNUM)
		{
			if (op == OPadd) { opRef = MM.bigAdd;  typ = MM.BigNum; }
			if (op == OPsub) { opRef = MM.bigSub;  typ = MM.BigNum; }
			if (opRef) {
				LINT global;
				if (funMakerNeedGlobal(c->fmk, (LB*)opRef, &global)) return NULL;
				if (bc_byte_or_int(c, global, OPrglobb, OPrglob)) return NULL;
				if (bufferAddchar(c->th, c->bytecode, OPswap)) return NULL;
			}
		}
		if ((c->fmk->forceNumbers == FORCE_NUMBER_MOD)|| (c->fmk->forceNumbers == FORCE_NUMBER_MODOPTI))
		{
			if (op == OPadd) { opRef = MM.bigAddMod;  typ = MM.BigNum; }
			if (op == OPsub) { opRef = MM.bigSubMod;  typ = MM.BigNum; }
			if (opRef) {
				LINT global;
				if (funMakerNeedGlobal(c->fmk, (LB*)opRef, &global)) return NULL;
				if (bc_byte_or_int(c, global, OPrglobb, OPrglob)) return NULL;
				if (bufferAddchar(c->th, c->bytecode, OPswap)) return NULL;
			}
		}
		if (typeUnify(c,t,typ)) return NULL;
		if (!(t=compileA4(c))) return NULL;
		if (typeUnify(c,t,typ)) return NULL;
		if (opRef)
		{
			if ((c->fmk->forceNumbers == FORCE_NUMBER_MOD) || (c->fmk->forceNumbers == FORCE_NUMBER_MODOPTI))
			{
				if (bc_byte_or_int(c, c->fmk->forceModulo, OPrlocb, OPrloc)) return NULL;
				if (bc_byte_or_int(c, 3, OPexecb, OPexec)) return NULL;
			}
			else if (bc_byte_or_int(c, 2, OPexecb, OPexec)) return NULL;
		}
		else if (bufferAddchar(c->th, c->bytecode,op)) return NULL;
		t=typ;
    }
}

Type* compileA2(Compiler* c)
{
	Type* t;
	Type* typ;
	int op;
	Ref* opRef = NULL;
	
	if (!(t=compileA3(c))) return NULL;
	while(1)
    {
		Type* t2;
		if (!parserNext(c)) return NULL;
		if (!strcmp(c->parser->token,"==")) { op=OPeq; typ=NULL;}
		else if (!strcmp(c->parser->token,"!=")) { op=OPne; typ=NULL;}
		else if (!strcmp(c->parser->token,"<>")) { op=OPne; typ=NULL;}
		else if (!strcmp(c->parser->token,"<")) { op=OPlt; typ=MM.I;}
		else if (!strcmp(c->parser->token,">")) { op=OPgt; typ=MM.I;}
		else if (!strcmp(c->parser->token,"<=")) { op=OPle; typ=MM.I;}
		else if (!strcmp(c->parser->token,">=")) { op=OPge; typ=MM.I;}
		else if (!strcmp(c->parser->token,"<.")) { op=OPltf; typ=MM.F;}
		else if (!strcmp(c->parser->token,">.")) { op=OPgtf; typ=MM.F;}
		else if (!strcmp(c->parser->token,"<=.")) { op=OPlef; typ=MM.F;}
		else if (!strcmp(c->parser->token,">=.")) { op=OPgef; typ=MM.F;}
		else
		{
			parserGiveback(c);
			return t;
		}
		if (c->fmk->forceNumbers==FORCE_NUMBER_FLOAT)
		{
			if (op==OPlt) { op=OPltf; typ=MM.F;}
			if (op==OPgt) { op=OPgtf; typ=MM.F;}
			if (op==OPle) { op=OPlef; typ=MM.F;}
			if (op==OPge) { op=OPgef; typ=MM.F;}
		}
		if ((c->fmk->forceNumbers == FORCE_NUMBER_BIGNUM) || (c->fmk->forceNumbers == FORCE_NUMBER_MOD) || (c->fmk->forceNumbers == FORCE_NUMBER_MODOPTI))
		{
			if (op == OPgt) { opRef = MM.bigGT;  typ = MM.BigNum; }
			if (op == OPge) { opRef = MM.bigGE;  typ = MM.BigNum; }
			if (op == OPlt) { opRef = MM.bigLT;  typ = MM.BigNum; }
			if (op == OPle) { opRef = MM.bigLE;  typ = MM.BigNum; }
			if (opRef) {
				LINT global;
				if (funMakerNeedGlobal(c->fmk, (LB*)opRef, &global)) return NULL;
				if (bc_byte_or_int(c, global, OPrglobb, OPrglob)) return NULL;
				if (bufferAddchar(c->th, c->bytecode, OPswap)) return NULL;
			}
		}

		if ((typ)&&(typeUnify(c,t,typ))) return NULL;

		if (!(t2=compileA3(c))) return NULL;
		if (typeUnify(c,t,t2)) return NULL;

		if (opRef)
		{
			if (bc_byte_or_int(c, 2, OPexecb, OPexec)) return NULL;
		}
		else if (bufferAddchar(c->th, c->bytecode, op)) return NULL;
		t=MM.Boolean;
    }
}

Type* compileA1(Compiler* c)
{
	Type* t;
	
	if ((parserNext(c))&&(!strcmp(c->parser->token,"!")))
    {
		if (!(t=compileA1(c))) return NULL;
		if (typeUnify(c,t,MM.Boolean)) return NULL;
		if (bufferAddchar(c->th, c->bytecode,OPnon)) return NULL;
		return t;
	}
	parserGiveback(c);
	return compileA2(c);
}

Type* compileArithm(Compiler* c)
{
	Type* t;
	
	if (!(t=compileA1(c))) return NULL;
	while(1)
    {
		LINT bc_i;
		if (!parserNext(c)) return NULL;
		if ((strcmp(c->parser->token,"&&"))&&(strcmp(c->parser->token,"||")))
        {
			parserGiveback(c);
			return t;
        }
		if (typeUnify(c,t,MM.Boolean)) return NULL;

		if (bufferAddchar(c->th, c->bytecode,OPdup)) return NULL;
		if (strcmp(c->parser->token,"&&")&& bufferAddchar(c->th, c->bytecode,OPnon)) return NULL;
		if (bufferAddchar(c->th, c->bytecode,OPelse)) return NULL;
		bc_i=bytecodeAddEmptyJump(c);
		if (bc_i < 0) return NULL;

		if (bufferAddchar(c->th, c->bytecode,OPdrop)) return NULL;
		if (!(t=compileA1(c))) return NULL;
		if (typeUnify(c,t,MM.Boolean)) return NULL;
		
		bytecodeSetJump(c,bc_i,bytecodePin(c));
    }
}

Type* compileList(Compiler* c)
{
	Type* t;
	Type* u;
	
	if (!(t=compileArithm(c))) return NULL;;
	
	if (!parserNext(c)) return NULL;
	if (strcmp(c->parser->token,":"))
    {
		parserGiveback(c);
		return t;
    }
	TYPEPUSH_NULL(c,t);
	if (!(t=compileExpression(c))) return NULL;
	TYPEPUSH_NULL(c,t);
	u = typeCopy(c->th, MM.fun_u0_list_u0_list_u0); if (!u) return NULL;
	if (!(t=typeUnifyFromStack(c,u))) return NULL;

	if (bufferAddchar(c->th, c->bytecode,OPmklist)) return NULL;
	
	return t;
}

Type* compileSpecial(Compiler* c)
{
	Type* t;
	if ((!parserNext(c))||(!islabel(c->parser->token))) return compileError(c,"Compiler: label expected (found '%s')\n",compileToken(c));

	if (!strcmp(c->parser->token, "atomic"))
	{
		if (c->pkg!=MM.system) return compileError(c, "Compiler: atomic is not allowed to simple users\n");
		if (bufferAddchar(c->th, c->bytecode, OPatomic)) return NULL;
		if (bufferAddchar(c->th, c->bytecode, 1)) return NULL;
		if (parserAssume(c, "{")) return NULL;
		if (!(t = compileProgram(c))) return NULL;
		if (parserAssume(c, "}")) return NULL;
		if (bufferAddchar(c->th, c->bytecode, OPatomic)) return NULL;
		if (bufferAddchar(c->th, c->bytecode, 0)) return NULL;
		return t;
	}
	if (!strcmp(c->parser->token,"float"))
	{
		int backup=c->fmk->forceNumbers;
		c->fmk->forceNumbers=FORCE_NUMBER_FLOAT;
		if (parserAssume(c,"{")) return NULL;
		if (!(t= compileProgram(c))) return NULL;
		if (parserAssume(c,"}")) return NULL;
		c->fmk->forceNumbers=backup;
		return t;
	}
	if (!strcmp(c->parser->token,"integer"))
	{
		int backup=c->fmk->forceNumbers;
		c->fmk->forceNumbers=FORCE_NUMBER_NONE;
		if (parserAssume(c,"{")) return NULL;
		if (!(t= compileProgram(c))) return NULL;
		if (parserAssume(c,"}")) return NULL;
		c->fmk->forceNumbers=backup;
		return t;
	}
	if (!strcmp(c->parser->token, "bigNum"))
	{
		int backup = c->fmk->forceNumbers;
		c->fmk->forceNumbers = FORCE_NUMBER_BIGNUM;
		if (parserAssume(c, "{")) return NULL;
		if (!(t = compileProgram(c))) return NULL;
		if (parserAssume(c, "}")) return NULL;
		c->fmk->forceNumbers = backup;
		return t;
	}
	if (!strcmp(c->parser->token, "mod"))
	{
		Locals* modulo;
		Locals* localsBefore = c->fmk->locals;
		LINT moduloBackup = c->fmk->forceModulo;
		int backup = c->fmk->forceNumbers;
		c->fmk->forceNumbers = FORCE_NUMBER_BIGNUM;

		if (!(t = compileExpression(c))) return NULL;
		if (typeUnify(c, t, MM.BigNum)) return NULL;
		modulo = funMakerAddLocal(c, ""); if (!modulo) return NULL;
		c->fmk->forceModulo = modulo->index;
		c->fmk->forceNumbers = FORCE_NUMBER_MOD;
		if (bc_byte_or_int(c, c->fmk->forceModulo, OPslocb, OPsloc)) return NULL;

		if (parserAssume(c, "{")) return NULL;
		if (!(t = compileProgram(c))) return NULL;
		if (parserAssume(c, "}")) return NULL;
		c->fmk->locals = localsBefore;
		c->fmk->forceNumbers = backup;
		c->fmk->forceModulo = moduloBackup;
		return t;
	}
	if (!strcmp(c->parser->token, "modBarrett"))
	{
		Locals* modulo;
		Locals* mu;
		Locals* localsBefore = c->fmk->locals;
		LINT moduloBackup = c->fmk->forceModulo;
		LINT muBackup = c->fmk->forceMu;
		int backup = c->fmk->forceNumbers;
		c->fmk->forceNumbers = FORCE_NUMBER_BIGNUM;

		if (!(t = compileExpression(c))) return NULL;
		if (typeUnify(c, t, MM.BigNum)) return NULL;
		modulo = funMakerAddLocal(c, ""); if (!modulo) return NULL;
		c->fmk->forceModulo = modulo->index;
		if (bc_byte_or_int(c, c->fmk->forceModulo, OPslocb, OPsloc)) return NULL;

		if (!(t = compileExpression(c))) return NULL;
		if (typeUnify(c, t, MM.BigNum)) return NULL;
		mu = funMakerAddLocal(c, ""); if (!mu) return NULL;
		c->fmk->forceMu = mu->index;
		if (bc_byte_or_int(c, c->fmk->forceMu, OPslocb, OPsloc)) return NULL;

		c->fmk->forceNumbers = FORCE_NUMBER_MODOPTI;
		if (parserAssume(c, "{")) return NULL;
		if (!(t = compileProgram(c))) return NULL;
		if (parserAssume(c, "}")) return NULL;
		c->fmk->forceNumbers = backup;
		c->fmk->forceModulo = moduloBackup;
		c->fmk->forceMu = muBackup;
		c->fmk->locals = localsBefore;
		return t;
	}
	return compileError(c,"Compiler: unknown special (found '%s')\n",compileToken(c));
}
Type* compileExpression(Compiler* c)
{
	if ((parserNext(c))&&(!strcmp(c->parser->token,"\\"))) return compileSpecial(c);
	parserGiveback(c);
	return compileList(c);
}

Type* compileProgram(Compiler* c)
{
	Type* t;
	while(1)
	{
		if (!(t=compileExpression(c))) return NULL;;
		if (!parserNext(c)) return NULL;
		if (strcmp(c->parser->token,";"))
		{
			parserGiveback(c);
			return t;
		}
		if (!parserNext(c)) return NULL;
		if ((!strcmp(c->parser->token,")"))|| (!strcmp(c->parser->token, "}")))
		{
			parserGiveback(c);
			return t;
		}
		parserGiveback(c);
		if (bufferAddchar(c->th, c->bytecode,OPdrop)) return NULL;
	}
}
