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

Type* compileFile3(Compiler* c)
{
	Ref* ref;
	//c->nbDerivations counts the structure derivations for this compiling. Therefore it is the maximum length of derivation cycles.
	for(ref= c->pkg->first;ref;ref=ref->next) if ((ref->code==REFCODE_STRUCT)&&(!compileStructure3(c,ref, c->nbDerivations))) return NULL;
	return MM.I;
}

Type* compileStep3(Compiler* c)
{
	Pkg* pkgSave = c->pkg;
	Pkg* pkg;

	for (pkg = MM.listPkgs; pkg->stage == PKG_STAGE_2; pkg = pkg->listNext)
	{
		c->pkg = pkg;
		pkg->stage = PKG_STAGE_3;
		if (!compileFile3(c)) return NULL;
	}
	c->pkg = pkgSave;
	return compileFile3(c);
}