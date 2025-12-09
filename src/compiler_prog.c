// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

Type* compileA6(Compiler* c)
{
	Type* t;

	if (!parserNext(c)) return compileTerm(c);
	
	if (!strcmp(c->parser->token,"-"))
	{
		if (!parserNext(c)) return compileA6(c);
		if (isFloat(c->parser->token))
		{
			LFLOAT i=-floatFromAsc(c->parser->token);
			if (bufferAddChar(c->bytecode,OPfloat)) return NULL;
			if (bufferAddInt(c->bytecode,*(LINT*)&i)) return NULL;
			return MM.Float;
		}
		if ((c->parser->token[0] == '0') && (c->parser->token[1] == 'd')
			&& (isDecimal(c->parser->token + 2)))
		{
			LINT i = -intFromAsc(c->parser->token + 2, 0);
			if (bcint_byte_or_int(c, i)) return NULL;
			return MM.Int;
		}
		if ((c->parser->token[0] == '0') && (c->parser->token[1] == 'x')
			&& (isHexstring(c->parser->token + 2)))
		{
			LINT i = -intFromHex(c->parser->token + 2);
			if (bcint_byte_or_int(c, i)) return NULL;
			return MM.Int;
		}
		if ((c->parser->token[0] == '0') && (c->parser->token[1] == 'b')
			&& (isBin(c->parser->token + 2)))
		{
			LINT i = -intFromBin(c->parser->token + 2);
			if (bcint_byte_or_int(c, i)) return NULL;
			return MM.Int;
		}
		if (isDecimal(c->parser->token))
		{
			LINT i;
			if ((c->fmk->forceNumbers == FORCE_NUMBER_BIGNUM)|| (c->fmk->forceNumbers == FORCE_NUMBER_MOD) || (c->fmk->forceNumbers == FORCE_NUMBER_MODOPTI))
			{
				LINT global;
				bignum b = (bignum)bigAlloc(bignumFromDec(c->parser->token));
				if (!b) return NULL;
				bignumSignSet(b, 1 - bignumSign(b));
				if (funMakerAddGlobal(c->fmk, (LB*)b, &global)) return NULL;
				if (bc_byte_or_int(c, global, OPconstb, OPconst)) return NULL;
				return MM.BigNum;
			}
			i=-intFromAsc(c->parser->token,1);

			if (c->fmk->forceNumbers==FORCE_NUMBER_FLOAT)
			{
				LFLOAT f=(LFLOAT)i;
				if (bufferAddChar(c->bytecode,OPfloat)) return NULL;
				if (bufferAddInt(c->bytecode,*(LINT*)&f)) return NULL;
				return MM.Float;
			}
			if (bcint_byte_or_int(c,i)) return NULL;
			return MM.Int;
		}
		parserRewind(c);
		if (!(t = compileA6(c))) return NULL;
		if (c->fmk->forceNumbers == FORCE_NUMBER_BIGNUM)
		{
			if (typeUnify(c, t, MM.BigNum)) return NULL;
			if (bc_opcode(c, MM.bigNeg)) return NULL;
			return MM.BigNum;
		}
		if ((c->fmk->forceNumbers == FORCE_NUMBER_MOD) || (c->fmk->forceNumbers == FORCE_NUMBER_MODOPTI))
		{
			if (typeUnify(c, t, MM.BigNum)) return NULL;
			if (bc_byte_or_int(c, c->fmk->forceModulo, OPrlocb, OPrloc)) return NULL;
			if (bc_opcode(c, MM.bigNegMod)) return NULL;
			return MM.BigNum;
		}
		if (c->fmk->forceNumbers == FORCE_NUMBER_FLOAT)
		{
			if (typeUnify(c, t, MM.Float)) return NULL;
			if (bufferAddChar(c->bytecode, OPnegf)) return NULL;
			return MM.Float;
		}
		if (typeUnify(c,t,MM.Int)) return NULL;
		if (bufferAddChar(c->bytecode,OPneg)) return NULL;
		return MM.Int;
	}
	else if (!strcmp(c->parser->token,"~"))
	{
		if (!(t=compileA6(c))) return NULL;
		if (bufferAddChar(c->bytecode,OPnot)) return NULL;
		if (typeUnify(c,t,MM.Int)) return NULL;
		return MM.Int;
	}
	else if (!strcmp(c->parser->token,"-."))
	{
		if (!(t=compileA6(c))) return NULL;
		if (typeUnify(c,t,MM.Float)) return NULL;
		if (bufferAddChar(c->bytecode,OPnegf)) return NULL;
		return MM.Float;
	}
	else if (isFloat(c->parser->token))
	{
		LFLOAT i=floatFromAsc(c->parser->token);
		if (bufferAddChar(c->bytecode,OPfloat)) return NULL;
		if (bufferAddInt(c->bytecode,*(LINT*)&i)) return NULL;
		return MM.Float;
	}
	parserRewind(c);
	return compileTerm(c);
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
			parserRewind(c);
			return t;
		}
		if (typeUnify(c,t,MM.Int)) return NULL;
		if (!(t=compileA6(c))) return NULL;
		if (typeUnify(c,t,MM.Int)) return NULL;
		if (bufferAddChar(c->bytecode,op)) return NULL;
    }
}

