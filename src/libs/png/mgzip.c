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
#include<stdio.h>
#include<stdlib.h>
#include"zlib.h"
#include"zutil.h"

// header sample gzip :
//00000000 1F 8B 08 00 00 00 00 00 00 03 CD 98 D1 6B 1C 37
// 1F 8B : magic
// 08 : method (DEFLATED)
// 00 : flag
// 6 bytes : time, xflags and OS code (03)
// then data (CD ...)

// deflate -> small 2 bytes header 78 9C
//00000000 78 9C [ CD 98 D1 6B 1C 37 10 C6 DF F7 AF 98 E7 C4
//...
//00000580 19 62 FF 07 ] E2 05 26 1C
// -> E2 05 26 1C : 'Write the trailer'

// gzip -> call deflateInit2_ after writing the header
//00000000 1F 8B 08 00 00 00 00 00 00 03 [ CD 98 D1 6B 1C 37
//...
//00000580 59 1C E2 1F C0 CC 87 B6 19 62 FF 07 ] A7 42 5D FF
//00000590 FC 20 00 00
// -> A7 42 5D FF : crc LSB
// -> FC 20 00 00 : size input (LSB)


#define CHUNK 16384
char* gzipDeflate(int mode, char* src,int len,int* outlen, void* fmalloc, void* ffree, void* user)
{
	int ret;
	unsigned have;
	z_stream strm;
	int maxout=len+CHUNK;
	char* out;
	char* res;

	// allocate deflate state
	strm.zalloc = fmalloc;
	strm.zfree = ffree;
	strm.opaque = user;

	res= (char*)((*strm.zalloc)(user,1,maxout));
	*outlen = 0;

	if (mode==0) ret = deflateInit2(&strm, Z_DEFAULT_COMPRESSION,
		Z_DEFLATED, -MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY);	// deflateInit2(strm, level, method, windowBits, memLevel, strategy)
	else ret = deflateInit(&strm, Z_DEFAULT_COMPRESSION);	// deflateInit(strm, level)

	if (ret != Z_OK) return res;

	out=res;

    // compress until end of file
	strm.avail_in = len;
	strm.next_in = (Bytef*)src;
	// run deflate() on input until output buffer not full, finish compression if all of source has been read in
	do
	{
		strm.avail_out = maxout;
		strm.next_out = (Bytef*)out;
		ret = deflate(&strm, Z_FINISH);    // no bad return value
		have = maxout - strm.avail_out;
//		printf("output %d bytes\n",have);
		out+=have;
		(*outlen)+=have;
	} while (strm.avail_out == 0);

//	printf("outlen=%d\n", *outlen);
	// clean up and return
	deflateEnd(&strm);
	return res;
}

char* gzipInflate(int mode, char* src,int len,int* outlen, void* fmalloc, void* ffree, void* user)
{
	int count = 0;
	int ret;
	unsigned have;
	z_stream strm;

	int maxout=CHUNK;
	char* res;

	// allocate deflate state
	strm.zalloc = fmalloc;
	strm.zfree = ffree;
	strm.opaque = user;
	strm.avail_in = 0;
    strm.next_in = Z_NULL;

	res = (char*)((*strm.zalloc)(user, 1, maxout));
//	res = malloc(maxout);
	*outlen = 0;

	if (mode==0) ret = inflateInit2(&strm, -MAX_WBITS);
	else ret = inflateInit2(&strm, MAX_WBITS);
	if (ret != Z_OK) return res;

	strm.avail_in = len;
	strm.next_in = (Bytef*)src;
	// run inflate() on input until output buffer not full
	do
	{
		int avail=maxout-(*outlen);
		if (avail<CHUNK)
		{
			char* newout = (char*)((*strm.zalloc)(user, 1, maxout * 2));
			if (!newout) return NULL;
			memcpy(newout, res, maxout);
			res = newout;
			maxout*=2;
//			printf("realloc %llx %d %d\n", res, count, maxout);
//			res=(char*)realloc(res,maxout);
		}
		strm.avail_out = maxout-(*outlen);
		strm.next_out = (Bytef*)&res[*outlen];
		ret = inflate(&strm, Z_NO_FLUSH);
//		ret = inflate(&strm, Z_PARTIAL_FLUSH);
		if ((ret!=Z_OK)&&(ret!=Z_STREAM_END))
		{
//			printf("error %d %d out=%d\n",count,ret, strm.avail_out);
//			_myHexDump(res, strm.avail_out, 0);
			*outlen=0;
			goto cleanup;
		}
		have = (maxout-(*outlen)) - strm.avail_out;
//		printf("output %d bytes\n",have);
		(*outlen)+=have;
		count++;
	} while (strm.avail_out == 0);

//	printf(">>>>>>>>>>>outlen %d bytes\n", *outlen);
	// clean up and return
cleanup:
	inflateEnd(&strm);
	return res;
}

unsigned int mcrc32(unsigned int crc, unsigned char *buf, int size)
{
	return crc32(crc,buf,size);
}

unsigned int madler32(unsigned int adler, unsigned char* buf, int size)
{
	return adler32(adler, buf, size);
}
