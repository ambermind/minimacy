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

int latinToU16(Thread* th, Buffer* tmp, char* src, LINT len)
{
	LINT i;
	int k;
	bufferReinit(tmp);
	for (i = 0; i < len; i++)
	{
		if ((k = bufferAddchar(th, tmp, src[i]))) return k;
		if ((k = bufferAddchar(th, tmp, 0))) return k;
	}
	return 0;
}

int u16ToLatin(Thread* th, Buffer* tmp, char* src, LINT len)
{
	LINT i;
	int k;
	if (len & 1) return EXEC_FORMAT;
	bufferReinit(tmp);
	for (i = 0; i < len; i+=2) if (!src[i + i + 1]) if ((k = bufferAddchar(th, tmp, src[i + i]))) return k;
	return 0;
}

int u16ToU8(Thread* th, Buffer* tmp, char* src, LINT len)
{
	LINT i;
	int k;

	if (len & 1) return EXEC_FORMAT;
	bufferReinit(tmp);
	for(i=0;i<len;i+=2)
	{
		int c=(src[i*2]&255)+((src[i*2+1]&255)<<8);
		if (c>=2048)
		{
			if ((k = bufferAddchar(th, tmp, 0xe0 + ((c >> 12) & 0x0f)))) return k;
			if ((k = bufferAddchar(th, tmp, 0x80 + ((c >> 6) & 0x3f)))) return k;
			if ((k = bufferAddchar(th, tmp, 0x80 + (c & 0x3f)))) return k;
		}
		else if (c>=128)
		{
			if ((k = bufferAddchar(th, tmp, 0xc0 + ((c >> 6) & 0x1f)))) return k;
			if ((k = bufferAddchar(th, tmp, 0x80 + (c & 0x3f)))) return k;
		}
		else if ((k = bufferAddchar(th, tmp, c))) return k;
	}
	return 0;
}
int u8ToU16(Thread* th, Buffer* tmp, char* src, LINT len)
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
			c = ((c & 0xf) << 12) + ((src[i + 1] & 0x3f) << 6) + (src[i + 2] & 0x3f);
			i += 2;
		}
		else if ((c & 0xf8) == 0xf0)
		{
			i += 3;
			continue;
		}
		if ((k = bufferAddchar(th, tmp, c))) return k;
		c >>= 8;
		if ((k = bufferAddchar(th, tmp, c))) return k;
	}
	return 0;
}

int u8ToLatin(Thread* th, Buffer* tmp, char* src, LINT len)
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
			if (c <= 255) if ((k = bufferAddchar(th, tmp, c))) return k;
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
            if ((k = bufferAddchar(th, tmp, c))) return k;
        }
        else return EXEC_FORMAT;
	}
	return 0;
}
int latinToU8(Thread* th, Buffer* tmp, char* src, LINT len)
{
	LINT i;
	int k;
	bufferReinit(tmp);
	for (i = 0; i < len; i++) if (src[i] & 128)
	{
		int c = src[i] & 255;
		if ((k = bufferAddchar(th, tmp, 0xc0 + ((c >> 6) & 0x1f)))) return k;
		if ((k = bufferAddchar(th, tmp, 0x80 + (c & 0x3f)))) return k;
	}
	else if ((k = bufferAddchar(th, tmp, src[i]))) return k;
	return 0;
}


// parse a string (src points on the first double quote)
int sourceToStr(Thread* th, Buffer* tmp, char* src, LINT len)
{
	int c, n, i,k;
	char separator = *(src++);

	bufferReinit(tmp);
	n = 0;
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
					if (ishex(c))
					{
						src++;
						i = htoc(c);
						c = *src;
						if (ishex(c))
						{
							src++;
							i = (i << 4) + htoc(c);
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
				if ((k=bufferAddchar(th, tmp, c))) return k;
			}
		}
		else if (c == separator)
		{
			if (*src) return EXEC_FORMAT;	// check that the separator is the last char
			return 0;
		}
		else if (c == 0) return EXEC_FORMAT;
		else if ((k=bufferAddchar(th, tmp, c))) return k;
	}
}

