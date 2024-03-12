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
#include "crypto_md5.h"
#include "crypto_sha1.h"
#include "crypto_sha2.h"
#include "crypto_aes.h"
#include "crypto_des.h"
#include "crypto_rc4.h"
#include "crypto_crc32.h"
#include "crypto_adler32.h"


typedef struct
{
	LB header;
	FORGET forget;
	MARK mark;

	MD5_CTX context;
}LSmd5;

typedef struct
{
	LB header;
	FORGET forget;
	MARK mark;

	SHA_CTX context;
}LSsha1;

typedef struct
{
	LB header;
	FORGET forget;
	MARK mark;

	SHA256_CTX context;
}LSsha256;

typedef struct
{
	LB header;
	FORGET forget;
	MARK mark;

	SHA384_CTX context;
}LSsha384;

typedef struct
{
	LB header;
	FORGET forget;
	MARK mark;

	SHA512_CTX context;
}LSsha512;

typedef struct
{
	LB header;
	FORGET forget;
	MARK mark;

	AesCtx context;
}LScbc;

typedef struct
{
	LB header;
	FORGET forget;
	MARK mark;

	DesCtx context;
}LSdes;

typedef struct
{
	LB header;
	FORGET forget;
	MARK mark;

	Rc4Ctx context;
}LSrc4;

//------------ MD5
int fun_md5Create(Thread* th)
{
	LSmd5* md5 = (LSmd5*)memoryAllocExt(th, sizeof(LSmd5), DBG_BIN, NULL, NULL); if (!md5) return EXEC_OM;
	MD5Init(&md5->context);
	FUN_RETURN_PNT((LB*)md5);
}

int fun_md5Output(Thread* th)
{
	LB* buffer;
	LSmd5* md5 = (LSmd5*)STACK_PNT(th, 0);
	if (!md5) FUN_RETURN_NIL;

	buffer = memoryAllocStr(th, NULL, 16); if (!buffer) return EXEC_OM;
	MD5Final((unsigned char*)STR_START(buffer),&md5->context);
	FUN_RETURN_PNT(buffer);
}

int fun_md5Process(Thread* th)
{
	int lenIsNil = STACK_IS_NIL(th,0);
	LINT len = STACK_INT(th, 0);
	LINT start = STACK_INT(th, 1);
	LB* src = STACK_PNT(th, 2);
	LSmd5* md5 = (LSmd5*)STACK_PNT(th, 3);
	if (!md5) FUN_RETURN_NIL;
	FUN_SUBSTR(src,start,len,lenIsNil,STR_LENGTH(src));

	MD5Update(&md5->context, (unsigned char*)STR_START(src) + start, (int)len);
	FUN_RETURN_PNT((LB*)md5);
}

//------------ SHA1
int fun_sha1Create(Thread* th)
{
	LSsha1* sha = (LSsha1*)memoryAllocExt(th,sizeof(LSsha1), DBG_BIN, NULL, NULL); if (!sha) return EXEC_OM;
	SHAInit(&sha->context);
	FUN_RETURN_PNT((LB*)sha);
}

int fun_sha1Output(Thread* th)
{
	LB* buffer;
	LSsha1* sha = (LSsha1*)STACK_PNT(th, 0);
	if (!sha) FUN_RETURN_NIL;

	buffer = memoryAllocStr(th,NULL, 20); if (!buffer) return EXEC_OM;
	SHAFinal(&sha->context,(unsigned char*)STR_START(buffer));
	FUN_RETURN_PNT(buffer);
}
int fun_sha1Process(Thread* th)
{
	int lenIsNil = STACK_IS_NIL(th,0);
	LINT len = STACK_INT(th, 0);
	LINT start = STACK_INT(th, 1);
	LB* src = STACK_PNT(th, 2);
	LSsha1* sha = (LSsha1*)STACK_PNT(th, 3);
	if (!sha) FUN_RETURN_NIL;
	FUN_SUBSTR(src, start,len,lenIsNil,STR_LENGTH(src));
	SHAUpdate(&sha->context, (unsigned char*)STR_START(src) + start, (int)len);
	FUN_RETURN_PNT((LB*)sha);
}

//------------ SHA256
int fun_sha256Create(Thread* th)
{
	LSsha256* sha = (LSsha256*)memoryAllocExt(th,sizeof(LSsha256), DBG_BIN, NULL, NULL); if (!sha) return EXEC_OM;
	sha256create(&sha->context);
	FUN_RETURN_PNT((LB*)sha);
}

