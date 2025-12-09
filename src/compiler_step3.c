// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

Type* compileStep3(Compiler* c)
{
	Def* def;
	//c->nbDerivations counts the structure derivations for this compiling. Therefore it is the maximum length of derivation cycles.
	for(def= c->pkg->first;def;def=def->next) if (def->proto&&(def->code==DEF_CODE_STRUCT)&&(!compileStructure3(c,def, c->nbDerivations))) return NULL;
	return MM.Int;
}
