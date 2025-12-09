// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"
#include "crypto_hash.h"
#include "crypto_aes.h"
#include "crypto_des.h"
#include "crypto_rc4.h"
#include "crypto_checksum.h"

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

	SHA1_CTX context;
}LSsha1;

typedef struct
{
	LB header;
	FORGET forget;
	MARK mark;

	SHA2_CTX context;
}LSsha256;

typedef struct
{
	LB header;
	FORGET forget;
	MARK mark;

	SHA2L_CTX context;
}LSsha384;

typedef struct
{
	LB header;
	FORGET forget;
	MARK mark;

	SHA2L_CTX context;
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
	LSmd5* md5 = (LSmd5*)memoryAllocNative(sizeof(LSmd5), DBG_BIN, NULL, NULL); if (!md5) return EXEC_OM;
	md5Create(&md5->context);
	FUN_RETURN_PNT((LB*)md5);
}

int fun_md5Output(Thread* th)
{
	LB* buffer;
	LSmd5* md5 = (LSmd5*)STACK_PNT(th, 0);
	if (!md5) FUN_RETURN_NIL;

	buffer = memoryAllocStr(NULL, 16); if (!buffer) return EXEC_OM;
	md5Output(&md5->context,STR_START(buffer));
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

	md5Process(&md5->context, STR_START(src) + start, len);
	FUN_RETURN_PNT((LB*)md5);
}

//------------ SHA1
int fun_sha1Create(Thread* th)
{
	LSsha1* sha = (LSsha1*)memoryAllocNative(sizeof(LSsha1), DBG_BIN, NULL, NULL); if (!sha) return EXEC_OM;
	sha1Create(&sha->context);
	FUN_RETURN_PNT((LB*)sha);
}

int fun_sha1Output(Thread* th)
{
	LB* buffer;
	LSsha1* sha = (LSsha1*)STACK_PNT(th, 0);
	if (!sha) FUN_RETURN_NIL;

	buffer = memoryAllocStr(NULL, 20); if (!buffer) return EXEC_OM;
	sha1Output(&sha->context,STR_START(buffer));
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
	sha1Process(&sha->context, STR_START(src) + start, len);
	FUN_RETURN_PNT((LB*)sha);
}

//------------ SHA256
int fun_sha256Create(Thread* th)
{
	LSsha256* sha = (LSsha256*)memoryAllocNative(sizeof(LSsha256), DBG_BIN, NULL, NULL); if (!sha) return EXEC_OM;
	sha256Create(&sha->context);
	FUN_RETURN_PNT((LB*)sha);
}

int fun_sha256Output(Thread* th)
{
	LB* buffer;
	LSsha256* sha=(LSsha256*)STACK_PNT(th,0);
	if (!sha) FUN_RETURN_NIL;
	
	buffer=memoryAllocStr(NULL,256/8); if (!buffer) return EXEC_OM;
	sha256Output(&sha->context,STR_START(buffer));
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
	sha256Process(&sha->context,STR_START(src)+start,len);
	FUN_RETURN_PNT((LB*)sha);
}

//------------ SHA384
int fun_sha384Create(Thread* th)
{
	LSsha384* sha = (LSsha384*)memoryAllocNative(sizeof(LSsha384), DBG_BIN, NULL, NULL); if (!sha) return EXEC_OM;
	sha384Create(&sha->context);
	FUN_RETURN_PNT((LB*)sha);
}

int fun_sha384Output(Thread* th)
{
	LB* buffer;
	LSsha384* sha = (LSsha384*)STACK_PNT(th, 0);
	if (!sha) FUN_RETURN_NIL;

	buffer = memoryAllocStr(NULL, 384 / 8); if (!buffer) return EXEC_OM;
	sha384Output(&sha->context, STR_START(buffer));
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
	sha384Process(&sha->context, STR_START(src) + start, len);
	FUN_RETURN_PNT((LB*)sha);
}

//------------ SHA512
int fun_sha512Create(Thread* th)
{
	LSsha512* sha=(LSsha512*)memoryAllocNative(sizeof(LSsha512),DBG_BIN,NULL,NULL); if (!sha) return EXEC_OM;
	sha512Create(&sha->context);
	FUN_RETURN_PNT((LB*)sha);
}