Type* compileA4(Compiler* c)
{
	Type* t;
	Type* typ;
	int op;
	LINT opDef = -1;

	if (!(t=compileA5(c))) return NULL;
	while(1)
    {
		if (!parserNext(c)) return NULL;
		if (!strcmp(c->parser->token,"*")) { op=OPmul; typ=MM.Int;}
		else if (!strcmp(c->parser->token, "**")) { op = OPpowint; typ = MM.Int; }
		else if (!strcmp(c->parser->token,"/")) { op=OPdiv; typ=MM.Int;}
		else if (!strcmp(c->parser->token,"%")) { op=OPmod; typ=MM.Int;}
		else if (!strcmp(c->parser->token,"*.")) { op=OPmulf; typ=MM.Float;}
		else if (!strcmp(c->parser->token,"**.")) { op=OPpow; typ=MM.Float;}
		else if (!strcmp(c->parser->token,"/.")) { op=OPdivf; typ=MM.Float;}
		else if (!strcmp(c->parser->token,"%.")) { op=OPmodf; typ=MM.Float;}
		else
		{
			parserRewind(c);
			return t;
		}
		if (c->fmk->forceNumbers==FORCE_NUMBER_FLOAT)
		{
			if (op==OPmul) { op=OPmulf; typ=MM.Float;}
			if (op == OPpowint) { op = OPpow; typ = MM.Float; }
			if (op==OPdiv) { op=OPdivf; typ=MM.Float;}
			if (op==OPmod) { op=OPmodf; typ=MM.Float;}
		}
		if (c->fmk->forceNumbers == FORCE_NUMBER_BIGNUM)
		{
			if (op == OPmul) { opDef = MM.bigMul;  typ = MM.BigNum; }
			if (op == OPpowint) { opDef = MM.bigExp;  typ = MM.BigNum; }
			if (op == OPdiv) { opDef = MM.bigDiv;  typ = MM.BigNum; }
			if (op == OPmod) { opDef = MM.bigMod;  typ = MM.BigNum; }
		}
		if (c->fmk->forceNumbers == FORCE_NUMBER_MOD)
		{
			if (op == OPmul) { opDef = MM.bigMulMod;  typ = MM.BigNum; }
			if (op == OPpowint) { opDef = MM.bigExpMod;  typ = MM.BigNum; }
			if (op == OPdiv) { opDef = MM.bigDivMod;  typ = MM.BigNum; }
		}
		if (c->fmk->forceNumbers == FORCE_NUMBER_MODOPTI)
		{
			if (op == OPmul) { opDef = MM.bigMulModBarrett;  typ = MM.BigNum; }
			if (op == OPpowint) { opDef = MM.bigExpModBarrett;  typ = MM.BigNum; }
			if (op == OPdiv) { opDef = MM.bigDivModBarrett;  typ = MM.BigNum; }
			if (op == OPmod) { opDef = MM.bigModBarrett;  typ = MM.BigNum; }
		}
		if (typeUnify(c,t,typ)) return NULL;

		if (opDef == MM.bigModBarrett)
		{
			if (bc_byte_or_int(c, c->fmk->forceModulo, OPrlocb, OPrloc)) return NULL;
			if (bc_byte_or_int(c, c->fmk->forceMu, OPrlocb, OPrloc)) return NULL;
			if (bc_opcode(c, opDef)) return NULL;
			continue;
		}

		if (!(t=compileA5(c))) return NULL;
		if (typeUnify(c,t,typ)) return NULL;
		if (opDef >= 0) {
			if ((c->fmk->forceNumbers == FORCE_NUMBER_MOD) || (c->fmk->forceNumbers == FORCE_NUMBER_MODOPTI))
			{
				if (bc_byte_or_int(c, c->fmk->forceModulo, OPrlocb, OPrloc)) return NULL;

				if (c->fmk->forceNumbers == FORCE_NUMBER_MODOPTI)
				{
					if (bc_byte_or_int(c, c->fmk->forceMu, OPrlocb, OPrloc)) return NULL;
				}
			}
			if (bc_opcode(c, opDef)) return NULL;
		}
		else if (bufferAddChar(c->bytecode, op)) return NULL;
		t=typ;
    }
}

