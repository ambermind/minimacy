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


void parserRestorechar(Compiler* c)
{
	if (c->parser && (c->parser->indexsavedchar != -1))
	{
		c->parser->src[c->parser->indexsavedchar] = c->parser->savedchar;
		c->parser->indexsavedchar = -1;
	}
}


void parserReset(Compiler* c)
{
	parserRestorechar(c);
	c->parser->index = 0;
	c->parser->again = 0;

	c->parser->index0 = 0;

	c->parser->token = NULL;
}

void parserJump(Compiler* c, int index)
{
	parserReset(c);
	c->parser->index = index;
}
int parserIndex(Compiler* c)
{
	if (c->parser->again) return c->parser->index0;
	return c->parser->index;
}
void parserRestoreFromDef(Compiler* c,Def* def)
{
	if (c->parser) parserReset(c);
	c->parser = def->parser;
	parserJump(c,(int)def->parserIndex);
}
void parserGiveback(Compiler* c)
{
	c->parser->again=1;
}

char* parserNext(Compiler* c)
{
	if (c->parser->again)
	{
		c->parser->again=0;
		return c->parser->token;
	}
	while(1)
    {
		int k;
		char* end;
		do
		{
			k=parserGettoken(c);
		} while (k==-2);
		if (k)
		{
			c->parser->token=NULL;
			return NULL;
		}
		if (strcmp(c->parser->token,"/*")) return c->parser->token;	// ok
		parserRestorechar(c);
		end = strstr(c->parser->token+2, "*/");
		if (!end)
		{
			compileError(c, "unbalanced comment reaches EOF\n");
			c->parser->token = NULL;
			return NULL;
		}
		c->parser->index += (int)(end - c->parser->token);
    }
}

void parserSavechar(Compiler* c,int i)
{
	c->parser->indexsavedchar=i;
	c->parser->savedchar=c->parser->src[i];
	c->parser->src[i]=0;
}

// undo the last read char
void parserAgainchar(Compiler* c)
{
	c->parser->index--;
}

int parserGettoken(Compiler* p)
{
	int c, d, f;

	parserRestorechar(p);

	do
	{   // looking for the beginning of the token
		// save the current position in source code
		p->parser->index0 = p->parser->index;
		c = parserNextchar(p);
		if (!c) return -1;	// end of file, no next token
	} while (c <= 32);

	p->parser->token = &p->parser->src[p->parser->index0];

	f = 0;
	if (c == '\"')	// token is a string
		while (1)
		{
			c = parserNextchar(p);
			if (!c)
			{
				compileError(p, "uncomplete string reaches EOF\n");
				return -1;
			}
			if ((c == '\"') && (f == 0))
			{
				parserSavechar(p, p->parser->index);
				return 0;
			}
			if (c == '\\') f = 1 - f;
			else f = 0;
		}
	if (isletnum(c))	// token is number or label
	{
		int onlynum = 1;
		while (1)
		{
			if (!isnum(c)) onlynum = 0;
			c = parserNextchar(p);
			if (!c) return 0;
			if ((c == '.') && (onlynum))	// floating number
			{
				int acceptE = 1;
				int acceptSign = 0;
				while (1)
				{
					c = parserNextchar(p);
					if (!c) return 0;
					if (isnum(c)) continue;
					if (acceptE&&(c == 'E' || c == 'e')) {
						acceptSign = 1;
						acceptE = 0;
						continue;
					}
					if ((c == '+' || c == '-') && acceptSign) {
						acceptSign = 0;
						continue;
					}
					parserAgainchar(p);
					parserSavechar(p, p->parser->index);
					return 0;
				}
			}
			if (!isletnum(c))
			{
				parserAgainchar(p);
				parserSavechar(p, p->parser->index);
				return 0;
			}
		}
	}
	d = parserNextchar(p);
	if (!d)	return 0;	// end of file on a special char

	if (((c == '&') && (d == '&'))
		|| ((c == '|') && (d == '|'))
		|| ((c == '^') && (d == '^'))
		|| ((c == ';') && (d == ';'))
		|| ((c == '-') && (d == '>'))
		|| ((c == '<') && ((d == '<') || (d == '>')))
		|| ((c == '>') && (d == '>'))
		|| ((c == '=') && (d == '='))
		|| (((c == '+') || (c == '-') || (c == '*') || (c == '/')) && (d == '.'))
		|| (((c == '=') || (c == '<') || (c == '>')) && (d == '.'))
		|| ((c == '/') && (d == '*'))
		|| ((c == '*') && (d == '/'))
		|| ((c == '%') && (d == '.'))
		)
	{
		// double token recognized
	}
	else if ((c == '/') && (d == '/'))
	{
		do	// comment //
		{
			c = parserNextchar(p);
			if (c == 10) return -2;
		} while (c);
		return -1;	// end of file
	}
	else if (((c == '!') || (c == '>') || (c == '<')) && (d == '='))
	{
		d = parserNextchar(p);
		if (!d)	return 0; // end of file
		if (d != '.') parserAgainchar(p);
	}
	else if ((c == '*') && (d == '*'))
	{
		d = parserNextchar(p);
		if (!d)	return 0; // end of file
		if (d != '.') parserAgainchar(p);
	}
	else
		parserAgainchar(p);
	parserSavechar(p, p->parser->index);
	return 0;
}


