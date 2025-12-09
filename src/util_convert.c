// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

// following tables are based on Windows-1252 encoding: https://en.wikipedia.org/wiki/Windows-1252
// it extends Latin-1 or Unicode encoding with unused range 0x80-0x9f
// in Unicode this range is used for control codes which are unlikely found in a string

// these tables work only on 8-bits codes
// be aware there are other characters in unicode that should be handled as well for u8 or u16:
// - more unusual accented characters
// - lowercase/uppercase for other alphabets such as Greek
// see https://en.wikipedia.org/wiki/List_of_Unicode_characters

// replace accented and/or uppercase with unaccented lowercase to simplify search functions
const char SearchCase[256] = {
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
	32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
	64,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,91,92,93,94,95,
	96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
	128,129,130,131,132,133,134,135,136,137,115,139,111,141,122,143,144,145,146,147,148,149,150,151,152,153,115,155,111,157,122,121,
	160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
	97,97,97,97,97,97,97,99,101,101,101,101,105,105,105,105,100,110,111,111,111,111,111,215,111,117,117,117,117,121,116,223,
	97,97,97,97,97,97,97,99,101,101,101,101,105,105,105,105,100,110,111,111,111,111,111,247,111,117,117,117,117,121,116,121,
};
const char LowerCase[256]={
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
	32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
	64,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,91,92,93,94,95,
	96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
	128,129,130,131,132,133,134,135,136,137,154,139,156,141,158,143,144,145,146,147,148,149,150,151,152,153,154,155,156,157,158,255,
	160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
	224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,215,248,249,250,251,252,253,254,223,
	224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255,
};
const char UpperCase[256]={
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
	32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
	64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,
	96,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,123,124,125,126,127,
	128,129,130,131,132,133,134,135,136,137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,138,155,140,157,142,159,
	160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
	192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,222,223,
	192,193,194,195,196,197,198,199,200,201,202,203,204,205,206,207,208,209,210,211,212,213,214,247,216,217,218,219,220,221,222,255,
};
// replace accented with unaccented
const char Unaccented[256]={
	0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,
	32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,
	64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79,80,81,82,83,84,85,86,87,88,89,90,91,92,93,94,95,
	96,97,98,99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,120,121,122,123,124,125,126,127,
	128,129,130,131,132,133,134,135,136,137,83,139,79,141,90,143,144,145,146,147,148,149,150,151,152,153,154,155,111,157,122,121,
	160,161,162,163,164,165,166,167,168,169,170,171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,188,189,190,191,
	65,65,65,65,65,65,65,67,69,69,69,69,73,73,73,73,208,78,79,79,79,79,79,215,79,85,85,85,85,89,222,223,
	97,97,97,97,97,97,97,99,101,101,101,101,105,105,105,105,240,110,111,111,111,111,111,247,111,117,117,117,117,121,254,121,
};


int isU8(char* src, LINT len)
{
	LINT i;
	for (i = 0; i < len; i++)
	{
		int c = src[i] & 255;
		if (c < 128) continue;
		if (c < 0xc2) return 0;

		if ((src[i + 1] & 0xc0) != 0x80) return 0;
		i++;
		if (c < 0xe0) continue;

		if ((src[i + 1] & 0xc0) != 0x80) return 0;
		i++;
		if (c < 0xf0) continue;

		if ((src[i + 1] & 0xc0) != 0x80) return 0;
		i++;

		if (c > 0xf4) return 0;
	}
	return 1;
}