int fun_sha256Output(Thread* th)
{
	LB* buffer;
	LSsha256* sha=(LSsha256*)STACK_PNT(th,0);
	if (!sha) FUN_RETURN_NIL;
	
	buffer=memoryAllocStr(th,NULL,256/8); if (!buffer) return EXEC_OM;
	sha256result(&sha->context,STR_START(buffer));
	FUN_RETURN_PNT(buffer);
}
int fun_sha256Process(Thread* th)
{
	int lenIsNil = STACK_IS_NIL(th,0);
	LINT len = STACK_INT(th, 0);
	LINT start=STACK_INT(th,1);
	LB* src=STACK_PNT(th,2);
	LSsha256* sha=(LSsha256*)STACK_PNT(th,3);
	if (!sha) FUN_RETURN_NIL;
	FUN_SUBSTR(src,start,len,lenIsNil,STR_LENGTH(src));
	sha256process(&sha->context,STR_START(src)+start,(int)len);
	FUN_RETURN_PNT((LB*)sha);
}

//------------ SHA384
int fun_sha384Create(Thread* th)
{
	LSsha384* sha = (LSsha384*)memoryAllocExt(th, sizeof(LSsha384), DBG_BIN, NULL, NULL); if (!sha) return EXEC_OM;
	sha384create(&sha->context);
	FUN_RETURN_PNT((LB*)sha);
}

int fun_sha384Output(Thread* th)
{
	LB* buffer;
	LSsha384* sha = (LSsha384*)STACK_PNT(th, 0);
	if (!sha) FUN_RETURN_NIL;

	buffer = memoryAllocStr(th, NULL, 384 / 8); if (!buffer) return EXEC_OM;
	sha384result(&sha->context, STR_START(buffer));
	FUN_RETURN_PNT(buffer);
}
int fun_sha384Process(Thread* th)
{
	int lenIsNil = STACK_IS_NIL(th,0);
	LINT len = STACK_INT(th, 0);
	LINT start = STACK_INT(th, 1);
	LB* src = STACK_PNT(th, 2);
	LSsha384* sha = (LSsha384*)STACK_PNT(th, 3);
	if (!sha) FUN_RETURN_NIL;
	FUN_SUBSTR(src, start, len, lenIsNil, STR_LENGTH(src));
	sha384process(&sha->context, STR_START(src) + start, (int)len);
	FUN_RETURN_PNT((LB*)sha);
}

//------------ SHA512
int fun_sha512Create(Thread* th)
{
	LSsha512* sha=(LSsha512*)memoryAllocExt(th, sizeof(LSsha512),DBG_BIN,NULL,NULL); if (!sha) return EXEC_OM;
	sha512create(&sha->context);
	FUN_RETURN_PNT((LB*)sha);
}

int fun_sha512Output(Thread* th)
{
	LB* buffer;
	LSsha512* sha=(LSsha512*)STACK_PNT(th,0);
	if (!sha) FUN_RETURN_NIL;
	
	buffer=memoryAllocStr(th, NULL,512/8); if (!buffer) return EXEC_OM;
	sha512result(&sha->context,STR_START(buffer));
	FUN_RETURN_PNT(buffer);
}

int fun_sha512Process(Thread* th)
{
	int lenIsNil = STACK_IS_NIL(th,0);
	LINT len = STACK_INT(th, 0);
	LINT start=STACK_INT(th,1);
	LB* src=STACK_PNT(th,2);
	LSsha512* sha=(LSsha512*)STACK_PNT(th,3);
	if (!sha) FUN_RETURN_NIL;
	FUN_SUBSTR(src, start, len, lenIsNil, STR_LENGTH(src));
	sha512process(&sha->context,STR_START(src)+start,(int)len);
	FUN_RETURN_PNT((LB*)sha);
}


