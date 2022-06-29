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
	return STACKPUSH(th, PNTTOVAL(md5));
}

int fun_md5Output(Thread* th)
{
	LB* buffer;
	LSmd5* md5 = (LSmd5*)VALTOPNT(STACKGET(th, 0));
	if (!md5) return 0;

	buffer = memoryAllocStr(th, NULL, 16); if (!buffer) return EXEC_OM;
	MD5Final((unsigned char*)STRSTART(buffer),&md5->context);
	STACKSET(th, 0, PNTTOVAL(buffer));
	return 0;
}
int fun_md5Process(Thread* th)
{
	LINT len;

	LW vlen = STACKGET(th, 0);
	LINT index = VALTOINT(STACKGET(th, 1));
	LB* src = VALTOPNT(STACKGET(th, 2));
	LSmd5* md5 = (LSmd5*)VALTOPNT(STACKGET(th, 3));
	if ((!src) || (!md5)) goto cleanup;
	len = (vlen == NIL) ? STRLEN(src) : VALTOINT(vlen);
	if ((index < 0) || (len < 0)) goto cleanup;
	if (index + len > STRLEN(src)) len = STRLEN(src) - index;
	if (len < 0) goto cleanup;
	MD5Update(&md5->context, (unsigned char*)STRSTART(src) + index, (int)len);
cleanup:
	STACKDROPN(th, 3);
	return 0;
}

//------------ SHA1
int fun_sha1Create(Thread* th)
{
	LSsha1* sha1 = (LSsha1*)memoryAllocExt(th,sizeof(LSsha1), DBG_BIN, NULL, NULL); if (!sha1) return EXEC_OM;
	SHAInit(&sha1->context);
	return STACKPUSH(th, PNTTOVAL(sha1));
}

int fun_sha1Output(Thread* th)
{
	LB* buffer;
	LSsha1* sha1 = (LSsha1*)VALTOPNT(STACKGET(th, 0));
	if (!sha1) return 0;

	buffer = memoryAllocStr(th,NULL, 20); if (!buffer) return EXEC_OM;
	SHAFinal(&sha1->context,(unsigned char*)STRSTART(buffer));
	STACKSET(th, 0, PNTTOVAL(buffer));
	return 0;
}
int fun_sha1Process(Thread* th)
{
	LINT len;

	LW vlen = STACKGET(th, 0);
	LINT index = VALTOINT(STACKGET(th, 1));
	LB* src = VALTOPNT(STACKGET(th, 2));
	LSsha1* sha1 = (LSsha1*)VALTOPNT(STACKGET(th, 3));
	if ((!src) || (!sha1)) goto cleanup;
	len = (vlen == NIL) ? STRLEN(src) : VALTOINT(vlen);
	if ((index < 0) || (len < 0)) goto cleanup;
	if (index + len > STRLEN(src)) len = STRLEN(src) - index;
	if (len < 0) goto cleanup;
	SHAUpdate(&sha1->context, (unsigned char*)STRSTART(src) + index, (int)len);
cleanup:
	STACKDROPN(th, 3);
	return 0;
}

//------------ SHA256
int fun_sha256Create(Thread* th)
{
	LSsha256* sha = (LSsha256*)memoryAllocExt(th,sizeof(LSsha256), DBG_BIN, NULL, NULL); if (!sha) return EXEC_OM;
	sha256create(&sha->context);
	return STACKPUSH(th,PNTTOVAL(sha));
}