int fun_sha512Output(Thread* th)
{
	LB* buffer;
	LSsha512* sha=(LSsha512*)STACK_PNT(th,0);
	if (!sha) FUN_RETURN_NIL;
	
	buffer=memoryAllocStr(NULL,512/8); if (!buffer) return EXEC_OM;
	sha512Output(&sha->context,STR_START(buffer));
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
	sha512Process(&sha->context,STR_START(src)+start,len);
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

	d=(LScbc*)memoryAllocNative(sizeof(LScbc),DBG_BIN,NULL,NULL); if (!d) return EXEC_OM;

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

	p = memoryAllocStr(NULL, AES_BLOCKLEN); if (!p) return EXEC_OM;
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

	d = (LSdes*)memoryAllocNative(sizeof(LSdes), DBG_BIN, NULL, NULL); if (!d) return EXEC_OM;

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

	p = memoryAllocStr(NULL, DES_BLOCKLEN); if (!p) return EXEC_OM;
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
	
	d = (LSrc4*)memoryAllocNative(sizeof(LSrc4), DBG_BIN, NULL, NULL); if (!d) return EXEC_OM;
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

	buffer = memoryAllocStr(NULL, len); if (!buffer) return EXEC_OM;
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
int fun_crc7(Thread* th)
{
	LINT v;

	LINT crc = STACK_INT(th, 0);
	LB* src = STACK_PNT(th, 1);
	if (!src) FUN_RETURN_NIL;
	v = crc7Str((unsigned int)crc, STR_START(src), (int)STR_LENGTH(src));
	FUN_RETURN_INT(v & 0xff);
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

int systemCryptoInit(Pkg *system)
{
	pkgAddType(system, "Md5");
	pkgAddType(system, "Sha1");
	pkgAddType(system, "Sha256");
	pkgAddType(system, "Sha384");
	pkgAddType(system, "Sha512");
	pkgAddType(system, "Aes");
	pkgAddType(system, "Des");
	pkgAddType(system, "Rc4");

	static const Native nativeDefs[] = { 
		{ NATIVE_FUN, "md5Create", fun_md5Create, "fun -> Md5"},
		{ NATIVE_FUN, "md5Process", fun_md5Process, "fun Md5 Str Int Int -> Md5" },
		{ NATIVE_FUN, "md5ProcessBytes", fun_md5Process, "fun Md5 Bytes Int Int -> Md5" },
		{ NATIVE_FUN, "md5Output", fun_md5Output, "fun Md5 -> Str" },
		{ NATIVE_FUN, "sha1Create", fun_sha1Create, "fun -> Sha1" },
		{ NATIVE_FUN, "sha1Process", fun_sha1Process, "fun Sha1 Str Int Int -> Sha1" },
		{ NATIVE_FUN, "sha1ProcessBytes", fun_sha1Process, "fun Sha1 Bytes Int Int -> Sha1" },
		{ NATIVE_FUN, "sha1Output", fun_sha1Output, "fun Sha1 -> Str" },
		{ NATIVE_FUN, "sha256Create", fun_sha256Create, "fun -> Sha256" },
		{ NATIVE_FUN, "sha256Process", fun_sha256Process, "fun Sha256 Str Int Int -> Sha256" },
		{ NATIVE_FUN, "sha256ProcessBytes", fun_sha256Process, "fun Sha256 Bytes Int Int -> Sha256" },
		{ NATIVE_FUN, "sha256Output", fun_sha256Output, "fun Sha256 -> Str" },
		{ NATIVE_FUN, "sha384Create", fun_sha384Create, "fun -> Sha384" },
		{ NATIVE_FUN, "sha384Process", fun_sha384Process, "fun Sha384 Str Int Int -> Sha384" },
		{ NATIVE_FUN, "sha384ProcessBytes", fun_sha384Process, "fun Sha384 Bytes Int Int -> Sha384" },
		{ NATIVE_FUN, "sha384Output", fun_sha384Output, "fun Sha384 -> Str" },
		{ NATIVE_FUN, "sha512Create", fun_sha512Create, "fun -> Sha512" },
		{ NATIVE_FUN, "sha512Process", fun_sha512Process, "fun Sha512 Str Int Int -> Sha512" },
		{ NATIVE_FUN, "sha512ProcessBytes", fun_sha512Process, "fun Sha512 Bytes Int Int -> Sha512" },
		{ NATIVE_FUN, "sha512Output", fun_sha512Output, "fun Sha512 -> Str" },
		{ NATIVE_INT, "AES_BLOCK", (void*)AES_BLOCKLEN, "Int" },
		{ NATIVE_FUN, "aesCreate", fun_aesCreate, "fun Str -> Aes" },
		{ NATIVE_FUN, "aesEncrypt", fun_aesEncrypt, "fun Aes Str Int -> Aes" },
		{ NATIVE_FUN, "aesEncryptBytes", fun_aesEncrypt, "fun Aes Bytes Int -> Aes" },
		{ NATIVE_FUN, "aesDecrypt", fun_aesDecrypt, "fun Aes Str Int -> Aes" },
		{ NATIVE_FUN, "aesDecryptBytes", fun_aesDecrypt, "fun Aes Bytes Int -> Aes" },
		{ NATIVE_FUN, "aesOutput", fun_aesOutput, "fun Aes -> Str" },
		{ NATIVE_FUN, "aesWriteBytes", fun_aesWriteBytes, "fun Aes Bytes Int -> Aes" },
		{ NATIVE_INT, "DES_BLOCK", (void*)DES_BLOCKLEN, "Int" },
		{ NATIVE_FUN, "desCreate", fun_desCreate, "fun Str -> Des" },
		{ NATIVE_FUN, "desEncrypt", fun_desEncrypt, "fun Des Str Int -> Des" },
		{ NATIVE_FUN, "desDecrypt", fun_desDecrypt, "fun Des Str Int -> Des" },
		{ NATIVE_FUN, "desOutput", fun_desOutput, "fun Des -> Str" },
		{ NATIVE_FUN, "desWriteBytes", fun_desWriteBytes, "fun Des Bytes Int -> Des" },
		{ NATIVE_FUN, "rc4Create", fun_rc4Create, "fun Str -> Rc4" },
		{ NATIVE_FUN, "rc4Output", fun_rc4Output, "fun Rc4 Str Int Int -> Str" },
		{ NATIVE_FUN, "rc4WriteBytes", fun_rc4WriteBytes, "fun Rc4 Bytes Int Str Int Int -> Rc4" },
		{ NATIVE_FUN, "strCrc32", fun_crc32, "fun Str Int -> Int" },
		{ NATIVE_FUN, "bytesCrc32", fun_crc32, "fun Bytes Int -> Int" },
		{ NATIVE_FUN, "strCrc7", fun_crc7, "fun Str Int -> Int" },
		{ NATIVE_FUN, "bytesCrc7", fun_crc7, "fun Bytes Int -> Int" },
		{ NATIVE_FUN, "strAdler32", fun_adler32, "fun Str Int -> Int" },
		{ NATIVE_FUN, "bytesAdler32", fun_adler32, "fun Bytes Int -> Int" },
	};
	NATIVE_DEF(nativeDefs);
	return 0;
}