// read a mandatory keyword
int parserAssume(Compiler* c,char* keyword)
{
	if ((!parserNext(c))||strcmp(c->parser->token,keyword))
	{
		compileError(c,"Compiler: '%s' expected (found %s)\n",keyword,compileToken(c));
		return COMPILER_ERR_SN;
	}
	return 0;
}

int parserUntil(Compiler* c, char* keyword)
{
	while (parserNext(c))
	{
		if (!strcmp(c->parser->token, keyword)) return 0;
	}
	compileError(c, "Compiler: '%s' expected\n", keyword);
	return COMPILER_ERR_SN;
}

// parse a string (c->parser->token points to the first double quote)
int parserGetstring(Compiler* c, Buffer* b)
{
	return strFromSource(c->th, b, c->parser->token, strlen(c->parser->token));
}

int parserIsFinal(Compiler* c)
{
	Def* def;
	char* p;
	char* token = c->parser->token;
	if (!token) return 1;
	p = strstr(" ) } ] : , ; ;; -> then else do with catch ", token);
	if (p && (p[-1]==32) && (p[strlen(token)]==32)) return 1;
	def = compileGetDef(c);

	// the following test allows to initialize a field with a direct function call, without parenthesis: [xP=sin 1. yP=0.], instead of [xP=(sin 1.) yP=0.] for a Struct Point = [xP yP];;
	if (def && def->code == DEF_CODE_FIELD) return 1;
//	PRINTF(LOG_DEV,"isNotFinal %s '%s'\n", token, defName(def));
	return 0;
}

// read the next char
int parserNextchar(Compiler* p)
{
	Parser* parser;
	int c = p->parser->src[p->parser->index];
	if (c)
	{
		p->parser->index++;
		return c;
	}
	parser = p->parser;
	if (!parser->parent) return 0;
	if (!parser->mayGetBackToParent) return 0;
	p->parser = parser->parent;
	parserRestorechar(p);
	return 10;
}

void parserMark(LB* user)
{
	Parser* parser = (Parser*)user;
	MEMORYMARK(user, (LB*)parser->name);
	MEMORYMARK(user, (LB*)parser->block);
}
// Warning: we assume that the parser is used under memoryEnterFast
int parserFromData(Compiler* c, char* name, char* data)
{
	Parser* parser = (Parser*)memoryAllocExt(c->th,sizeof(Parser), DBG_BIN, NULL, parserMark); if (!parser) return EXEC_OM;
//	PRINTF(LOG_DEV,"parserFromData %s "LSX" -> "LSX"\n", name, c, parser);
	parser->name = NULL;
	parser->block = NULL;
	parser->parent = c->parser;
	parser->mayGetBackToParent = 0;
	c->parser = parser;
	parser->nextLib = c->parserLib;
	c->parserLib = parser;

	if (!name) name = "";
	c->parser->name = memoryAllocStr(c->th, name, -1); if (!c->parser->name) return EXEC_OM;
	c->parser->src = NULL;
	c->parser->indexsavedchar = -1;
	parser->block = memoryAllocStr(c->th, data, -1); if (!parser->block) return EXEC_OM;
	c->parser->src = STRSTART(parser->block);	// under memoryEnterFast, there is no need to keep an eye on the corresponding LB*
	parserReset(c);
	return 0;
}
	
Type* parserFromIncludes(Compiler* c,char* name)
{
	LB* src;
	Parser* parser = c->parserLib;
	while (parser)
	{
		if (!strcmp(STRSTART(parser->name), name))
		{
			parser->parent = c->parser;
			parser->mayGetBackToParent = 0;

			c->parser = parser;
			parserReset(c);
			return MM.S;
		}
		parser = parser->nextLib;
	}
	src = fsReadPackage(c->th, name, NULL, 0);
	if (!src) return compileError(c, "Compiler: file not found ('%s')\n", name);
	if (parserFromData(c, name, STRSTART(src))) return NULL;
	return MM.S;
}