LINT stringLengthU16(char* src,LINT len)
{
	if (len&1) return 0;
	return len>>1;
}
LINT stringLengthU8(char* src,LINT len)
{
	LINT i,n;
	i=n=0;
	while(i<len)
	{
		LINT c=src[i++];
		if ((c&0xe0)==0xc0) i++;
		else if ((c&0xf0)==0xe0) i+=2;
		else if ((c&0xf8)==0xf0) i+=3;
		n++;
	}
	return n;
}
LINT stringLengthLatin(char* src,LINT len)
{
	return len;
}
LINT u8Value(char* src,int *len)
{
	LINT first = src[0] & 255;
	*len = 1;
	if (first < 128) return first;
	if ((first < 0xc2) || (first > 0xf4)) return -1;
	
	if ((src[1] & 0xc0) != 0x80) return -1;
	*len = 2;
	if ((first & 0xe0) == 0xc0) return ((first & 0x1f) << 6) + (src[1] & 0x3f);
	
	if ((src[2] & 0xc0) != 0x80) return -1;
	*len = 3;
	if ((first & 0xf0) == 0xe0) return ((first & 0xf) << 12) + ((src[1] & 0x3f) << 6) + (src[2] & 0x3f);

	if ((src[3] & 0xc0) != 0x80) return -1;
	if ((first == 0xf4) && ((src[1] & 0xf0) != 0x80)) return -1;

	*len = 4;
	return ((first & 0xf) << 18) + ((src[1] & 0x3f) << 12) + ((src[2] & 0x3f) << 6) + (src[3] & 0x3f);
}
//
LINT u8Previous(char* src, int i)
{
	while (i > 0) {
		LINT c = src[--i] & 255;
		if ((c & 0xc0) != 0x80) return i;
	}
	return 0;
}
LINT u8Next(char* src)
{
	LINT c = src[0] & 255;
	if (c < 0xc2) return 1;
	if ((c & 0xe0) == 0xc0) return 2;
	if ((c & 0xf0) == 0xe0) return 3;
	if ((c & 0xf8) == 0xf0) return 4;
	return 1;
}
int u16LeFromLatin(Buffer* tmp, char* src, LINT len)
{
	LINT i;
	int k;
	bufferReinit(tmp);
	for (i = 0; i < len; i++)
	{
		if ((k = bufferAddChar(tmp, src[i]))) return k;
		if ((k = bufferAddChar(tmp, 0))) return k;
	}
	return 0;
}

int latinFromU16Le(Buffer* tmp, char* src, LINT len)
{
	LINT i;
	int k;
	if (len & 1) return EXEC_FORMAT;
	bufferReinit(tmp);
	for (i = 0; i < len; i+=2) if (!src[i + 1]) if ((k = bufferAddChar(tmp, src[i]))) return k;
	return 0;
}

int u16BeFromLatin(Buffer* tmp, char* src, LINT len)
{
	LINT i;
	int k;
	bufferReinit(tmp);
	for (i = 0; i < len; i++)
	{
		if ((k = bufferAddChar(tmp, 0))) return k;
		if ((k = bufferAddChar(tmp, src[i]))) return k;
	}
	return 0;
}

int latinFromU16Be(Buffer* tmp, char* src, LINT len)
{
	LINT i;
	int k;
	if (len & 1) return EXEC_FORMAT;
	bufferReinit(tmp);
	for (i = 0; i < len; i += 2) if (!src[i]) if ((k = bufferAddChar(tmp, src[i+1]))) return k;
	return 0;
}

int u8Write(Buffer* tmp, int c)
{
	int k;
	if (c >= 2048)
	{
		if ((k = bufferAddChar(tmp, 0xe0 + ((c >> 12) & 0x0f)))) return k;
		if ((k = bufferAddChar(tmp, 0x80 + ((c >> 6) & 0x3f)))) return k;
		if ((k = bufferAddChar(tmp, 0x80 + (c & 0x3f)))) return k;
	}
	else if (c >= 128)
	{
		if ((k = bufferAddChar(tmp, 0xc0 + ((c >> 6) & 0x1f)))) return k;
		if ((k = bufferAddChar(tmp, 0x80 + (c & 0x3f)))) return k;
	}
	else if ((k = bufferAddChar(tmp, c))) return k;
	return 0;

}
int u8FromU16Le(Buffer* tmp, char* src, LINT len)
{
	LINT i;
	int k;

	if (len & 1) return EXEC_FORMAT;
	bufferReinit(tmp);
	for(i=0;i<len;i+=2)
	{
		int c=(src[i]&255)+((src[i+1]&255)<<8);
		if ((k=u8Write(tmp,c))) return k;
	}
	return 0;
}
int u16LeFromU8(Buffer* tmp, char* src, LINT len)
{
	LINT i;
	int k;
	bufferReinit(tmp);

	for (i = 0; i < len; i++)
	{
		int c = src[i];
		if ((c & 0xe0) == 0xc0)
		{
			c = ((c & 0x1f) << 6) + (src[i + 1] & 0x3f);
			i++;
		}
		else if ((c & 0xf0) == 0xe0)
		{
			if (i+2<len) c = ((c & 0xf) << 12) + ((src[i + 1] & 0x3f) << 6) + (src[i + 2] & 0x3f);
			i += 2;
		}
		else if ((c & 0xf8) == 0xf0)
		{
			i += 3;
			continue;
		}
		if ((k = bufferAddChar(tmp, c))) return k;
		c >>= 8;
		if ((k = bufferAddChar(tmp, c))) return k;
	}
	return 0;
}
int u8FromU16Be(Buffer* tmp, char* src, LINT len)
{
	LINT i;
	int k;

	if (len & 1) return EXEC_FORMAT;
	bufferReinit(tmp);
	for (i = 0; i < len; i += 2)
	{
		int c = (src[i+1] & 255) + ((src[i] & 255) << 8);
		if ((k = u8Write(tmp, c))) return k;
	}
	return 0;
}
int u16BeFromU8(Buffer* tmp, char* src, LINT len)
{
	LINT i;
	int k;
	bufferReinit(tmp);

	for (i = 0; i < len; i++)
	{
		int c = src[i];
		int d;
		if ((c & 0xe0) == 0xc0)
		{
			c = ((c & 0x1f) << 6) + (src[i + 1] & 0x3f);
			i++;
		}
		else if ((c & 0xf0) == 0xe0)
		{
			if (i + 2 < len) c = ((c & 0xf) << 12) + ((src[i + 1] & 0x3f) << 6) + (src[i + 2] & 0x3f);
			i += 2;
		}
		else if ((c & 0xf8) == 0xf0)
		{
			i += 3;
			continue;
		}
		d = c >> 8;
		if ((k = bufferAddChar(tmp, d))) return k;
		if ((k = bufferAddChar(tmp, c))) return k;
	}
	return 0;
}

