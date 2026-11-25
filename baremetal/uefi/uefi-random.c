// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"inc/efi.h"
#include"inc/efilib.h"
#include"../../src/minimacy.h"
#include"../../src/crypto_hash.h"
#include"uefi.h"

EFI_RNG_PROTOCOL *EfiRng=NULL;
SHA2_CTX RandomHash;
int RandomHasRdrand;
int RandomHasRdseed;

static void cpuid(UINT32 leaf, UINT32 sub, UINT32 r[4])
{
    __asm__ volatile("cpuid" : "=a"(r[0]), "=b"(r[1]), "=c"(r[2]), "=d"(r[3])
                             : "a"(leaf), "c"(sub));
}

static int hasRdrand(void)
{
    UINT32 r[4]; cpuid(1, 0, r);
    return (r[2] >> 30) & 1;                    // CPUID.1:ECX[30]
}

static int hasRdseed(void)
{
    UINT32 r[4]; cpuid(0, 0, r);
    if (r[0] < 7) return 0;
    cpuid(7, 0, r);
    return (r[1] >> 18) & 1;                    // CPUID.7.0:EBX[18]
}

// both instructions may fail temporary (CF=0), then retry
static int rdseed64(UINT64 *v)
{
    for (int i = 0; i < 100; i++) {
        unsigned char ok;
        __asm__ volatile("rdseed %0; setc %1" : "=r"(*v), "=qm"(ok));
        if (ok) return 0;
        __asm__ volatile("pause");
    }
    return 1;
}

static int rdrand64(UINT64 *v)
{
    for (int i = 0; i < 10; i++) {               // Intel recommands 10 tries
        unsigned char ok;
        __asm__ volatile("rdrand %0; setc %1" : "=r"(*v), "=qm"(ok));
        if (ok) return 0;
    }
    return 1;
}

void randomNext(int withRdSeed) {
	char buffer[32];
	char* p=buffer;
	*(int*)p = (int)rdtsc(); p+=4;
	*(int*)p = hwTime(); p+=4;
	if (EfiRng) {
		EFI_STATUS Status = EfiRng->GetRNG(EfiRng,NULL,8,p);
		if (Status==EFI_SUCCESS) p+=8;
	}
	if (RandomHasRdseed && withRdSeed && !rdseed64((UINT64*)p)) p+=8;
	if (RandomHasRdrand && !rdrand64((UINT64*)p)) p+=8;
	sha256Process(&RandomHash, buffer, p-buffer);
}

void hwRandomInit(void) {
	sha256Create(&RandomHash);
	randomNext(1);
}

void rngInit()
{
	EFI_STATUS Status;
	EfiRng=NULL;
	Status = BS->LocateProtocol(&gEfiRngProtocolGuid,NULL,(void**)&EfiRng);
	if (Status!=EFI_SUCCESS) EfiRng=NULL;
	RandomHasRdrand=hasRdrand();
	RandomHasRdseed=hasRdseed();
	PRINTF(LOG_SYS,"> UEFI: available RNG:");
	if (EfiRng) PRINTF(LOG_SYS," UEFI,");
	if (RandomHasRdrand) PRINTF(LOG_SYS," Rdrand,");
	if (RandomHasRdseed) PRINTF(LOG_SYS," Rdseed,");
	PRINTF(LOG_SYS, " cycles\n");
}

void hwRandomBytes(char* dst, LINT len)
{
    for (LINT i = 0; i < len; i += 24) {
		char digest[32];
		randomNext(0);
		sha256Output(&RandomHash, digest);
		memcpy(dst+i, digest, (len - i < 24) ? len - i : 24);
    }
}
int hwHasRandom()
{
	return (EfiRng||RandomHasRdrand||RandomHasRdseed)?1:0;
}