int fun_sha256Output(Thread* th)
{
	LB* buffer;
	LSsha256* sha=(LSsha256*)VALTOPNT(STACKGET(th,0));
	if (!sha) return 0;
	
	buffer=memoryAllocStr(th,NULL,256/8); if (!buffer) return EXEC_OM;
	sha256result(&sha->context,STRSTART(buffer));
	STACKSET(th,0,PNTTOVAL(buffer));
	return 0;
}
int fun_sha256Process(Thread* th)
{
	LINT len;

	LW vlen=STACKGET(th,0);
	LINT index=VALTOINT(STACKGET(th,1));
	LB* src=VALTOPNT(STACKGET(th,2));
	LSsha256* sha=(LSsha256*)VALTOPNT(STACKGET(th,3));
	if ((!src)||(!sha)) goto cleanup;
	len=(vlen==NIL)?STRLEN(src):VALTOINT(vlen);
	if ((index<0)||(len<0)) goto cleanup;
	if (index+len>STRLEN(src)) len=STRLEN(src)-index;
	if (len<0) goto cleanup;
	sha256process(&sha->context,STRSTART(src)+index,(int)len);
cleanup:
	STACKDROPN(th,3);
	return 0;
}

//------------ SHA384
int fun_sha384Create(Thread* th)
{
	LSsha384* sha = (LSsha384*)memoryAllocExt(th, sizeof(LSsha384), DBG_BIN, NULL, NULL); if (!sha) return EXEC_OM;
	sha384create(&sha->context);
	return STACKPUSH(th, PNTTOVAL(sha));
}

int fun_sha384Output(Thread* th)
{
	LB* buffer;
	LSsha384* sha = (LSsha384*)VALTOPNT(STACKGET(th, 0));
	if (!sha) return 0;

	buffer = memoryAllocStr(th, NULL, 384 / 8); if (!buffer) return EXEC_OM;

	sha384result(&sha->context, STRSTART(buffer));
	STACKSET(th, 0, PNTTOVAL(buffer));
	return 0;
}
int fun_sha384Process(Thread* th)
{
	LINT len;

	LW vlen = STACKGET(th, 0);
	LINT index = VALTOINT(STACKGET(th, 1));
	LB* src = VALTOPNT(STACKGET(th, 2));
	LSsha384* sha = (LSsha384*)VALTOPNT(STACKGET(th, 3));
	if ((!src) || (!sha)) goto cleanup;
	len = (vlen == NIL) ? STRLEN(src) : VALTOINT(vlen);
	if ((index < 0) || (len < 0)) goto cleanup;
	if (index + len > STRLEN(src)) len = STRLEN(src) - index;
	if (len < 0) goto cleanup;
	sha384process(&sha->context, STRSTART(src) + index, (int)len);
cleanup:
	STACKDROPN(th, 3);
	return 0;
}

//------------ SHA512
int fun_sha512Create(Thread* th)
{
	LSsha512* sha=(LSsha512*)memoryAllocExt(th, sizeof(LSsha512),DBG_BIN,NULL,NULL); if (!sha) return EXEC_OM;
	sha512create(&sha->context);
	return STACKPUSH(th,PNTTOVAL(sha));
}

int fun_sha512Output(Thread* th)
{
	LB* buffer;
	LSsha512* sha=(LSsha512*)VALTOPNT(STACKGET(th,0));
	if (!sha) return 0;
	
	buffer=memoryAllocStr(th, NULL,512/8); if (!buffer) return EXEC_OM;

	sha512result(&sha->context,STRSTART(buffer));
	STACKSET(th,0,PNTTOVAL(buffer));
	return 0;
}
int fun_sha512Process(Thread* th)
{
	LINT len;

	LW vlen=STACKGET(th,0);
	LINT index=VALTOINT(STACKGET(th,1));
	LB* src=VALTOPNT(STACKGET(th,2));
	LSsha512* sha=(LSsha512*)VALTOPNT(STACKGET(th,3));
	if ((!src)||(!sha)) goto cleanup;
	len=(vlen==NIL)?STRLEN(src):VALTOINT(vlen);
	if ((index<0)||(len<0)) goto cleanup;
	if (index+len>STRLEN(src)) len=STRLEN(src)-index;
	if (len<0) goto cleanup;
	sha512process(&sha->context,STRSTART(src)+index,(int)len);
cleanup:
	STACKDROPN(th,3);
	return 0;
}