int latinFromU8(Buffer* tmp, char* src, LINT len)
{
	LINT i;
	int k;
	bufferReinit(tmp);

	for(i=0;i<len;i++)
	{
		int c=src[i];
		if ((c & 0xe0) == 0xc0)
		{
			c = ((c & 0x1f) << 6) + (src[i + 1] & 0x3f);
			if (c <= 255) if ((k = bufferAddChar(tmp, c))) return k;
			i++;
		}
		else if ((c & 0xf0) == 0xe0)
		{
			i += 2;
		}
		else if ((c & 0xf8) == 0xf0)
		{
			i += 3;
		}
		else if (c < 0xc2)
        {
            if ((k = bufferAddChar(tmp, c))) return k;
        }
        else return EXEC_FORMAT;
	}
	return 0;
}
int u8FromLatin(Buffer* tmp, char* src, LINT len)
{
	LINT i;
	int k;
	bufferReinit(tmp);
	for (i = 0; i < len; i++) if (src[i] & 128)
	{
		int c = src[i] & 255;
		if ((k = bufferAddChar(tmp, 0xc0 + ((c >> 6) & 0x1f)))) return k;
		if ((k = bufferAddChar(tmp, 0x80 + (c & 0x3f)))) return k;
	}
	else if ((k = bufferAddChar(tmp, src[i]))) return k;
	return 0;
}


// parse a string (src points on the first double quote)
int strFromSource(Buffer* tmp, char* src, LINT len)
{
	int c, i,k;
	char separator = *(src++);

	bufferReinit(tmp);
	while (1)
	{
		c = *(src++);
		if (c == '\\')
		{
			c = (*(src++)) & 255;
			if (c == 0) return EXEC_FORMAT;
			if (c < 32)
			{
				while (((*src) & 255) < 32) src++;
			}
			else
			{
				if (c == 'n') c = 10;
				else if (c == 'r') c = 13;
				else if (c == 't') c = 9;
				else if (c == 'z') c = 0;
				else if (c == '$')
				{
					i = 0;
					c = *src;
					if (isHexchar(c))
					{
						src++;
						i = intFromHexchar(c);
						c = *src;
						if (isHexchar(c))
						{
							src++;
							i = (i << 4) + intFromHexchar(c);
						}
					}
					c = i;
				}
				else if ((c >= '0') && (c <= '9'))
				{
					i = c - '0';
					c = *src;
					if ((c >= '0') && (c <= '9'))
					{
						src++;
						i = (i * 10) + c - '0';
						c = *src;
						if ((c >= '0') && (c <= '9'))
						{
							src++;
							i = (i * 10) + c - '0';
						}
					}
					c = i;
				}
				if ((k=bufferAddChar(tmp, c))) return k;
			}
		}
		else if (c == separator)
		{
			if (*src) return EXEC_FORMAT;	// check that the separator is the last char
			return 0;
		}
		else if (c == 0) return EXEC_FORMAT;
		else if ((k=bufferAddChar(tmp, c))) return k;
	}
}

