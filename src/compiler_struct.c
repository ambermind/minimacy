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

// type name is already in the stack
// '=' has been parsed
Type* compileStructure1(Compiler* c, Ref* structRef)
{
	if ((!parserNext(c)) || ((!islabel(c->parser->token)) && strcmp(c->parser->token, "[") ))
		return compileError(c, "Compiler: type name or '[' expected (found '%s')\n", compileToken(c));

	if (strcmp(c->parser->token, "["))
	{
		parserGiveback(c);
		if (compilerSkipTypeDef(c)) return NULL;
		if (parserAssume(c, "+")) return NULL;
		if (parserAssume(c, "[")) return NULL;
	}

	while(1)
	{
		Type* fieldType;
		Ref* fieldRef;
		LB* fieldName;

		if ((!parserNext(c))||((!islabel(c->parser->token))&&strcmp(c->parser->token,"]")))
			return compileError(c,"Compiler: field name or ']' expected (found '%s')\n",compileToken(c));
		if (!islabel(c->parser->token)) break;

		if (pkgFirstGet(c->pkg, c->parser->token)) return compileError(c, "Compiler: '%s' already defined\n", compileToken(c));
		fieldName = memoryAllocStr(c->th, c->parser->token, -1); if (!fieldName) return NULL;
		if (parserNext(c) && !strcmp(c->parser->token, ":"))
		{
			if (compilerSkipTypeDef(c)) return NULL;
		}
		else parserGiveback(c);

		fieldType = typeAlloc(c->th,TYPECODE_FIELD, NULL, 2, typeAllocUndef(c->th), typeAllocUndef(c->th)); if (!fieldType) return NULL;

		fieldRef=refAlloc(c->th, REFCODE_FIELD,0,structRef->val,fieldType); if (!fieldRef) return NULL;
		fieldRef->proto = 1;
		structRef->val=PNTTOVAL((LB*)fieldRef);
		fieldRef->parent = structRef;

		if (pkgAddRef(c->th, c->pkg, fieldName, fieldRef)) return NULL;
	}
	if (parserAssume(c,";;")) return NULL;
	return structRef->type;
}
Type* compileStructure2(Compiler* c, Ref* structRef, Locals* labels)
{
	Type* structType = structRef->type;
	Type* derivative = typeDerivate(c->th, structRef->type,0); if (!derivative) return NULL;

	if ((!parserNext(c)) || ((!islabel(c->parser->token)) && strcmp(c->parser->token, "[")))
		return compileError(c, "Compiler: type name or '[' expected (found '%s')\n", compileToken(c));

	if (strcmp(c->parser->token, "["))
	{
		Ref* refParent;
		Type* t;
		if ((!(refParent = compileGetRef(c))) || (refParent->code != REFCODE_STRUCT))
			return compileError(c, "Compiler: type struct expected (found '%s')\n", compileToken(c));
		t = refParent->type;

		if (t->nb)
		{
			int i;
			if (parserAssume(c, "(")) return NULL;
			for (i = 0; i < t->nb; i++)
			{
				Locals* lb;
				if ((!parserNext(c)) || (!islabel(c->parser->token)))
					return compileError(c, "Compiler: parameter expected (found '%s')\n", compileToken(c));
				lb = localsGet(labels, 0, c->parser->token);
				if (!lb) return compileError(c, "Compiler: parameter expected (found '%s')\n", compileToken(c));
				if (lb->index != i) return compileError(c, "Compiler: wrong parameter, parent structure must have the same parameters in the same order than the derivative (found '%s')\n", compileToken(c));
				if (i < t->nb - 1)
				{
					if (parserAssume(c, ",")) return NULL;
				}
			}
			if (parserAssume(c, ")")) return NULL;
		}
		structRef->parent = refParent;
		c->nbDerivations++;
		if (parserAssume(c, "+")) return NULL;
		if (parserAssume(c, "[")) return NULL;
	}

	while(1)
	{
		Ref* refField;
		Type* t;

		if ((!parserNext(c))||((!islabel(c->parser->token))&& strcmp(c->parser->token, "+") && strcmp(c->parser->token,"]")))
			return compileError(c,"Compiler: field name or ']' expected (found '%s')\n",compileToken(c));

		if (!islabel(c->parser->token)) break;
			
		refField= pkgFirstGet(c->pkg, c->parser->token);
		if (!refField) return compileError(c, "Compiler: cannot find prototype of '%s'\n", compileToken(c));

		if (parserNext(c) && !strcmp(c->parser->token, ":"))
		{
			t = compilerParseTypeDef(c, 1, &labels);
			if (!t) return NULL;
		}
		else
		{ 
			parserGiveback(c);
			t = typeAllocWeak(c->th); if (!t) return NULL;
		}
		if (typeUnify(c, derivative, refField->type->child[TYPEFIELD_MAIN])) return NULL;
		if (typeUnify(c, t, refField->type->child[TYPEFIELD_FIELD])) return NULL;
	}
	if (parserAssume(c,";;")) return NULL;
	return structType;
}