//------------ AES
int fun_aesCreate(Thread* th)
{
	LScbc* d;
	LINT key_len;
	LINT NDROP=1-1;
	LW result=NIL;

	LB* key=VALTOPNT(STACKGET(th,0));
	if (!key) goto cleanup;
	key_len = STRLEN(key);
	if ((key_len !=16)&& (key_len != 24)&& (key_len != 32)) goto cleanup;

	d=(LScbc*)memoryAllocExt(th, sizeof(LScbc),DBG_BIN,NULL,NULL); if (!d) return EXEC_OM;

	AESCreate(&d->context,STRSTART(key),(int)key_len);
	result=PNTTOVAL(d);
cleanup:
	STACKSET(th,NDROP,result);
	STACKDROPN(th,NDROP);
	return 0;
}
int fun_aesEncrypt(Thread* th)
{
	LINT index=VALTOINT(STACKGET(th,0));
	LB* src=VALTOPNT(STACKGET(th,1));
	LScbc* d=(LScbc*)VALTOPNT(STACKGET(th,2));
	
	if ((!src)||(!d)||(index<0)) goto cleanup;
	if ((index+ AES_BLOCKLEN)>STRLEN(src)) goto cleanup;
	AESEncrypt(&d->context, STRSTART(src) + index);
cleanup:
	STACKDROPN(th,2);
	return 0;
}

int fun_aesDecrypt(Thread* th)
{
	LINT index = VALTOINT(STACKGET(th, 0));
	LB* src = VALTOPNT(STACKGET(th, 1));
	LScbc* d = (LScbc*)VALTOPNT(STACKGET(th, 2));

	if ((!src) || (!d) || (index < 0)) goto cleanup;
	if ((index + AES_BLOCKLEN) > STRLEN(src)) goto cleanup;
	AESDecrypt(&d->context, STRSTART(src) + index);
cleanup:
	STACKDROPN(th, 2);
	return 0;
}

int fun_aesOutput(Thread* th)
{
	LB* p;
	LScbc* d = (LScbc*)VALTOPNT(STACKGET(th, 0));
	if (!d) return 0;

	p = memoryAllocStr(th, NULL, AES_BLOCKLEN); if (!p) return EXEC_OM;
	AESOutput(&d->context, STRSTART(p));
	STACKSET(th, 0, PNTTOVAL(p));
	return 0;
}
int fun_aesWriteBytes(Thread* th)
{
	LINT index = VALTOINT(STACKGET(th, 0));
	LB* dst = VALTOPNT(STACKGET(th, 1));
	LScbc* d = (LScbc*)VALTOPNT(STACKGET(th, 2));

	if ((!dst) || (!d) || (index < 0)) goto cleanup;
	if ((index + AES_BLOCKLEN) > STRLEN(dst)) goto cleanup;
	AESOutput(&d->context, STRSTART(dst)+index);
cleanup:
	STACKDROPN(th, 2);
	return 0;
}

//------------ DES
int fun_desCreate(Thread* th)
{
	LSdes* d;
	LINT key_len;
	LINT NDROP = 1 - 1;
	LW result = NIL;

	LB* key = VALTOPNT(STACKGET(th, 0));

	if (!key) goto cleanup;
	key_len = STRLEN(key);
	if (key_len != 8) goto cleanup;

	d = (LSdes*)memoryAllocExt(th, sizeof(LSdes), DBG_BIN, NULL, NULL); if (!d) return EXEC_OM;

	DESCreate(&d->context, STRSTART(key));
	result = PNTTOVAL(d);
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}
int fun_desEncrypt(Thread* th)
{
	LINT index = VALTOINT(STACKGET(th, 0));
	LB* src = VALTOPNT(STACKGET(th, 1));
	LSdes* d = (LSdes*)VALTOPNT(STACKGET(th, 2));

	if ((!src) || (!d) || (index < 0)) goto cleanup;
	if ((index + DES_BLOCKLEN) > STRLEN(src)) goto cleanup;
	DESProcess(&d->context, STRSTART(src) + index,1);
cleanup:
	STACKDROPN(th, 2);
	return 0;
}