int sourceFromStr(Buffer* tmp, char* src, LINT len)
{
	LINT i;
	int c,k;
	bufferReinit(tmp);
	if ((k=bufferAddChar(tmp, 0x22))) return k;
	for (i = 0; i < len;i++)
	{
		c = src[i]&255;
		if ((c < 32) || (c >= 128))
		{
			if ((k=bufferAddChar(tmp, '\\'))) return k;
			if (c == 0) c = 'z';
			else if (c == 10) c = 'n';
			else if (c == 13) c = 'r';
			else if (c == 9) c = 't';

			if ((c >= 32) && (c < 128))
			{
				if ((k=bufferAddChar(tmp, c))) return k;
			}
			else if ((k=bufferPrintf(tmp, "$%02x", c))) return k;
		}
		else
		{
			if (c == '\\' || c == '"') if ((k = bufferAddChar(tmp, '\\'))) return k;
			if ((k = bufferAddChar(tmp, c))) return k;
		}
	}
	if ((k=bufferAddChar(tmp, 0x22))) return k;
	return 0;
}


int u8FromJson(Buffer* tmp, char* src, LINT len)
{
	int k, c;
	if (*(src++) != 0x22) return EXEC_FORMAT;
	bufferReinit(tmp);
	while ((c = *src) != 0x22)
	{
		if (c == 0) return EXEC_FORMAT;
		if (c == '\\')
		{
			src++; c = *src;
			if (c == 0x22) {
				if ((k = bufferAddChar(tmp, 0x22))) return k;
			}
			else if (c == '\\') {
				if ((k = bufferAddChar(tmp, '\\'))) return k;
			}
			else if (c == 'b') {
				if ((k = bufferAddChar(tmp, '\b'))) return k;
			}
			else if (c == 'f') {
				if ((k = bufferAddChar(tmp, '\f'))) return k;
			}
			else if (c == 'n') {
				if ((k = bufferAddChar(tmp, '\n'))) return k;
			}
			else if (c == 'r') {
				if ((k = bufferAddChar(tmp, '\r'))) return k;
			}
			else if (c == 't') {
				if ((k = bufferAddChar(tmp, '\t'))) return k;
			}
			else if (c == 'u')
			{
				src++;
				if ((!isHexchar(src[0])) || (!isHexchar(src[1])) || (!isHexchar(src[2])) || (!isHexchar(src[3]))) return EXEC_FORMAT;
				c = (intFromHexchar(src[0]) << 12) + (intFromHexchar(src[1]) << 8) + (intFromHexchar(src[2]) << 4) + intFromHexchar(src[3]);
				src += 3;
				if (c < 0x80) {
					if ((k = bufferAddChar(tmp, c))) return k;
				}
				else if (c < 0x800)
				{
					if ((k = bufferAddChar(tmp, 0xc0 + ((c >> 6) & 0x1f)))) return k;
					if ((k = bufferAddChar(tmp, 0x80 + (c & 0x3f)))) return k;
				}
				else
				{
					if ((k = bufferAddChar(tmp, 0xe0 + ((c >> 12) & 0x0f)))) return k;
					if ((k = bufferAddChar(tmp, 0x80 + ((c >> 6) & 0x3f)))) return k;
					if ((k = bufferAddChar(tmp, 0x80 + (c & 0x3f)))) return k;
				}
			}
			else if ((k = bufferAddChar(tmp, c))) return k;
		}
		else if ((k = bufferAddChar(tmp, c))) return k;
		src++;
	}
	src++;
	if (*src) return EXEC_FORMAT;
	return 0;
}