//------------ AES
int fun_aesCreate(Thread* th)
{
	LScbc* d;
	LINT key_len;

	LB* key=STACK_PNT(th,0);
	if (!key) FUN_RETURN_NIL;
	key_len = STR_LENGTH(key);
	if ((key_len !=16)&& (key_len != 24)&& (key_len != 32)) FUN_RETURN_NIL;

	d=(LScbc*)memoryAllocExt(th, sizeof(LScbc),DBG_BIN,NULL,NULL); if (!d) return EXEC_OM;

	AESCreate(&d->context,STR_START(key),(int)key_len);
	FUN_RETURN_PNT((LB*)d);
}
int fun_aesEncrypt(Thread* th)
{
	LINT index=STACK_INT(th,0);
	LB* src=STACK_PNT(th,1);
	LScbc* d=(LScbc*)STACK_PNT(th,2);
	if (!d) FUN_RETURN_NIL;
	FUN_CHECK_CONTAINS(src, index, AES_BLOCKLEN, STR_LENGTH(src));

	AESEncrypt(&d->context, STR_START(src) + index);
	FUN_RETURN_PNT((LB*)d);
}

int fun_aesDecrypt(Thread* th)
{
	LINT index = STACK_INT(th, 0);
	LB* src = STACK_PNT(th, 1);
	LScbc* d = (LScbc*)STACK_PNT(th, 2);
	if (!d) FUN_RETURN_NIL;
	FUN_CHECK_CONTAINS(src, index, AES_BLOCKLEN, STR_LENGTH(src));

	AESDecrypt(&d->context, STR_START(src) + index);
	FUN_RETURN_PNT((LB*)d);
}

int fun_aesOutput(Thread* th)
{
	LB* p;
	LScbc* d = (LScbc*)STACK_PNT(th, 0);
	if (!d) FUN_RETURN_NIL;

	p = memoryAllocStr(th, NULL, AES_BLOCKLEN); if (!p) return EXEC_OM;
	AESOutput(&d->context, STR_START(p));
	FUN_RETURN_PNT(p);
}
int fun_aesWriteBytes(Thread* th)
{
	LINT index = STACK_INT(th, 0);
	LB* dst = STACK_PNT(th, 1);
	LScbc* d = (LScbc*)STACK_PNT(th, 2);
	if (!d) FUN_RETURN_NIL;
	FUN_CHECK_CONTAINS(dst, index, AES_BLOCKLEN, STR_LENGTH(dst));

	AESOutput(&d->context, STR_START(dst)+index);
	FUN_RETURN_PNT((LB*)d);
}

//------------ DES
int fun_desCreate(Thread* th)
{
	LSdes* d;
	LINT key_len;

	LB* key = STACK_PNT(th, 0);
	if (!key) FUN_RETURN_NIL;
	key_len = STR_LENGTH(key);
	if (key_len != 8) FUN_RETURN_NIL;

	d = (LSdes*)memoryAllocExt(th, sizeof(LSdes), DBG_BIN, NULL, NULL); if (!d) return EXEC_OM;

	DESCreate(&d->context, STR_START(key));
	FUN_RETURN_PNT((LB*)d);
}
int fun_desEncrypt(Thread* th)
{
	LINT index = STACK_INT(th, 0);
	LB* src = STACK_PNT(th, 1);
	LSdes* d = (LSdes*)STACK_PNT(th, 2);
	if (!d) FUN_RETURN_NIL;
	FUN_CHECK_CONTAINS(src, index, DES_BLOCKLEN, STR_LENGTH(src));

	DESProcess(&d->context, STR_START(src) + index,1);
	FUN_RETURN_PNT((LB*)d);
}

int fun_desDecrypt(Thread* th)
{
	LINT index = STACK_INT(th, 0);
	LB* src = STACK_PNT(th, 1);
	LSdes* d = (LSdes*)STACK_PNT(th, 2);
	if (!d) FUN_RETURN_NIL;
	FUN_CHECK_CONTAINS(src, index, DES_BLOCKLEN, STR_LENGTH(src));

	DESProcess(&d->context, STR_START(src) + index,0);
	FUN_RETURN_PNT((LB*)d);
}

int fun_desOutput(Thread* th)
{
	LB* p;
	LSdes* d = (LSdes*)STACK_PNT(th, 0);
	if (!d) FUN_RETURN_NIL;

	p = memoryAllocStr(th, NULL, DES_BLOCKLEN); if (!p) return EXEC_OM;
	DESOutput(&d->context, STR_START(p));
	FUN_RETURN_PNT(p);
}
int fun_desWriteBytes(Thread* th)
{
	LINT index = STACK_INT(th, 0);
	LB* dst = STACK_PNT(th, 1);
	LSdes* d = (LSdes*)STACK_PNT(th, 2);
	if (!d) FUN_RETURN_NIL;
	FUN_CHECK_CONTAINS(dst, index, DES_BLOCKLEN, STR_LENGTH(dst));

	DESOutput(&d->context, STR_START(dst) + index);
	FUN_RETURN_PNT((LB*)d);
}