int fun_desDecrypt(Thread* th)
{
	LINT index = VALTOINT(STACKGET(th, 0));
	LB* src = VALTOPNT(STACKGET(th, 1));
	LSdes* d = (LSdes*)VALTOPNT(STACKGET(th, 2));
	if ((!src) || (!d) || (index < 0)) goto cleanup;
	if ((index + DES_BLOCKLEN) > STRLEN(src)) goto cleanup;
	DESProcess(&d->context, STRSTART(src) + index,0);
cleanup:
	STACKDROPN(th, 2);
	return 0;
}

int fun_desOutput(Thread* th)
{
	LB* p;
	LSdes* d = (LSdes*)VALTOPNT(STACKGET(th, 0));
	if (!d) return 0;

	p = memoryAllocStr(th, NULL, DES_BLOCKLEN); if (!p) return EXEC_OM;
	DESOutput(&d->context, STRSTART(p));
	STACKSET(th, 0, PNTTOVAL(p));
	return 0;
}
int fun_desWriteBytes(Thread* th)
{
	LINT index = VALTOINT(STACKGET(th, 0));
	LB* dst = VALTOPNT(STACKGET(th, 1));
	LSdes* d = (LSdes*)VALTOPNT(STACKGET(th, 2));
	if ((!dst) || (!d) || (index < 0)) goto cleanup;
	if ((index + DES_BLOCKLEN) > STRLEN(dst)) goto cleanup;
	DESOutput(&d->context, STRSTART(dst) + index);
cleanup:
	STACKDROPN(th, 2);
	return 0;
}

//------------ RC4
int fun_rc4Create(Thread* th)
{
	LSrc4* d;
	LB* key = VALTOPNT(STACKGET(th, 0));
	if (!key) return 0;
	
	d = (LSrc4*)memoryAllocExt(th, sizeof(LSrc4), DBG_BIN, NULL, NULL); if (!d) return EXEC_OM;
	RC4Create(&d->context, STRSTART(key),STRLEN(key));
	STACKSET(th, 0, PNTTOVAL(d));
	return 0;
}

int fun_rc4Output(Thread* th)
{
	LB* buffer;
	LINT len;
	LINT NDROP = 4 - 1;
	LW result = NIL;

	LW wlen = STACKGET(th, 0);
	LINT start = VALTOINT(STACKGET(th, 1));
	LB* msg = VALTOPNT(STACKGET(th, 2));
	LSrc4* d = (LSrc4*)VALTOPNT(STACKGET(th, 3));
	if ((!d) || (!msg) || (start < 0)) goto cleanup;
	len = (wlen == NIL) ? STRLEN(msg) : VALTOINT(wlen);
	if ((start + len) > STRLEN(msg)) len = STRLEN(msg) - start;

	buffer = memoryAllocStr(th, NULL, len); if (!buffer) return EXEC_OM;
	RC4Process(&d->context, STRSTART(msg) + start, len, STRSTART(buffer));
	result = PNTTOVAL(buffer);
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}

int fun_rc4WriteBytes(Thread* th)
{
	LINT len,len0;
	LINT NDROP = 6 - 1;
	LW result = NIL;

	LW wlen = STACKGET(th, 0);
	LINT start = VALTOINT(STACKGET(th, 1));
	LB* msg = VALTOPNT(STACKGET(th, 2));
	LINT offset = VALTOINT(STACKGET(th, 3));
	LB* bytes = VALTOPNT(STACKGET(th, 4));
	LSrc4* d = (LSrc4*)VALTOPNT(STACKGET(th, 5));
	if ((!d) || (!bytes) || (!msg) || (start < 0)) goto cleanup;
	len0 = (wlen == NIL) ? STRLEN(msg)-start : VALTOINT(wlen);
	len = rangeAdjust(offset, STRLEN(bytes), start, STRLEN(msg), len0, NULL);
	if (len < 0) goto cleanup;
	if (len != len0) goto cleanup;	// if we do not process the required number of bytes, it will break the bitstream

	RC4Process(&d->context, STRSTART(msg) + start, len, STRSTART(bytes)+offset);
	result = STACKGET(th, 5);
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}

