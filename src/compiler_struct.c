// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

// type name is already in the stack
// '=' has been parsed
Type* compileStructure1(Compiler* c, Def* structDef)
{
	int nFields = 0;
	if ((!parserNext(c)) || ((!isLabel(c->parser->token)) && strcmp(c->parser->token, "[") ))
		return compileError(c,"type name or '[' expected (found '%s')\n", compileToken(c));

	if (strcmp(c->parser->token, "["))
	{
		parserRewind(c);
		if (compilerSkipTypeDef(c)) return NULL;
		if (parserAssume(c, "+")) return NULL;
		if (parserAssume(c, "[")) return NULL;
	}

	while(1)
	{
		Type* fieldType;
		Def* fieldDef;
		LB* fieldName;
		LINT pIndex;

		if ((!parserNext(c))||((!isLabel(c->parser->token))&&strcmp(c->parser->token,"]")))
			return compileError(c,"field name or ']' expected (found '%s')\n",compileToken(c));
		

		pIndex = parserIndex(c);
		if (!isLabel(c->parser->token)) break;	// "]" case
		if (!exportLabelListIsSingle(c, c->parser->token)) return compileError(c,"the export declaration is not compatible with a type (should have no argument)\n");

		if (pkgFirstGet(c->pkg, c->parser->token)) return compileError(c,"'%s' already defined\n", compileToken(c));
		fieldName = memoryAllocStr(c->parser->token, -1); if (!fieldName) return NULL;

		if (!parserNext(c)) return compileError(c,"unexpected end of file\n");

		if (!strcmp(c->parser->token, ":"))
		{
			if (compilerSkipTypeDef(c)) return NULL;
		}
		else parserRewind(c);

		fieldType = typeAlloc(TYPECODE_FIELD, NULL, 2, typeAllocUndef(), typeAllocUndef()); if (!fieldType) return NULL;

		fieldDef=defAlloc(DEF_CODE_FIELD,0,structDef->val, structDef->valType, fieldType); if (!fieldDef) return NULL;
		fieldDef->proto = 1;
		defSet(structDef,VAL_FROM_PNT((LB*)fieldDef),VAL_TYPE_PNT);
		fieldDef->parent = structDef;
		defSetParser(fieldDef, c, pIndex);
		if (pkgAddDef(c->pkg, fieldName, fieldDef)) return NULL;
		nFields++;

		if ((!parserNext(c)) || (strcmp(c->parser->token, ",") && strcmp(c->parser->token, "]")))
			return compileError(c, "',' or ']' expected (found '%s')\n", compileToken(c));
		if (!strcmp(c->parser->token, "]")) parserRewind(c);
	}
	if (parserAssume(c,";;")) return NULL;
	return structDef->type;
}
Type* compileStructure2(Compiler* c, Def* structDef, Locals* labels)
{
	Type* structType = structDef->type;
	Type* derivative = typeDerivate(structDef->type); if (!derivative) return NULL;

	if ((!parserNext(c)) || ((!isLabel(c->parser->token)) && strcmp(c->parser->token, "[")))
		return compileError(c,"type name or '[' expected (found '%s')\n", compileToken(c));

	if (strcmp(c->parser->token, "["))
	{
		Def* defParent;
		Type* t;
		if ((!(defParent = compileGetDef(c))) || (defParent->code != DEF_CODE_STRUCT))
			return compileError(c,"type struct expected (found '%s')\n", compileToken(c));
		t = defParent->type;

		if (t->nb)
		{
			int i;
			if (parserAssume(c, "{")) return NULL;
			for (i = 0; i < t->nb; i++)
			{
				Locals* lb;
				if ((!parserNext(c)) || (!isLabel(c->parser->token)))
					return compileError(c,"parameter expected (found '%s')\n", compileToken(c));
				lb = localsGet(labels, 0, c->parser->token);
				if (!lb) return compileError(c,"parameter expected (found '%s')\n", compileToken(c));
				if (lb->index != i) return compileError(c,"wrong parameter, parent structure must have the same parameters in the same order than the derivative (found '%s')\n", compileToken(c));
				if (i < t->nb - 1)
				{
					if (parserAssume(c, ",")) return NULL;
				}
			}
			if (parserAssume(c, "}")) return NULL;
		}
		structDef->parent = defParent;
		c->nbDerivations++;
		if (parserAssume(c, "+")) return NULL;
		if (parserAssume(c, "[")) return NULL;
	}

	while(1)
	{
		Def* defField;
		Type* t;

		if ((!parserNext(c))||((!isLabel(c->parser->token))&& strcmp(c->parser->token,"]")))
			return compileError(c,"field name or ']' expected (found '%s')\n",compileToken(c));

		if (!isLabel(c->parser->token)) break;
			
		defField= pkgFirstGet(c->pkg, c->parser->token);
		if (!defField) return compileError(c,"cannot find prototype of '%s'\n", compileToken(c));

		if (!parserNext(c)) return compileError(c,"unexpected end of file\n");

		if (!strcmp(c->parser->token, ":"))
		{
			t = compilerParseTypeDef(c, 1, &labels);
			if (!t) return NULL;
		}
		else
		{ 
			parserRewind(c);
			t = typeAllocWeak(); if (!t) return NULL;
		}
		if (typeUnify(c, derivative, defField->type->child[TYPEFIELD_MAIN])) return NULL;
		if (typeUnify(c, t, defField->type->child[TYPEFIELD_FIELD])) return NULL;

		if ((!parserNext(c)) || (strcmp(c->parser->token, ",") && strcmp(c->parser->token, "]")))
			return compileError(c, "',' or ']' expected (found '%s')\n", compileToken(c));
		if (!strcmp(c->parser->token, "]")) parserRewind(c);
	}
	if (parserAssume(c,";;")) return NULL;
	return structType;
}