//------------ RC4
int fun_rc4Create(Thread* th)
{
	LSrc4* d;
	LB* key = STACK_PNT(th, 0);
	if (!key) FUN_RETURN_NIL;
	
	d = (LSrc4*)memoryAllocExt(th, sizeof(LSrc4), DBG_BIN, NULL, NULL); if (!d) return EXEC_OM;
	RC4Create(&d->context, STR_START(key),STR_LENGTH(key));
	FUN_RETURN_PNT((LB*)d);
}

int fun_rc4Output(Thread* th)
{
	LB* buffer;

	int lenIsNil = STACK_IS_NIL(th,0);
	LINT len = STACK_INT(th, 0);
	LINT start = STACK_INT(th, 1);
	LB* src = STACK_PNT(th, 2);
	LSrc4* d = (LSrc4*)STACK_PNT(th, 3);
	if (!d) FUN_RETURN_NIL;
	FUN_SUBSTR(src, start,len,lenIsNil,STR_LENGTH(src));

	buffer = memoryAllocStr(th, NULL, len); if (!buffer) return EXEC_OM;
	RC4Process(&d->context, STR_START(src) + start, len, STR_START(buffer));
	FUN_RETURN_PNT(buffer);
}

int fun_rc4WriteBytes(Thread* th)
{
	int lenIsNil = STACK_IS_NIL(th,0);
	LINT len = STACK_INT(th, 0);
	LINT start = STACK_INT(th, 1);
	LB* src = STACK_PNT(th, 2);
	LINT index = STACK_INT(th, 3);
	LB* dst = STACK_PNT(th, 4);
	LSrc4* d = (LSrc4*)STACK_PNT(th, 5);
	if (!d) FUN_RETURN_NIL;
	FUN_COPY_CROP(dst, index, STR_LENGTH(dst), src, start, len, lenIsNil, STR_LENGTH(src));

	RC4Process(&d->context, STR_START(src) + start, len, STR_START(dst)+ index);
	FUN_RETURN_PNT((LB*)d);
}

int fun_crc32(Thread* th)
{
	LINT v;

	LINT crc = STACK_INT(th, 0);
	LB* src = STACK_PNT(th, 1);
	if (!src) FUN_RETURN_NIL;
	v = crc32Str((unsigned int)crc, STR_START(src), (int)STR_LENGTH(src));
	FUN_RETURN_INT(v & 0xffffffff);
}

int fun_adler32(Thread* th)
{
	LINT v;

	LINT crc = STACK_INT(th, 0);
	LB* src = STACK_PNT(th, 1);
	if (!src) FUN_RETURN_NIL;
	v = adler32Str((unsigned int)crc, STR_START(src), (int)STR_LENGTH(src));
	FUN_RETURN_INT(v & 0xffffffff);
}