int coreCryptoInit(Thread* th, Pkg *system)
{
	Ref* Md5 = pkgAddType(th, system, "Md5");
	Type* fun_Md5 = typeAlloc(th,TYPECODE_FUN, NULL, 1, Md5->type);
	Type* fun_Md5_S = typeAlloc(th,TYPECODE_FUN, NULL, 2, Md5->type, MM.S);
	Type* fun_Md5_S_I_I_Md5 = typeAlloc(th,TYPECODE_FUN, NULL, 5,
		Md5->type, MM.S, MM.I, MM.I, Md5->type);

	Ref* Sha1 = pkgAddType(th, system, "Sha1");
	Type* fun_Sha1 = typeAlloc(th,TYPECODE_FUN, NULL, 1, Sha1->type);
	Type* fun_Sha1_S = typeAlloc(th,TYPECODE_FUN, NULL, 2, Sha1->type, MM.S);
	Type* fun_Sha1_S_I_I_Sha1 = typeAlloc(th,TYPECODE_FUN, NULL, 5,
		Sha1->type, MM.S, MM.I, MM.I, Sha1->type);

	Ref* Sha256=pkgAddType(th, system,"Sha256");
	Type* fun_Sha256=typeAlloc(th,TYPECODE_FUN,NULL,1,Sha256->type);
	Type* fun_Sha256_S=typeAlloc(th,TYPECODE_FUN,NULL,2,Sha256->type,MM.S);
	Type* fun_Sha256_S_I_I_Sha256=typeAlloc(th,TYPECODE_FUN,NULL,5,
		Sha256->type,MM.S,MM.I,MM.I,Sha256->type);

	Ref* Sha384=pkgAddType(th, system,"Sha384");
	Type* fun_Sha384=typeAlloc(th,TYPECODE_FUN,NULL,1,Sha384->type);
	Type* fun_Sha384_S=typeAlloc(th,TYPECODE_FUN,NULL,2,Sha384->type,MM.S);
	Type* fun_Sha384_S_I_I_Sha384=typeAlloc(th,TYPECODE_FUN,NULL,5,
		Sha384->type,MM.S,MM.I,MM.I,Sha384->type);

	Ref* Sha512=pkgAddType(th, system,"Sha512");
	Type* fun_Sha512=typeAlloc(th,TYPECODE_FUN,NULL,1,Sha512->type);
	Type* fun_Sha512_S=typeAlloc(th,TYPECODE_FUN,NULL,2,Sha512->type,MM.S);
	Type* fun_Sha512_S_I_I_Sha512=typeAlloc(th,TYPECODE_FUN,NULL,5,
		Sha512->type,MM.S,MM.I,MM.I,Sha512->type);

	Ref* Aes = pkgAddType(th, system, "Aes");
	Type* fun_S_Aes = typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.S, Aes->type);
	Type* fun_Aes_S_I_Aes = typeAlloc(th,TYPECODE_FUN, NULL, 4, Aes->type, MM.S, MM.I, Aes->type);
	Type* fun_Aes_S = typeAlloc(th,TYPECODE_FUN, NULL, 2, Aes->type, MM.S);
	Type* fun_Aes_B_I_Aes = typeAlloc(th,TYPECODE_FUN, NULL, 4, Aes->type, MM.Bytes, MM.I, Aes->type);

	Ref* Des = pkgAddType(th, system, "Des");
	Type* fun_S_Des = typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.S, Des->type);
	Type* fun_Des_S_I_Des = typeAlloc(th,TYPECODE_FUN, NULL, 4, Des->type, MM.S, MM.I, Des->type);
	Type* fun_Des_S = typeAlloc(th,TYPECODE_FUN, NULL, 2, Des->type, MM.S);
	Type* fun_Des_B_I_Des = typeAlloc(th,TYPECODE_FUN, NULL, 4, Des->type, MM.Bytes, MM.I, Des->type);

	Ref* Rc4 = pkgAddType(th, system, "Rc4");
	Type* fun_S_Rc4 = typeAlloc(th,TYPECODE_FUN, NULL, 2, MM.S, Rc4->type);
	Type* fun_Rc4_S_I_I_S = typeAlloc(th,TYPECODE_FUN, NULL, 5, Rc4->type, MM.S, MM.I, MM.I, MM.S);
	Type* fun_Rc4_B_I_S_I_I_Rc4 = typeAlloc(th,TYPECODE_FUN, NULL, 7, Rc4->type, MM.Bytes, MM.I, MM.S, MM.I, MM.I, Rc4->type);

	pkgAddFun(th, system, "md5Create",fun_md5Create, fun_Md5);
	pkgAddFun(th, system, "md5Process",fun_md5Process, fun_Md5_S_I_I_Md5);
	pkgAddFun(th, system, "md5Output",fun_md5Output, fun_Md5_S);

	pkgAddFun(th, system,"sha1Create",fun_sha1Create,fun_Sha1);
	pkgAddFun(th, system,"sha1Process",fun_sha1Process,fun_Sha1_S_I_I_Sha1);
	pkgAddFun(th, system,"sha1Output",fun_sha1Output,fun_Sha1_S);

	pkgAddFun(th, system,"sha256Create",fun_sha256Create,fun_Sha256);
	pkgAddFun(th, system,"sha256Process",fun_sha256Process,fun_Sha256_S_I_I_Sha256);
	pkgAddFun(th, system,"sha256Output",fun_sha256Output,fun_Sha256_S);

	pkgAddFun(th, system,"sha384Create",fun_sha384Create,fun_Sha384);
	pkgAddFun(th, system,"sha384Process",fun_sha384Process,fun_Sha384_S_I_I_Sha384);
	pkgAddFun(th, system,"sha384Output",fun_sha384Output,fun_Sha384_S);

	pkgAddFun(th, system,"sha512Create",fun_sha512Create,fun_Sha512);
	pkgAddFun(th, system,"sha512Process",fun_sha512Process,fun_Sha512_S_I_I_Sha512);
	pkgAddFun(th, system,"sha512Output",fun_sha512Output,fun_Sha512_S);

	pkgAddConst(th, system,"AES_BLOCK",INTTOVAL(AES_BLOCKLEN),MM.I);
	pkgAddFun(th, system,"aesCreate",fun_aesCreate,fun_S_Aes);
	pkgAddFun(th, system,"aesEncrypt",fun_aesEncrypt,fun_Aes_S_I_Aes);
	pkgAddFun(th, system,"aesDecrypt",fun_aesDecrypt,fun_Aes_S_I_Aes);
	pkgAddFun(th, system,"aesOutput",fun_aesOutput, fun_Aes_S);
	pkgAddFun(th, system, "aesWriteBytes",fun_aesWriteBytes, fun_Aes_B_I_Aes);

	pkgAddConst(th, system, "DES_BLOCK",INTTOVAL(DES_BLOCKLEN), MM.I);
	pkgAddFun(th, system, "desCreate",fun_desCreate, fun_S_Des);
	pkgAddFun(th, system, "desEncrypt",fun_desEncrypt, fun_Des_S_I_Des);
	pkgAddFun(th, system, "desDecrypt",fun_desDecrypt, fun_Des_S_I_Des);
	pkgAddFun(th, system, "desOutput",fun_desOutput, fun_Des_S);
	pkgAddFun(th, system, "desWriteBytes",fun_desWriteBytes, fun_Des_B_I_Des);

	pkgAddFun(th, system, "rc4Create",fun_rc4Create, fun_S_Rc4);
	pkgAddFun(th, system, "rc4Output",fun_rc4Output, fun_Rc4_S_I_I_S);
	pkgAddFun(th, system, "rc4WriteBytes",fun_rc4WriteBytes, fun_Rc4_B_I_S_I_I_Rc4);

	return 0;
}
