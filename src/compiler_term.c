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

Type* compilePrompt(Compiler* c)
{
	Type* t;
	if (!(t = compileExpression(c))) return NULL;	// get source code
	if (typeUnify(c, t, MM.Str)) return NULL;
	if (!(t = compileExpression(c))) return NULL;	// get pkg
	if (typeUnify(c, t, MM.Package)) return NULL;

	if (bufferAddChar(c->bytecode, OPprompt)) return NULL;
	if (bc_byte_or_int(c, 0, OPexecb, OPexec)) return NULL;
	if (bc_byte_or_int(c, 2, OPdftupb, OPdftup)) return NULL;
	return typeAlloc(TYPECODE_TUPLE, NULL, 2, MM.Type, t);
}

Type* compileCompile(Compiler* c)
{
	Type* t;
	if (!(t = compileExpression(c))) return NULL;	// get source code
	if (typeUnify(c, t, MM.Str)) return NULL;
	if (!(t = compileExpression(c))) return NULL;	// get pkg
	if (typeUnify(c, t, MM.Package)) return NULL;

	if (bufferAddChar(c->bytecode, OPprompt)) return NULL;
	if (bufferAddChar(c->bytecode, OPdrop)) return NULL;
	return MM.Type;
}

Type* compileIf(Compiler* c)
{
	Type* t;	
	LINT bc_else,bc_goto;
	if (!(t=compileExpression(c))) return NULL;

	if (typeUnify(c,t,MM.Boolean)) return NULL;

	if (parserAssume(c,"then")) return NULL;
	if (bufferAddChar(c->bytecode,OPelse)) return NULL;
	bc_else=bytecodeAddEmptyJump(c);
	if (bc_else < 0) return NULL;
	if (!(t=compileExpression(c))) return NULL;

	if (bufferAddChar(c->bytecode,OPgoto)) return NULL;
	bc_goto=bytecodeAddEmptyJump(c);
	if (bc_goto < 0) return NULL;
	bytecodeSetJump(c,bc_else,bytecodePin(c));

	if ((parserNext(c))&&(!strcmp(c->parser->token,"else")))
	{
		Type* t2;
		if (!(t2=compileExpression(c))) return NULL;
		if (typeUnify(c,t,t2)) return NULL;
	}
	else
	{
		parserGiveback(c);
		if (bufferAddChar(c->bytecode,OPnil)) return NULL;
	}
	bytecodeSetJump(c,bc_goto,bytecodePin(c));
	return t;
}

Type* compileReturn(Compiler* c)
{
	Type* t;
	if (!(t=compileExpression(c))) return NULL;
	if (typeUnify(c,t,c->fmk->resultType)) return NULL;
	if (bufferAddChar(c->bytecode,OPfinal)) return NULL;
	if (bufferAddChar(c->bytecode,OPret)) return NULL;
	return typeAllocUndef();
}

Type* compileSpecial(Compiler* c)
{
	Type* t;
	if ((!parserNext(c)) || (!islabel(c->parser->token))) return compileError(c,"label expected (found '%s')\n", compileToken(c));

	if (!strcmp(c->parser->token, "atomic"))
	{
		if (c->pkg != MM.system) return compileError(c,"atomic is not allowed to simple users\n");
		if (bufferAddChar(c->bytecode, OPatomic)) return NULL;
		if (bufferAddChar(c->bytecode, 1)) return NULL;
		if (!(t = compileExpression(c))) return NULL;
		if (bufferAddChar(c->bytecode, OPatomic)) return NULL;
		if (bufferAddChar(c->bytecode, 0)) return NULL;
		return t;
	}
	if (!strcmp(c->parser->token, "float"))
	{
		int backup = c->fmk->forceNumbers;
		c->fmk->forceNumbers = FORCE_NUMBER_FLOAT;
		if (!(t = compileExpression(c))) return NULL;
		c->fmk->forceNumbers = backup;
		return t;
	}
	if (!strcmp(c->parser->token, "integer"))
	{
		int backup = c->fmk->forceNumbers;
		c->fmk->forceNumbers = FORCE_NUMBER_NONE;
		if (!(t = compileExpression(c))) return NULL;
		c->fmk->forceNumbers = backup;
		return t;
	}
	if (!strcmp(c->parser->token, "bigNum"))
	{
		int backup = c->fmk->forceNumbers;
		c->fmk->forceNumbers = FORCE_NUMBER_BIGNUM;
		if (!(t = compileExpression(c))) return NULL;
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

		if (!(t = compileExpression(c))) return NULL;
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
		if (!(t = compileExpression(c))) return NULL;
		c->fmk->forceNumbers = backup;
		c->fmk->forceModulo = moduloBackup;
		c->fmk->forceMu = muBackup;
		c->fmk->locals = localsBefore;
		return t;
	}
	return compileError(c,"unknown special (found '%s')\n", compileToken(c));
}

