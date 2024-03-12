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

Type* compileStep3(Compiler* c)
{
	Def* def;
	//c->nbDerivations counts the structure derivations for this compiling. Therefore it is the maximum length of derivation cycles.
	for(def= c->pkg->first;def;def=def->next) if (def->proto&&(def->code==DEF_CODE_STRUCT)&&(!compileStructure3(c,def, c->nbDerivations))) return NULL;
	return MM.Int;
}