int strToSource(Thread* th, Buffer* tmp, char* src, LINT len)
{
	LINT i;
	int c,k;
	bufferReinit(tmp);
	if ((k=bufferAddchar(th, tmp, 0x22))) return k;
	for (i = 0; i < len;i++)
	{
		c = src[i]&255;
		if ((c < 32) || (c >= 128))
		{
			if ((k=bufferAddchar(th, tmp, '\\'))) return k;
			if (c == 0) c = 'z';
			else if (c == 10) c = 'n';
			else if (c == 13) c = 'r';
			else if (c == 9) c = 't';

			if ((c >= 32) && (c < 128))
			{
				if ((k=bufferAddchar(th, tmp, c))) return k;
			}
			else if ((k=bufferPrintf(th, tmp, "$%02x", c))) return k;
		}
		else
		{
			if (c == '\\' || c == '"') if ((k = bufferAddchar(th, tmp, '\\'))) return k;
			if ((k = bufferAddchar(th, tmp, c))) return k;
		}
	}
	if ((k=bufferAddchar(th, tmp, 0x22))) return k;
	return 0;
}


int jsonToU8(Thread* th, Buffer* tmp, char* src, LINT len)
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
				if ((k = bufferAddchar(th, tmp, 0x22))) return k;
			}
			else if (c == '\\') {
				if ((k = bufferAddchar(th, tmp, '\\'))) return k;
			}
			else if (c == 'b') {
				if ((k = bufferAddchar(th, tmp, '\b'))) return k;
			}
			else if (c == 'f') {
				if ((k = bufferAddchar(th, tmp, '\f'))) return k;
			}
			else if (c == 'n') {
				if ((k = bufferAddchar(th, tmp, '\n'))) return k;
			}
			else if (c == 'r') {
				if ((k = bufferAddchar(th, tmp, '\r'))) return k;
			}
			else if (c == 't') {
				if ((k = bufferAddchar(th, tmp, '\t'))) return k;
			}
			else if (c == 'u')
			{
				src++;
				if ((!ishex(src[0])) || (!ishex(src[1])) || (!ishex(src[2])) || (!ishex(src[3]))) return EXEC_FORMAT;
				c = (htoc(src[0]) << 12) + (htoc(src[1]) << 8) + (htoc(src[2]) << 4) + htoc(src[3]);
				src += 3;
				if (c < 0x80) {
					if ((k = bufferAddchar(th, tmp, c))) return k;
				}
				else if (c < 0x800)
				{
					if ((k = bufferAddchar(th, tmp, 0xc0 + ((c >> 6) & 0x1f)))) return k;
					if ((k = bufferAddchar(th, tmp, 0x80 + (c & 0x3f)))) return k;
				}
				else
				{
					if ((k = bufferAddchar(th, tmp, 0xe0 + ((c >> 12) & 0x0f)))) return k;
					if ((k = bufferAddchar(th, tmp, 0x80 + ((c >> 6) & 0x3f)))) return k;
					if ((k = bufferAddchar(th, tmp, 0x80 + (c & 0x3f)))) return k;
				}
			}
			else if ((k = bufferAddchar(th, tmp, c))) return k;
		}
		else if ((k = bufferAddchar(th, tmp, c))) return k;
		src++;
	}
	src++;
	if (*src) return EXEC_FORMAT;
	return 0;
}

int u8ToJson(Thread* th, Buffer* tmp, char* src, LINT len)
{
	int k;
	LINT i;
	bufferReinit(tmp);
	if ((k = bufferAddchar(th, tmp, 0x22))) return k;
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
			if ((k = bufferPrintf(th, tmp, "\\u%04x", c & 0xffff))) return k;
		}
		else if ((c == 0x22) || (c == '\\') || (c < 32))
		{
			if ((k = bufferAddchar(th, tmp, '\\'))) return k;
			if (c == 0x22) { if ((k = bufferAddchar(th, tmp, 0x22))) return k; }
			else if (c == '\\') { if ((k = bufferAddchar(th, tmp, '\\'))) return k; }
			else if (c == '\b') { if ((k = bufferAddchar(th, tmp, 'b'))) return k; }
			else if (c == '\f') { if ((k = bufferAddchar(th, tmp, 'f'))) return k; }
			else if (c == '\n') { if ((k = bufferAddchar(th, tmp, 'n'))) return k; }
			else if (c == '\r') { if ((k = bufferAddchar(th, tmp, 'r'))) return k; }
			else if (c == '\t') { if ((k = bufferAddchar(th, tmp, 't'))) return k; }
			else if (bufferPrintf(th, tmp, "u%04x", c & 0xffff)) return k;
		}
		else if (c < 0x80)
		{
			if ((k = bufferAddchar(th, tmp, c))) return k;
		}
		else return EXEC_FORMAT;
	}
	if ((k = bufferAddchar(th, tmp, 0x22))) return k;
	return 0;
}