Type* compileTerm(Compiler* c,int noPoint)
{
	Type* t;
	if (!parserNext(c)) return compileError(c,"term expected (found '%s')\n",compileToken(c));

	if (!strcmp(c->parser->token,"("))
	{
		if (!(t=compileProgram(c))) return NULL;
		if (parserAssume(c,")")) return NULL;
		if (noPoint) return t;
		return compileGetPoint(c, t);
	}
	else if (!strcmp(c->parser->token,"["))
	{
		int nval=0;
		int parserSave = parserIndex(c);
		if (parserNext(c))
		{
			if (islabel(c->parser->token))
			{
				Def* p=compileGetDef(c);
				if ((p)&&(p->code==DEF_CODE_FIELD))	return compileFields(c,p);
				if ((p) && (p->code == DEF_CODE_STRUCT))	return compileEmptyStruct(c, p);
			}
			parserJump(c,parserSave);
		}
		while(1)
		{
			if (!parserNext(c)) return compileError(c,"']' expected (found '%s')\n",compileToken(c));

			if (!strcmp(c->parser->token,"]"))
			{
				if (bc_byte_or_int(c,nval,OPdftupb,OPdftup)) return NULL;
				return typeAllocFromStack(NULL, TYPECODE_TUPLE, nval);
			}
			parserGiveback(c);
			if (!(t=compileExpression(c))) return NULL;
			TYPE_PUSH_NULL(t);
			nval++;
		}
	}
	else if (!strcmp(c->parser->token,"{"))
	{
		Type* type;
		int nval=0;
		Type* u = typeAllocUndef();  if (!u) return NULL;
		type=typeAlloc(TYPECODE_ARRAY,NULL,1,u); if (!type) return NULL;
		while(1)
		{
			if (!parserNext(c)) return compileError(c,"'}' expected (found '%s')\n",compileToken(c));

			if (!strcmp(c->parser->token,"}"))
			{
				if (bc_byte_or_int(c,nval, OPdfarrayb, OPdfarray)) return NULL;
				return type;
			}
			parserGiveback(c);
			if (!(t=compileExpression(c))) return NULL;
			if (typeUnify(c,t,type->child[0])) return NULL;
			nval++;
		}
	}
	else if (!strcmp(c->parser->token,"if"))
		return compileIf(c);
	else if (!strcmp(c->parser->token,"let"))
		return compileLet(c);
	else if (!strcmp(c->parser->token,"set"))
		return compileSet(c);
	else if (!strcmp(c->parser->token,"while"))
		return compileWhile(c);
	else if (!strcmp(c->parser->token,"for"))
		return compileFor(c);
	else if (!strcmp(c->parser->token,"break"))
		return compileBreak(c);
	else if (!strcmp(c->parser->token,"match"))
		return compileMatch(c);
	else if (!strcmp(c->parser->token,"call"))
		return compileCall(c);
	else if (!strcmp(c->parser->token, "strFormat"))
		return compileFormat(c,MM.Str);
	else if (!strcmp(c->parser->token, "bytesFormat"))
		return compileFormat(c,MM.Bytes);
	else if ((!strcmp(c->parser->token,"return"))&&(!funMakerIsForVar(c)))
		return compileReturn(c);
	else if (!strcmp(c->parser->token,"lambda"))
		return compileLambda(c);
	else if (!strcmp(c->parser->token, "_prompt"))
		return compilePrompt(c);
	else if (!strcmp(c->parser->token, "_compile"))
		return compileCompile(c);
	else if (!strcmp(c->parser->token,"try"))
		return compileTry(c);
	else if (!strcmp(c->parser->token,"throw"))
		return compileThrow(c);
	else if (!strcmp(c->parser->token,"nil"))
	{
		if (bufferAddChar(c->bytecode,OPnil)) return NULL;
		return typeAllocUndef();
	}
	else if (!strcmp(c->parser->token, "true"))
	{
		if (bufferAddChar(c->bytecode, OPtrue)) return NULL;
		return MM.Boolean;
	}
	else if (!strcmp(c->parser->token, "false"))
	{
		if (bufferAddChar(c->bytecode, OPfalse)) return NULL;
		return MM.Boolean;
	}
	else if (!strcmp(c->parser->token, "hide"))
		return compileDefHide(c);
	else if (!strcmp(c->parser->token,"'"))
	{
		if (!parserNext(c)) return compileError(c,"'char expected (found '%s')\n",compileToken(c));

		if (bcint_byte_or_int(c,c->parser->token[0]&255)) return NULL;
		if (parserAssume(c,"'")) return NULL;
		return MM.Int;
	}
	else if (!strcmp(c->parser->token, "\\")) return compileSpecial(c);
	else if (islabel(c->parser->token))
		return compileDef(c,noPoint);
	else if (isdecimal(c->parser->token))
	{
		LINT i;
		if ((c->fmk->forceNumbers == FORCE_NUMBER_BIGNUM) || (c->fmk->forceNumbers == FORCE_NUMBER_MOD) || (c->fmk->forceNumbers == FORCE_NUMBER_MODOPTI))
		{
			LINT global;
			LB* res = bigAlloc(bignumFromDec(c->parser->token));
			if (!res) return NULL;
			if (funMakerAddGlobal(c->fmk, res, &global)) return NULL;
			if (bc_byte_or_int(c, global, OPconstb, OPconst)) return NULL;
			return MM.BigNum;
		}
		i=ls_atoi(c->parser->token,1);
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
	else if ((c->parser->token[0] == '0') && (c->parser->token[1] == 'd')
	&& (isdecimal(c->parser->token + 2)))
	{
		LINT i = ls_atoi(c->parser->token+2, 0);
		if (bcint_byte_or_int(c, i)) return NULL;
		return MM.Int;
	}
	else if ((c->parser->token[0]=='0')&&(c->parser->token[1]=='x')
		&&(ishexadecimal(c->parser->token+2)))
	{
		LINT i;
		if ((c->fmk->forceNumbers == FORCE_NUMBER_BIGNUM) || (c->fmk->forceNumbers == FORCE_NUMBER_MOD) || (c->fmk->forceNumbers == FORCE_NUMBER_MODOPTI))
		{
			LINT global;
			LB* res;
			res = bigAlloc(bignumFromHex(c->parser->token + 2));
			if (!res) return NULL;
			if (funMakerAddGlobal(c->fmk, res, &global)) return NULL;
			if (bc_byte_or_int(c, global, OPconstb, OPconst)) return NULL;
			return MM.BigNum;
		}
		i=ls_htoi(c->parser->token+2);
		if (bcint_byte_or_int(c,i)) return NULL;
		return MM.Int;
	}
	else if (c->parser->token[0]=='"')	// parse string constants
	{
		LINT global;
		LB *data;
		if (parserGetstring(c,MM.tmpBuffer)) // this function starts to reset the Buffer
			return compileError(c,"string has no ending\n");
		
		data=memoryAllocFromBuffer(MM.tmpBuffer); if (!data) return NULL;
		if (funMakerAddGlobal(c->fmk,data, &global)) return NULL;
		if (bc_byte_or_int(c,global,OPconstb,OPconst)) return NULL;
		return MM.Str;
	}
	else if (!strcmp(c->parser->token,"#"))
		return compilePointer(c);
	return compileError(c,"unexpected term '%s'\n",compileToken(c));
}	