Type* compileStructure3(Compiler* c, Def* structDef, int rec)
{
	Def* def;
	Def* defParent = structDef->parent;
	if (rec<0) return compileError(c,"loop detected in struct '%s' defined in package '%s'\n", defName(structDef), pkgName(structDef->header.pkg));
	if (!structDef->proto) return MM.Int;
	if (defParent)
	{
		if (!compileStructure3(c, defParent, rec - 1)) return NULL;
		defParent->dCI++;
		structDef->dI = defParent->dCI;
		structDef->index = defParent->index + 1;
	}
	for (def = (Def*)PNT_FROM_VAL(structDef->val); def; def = (Def*)PNT_FROM_VAL(def->val))
	{
		def->index = structDef->index++;
		def->proto = 0;
	}
	structDef->proto = 0;
	return MM.Int;
}

// first field has already been read, and is in 'def' argument
Type* compileFields(Compiler* c, Def* def)
{
	LINT global;
	Type* root;
	Def* d;
	Def* bigChild;
	Globals* g;


	if (funMakerAddGlobal(c->fmk, (LB*)NULL, &global)) return NULL;	// we may rewrite this global at the end of the function
	g = c->fmk->globals;

	if (bcint_byte_or_int(c,global)) return NULL;
	if (bufferAddChar(c->bytecode,OPstruct)) return NULL;

//	PRINTF(LOG_DEV,"compileFields, start with %s\n", defName(def));
	root = typeAllocUndef(); if (!root) return NULL;	// root is expected to become a1(BiggestChild)
	while(1)
	{
		Type* t;
		Type* tfield= typeInstance(c, def); if (!tfield) return NULL;
//		PRINTF(LOG_DEV,"unify main\n");
		if (typeUnify(c,root,tfield->child[TYPEFIELD_MAIN])) return NULL;
		
		if (parserAssume(c,"=")) return NULL;
		if (!(t=compileExpression(c))) return NULL;
//		PRINTF(LOG_DEV,"unify field\n");
		if (typeUnify(c,t,tfield->child[TYPEFIELD_FIELD])) return NULL;
		if (bc_byte_or_int(c,def->index, OPupdtb,OPupdt)) return NULL;
		
		if ((!parserNext(c)) || (strcmp(c->parser->token, ",") && strcmp(c->parser->token, "]")))
			return compileError(c, "',' or ']' expected (found '%s')\n", compileToken(c));
		if (!strcmp(c->parser->token, "]")) parserRewind(c);

		if ((!parserNext(c))||((!isLabel(c->parser->token))&&strcmp(c->parser->token,"]")))
			return compileError(c,"field name or ']' expected (found '%s')\n",compileToken(c));
		if (!strcmp(c->parser->token, "]")) break;

		// necessary isLabel=true
		def=compileGetDef(c);
		if ((!def)||(def->code!=DEF_CODE_FIELD)) return compileError(c,"'%s' is not a field name\n",compileToken(c));
	}
//	PRINTF(LOG_DEV,"typeUnderivate\n");
	root = typeUnderivate(c, root);
//	PRINTF(LOG_DEV,"done\n");

	bigChild = root->def;
	g->data = (LB*)bigChild;
	for (d = bigChild; d->parent; d = d->parent)
	{
		bcint_byte_or_int(c, d->dI);
		if (bc_byte_or_int(c, d->parent->index, OPupdtb, OPupdt)) return NULL;
	}
	return root;
}

// type name has already been read, and is in 'def' argument
Type* compileEmptyStruct(Compiler* c, Def* def)
{
	LINT global;
	Def* d = def;

	if (funMakerAddGlobal(c->fmk, (LB*)def, &global)) return NULL;
	if (bcint_byte_or_int(c, global)) return NULL;
	if (bufferAddChar(c->bytecode, OPstruct)) return NULL;

	for(d=def;d->parent;d=d->parent)
	{
		bcint_byte_or_int(c, d->dI);
		if (bc_byte_or_int(c, d->parent->index, OPupdtb, OPupdt)) return NULL;
	}

	if (parserAssume(c, "]")) return NULL;

	return typeCopy(def->type);
}