int coreCryptoInit(Thread* th, Pkg *system)
{
	Def* Md5 = pkgAddType(th, system, "Md5");
	Type* fun_Md5 = typeAlloc(th,TYPECODE_FUN, NULL, 1, Md5->type);
	Type* fun_Md5_S = typeAlloc(th,TYPECODE_FUN, NULL, 2, Md5->type, MM.Str);
	Type* fun_Md5_S_I_I_Md5 = typeAlloc(th,TYPECODE_FUN, NULL, 5,
		Md5->type, MM.Str, MM.Int, MM.Int, Md5->type);
	Type* fun_Md5_B_I_I_Md5 = typeAlloc(th, TYPECODE_FUN, NULL, 5,
		Md5->type, MM.Bytes, MM.Int, MM.Int, Md5->type);

	Def* Sha1 = pkgAddType(th, system, "Sha1");
	Type* fun_Sha1 = typeAlloc(th,TYPECODE_FUN, NULL, 1, Sha1->type);
	Type* fun_Sha1_S = typeAlloc(th,TYPECODE_FUN, NULL, 2, Sha1->type, MM.Str);
	Type* fun_Sha1_S_I_I_Sha1 = typeAlloc(th,TYPECODE_FUN, NULL, 5,
		Sha1->type, MM.Str, MM.Int, MM.Int, Sha1->type);
	Type* fun_Sha1_B_I_I_Sha1 = typeAlloc(th,TYPECODE_FUN, NULL, 5,
		Sha1->type, MM.Bytes, MM.Int, MM.Int, Sha1->type);

	Def* Sha256=pkgAddType(th, system,"Sha256");
	Type* fun_Sha256=typeAlloc(th,TYPECODE_FUN,NULL,1,Sha256->type);
	Type* fun_Sha256_S=typeAlloc(th,TYPECODE_FUN,NULL,2,Sha256->type,MM.Str);
	Type* fun_Sha256_S_I_I_Sha256=typeAlloc(th,TYPECODE_FUN,NULL,5,
		Sha256->type,MM.Str,MM.Int,MM.Int,Sha256->type);
	Type* fun_Sha256_B_I_I_Sha256=typeAlloc(th,TYPECODE_FUN,NULL,5,
		Sha256->type,MM.Bytes,MM.Int,MM.Int,Sha256->type);

	Def* Sha384=pkgAddType(th, system,"Sha384");
	Type* fun_Sha384=typeAlloc(th,TYPECODE_FUN,NULL,1,Sha384->type);
	Type* fun_Sha384_S=typeAlloc(th,TYPECODE_FUN,NULL,2,Sha384->type,MM.Str);
	Type* fun_Sha384_S_I_I_Sha384=typeAlloc(th,TYPECODE_FUN,NULL,5,
		Sha384->type,MM.Str,MM.Int,MM.Int,Sha384->type);
	Type* fun_Sha384_B_I_I_Sha384=typeAlloc(th,TYPECODE_FUN,NULL,5,
		Sha384->type,MM.Bytes,MM.Int,MM.Int,Sha384->type);

	Def* Sha512=pkgAddType(th, system,"Sha512");
	Type* fun_Sha512=typeAlloc(th,TYPECODE_FUN,NULL,1,Sha512->type);
	Type* fun_Sha512_S=typeAlloc(th,TYPECODE_FUN,NULL,2,Sha512->type,MM.Str);
	Type* fun_Sha512_S_I_I_Sha512=typeAlloc(th,TYPECODE_FUN,NULL,5,
		Sha512->type,MM.Str,MM.Int,MM.Int,Sha512->type);
	Type* fun_Sha512_B_I_I_Sha512=typeAlloc(th,TYPECODE_FUN,NULL,5,
		Sha512->type,MM.Bytes,MM.Int,MM.Int,Sha512->type);

	Def* Aes = pkgAddType(th, system, "Aes");
	Type* fun_S_Aes = typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Str, Aes->type);
	Type* fun_Aes_S_I_Aes = typeAlloc(th,TYPECODE_FUN, NULL, 4, Aes->type, MM.Str, MM.Int, Aes->type);
	Type* fun_Aes_S = typeAlloc(th,TYPECODE_FUN, NULL, 2, Aes->type, MM.Str);
	Type* fun_Aes_B_I_Aes = typeAlloc(th,TYPECODE_FUN, NULL, 4, Aes->type, MM.Bytes, MM.Int, Aes->type);

	Def* Des = pkgAddType(th, system, "Des");
	Type* fun_S_Des = typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Str, Des->type);
	Type* fun_Des_S_I_Des = typeAlloc(th,TYPECODE_FUN, NULL, 4, Des->type, MM.Str, MM.Int, Des->type);
	Type* fun_Des_S = typeAlloc(th,TYPECODE_FUN, NULL, 2, Des->type, MM.Str);
	Type* fun_Des_B_I_Des = typeAlloc(th,TYPECODE_FUN, NULL, 4, Des->type, MM.Bytes, MM.Int, Des->type);

	Def* Rc4 = pkgAddType(th, system, "Rc4");
	Type* fun_S_Rc4 = typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.Str, Rc4->type);
	Type* fun_Rc4_S_I_I_S = typeAlloc(th,TYPECODE_FUN, NULL, 5, Rc4->type, MM.Str, MM.Int, MM.Int, MM.Str);
	Type* fun_Rc4_B_I_S_I_I_Rc4 = typeAlloc(th,TYPECODE_FUN, NULL, 7, Rc4->type, MM.Bytes, MM.Int, MM.Str, MM.Int, MM.Int, Rc4->type);

	pkgAddFun(th, system, "md5Create",fun_md5Create, fun_Md5);
	pkgAddFun(th, system, "md5Process",fun_md5Process, fun_Md5_S_I_I_Md5);
	pkgAddFun(th, system, "md5ProcessBytes",fun_md5Process, fun_Md5_B_I_I_Md5);
	pkgAddFun(th, system, "md5Output",fun_md5Output, fun_Md5_S);

	pkgAddFun(th, system,"sha1Create",fun_sha1Create,fun_Sha1);
	pkgAddFun(th, system,"sha1Process",fun_sha1Process,fun_Sha1_S_I_I_Sha1);
	pkgAddFun(th, system,"sha1ProcessBytes",fun_sha1Process,fun_Sha1_B_I_I_Sha1);
	pkgAddFun(th, system,"sha1Output",fun_sha1Output,fun_Sha1_S);

	pkgAddFun(th, system,"sha256Create",fun_sha256Create,fun_Sha256);
	pkgAddFun(th, system,"sha256Process",fun_sha256Process,fun_Sha256_S_I_I_Sha256);
	pkgAddFun(th, system,"sha256ProcessBytes",fun_sha256Process,fun_Sha256_B_I_I_Sha256);
	pkgAddFun(th, system,"sha256Output",fun_sha256Output,fun_Sha256_S);

	pkgAddFun(th, system,"sha384Create",fun_sha384Create,fun_Sha384);
	pkgAddFun(th, system,"sha384Process",fun_sha384Process,fun_Sha384_S_I_I_Sha384);
	pkgAddFun(th, system,"sha384ProcessBytes",fun_sha384Process,fun_Sha384_B_I_I_Sha384);
	pkgAddFun(th, system,"sha384Output",fun_sha384Output,fun_Sha384_S);

	pkgAddFun(th, system,"sha512Create",fun_sha512Create,fun_Sha512);
	pkgAddFun(th, system,"sha512Process",fun_sha512Process,fun_Sha512_S_I_I_Sha512);
	pkgAddFun(th, system,"sha512ProcessBytes",fun_sha512Process,fun_Sha512_B_I_I_Sha512);
	pkgAddFun(th, system,"sha512Output",fun_sha512Output,fun_Sha512_S);

	pkgAddConstInt(th, system,"AES_BLOCK",AES_BLOCKLEN,MM.Int);
	pkgAddFun(th, system,"aesCreate",fun_aesCreate,fun_S_Aes);
	pkgAddFun(th, system,"aesEncrypt",fun_aesEncrypt,fun_Aes_S_I_Aes);
	pkgAddFun(th, system,"aesEncryptBytes",fun_aesEncrypt,fun_Aes_B_I_Aes);
	pkgAddFun(th, system,"aesDecrypt",fun_aesDecrypt,fun_Aes_S_I_Aes);
	pkgAddFun(th, system,"aesDecryptBytes",fun_aesDecrypt,fun_Aes_B_I_Aes);
	pkgAddFun(th, system,"aesOutput",fun_aesOutput, fun_Aes_S);
	pkgAddFun(th, system, "aesWriteBytes",fun_aesWriteBytes, fun_Aes_B_I_Aes);

	pkgAddConstInt(th, system, "DES_BLOCK",DES_BLOCKLEN, MM.Int);
	pkgAddFun(th, system, "desCreate",fun_desCreate, fun_S_Des);
	pkgAddFun(th, system, "desEncrypt",fun_desEncrypt, fun_Des_S_I_Des);
	pkgAddFun(th, system, "desDecrypt",fun_desDecrypt, fun_Des_S_I_Des);
	pkgAddFun(th, system, "desOutput",fun_desOutput, fun_Des_S);
	pkgAddFun(th, system, "desWriteBytes",fun_desWriteBytes, fun_Des_B_I_Des);

	pkgAddFun(th, system, "rc4Create",fun_rc4Create, fun_S_Rc4);
	pkgAddFun(th, system, "rc4Output",fun_rc4Output, fun_Rc4_S_I_I_S);
	pkgAddFun(th, system, "rc4WriteBytes",fun_rc4WriteBytes, fun_Rc4_B_I_S_I_I_Rc4);

	pkgAddFun(th, system, "strCrc32", fun_crc32, typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.Str, MM.Int, MM.Int));
	pkgAddFun(th, system, "bytesCrc32", fun_crc32, typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.Bytes, MM.Int, MM.Int));
	pkgAddFun(th, system, "strAdler32", fun_adler32, typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.Str, MM.Int, MM.Int));
	pkgAddFun(th, system, "bytesAdler32", fun_adler32, typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.Bytes, MM.Int, MM.Int));
	return 0;
}
