// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

#ifdef USE_RANDOM_C
void hwRandomInit(void)
{
}
void hwRandomBytes(char* dst, LINT len)
{
	pseudoRandomBytes(dst, len);
}
int hwHasRandom()
{
	return 0;
}
#endif

#ifdef USE_RANDOM_UNIX
FILE* fCrypt = 0;	// this should be deallocated before closing the application
void hwRandomInit(void)
{
	if (!fCrypt) fCrypt = fopen("/dev/urandom", "r");
	if (!fCrypt) pseudoRandomInit();
}

void hwRandomBytes(char* dst, LINT len)
{
	if (fCrypt) {
		len-=fread(dst, len, 1, fCrypt);
		if (len==0) return;
	}
	pseudoRandomBytes(dst, len);
}
int hwHasRandom(void)
{
	return fCrypt?1:0;
}
#endif
#ifdef USE_RANDOM_WIN
#include<Wincrypt.h>
HCRYPTPROV hCryptProv = (HCRYPTPROV)NULL;

void hwRandomInit(void)
{
	if (!hCryptProv) CryptAcquireContext(&hCryptProv, NULL, NULL, PROV_RSA_FULL, 0);
	if (!hCryptProv) pseudoRandomInit();
}
void hwRandomBytes(char* dst, LINT len)
{
	if (hCryptProv) {
		if (CryptGenRandom(hCryptProv, (DWORD)len, (BYTE*)dst)) return;
	}
	pseudoRandomBytes(dst, len);
}
int hwHasRandom()
{
	return hCryptProv?1:0;
}
#endif

