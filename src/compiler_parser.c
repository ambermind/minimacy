// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
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
	c->parser->index = c->parser->index0 = index;
}
int parserIndex(Compiler* c)
{
	if (c->parser->again) return c->parser->index0;
	return c->parser->index;
}
void parserRestoreFromDef(Compiler* c,Def* d)
{
	if (c->parser) parserReset(c);
	c->parser = d->parser;
	parserJump(c,(int)d->parserIndex);
}
void parserRewind(Compiler* c)
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

int parserGettoken(Compiler* c)
{
	int ch, d, f;

	parserRestorechar(c);

	do
	{   // looking for the beginning of the token
		// save the current position in source code
		c->parser->index0 = c->parser->index;
		ch = parserNextchar(c);
		if (!ch) return -1;	// end of file, no next token
	} while (ch <= 32);

	c->parser->token = &c->parser->src[c->parser->index0];

	f = 0;
	if (ch == '\"')	// token is a string
		while (1)
		{
			ch = parserNextchar(c);
			if (!ch)
			{
				compileError(c, "uncomplete string reaches EOF\n");
				return -1;
			}
			if ((ch == '\"') && (f == 0))
			{
				parserSavechar(c, c->parser->index);
				return 0;
			}
			if (ch == '\\') f = 1 - f;
			else f = 0;
		}
	if (isLetterOrNumber(ch))	// token is number or label
	{
		int onlynum = 1;
		while (1)
		{
			if (!isNumber(ch)) onlynum = 0;
			ch = parserNextchar(c);
			if (!ch) return 0;
			if ((ch == '.') && (onlynum))	// floating number
			{
				int acceptE = 1;
				int acceptSign = 0;
				while (1)
				{
					ch = parserNextchar(c);
					if (!ch) return 0;
					if (isNumber(ch)) continue;
					if (acceptE&&(ch == 'E' || ch == 'e')) {
						acceptSign = 1;
						acceptE = 0;
						continue;
					}
					if ((ch == '+' || ch == '-') && acceptSign) {
						acceptSign = 0;
						continue;
					}
					parserAgainchar(c);
					parserSavechar(c, c->parser->index);
					return 0;
				}
			}
			if (!isLetterOrNumber(ch))
			{
				parserAgainchar(c);
				parserSavechar(c, c->parser->index);
				return 0;
			}
		}
	}
	d = parserNextchar(c);
	if (!d)	return 0;	// end of file on a special char

	if (((ch == '&') && (d == '&'))
		|| ((ch == '|') && (d == '|'))
		|| ((ch == '^') && (d == '^'))
		|| ((ch == ';') && (d == ';'))
		|| ((ch == ':') && (d == ':'))
		|| ((ch == '-') && (d == '>'))
		|| ((ch == '<') && ((d == '<') || (d == '>')))
		|| ((ch == '>') && (d == '>'))
		|| ((ch == '=') && (d == '='))
		|| (((ch == '+') || (ch == '-') || (ch == '*') || (ch == '/')) && (d == '.'))
		|| (((ch == '=') || (ch == '<') || (ch == '>')) && (d == '.'))
		|| ((ch == '/') && (d == '*'))
		|| ((ch == '*') && (d == '/'))
		|| ((ch == '%') && (d == '.'))
		)
	{
		// double token recognized
	}
	else if ((ch == '/') && (d == '/'))
	{
		do	// comment //
		{
			ch = parserNextchar(c);
			if (ch == 10) return -2;
		} while (ch);
		return -1;	// end of file
	}
	else if (((ch == '!') || (ch == '>') || (ch == '<')) && (d == '='))
	{
		d = parserNextchar(c);
		if (!d)	return 0; // end of file
		if (d != '.') parserAgainchar(c);
	}
	else if ((ch == '*') && (d == '*'))
	{
		d = parserNextchar(c);
		if (!d)	return 0; // end of file
		if (d != '.') parserAgainchar(c);
	}
	else
		parserAgainchar(c);
	parserSavechar(c, c->parser->index);
	return 0;
}


// read a mandatory keyword
int parserAssume(Compiler* c,char* keyword)
{
	if ((!parserNext(c))||strcmp(c->parser->token,keyword))
	{
		compileError(c,"'%s' expected (found %s)\n",keyword,compileToken(c));
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
	compileError(c,"'%s' expected\n", keyword);
	return COMPILER_ERR_SN;
}

// parse a string (c->parser->token points to the first double quote)
int parserGetstring(Compiler* c, Buffer* b)
{
	return strFromSource(b, c->parser->token, strlen(c->parser->token));
}

int parserIsFinal(Compiler* c)
{
	char* p;
	char* token = c->parser->token;
	if (!token) return 1;
	p = strstr(" ) } ] :: , ; ;; -> then else do with until in ", token);
	if (p && (p[-1]==32) && (p[strlen(token)]==32)) return 1;
	return 0;
}

// read the next char
int parserNextchar(Compiler* c)
{
	Parser* parser = c->parser;
	int ch = parser->src[parser->index];
	if (ch)
	{
		parser->index++;
		return ch;
	}
	if (parser==c->mainParser) return 0;
	if (!parser->mayGetBackToParent) return 0;
	c->parser = c->mainParser;
	parserRestorechar(c);
	return 10;
}

void parserMark(LB* user)
{
	Parser* parser = (Parser*)user;
	MARK_OR_MOVE(parser->name);
	MARK_OR_MOVE(parser->block);
	if (MOVING_BLOCKS) {
		LINT offset = parser->token - parser->src;
		parser->src = STR_START(parser->block);
		parser->token = parser->src + offset;
	}
}

int parserFromData(Compiler* c, char* name, LB* srcBlock)
{
	Parser* parser;
	TMP_PUSH(srcBlock, EXEC_OM);
	parser = (Parser*)memoryAllocNative(sizeof(Parser), DBG_BIN, NULL, parserMark); if (!parser) return EXEC_OM;
//	PRINTF(LOG_DEV,"parserFromData %s "LSX" "LSX" -> "LSX"\n", name, c, srcBlock, parser);
	parser->name = NULL;
	parser->block = srcBlock;
	parser->src = STR_START(parser->block);
	parser->mayGetBackToParent = 0;
	parser->indexsavedchar = -1;
	c->parser = parser;

	if (!name) name = "";
	parser->name = memoryAllocStr(name, -1); if (!parser->name) return EXEC_OM;
	TMP_PULL();
	parserReset(c);
	return 0;
}

Type* parserFromIncludes(Compiler* c,char* name)
{
	LB* src = fsReadPackage(name, NULL, 0);
	if (!src) return compileError(c,"file not found ('%s')\n", name);
	if (parserFromData(c, name, src)) return NULL;
	return MM.Str;
}