Type* compileA3(Compiler* c)
{
	Type* t;
	Type* typ;
	int op;
	LINT opDef = -1;
	
	if (!(t=compileA4(c))) return NULL;
	while(1)
    {
		if (!parserNext(c)) return NULL;
		if (!strcmp(c->parser->token,"+")) { op=OPadd; typ=MM.Int;}
		else if (!strcmp(c->parser->token,"-")) { op=OPsub; typ=MM.Int;}
		else if (!strcmp(c->parser->token,"+.")) { op=OPaddf; typ=MM.Float;}
		else if (!strcmp(c->parser->token,"-.")) { op=OPsubf; typ=MM.Float;}
		else
		{
			parserRewind(c);
			return t;
		}
		if (c->fmk->forceNumbers==FORCE_NUMBER_FLOAT)
		{
			if (op==OPadd) { op=OPaddf; typ=MM.Float;}
			if (op==OPsub) { op=OPsubf; typ=MM.Float;}
		}
		if (c->fmk->forceNumbers == FORCE_NUMBER_BIGNUM)
		{
			if (op == OPadd) { opDef = MM.bigAdd;  typ = MM.BigNum; }
			if (op == OPsub) { opDef = MM.bigSub;  typ = MM.BigNum; }
		}
		if ((c->fmk->forceNumbers == FORCE_NUMBER_MOD)|| (c->fmk->forceNumbers == FORCE_NUMBER_MODOPTI))
		{
			if (op == OPadd) { opDef = MM.bigAddMod;  typ = MM.BigNum; }
			if (op == OPsub) { opDef = MM.bigSubMod;  typ = MM.BigNum; }
		}
		if (typeUnify(c,t,typ)) return NULL;
		if (!(t=compileA4(c))) return NULL;
		if (typeUnify(c,t,typ)) return NULL;
		if (opDef >= 0) {
			if ((c->fmk->forceNumbers == FORCE_NUMBER_MOD) || (c->fmk->forceNumbers == FORCE_NUMBER_MODOPTI))
			{
				if (bc_byte_or_int(c, c->fmk->forceModulo, OPrlocb, OPrloc)) return NULL;
			}
			if (bc_opcode(c, opDef)) return NULL;
		}
		else if (bufferAddChar(c->bytecode, op)) return NULL;
		t=typ;
    }
}

