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
int hwHasRandom()
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