Type* compileStructure3(Compiler* c, Ref* structRef, int rec)
{
	Ref* ref;
	Ref* refParent = structRef->parent;
	if (rec<0) return compileError(c, "Compiler: loop detected in struct '%s' defined in package '%s'\n", refName(structRef), pkgName(structRef->pkg));
	if (!structRef->proto) return MM.I;
	if (refParent)
	{
		if (!compileStructure3(c, refParent, rec - 1)) return NULL;
		refParent->dCI++;
		structRef->dI = refParent->dCI;
		structRef->index = refParent->index + 1;
	}
	for (ref = (Ref*)VALTOPNT(structRef->val); ref; ref = (Ref*)VALTOPNT(ref->val))
	{
		ref->index = structRef->index++;
		ref->proto = 0;
	}
	structRef->proto = 0;
	return MM.I;
}

// first field has already been read, and is in 'ref' argument
Type* compileFields(Compiler* c, Ref* ref)
{
	LINT global;
	Type* root;
	Ref* r;
	Ref* bigChild;
	Globals* g;


	if (funMakerAddGlobal(c->fmk, (LB*)NULL, &global)) return NULL;	// we may rewrite this global at the end of the function
	g = c->fmk->globals;

	if (bcint_byte_or_int(c,global)) return NULL;
	if (bufferAddchar(c->th, c->bytecode,OPstruct)) return NULL;

//	printf("compileFields, start with %s\n", refName(ref));
	root = typeAllocUndef(c->th); if (!root) return NULL;	// root is expected to become a1(BiggestChild)
	while(1)
	{
		Type* t;
		Type* tfield= typeInstance(c, ref); if (!tfield) return NULL;
//		printf("unify main\n");
		if (typeUnify(c,root,tfield->child[TYPEFIELD_MAIN])) return NULL;
		
		if (parserAssume(c,"=")) return NULL;
		if (!(t=compileExpression(c))) return NULL;
//		printf("unify field\n");
		if (typeUnify(c,t,tfield->child[TYPEFIELD_FIELD])) return NULL;
		if (bc_byte_or_int(c,ref->index, OPupdtb,OPupdt)) return NULL;
		
		if ((!parserNext(c))||((!islabel(c->parser->token))&&strcmp(c->parser->token,"]")))
			return compileError(c,"Compiler: field name or ']' expected (found '%s')\n",compileToken(c));
		if (!strcmp(c->parser->token, "]")) break;
		// necessary islabel=true
		ref=compileGetRef(c);
		if ((!ref)||(ref->code!=REFCODE_FIELD)) return compileError(c,"Compiler: '%s' is not a field name\n",compileToken(c));
	}
//	printf("typeUnderivate\n");
	root = typeUnderivate(c, root);
//	printf("done\n");

	bigChild = root->ref;
	g->data = (LB*)bigChild;
	for (r = bigChild; r->parent; r = r->parent)
	{
		bcint_byte_or_int(c, r->dI);
		if (bc_byte_or_int(c, r->parent->index, OPupdtb, OPupdt)) return NULL;
	}
	return root;
}

// type name has already been read, and is in 'ref' argument
Type* compileEmptyStruct(Compiler* c, Ref* ref)
{
	LINT global;
	Ref* r = ref;

	if (funMakerAddGlobal(c->fmk, (LB*)ref, &global)) return NULL;
	if (bcint_byte_or_int(c, global)) return NULL;
	if (bufferAddchar(c->th, c->bytecode, OPstruct)) return NULL;

	for(r=ref;r->parent;r=r->parent)
	{
		bcint_byte_or_int(c, r->dI);
		if (bc_byte_or_int(c, r->parent->index, OPupdtb, OPupdt)) return NULL;
	}

	if (parserAssume(c, "]")) return NULL;

	return typeCopy(c->th, ref->type);
}