int jsonFromU8(Buffer* tmp, char* src, LINT len)
{
	int k;
	LINT i;
	bufferReinit(tmp);
	if ((k = bufferAddChar(tmp, 0x22))) return k;
	for (i = 0; i < len; i++)
	{
		int c = src[i] & 255;
		if (c >= 0xc2)
		{
			if ((c & 0xe0) == 0xc0)
			{
				i++;
				if (!src[i]) continue;
				c = ((c & 0x1f) << 6) + (src[i] & 0x3f);
			}
			else if ((c & 0xf0) == 0xe0)
			{
				i += 2;
				if ((!src[i - 1]) || (!src[i])) continue;
				c = ((c & 0xf) << 12) + ((src[i - 1] & 0x3f) << 6) + (src[i] & 0x3f);
			}
			else
			{
				i += 3;
				continue;	// would require more than 4 hex digits
			}
			if ((k = bufferPrintf(tmp, "\\u%04x", c & 0xffff))) return k;
		}
		else if ((c == 0x22) || (c == '\\') || (c < 32))
		{
			if ((k = bufferAddChar(tmp, '\\'))) return k;
			if (c == 0x22) { if ((k = bufferAddChar(tmp, 0x22))) return k; }
			else if (c == '\\') { if ((k = bufferAddChar(tmp, '\\'))) return k; }
			else if (c == '\b') { if ((k = bufferAddChar(tmp, 'b'))) return k; }
			else if (c == '\f') { if ((k = bufferAddChar(tmp, 'f'))) return k; }
			else if (c == '\n') { if ((k = bufferAddChar(tmp, 'n'))) return k; }
			else if (c == '\r') { if ((k = bufferAddChar(tmp, 'r'))) return k; }
			else if (c == '\t') { if ((k = bufferAddChar(tmp, 't'))) return k; }
			else if (bufferPrintf(tmp, "u%04x", c & 0xffff)) return k;
		}
		else if (c < 0x80)
		{
			if ((k = bufferAddChar(tmp, c))) return k;
		}
		else return EXEC_FORMAT;
	}
	if ((k = bufferAddChar(tmp, 0x22))) return k;
	return 0;
}


int xmlToStr(Buffer* tmp, char* src, LINT len, int latin)
{
	int k;
	bufferReinit(tmp);

	while (1)
	{
		LINT c = 0;
		char* semicolumn;
		char* from = src;
		src = strstr(from, "&");
		if (!src)
		{
			if ((k = bufferAddStr(tmp, from))) return k;
			return 0;
		}
		if ((k = bufferAddBin(tmp, from, src - from))) return k;
		src++;
		semicolumn = strstr(src, ";");
		if (!semicolumn) return EXEC_FORMAT;
		if (src[0] == '#')
		{
			if (src[1] == 'x') c = (int)intFromHex(src + 2);
			else c = (int)intFromAsc(src + 1, 0);
		}
		else c = codeFromEntity(src, semicolumn - src);

		if (latin)
		{
			if ((c >= 0) && (c <= 255))
			{
				if ((k = bufferAddChar(tmp, (char)c))) return k;
			}
		}
		else
		{
			if (c >= 2048)
			{
				if ((k = bufferAddChar(tmp, (char)(0xe0 + ((c >> 12) & 0x0f))))) return k;
				if ((k = bufferAddChar(tmp, (char)(0x80 + ((c >> 6) & 0x3f))))) return k;
				if ((k = bufferAddChar(tmp, (char)(0x80 + (c & 0x3f))))) return k;
			}
			else if (c >= 128)
			{
				if ((k = bufferAddChar(tmp, (char)(0xc0 + ((c >> 6) & 0x1f))))) return k;
				if ((k = bufferAddChar(tmp, (char)((c & 0x3f))))) return k;
			}
			else if ((k = bufferAddChar(tmp, (char)(c)))) return k;
		}
		src = semicolumn + 1;
	}
}

int u8FromXml(Buffer* tmp, char* src, LINT len) { return xmlToStr(tmp, src, len, 0);  }
int latinFromXml(Buffer* tmp, char* src, LINT len) { return xmlToStr(tmp, src, len, 1); }

int xmlFromStr(Buffer* tmp, char* src, LINT len)
{
	LINT i;
	int k;
	bufferReinit(tmp);
	for (i = 0; i < len; i++)
	{
		char c = src[i];
		if (c == '&') {
			if ((k = bufferAddStr(tmp, "&amp;"))) return k;
		}
		else if (c == '"') {
			if ((k = bufferAddStr(tmp, "&quot;"))) return k;
		}
		else if (c == 39) {
			if ((k = bufferAddStr(tmp, "&apos;"))) return k;
		}
		else if (c == '<') {
			if ((k = bufferAddStr(tmp, "&lt;"))) return k;
		}
		else if (c == '>') {
			if ((k = bufferAddStr(tmp, "&gt;"))) return k;
		}
		else if ((k = bufferAddChar(tmp, c))) return k;
	}
	return 0;
}