Type* compileA2(Compiler* c)
{
	Type* t;
	Type* typ;
	Type* t2;
	int op;
	LINT opDef = -1;
	
	if (!(t=compileA3(c))) return NULL;

	if (!parserNext(c)) return NULL;
	if (!strcmp(c->parser->token,"==")) { op=OPeq; typ=NULL;}
	else if (!strcmp(c->parser->token,"!=")) { op=OPne; typ=NULL;}
	else if (!strcmp(c->parser->token,"<>")) { op=OPne; typ=NULL;}
	else if (!strcmp(c->parser->token,"<")) { op=OPlt; typ=MM.Int;}
	else if (!strcmp(c->parser->token,">")) { op=OPgt; typ=MM.Int;}
	else if (!strcmp(c->parser->token,"<=")) { op=OPle; typ=MM.Int;}
	else if (!strcmp(c->parser->token,">=")) { op=OPge; typ=MM.Int;}
	else if (!strcmp(c->parser->token,"<.")) { op=OPltf; typ=MM.Float;}
	else if (!strcmp(c->parser->token,">.")) { op=OPgtf; typ=MM.Float;}
	else if (!strcmp(c->parser->token,"<=.")) { op=OPlef; typ=MM.Float;}
	else if (!strcmp(c->parser->token,">=.")) { op=OPgef; typ=MM.Float;}
	else
	{
		parserRewind(c);
		return t;
	}
	if (c->fmk->forceNumbers==FORCE_NUMBER_FLOAT)
	{
		if (op==OPlt) { op=OPltf; typ=MM.Float;}
		if (op==OPgt) { op=OPgtf; typ=MM.Float;}
		if (op==OPle) { op=OPlef; typ=MM.Float;}
		if (op==OPge) { op=OPgef; typ=MM.Float;}
	}
	if ((c->fmk->forceNumbers == FORCE_NUMBER_BIGNUM) || (c->fmk->forceNumbers == FORCE_NUMBER_MOD) || (c->fmk->forceNumbers == FORCE_NUMBER_MODOPTI))
	{
		if (op == OPgt) { opDef = MM.bigGT;  typ = MM.BigNum; }
		if (op == OPge) { opDef = MM.bigGE;  typ = MM.BigNum; }
		if (op == OPlt) { opDef = MM.bigLT;  typ = MM.BigNum; }
		if (op == OPle) { opDef = MM.bigLE;  typ = MM.BigNum; }
	}

	if ((typ)&&(typeUnify(c,t,typ))) return NULL;

	if (!(t2=compileA3(c))) return NULL;
	if (typeUnify(c,t,t2)) return NULL;

	if (opDef >= 0) {
		if (bc_opcode(c, opDef)) return NULL;
	}
	else if (bufferAddChar(c->bytecode, op)) return NULL;
	return MM.Boolean;
}

Type* compileA1(Compiler* c)
{
	Type* t;
	
	if ((parserNext(c))&&(!strcmp(c->parser->token,"!")))
    {
		if (!(t=compileA1(c))) return NULL;
		if (typeUnify(c,t,MM.Boolean)) return NULL;
		if (bufferAddChar(c->bytecode,OPnon)) return NULL;
		return t;
	}
	parserRewind(c);
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
			parserRewind(c);
			return t;
        }
		if (typeUnify(c,t,MM.Boolean)) return NULL;

		if (bufferAddChar(c->bytecode,OPdup)) return NULL;
		if (strcmp(c->parser->token,"&&")&& bufferAddChar(c->bytecode,OPnon)) return NULL;
		if (bufferAddChar(c->bytecode,OPelse)) return NULL;
		bc_i=bytecodeAddEmptyJump(c);
		if (bc_i < 0) return NULL;

		if (bufferAddChar(c->bytecode,OPdrop)) return NULL;
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
	if (strcmp(c->parser->token,"::"))
    {
		parserRewind(c);
		return t;
    }
	TYPE_PUSH_NULL(t);
	if (!(t=compileExpression(c))) return NULL;
	TYPE_PUSH_NULL(t);
	u = typeCopy(MM.fun_u0_list_u0_list_u0); if (!u) return NULL;
	if (!(t=typeUnifyFromStack(c,u))) return NULL;

	if (bufferAddChar(c->bytecode,OPmklist)) return NULL;
	
	return t;
}

Type* compileExpression(Compiler* c)
{
	Type* definedType = NULL;
	Type* result;
	if (!parserNext(c)) return compileError(c, "unexpected end of file\n");

	if (!strcmp(c->parser->token, ":")) {
		definedType = compilerParseTypeDef(c, 0, &c->fmk->typeLabels);
		if (!definedType) return NULL;
	}
	else parserRewind(c);
	result = compileList(c);

	if (definedType && typeUnify(c, result, definedType)) return NULL;
	return result;
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
			parserRewind(c);
			return t;
		}
		if (!parserNext(c)) return NULL;
		if (!strcmp(c->parser->token,")"))
		{
			parserRewind(c);
			return t;
		}
		parserRewind(c);
		if (bufferAddChar(c->bytecode,OPdrop)) return NULL;
	}
}