int xmlToStr(Thread* th, Buffer* tmp, char* src, LINT len, int latin)
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
			if ((k = bufferAddStr(th, tmp, from))) return k;
			return 0;
		}
		if ((k = bufferAddBin(th, tmp, from, src - from))) return k;
		src++;
		semicolumn = strstr(src, ";");
		if (!semicolumn) return EXEC_FORMAT;
		if (src[0] == '#')
		{
			if (src[1] == 'x') c = (int)ls_htoi(src + 2);
			else c = (int)ls_atoi(src + 1, 0);
		}
		else c = codeFromEntity(src, semicolumn - src);

		if (latin)
		{
			if ((c >= 0) && (c <= 255))
			{
				if ((k = bufferAddchar(th, tmp, (char)c))) return k;
			}
		}
		else
		{
			if (c >= 2048)
			{
				if ((k = bufferAddchar(th, tmp, (char)(0xe0 + ((c >> 12) & 0x0f))))) return k;
				if ((k = bufferAddchar(th, tmp, (char)(0x80 + ((c >> 6) & 0x3f))))) return k;
				if ((k = bufferAddchar(th, tmp, (char)(0x80 + (c & 0x3f))))) return k;
			}
			else if (c >= 128)
			{
				if ((k = bufferAddchar(th, tmp, (char)(0xc0 + ((c >> 6) & 0x1f))))) return k;
				if ((k = bufferAddchar(th, tmp, (char)((c & 0x3f))))) return k;
			}
			else if ((k = bufferAddchar(th, tmp, (char)(c)))) return k;
		}
		src = semicolumn + 1;
	}
}

int xmlToU8(Thread* th, Buffer* tmp, char* src, LINT len) { return xmlToStr(th, tmp, src, len, 0);  }
int xmlToLatin(Thread* th, Buffer* tmp, char* src, LINT len) { return xmlToStr(th, tmp, src, len, 1); }

int strToXml(Thread* th, Buffer* tmp, char* src, LINT len)
{
	LINT i;
	int k;
	bufferReinit(tmp);
	for (i = 0; i < len; i++)
	{
		char c = src[i];
		if (c == '&') {
			if ((k = bufferAddStr(th, tmp, "&amp;"))) return k;
		}
		else if (c == '"') {
			if ((k = bufferAddStr(th, tmp, "&quot;"))) return k;
		}
		else if (c == 39) {
			if ((k = bufferAddStr(th, tmp, "&apos;"))) return k;
		}
		else if (c == '<') {
			if ((k = bufferAddStr(th, tmp, "&lt;"))) return k;
		}
		else if (c == '>') {
			if ((k = bufferAddStr(th, tmp, "&gt;"))) return k;
		}
		else if ((k = bufferAddchar(th, tmp, c))) return k;
	}
	return 0;
}

int strToLF(Thread* th, Buffer* tmp, char* src, LINT len)
{
	LINT i;
	int k;
	bufferReinit(tmp);
	for (i = 0; i < len; i++)
	{
		char c = src[i];
		if (c != 13) k = bufferAddchar(th, tmp, c);
		else if (src[i + 1] != 10) k = bufferAddchar(th, tmp, 10);
		else k = 0;
		if (k) return k;
	}
	return 0;
}
int strToCR(Thread* th, Buffer* tmp, char* src, LINT len)
{
	LINT i;
	int k;
	char last=0;
	bufferReinit(tmp);
	for (i = 0; i < len; i++)
	{
		char c = src[i];
		if (c != 10) k = bufferAddchar(th, tmp, c);
		else if (last != 13) k = bufferAddchar(th, tmp, 13);
		else k = 0;
		if (k) return k;
		last = c;
	}
	return 0;
}
int strToCRLF(Thread* th, Buffer* tmp, char* src, LINT len)
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
			if ((last!=13)&&((k = bufferAddchar(th, tmp, 13)))) return k;
			k = bufferAddchar(th, tmp, 10);
		}
		else if (c == 13)
		{
			if ((k = bufferAddchar(th, tmp, 13))) return k;
			if (src[i + 1] != 10) k = bufferAddchar(th, tmp, 10);
		}
		else k = bufferAddchar(th, tmp, c);
		if (k) return k;
		last = c;
	}
	return 0;
}