int strWithLF(Buffer* tmp, char* src, LINT len)
{
	LINT i;
	int k;
	bufferReinit(tmp);
	for (i = 0; i < len; i++)
	{
		char c = src[i];
		if (c != 13) k = bufferAddChar(tmp, c);
		else if (src[i + 1] != 10) k = bufferAddChar(tmp, 10);
		else k = 0;
		if (k) return k;
	}
	return 0;
}
int strWithCR(Buffer* tmp, char* src, LINT len)
{
	LINT i;
	int k;
	char last=0;
	bufferReinit(tmp);
	for (i = 0; i < len; i++)
	{
		char c = src[i];
		if (c != 10) k = bufferAddChar(tmp, c);
		else if (last != 13) k = bufferAddChar(tmp, 13);
		else k = 0;
		if (k) return k;
		last = c;
	}
	return 0;
}
int strWithCRLF(Buffer* tmp, char* src, LINT len)
{
	LINT i;
	int k;
	char last = 0;
	bufferReinit(tmp);
	for (i = 0; i < len; i++)
	{
		char c = src[i];
		if (c == 10)
		{
			if ((last!=13)&&((k = bufferAddChar(tmp, 13)))) return k;
			k = bufferAddChar(tmp, 10);
		}
		else if (c == 13)
		{
			if ((k = bufferAddChar(tmp, 13))) return k;
			if (src[i + 1] != 10) k = bufferAddChar(tmp, 10);
		}
		else k = bufferAddChar(tmp, c);
		if (k) return k;
		last = c;
	}
	return 0;
}

#define STRCASE(fun,table) \
int fun(Buffer* tmp, char* src, LINT len)	\
{	\
	LINT i;	\
	int k;	\
	bufferReinit(tmp);	\
	for (i = 0; i < len; i++)	\
	{	\
		unsigned char c = (unsigned char)src[i];	\
		if ((k = bufferAddChar(tmp, table[c]))) return k;	\
	}	\
	return 0;	\
}


#define STRCASEU8(fun,table) \
int fun(Buffer* tmp, char* src, LINT len)	\
{	\
	LINT i = 0;	\
	int k;	\
	bufferReinit(tmp);	\
	while (i < len)	\
	{	\
		int size;	\
		int val = (int)u8Value(&src[i], &size);	\
		if (val < 256) val = table[val];	\
		if ((k = u8Write(tmp, val))) return k;	\
		i += size;	\
	}	\
	return 0;	\
}

#define STRCASEU16LE(fun,table) \
int fun(Buffer* tmp, char* src, LINT len)	\
{	\
	LINT i;	\
	int k;	\
	if (len & 1) return EXEC_FORMAT;	\
	bufferReinit(tmp);	\
	for (i = 0; i < len; i+=2)	\
	{	\
		unsigned char cl = (unsigned char)src[i];	\
		unsigned char ch = (unsigned char)src[i+1];	\
		if ((k = bufferAddChar(tmp, ch?cl:table[cl]))) return k;	\
		if ((k = bufferAddChar(tmp, ch))) return k;	\
	}	\
	return 0;	\
}
#define STRCASEU16BE(fun,table) \
int fun(Buffer* tmp, char* src, LINT len)	\
{	\
	LINT i;	\
	int k;	\
	if (len & 1) return EXEC_FORMAT;	\
	bufferReinit(tmp);	\
	for (i = 0; i < len; i+=2)	\
	{	\
		unsigned char ch = (unsigned char)src[i];	\
		unsigned char cl = (unsigned char)src[i+1];	\
		if ((k = bufferAddChar(tmp, ch))) return k;	\
		if ((k = bufferAddChar(tmp, ch?cl:table[cl]))) return k;	\
	}	\
	return 0;	\
}

STRCASE(strSearchcase, SearchCase)
STRCASE(strLowercase, LowerCase)
STRCASE(strUppercase, UpperCase)
STRCASE(strUnaccented, Unaccented)
STRCASEU8(strSearchcaseU8, SearchCase)
STRCASEU8(strLowercaseU8, LowerCase)
STRCASEU8(strUppercaseU8, UpperCase)
STRCASEU8(strUnaccentedU8, Unaccented)
STRCASEU16LE(strSearchcaseU16Le, SearchCase)
STRCASEU16LE(strLowercaseU16Le, LowerCase)
STRCASEU16LE(strUppercaseU16Le, UpperCase)
STRCASEU16LE(strUnaccentedU16Le, Unaccented)
STRCASEU16BE(strSearchcaseU16Be, SearchCase)
STRCASEU16BE(strLowercaseU16Be, LowerCase)
STRCASEU16BE(strUppercaseU16Be, UpperCase)
STRCASEU16BE(strUnaccentedU16Be, Unaccented)
