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

int BignumCounter;
int BigCounter = 0;

#define BIGREGISTERS 32
BignumRegister BigRegisters[BIGREGISTERS];
bignum BigList=NULL;
LINT BigCount;	// number of available registers, should be BIGREGISTERS when idle


void bignumDebug(bignum a,char* label)
{
	int i;
	if (label) printf("%s", label);
	if (bignumSign(a)) printf("-");
	for (i = bignumLen(a) - 1; i >= 0; i--) printf("%08x", bignumGet(a, i));
	printf("\n");
}
LB* bignumStringAlloc(Thread* th, char* src,LINT len)
{
	NEED_FAST_ALLOC(th)
	return memoryAllocStr(th, src,len);
}

void bignumOptimize(bignum b)
{
	if (!b) return;
	while((bignumLen(b)>1)&&(!bignumGet(b,bignumLen(b)-1))) bignumLenSet(b,bignumLen(b)-1);
	if ((bignumLen(b)==1)&&(!bignumGet(b,0))) bignumSignSet(b,0);	// zero is always positive
}

bignum bignumCreate(LINT nword)
{
	bignum b;
	if (!BigList)
	{
//		PRINTF(LOG_ERR, "Out of bignum registers\n");
		return NULL;
	}
	
	if (nword<1) nword=1;
	if (nword > BIGNUM_MAXWORDS)
	{
//		PRINTF(LOG_ERR, "Bignum too long (" LSD " bits, max is " LSD ")\n",nword*sizeof(uint), BIGNUM_MAXWORDS *sizeof(uint));
		return NULL;
	}
	b = BigList;
	BigList = (bignum)BigList->header.nextBlock;
	BigCount--;
	b->len=(uint)nword;
	b->sign=0;
	memset(b->data, 0, nword * sizeof(uint));
	BignumCounter++;
	return b;
}
bignum bignumCopy(bignum a)
{
	LINT nword = bignumLen(a);
	bignum b = bignumCreate(nword); if (!b) return NULL;
	bignumSignSet(b, bignumSign(a));
	memcpy(b->data, a->data, nword * sizeof(uint));
	return b;
}

void bignumRelease(bignum b)
{
	b->header.nextBlock = (LB*)BigList;
	BigList = b;
	BigCount++;
}

bignum bignumReplace(bignum x, bignum y)
{
	bignumRelease(x);
	return y;
}

LINT bignumIsNull(bignum a)
{
    if ((bignumLen(a)==1)&&(!bignumGet(a,0))) return 1;
    return 0;
}

bignum bignumFromUInt(LINT v0)
{
	bignum b;
	LINT nb=1;
#ifdef ATOMIC_32
	b = bignumCreate(nb); if (!b) return NULL;
	bignumSet(b, 0, (uint)v0);
#else
	LINT v=v0>>32;
	if ((v)&&(v!=-1)) nb=2;
	b=bignumCreate(nb); if (!b) return NULL;
	bignumSet(b,0,(uint)v0);
	if (nb>1) bignumSet(b,1,(uint)(v0>>32));
#endif
	return b;
}
bignum bignumFromInt(LINT v0)
{
	bignum b;
	LINT w0=(v0>0)?v0:-v0;
#ifdef ATOMIC_32
	b = bignumCreate(1); if (!b) return NULL;
	bignumSet(b, 0, (uint)w0);
#else
	LINT nb=(w0>>32)?2:1;
	b=bignumCreate(nb); if (!b) return NULL;
	bignumSet(b,0,(uint)w0);
	if (nb>1) bignumSet(b,1,(uint)(w0>>32));
#endif
	if (v0<0) bignumSignSet(b,1);
	return b;
}

LINT bignumToInt(bignum b)
{
	LINT x,y;
#ifdef ATOMIC_32
#else
	if (bignumLen(b)>1)
	{
		x=bignumGet(b,1);
		x&=0x7fffffff;
		x<<=32;
		y=bignumGet(b,0);
		y&=0xffffffff;
		x+=y;
		if (bignumSign(b)) x=-x;
		return x;
	}
#endif
	x=bignumGet(b,0)&0x7fffffff;
	if (bignumSign(b)) x=-x;
	return x;
}

bignum bignumFromBin(char* src,LINT n)
{
	bignum b;
	LINT i;
	unsigned char* p;
	if (n<=0) return bignumFromUInt(0);
	p=(unsigned char*)src;
	b=bignumCreate((n+3)>>2); if (!b) return NULL;
	for(i=0;i<n;i++)
	{
		LINT ind=n-i-1;
		uint x=bignumGet(b,ind>>2);
		bignumSet(b,ind>>2,x+((p[i]&255)<<(8*(ind&3))));
	}
	bignumOptimize(b);
	return b;
}


LINT bignumStringBin(bignum b, LINT outlen, char* p)
{
	LINT i;
	LINT zlen, vlen;
	LINT len = bignumLen(b) * 4;
	uint last = bignumGet(b, bignumLen(b) - 1);
	if (bignumSign(b)) return -1;	// cannot export negative number
	if (!(last >> 8)) len -= 3;
	else if (!(last >> 16)) len -= 2;
	else if (!(last >> 24)) len -= 1;
	if (outlen <= 0) outlen = len;
	if (!p) return outlen;
	zlen = (outlen > len) ? (outlen - len) : 0;
	vlen = (outlen > len) ? len : outlen;

	for (i = 0; i < zlen; i++) p[i] = 0;
	for (i = 0; i < vlen; i++)
	{
		uint x = bignumGet(b, i >> 2);
		x >>= 8 * (i & 3);
		p[outlen - 1 - i] = x;
	}
	return len;
}

bignum bignumFromSignedBin(char* src,LINT n)
{
	bignum b;
	LINT i;
	unsigned char sign;
	unsigned char* p;
	if (n<=1) return bignumFromUInt(0);
	p=(unsigned char*)src;
	sign = *(p++); n--;
	b=bignumCreate((n+3)>>2); if (!b) return NULL;
	for(i=0;i<n;i++)
	{
		LINT ind=n-i-1;
		uint x=bignumGet(b,ind>>2);
		bignumSet(b,ind>>2,x+((p[i]&255)<<(8*(ind&3))));
	}
	if (sign) bignumSignSet(b, 1);
	bignumOptimize(b);
	return b;
}

LINT bignumStringSignedBin(bignum b, char* p)
{
	LINT i;
	LINT len = bignumLen(b) * 4;
	uint last = bignumGet(b, bignumLen(b) - 1);

	if (!(last >> 8)) len -= 3;
	else if (!(last >> 16)) len -= 2;
	else if (!(last >> 24)) len -= 1;
	if (!p) return len + 1;

	*(p++) = bignumSign(b) ? 1 : 0;
	for (i = 0; i < len; i++)
	{
		uint x = bignumGet(b, i >> 2);
		x >>= 8 * (i & 3);
		p[len - 1 - i] = x;
	}
	return len;
}

bignum bignumFromDec(char* src)
{
	bignum b;
	LINT lensrc=strlen(src);
	LINT len=(lensrc+8)/9; // il faut au plus un mot par bloc de 9 chiffres decimaux
	if (!len) len=1;
	b=bignumCreate(len); if (!b) return NULL;

	if ((*src)=='-')
	{
		bignumSignSet(b,1);
		src++;
	}
	while(((*src)>='0')&&((*src)<='9'))
	{
		LINT i;
		ulonglong cc=(*(src++))-'0';
		for(i=0;i<len;i++)
		{
			ulonglong x=bignumGet(b,i);
			cc+=x*10;
			bignumSet(b,i,cc);
			cc>>=32;
		}
	}
	bignumOptimize(b);
	return b;
}

LINT charMul4Daa(char *buf,LINT c,LINT len)
{
	LINT i,x;

	for(i=0;i<len;i++)
	{
		x=buf[i]*4+c;
		c=0;
		while(x>9)
		{
			c++;
			x-=10;
		}
		buf[i]=(char)x;
	}
	return 0;
}

LB* bignumStringDec(Thread* th,bignum b)
{
	LB* p_dst;
	char* buf;
	char* dst;
	char* result;
	LINT i,l;
	LINT len=bignumLen(b)*10;
	LB* p_buf = bignumStringAlloc(th, NULL, len); if (!p_buf) return NULL;
	p_dst = bignumStringAlloc(th, NULL, len + 2); if (!p_dst) return NULL;
	buf=STRSTART(p_buf);
	dst=STRSTART(p_dst);
	result=dst;
	if (bignumSign(b)) *(dst++)='-';
	for(i=0;i<len;i++)buf[i]=0;
	for(i=bignumLen(b)-1;i>=0;i--)
	{
		LINT j;
		uint x=bignumGet(b,i);
		for(j=28;j>=0;j-=4)
		{
			charMul4Daa(buf,0,len);
			charMul4Daa(buf,(x>>j)&15,len);
		}
	}
	l=len-1;
	while((l>0)&&(buf[l]==0)) l--;
	while(l>=0) *(dst++)=buf[l--]+48;
	*dst=0;	// we may truncate the output
	return bignumStringAlloc(th, result, -1);	// therefore we instantiate another string with the exact length
}

LINT _bignumhtoc(LINT c)
{
	if ((c>='0')&&(c<='9')) return c-'0';
	else if ((c>='A')&&(c<='F')) return c-'A'+10;
	else if ((c>='a')&&(c<='f')) return c-'a'+10;
	return 0;
}
bignum bignumFromHex(Thread* th, char* src)
{
	char* bin;
	bignum res;
	LINT len=strlen(src);
	LINT start;
	LINT k = 0;
	LINT i;
	LINT n = 0;
	LINT sign = 0;
	LB* p;;
	if ((*src)=='-')
	{
		sign=1;
		src++;
		len--;
	}
	start = len & 1;
	p = bignumStringAlloc(th, NULL, (len + 1) / 2); if (!p) return NULL;
	bin = STRSTART(p);
	for(i=start;i<len+start;i++)
	{
		LINT c=_bignumhtoc(*(src++));
		k=(k<<4)+c;
		if (i&1) bin[n++]=(char)k;
	}
	res=bignumFromBin(bin,n);
	if (sign) bignumSignSet(res,1);
	return res;
}
LINT bignumStringHex(bignum b,char* dst)
{
	LINT i;
	LINT len;
	LINT buffer_len;
	char buffer[16];
	sprintf(buffer,"%x",bignumGet(b,bignumLen(b)-1));	// we need this to count the exact number of hex digits
	buffer_len = strlen(buffer);
	len= buffer_len+(bignumLen(b)-1)*8+((bignumSign(b))?1:0);
	if (buffer_len & 1) len++;

	if (!dst) return len;
	if (bignumSign(b)) *(dst++)='-';

	if (buffer_len & 1) *(dst++) = '0';
	sprintf(dst,"%x",bignumGet(b,bignumLen(b)-1));
	dst+=strlen(dst);
	
	for(i=1;i<bignumLen(b);i++)
	{
		sprintf(dst,"%08x",bignumGet(b,bignumLen(b)-1-i));
		dst+=strlen(dst);
	}
	return len;
}

int bignumToStr(Thread* th, bignum b, char* dest)
{
	char* buf;
	LINT lenResult = 0;
	LINT i, l;
	LINT len = bignumLen(b) * 10;
	LB* p_buf = bignumStringAlloc(th, NULL, len); if (!p_buf) return 0;

	buf = STRSTART(p_buf);
	if (bignumSign(b))
	{
		lenResult = 1;
		if (dest) *(dest++) = '-';
	}
	for (i = 0; i < len; i++)buf[i] = 0;
	for (i = bignumLen(b) - 1; i >= 0; i--)
	{
		LINT j;
		uint x = bignumGet(b, i);
		for (j = 28; j >= 0; j -= 4)
		{
			charMul4Daa(buf, 0, len);
			charMul4Daa(buf, (x >> j) & 15, len);
		}
	}
	l = len - 1;
	while ((l > 0) && (buf[l] == 0)) l--;
	lenResult += l+1;
	if (dest)
	{
		while (l >= 0) *(dest++)= buf[l--] + 48;
	}
	return (int)lenResult;
}
int bignumToBuffer(Thread* th, bignum b, Buffer* buffer)
{
	int k;
	char* buf;
	LINT i, l;
	LINT len = bignumLen(b) * 10;
	LB* p_buf = bignumStringAlloc(th, NULL, len); if (!p_buf) return EXEC_OM;

	buf = STRSTART(p_buf);
	if (bignumSign(b))
	{
		if ((k = bufferAddchar(th, buffer, '-'))) return k;
	}
	for (i = 0; i < len; i++)buf[i] = 0;
	for (i = bignumLen(b) - 1; i >= 0; i--)
	{
		LINT j;
		uint x = bignumGet(b, i);
		for (j = 28; j >= 0; j -= 4)
		{
			charMul4Daa(buf, 0, len);
			charMul4Daa(buf, (x >> j) & 15, len);
		}
	}
	l = len - 1;
	while ((l > 0) && (buf[l] == 0)) l--;
	while (l >= 0) if ((k = bufferAddchar(th, buffer, buf[l--] + 48))) return k;
	return 0;
}

bignum bignumRand(LINT nbits,LW exact)
{
	bignum b;
	LINT i;
	uint ibit,x;
	LINT len=(nbits+31)>>5;
	if (len<1) len=1;
	b=bignumCreate(len); if (!b) return NULL;

	for(i=0;i<len;i++) lsRand((char*)&bignumGet(b,i),sizeof(uint));
	x=bignumGet(b,len-1);
	ibit=1<<((nbits-1)&31);
	x=(x&(ibit+ibit-1));
	if (exact==MM.trueRef) x|=ibit;
	bignumSet(b,len-1,x);
	bignumOptimize(b);
	return b;
}

LINT bignumNbits(bignum b)
{
	LINT n=bignumLen(b);
	uint x=bignumGet(b,n-1);
	n=(n-1)*32;
	while(x)
	{
		x>>=1;
		n++;
	}
	return n;
}
LINT bignumLowestBit(bignum b)
{
	LINT i,j;
	if (bignumIsNull(b)) return 0;
	for(i=0;i<bignumLen(b);i++)
	for(j=0;j<32;j++) if (bignumGet(b,i)&(1<<j)) return i*32+j;
	return 0;	// should never happen
}
bignum bignumAbs(bignum a)
{
	bignum b = bignumCopy(a); if (!b) return NULL;
	bignumSignSet(b,0);
	return b;
}
void bignumNegFix(bignum a)
{
	bignumSignSet(a,1-bignumSign(a));
}
bignum bignumNeg(bignum a)
{
	bignum b=bignumCopy(a); if (!b) return NULL;
	bignumSignSet(b,1-bignumSign(b));
	return b;
}
LINT bignumPositive(bignum a)
{
	return bignumSign(a)?0:1;
}

void bignumASR1Fix(bignum a)
{
	LINT i;
	uint cc=0;
	for(i=bignumLen(a)-1;i>=0;i--)
	{
		uint x=bignumGet(a,i);
		bignumSet(a,i,(x>>1)+(cc<<31));
		cc=x&1;
	}
	bignumOptimize(a);
}
bignum bignumASR1(bignum a)
{
	bignum b=bignumCopy(a); if (!b) return NULL;
	bignumASR1Fix(b);
	return b;
}
bignum bignumASL1(bignum a)
{
	LINT i;
	uint cc=0;
	bignum b=bignumCreate(bignumLen(a)+1); if (!b) return NULL;

	for(i=0;i<bignumLen(a);i++)
	{
		uint x=bignumGet(a,i);
		bignumSet(b,i,(x<<1)+cc);
		cc=(x>>31)&1;
	}
	bignumSet(b,i,cc);
	bignumOptimize(b);
	return b;
}

LINT bignumEquals(bignum a,bignum b)
{
	LINT i;
	if (bignumSign(a)!=bignumSign(b)) return 0;
	if (bignumLen(a)!=bignumLen(b)) return 0;
	for(i=0;i<bignumLen(a);i++) if (bignumGet(a,i)!=bignumGet(b,i)) return 0;
	return 1;
}
LINT bignumGabs(bignum a,bignum b,LINT ifequal)
{
	LINT i;
	if (bignumLen(a)<bignumLen(b)) return 0;
	if (bignumLen(a)>bignumLen(b)) return 1;
	i=bignumLen(a)-1;
	while(i>=0)
	{
		if (bignumGet(a,i)<bignumGet(b,i)) return 0;
		if (bignumGet(a,i)>bignumGet(b,i)) return 1;
		i--;
	}
	return ifequal;
}

LINT bignumGreater(bignum a,bignum b)
{
	if (bignumSign(a)^bignumSign(b)) return bignumSign(a)?0:1;
	return bignumSign(a)?bignumGabs(b,a,0):bignumGabs(a,b,0);
}
LINT bignumGreaterEqual(bignum a,bignum b)
{
	if (bignumSign(a)^bignumSign(b)) return bignumSign(a)?0:1;
	return bignumSign(a)?bignumGabs(b,a,1):bignumGabs(a,b,1);
}
LINT bignumLower(bignum b, bignum a)
{
	if (bignumSign(a) ^ bignumSign(b)) return bignumSign(a) ? 0 : 1;
	return bignumSign(a) ? bignumGabs(b, a, 0) : bignumGabs(a, b, 0);
}
LINT bignumLowerEqual(bignum b, bignum a)
{
	if (bignumSign(a) ^ bignumSign(b)) return bignumSign(a) ? 0 : 1;
	return bignumSign(a) ? bignumGabs(b, a, 1) : bignumGabs(a, b, 1);
}
LINT bignumCmp(bignum a,bignum b)
{
	if (bignumEquals(a,b)) return 0;
	if (bignumGreater(a,b)) return 1;
	return -1;
}

LINT bignumIsOne(bignum a)
{
	if ((bignumLen(a)==1)&&(bignumSign(a)==0)&&(bignumGet(a,0)==1)) return 1;
	return 0;
}
LINT bignumIsEven(bignum a)
{
	if (bignumGet(a,0)&1) return 0;
	return 1;
}
LINT bignumBit(bignum a,LINT i)
{
	if ((i<0)||(i>=bignumLen(a)*32)) return 0;
	if (bignumGet(a,i>>5)&(1<<(i&31))) return 1;
	return 0;
}
bignum bignumModPower2(bignum a,LINT n)
{
	LINT len, k;
	bignum b;
	if (n <= 0) return bignumFromInt(0);
	b= bignumCopy(a); if (!b) return NULL;
	len=(n+31)>>5;
	k=bignumLen(b)*32;


	if (n>=k) return b;
	bignumLenSet(b,len);
	if (n&31) bignumSet(b,len-1,bignumGet(b,len-1)&((1<<(n&31))-1));
	bignumOptimize(b);
	return b;
}
bignum bignumPower2(LINT n)
{
	bignum b;
	if (n < 0) return bignumFromInt(0);
	if (n == 0) return bignumFromInt(1);
	b= bignumCreate(1 + (n >> 5)); if (!b) return NULL;
	bignumSet(b,(n>>5),1<<(n&31));
	return b;
}


bignum bignumASR(bignum a,LINT n)
{
	bignum b;
	LINT lenb,i;
	LINT na=bignumNbits(a);
	LINT nb=na-n;
	LINT off=n/32;
	LINT shr=n&31;
	if (n < 0) return bignumFromUInt(0);
	if (n == 0) return bignumCopy(a);
	if (nb<=0) return bignumFromUInt(0);
	lenb=(nb+31)>>5;
	b=bignumCreate(lenb); if (!b) return NULL;

	for(i=0;i<lenb;i++)
	{
		if (shr)
		{
			if (off+i+1<bignumLen(a)) bignumSet(b,i,((bignumGet(a,off+i))>>shr)+((bignumGet(a,off+i+1))<<(32-shr)));
			else bignumSet(b,i,((bignumGet(a,off+i))>>shr));
		}
		else bignumSet(b,i,bignumGet(a,off+i));
	}
	return b;
}
bignum bignumXor(bignum a, bignum b)
{
	LINT i;
	bignum c = bignumCreate((bignumLen(a) > bignumLen(b)) ? bignumLen(a) : bignumLen(b)); if (!c) return NULL;
	for (i = 0; i < bignumLen(c); i++)
	{
		ulonglong x = (i < bignumLen(a)) ? bignumGet(a, i) : 0;
		ulonglong y = (i < bignumLen(b)) ? bignumGet(b, i) : 0;
		bignumSet(c, i, x ^ y);
	}
	bignumSignSet(c, bignumSign(a) ^ bignumSign(b));
	bignumOptimize(c);
	return c;
}

bignum bignumMul(bignum a,bignum b)
{
	LINT i,j;
	bignum c=bignumCreate(1+bignumLen(a)+bignumLen(b)); if (!c) return NULL;

	for(i=0;i<bignumLen(a);i++)
	{
		ulonglong cc=0;
		for(j=0;j<bignumLen(b);j++)
		{
			ulonglong x=bignumGet(a,i);
			ulonglong y=bignumGet(b,j);
			cc+=x*y;
			cc+=bignumGet(c,i+j);
			bignumSet(c,i+j,cc);
			cc>>=32;
		}
		cc+=bignumGet(c,i+j);
		bignumSet(c,i+j,cc);
	}
	bignumSignSet(c,bignumSign(a)^bignumSign(b));
	bignumOptimize(c);
	return c;
}
bignum bignumExp(bignum a, bignum e)
{
	bignum r;
	
	if ((bignumSign(e))||(bignumIsNull(a))) return bignumFromInt(0);

	e = bignumCopy(e); if (!e) return NULL;
	a = bignumCopy(a); if (!a) return NULL;
	r = bignumFromInt(1); if (!r) return NULL;
	while (!bignumIsNull(e))
	{
		if (!bignumIsEven(e))
		{
			r = bignumReplace(r, bignumMul(r, a));
		}
		a = bignumReplace(a, bignumMul(a, a));
		bignumASR1Fix(e);

	}
	bignumRelease(e);
	bignumRelease(a);
	return r;
}

bignum bignumAddSub(bignum a,bignum b,LINT sub)
{
	LINT i;
	ulonglong cc=0;
	bignum c=bignumCreate(1+((bignumLen(a)>bignumLen(b))?bignumLen(a):bignumLen(b))); if (!c) return NULL;

	if (!(bignumSign(a)^bignumSign(b)^sub))
	{
		for(i=0;i<bignumLen(c);i++)
		{
			ulonglong x=(i<bignumLen(a))?bignumGet(a,i):0;
			ulonglong y=(i<bignumLen(b))?bignumGet(b,i):0;
			cc+=x+y;
			bignumSet(c,i,cc);
			cc>>=32;
		}
		bignumSignSet(c,bignumSign(a));
	}
	else
	{
		if (!bignumGabs(a,b,1))
		{
			bignum z;
			bignumSignSet(c,bignumSign(b)^sub);
			z=a; a=b; b=z;
		}
		else bignumSignSet(c,bignumSign(a));
		for(i=0;i<bignumLen(b);i++)
		{
			uint x=bignumGet(a,i);
			uint y=bignumGet(b,i);
			bignumSet(c,i,x-y-cc);
			cc=(x==y)?cc:((x<y)?1:0);
		}
		for(i=bignumLen(b);i<bignumLen(a);i++)
		{
			uint x=bignumGet(a,i);
			bignumSet(c,i,x-cc);
			if (x) cc=0;
		}
	}
	bignumOptimize(c);
	return c;
}
bignum bignumAdd(bignum a,bignum b)
{
	return bignumAddSub(a,b,0);
}
bignum bignumSub(bignum a,bignum b)
{
	return bignumAddSub(a,b,1);
}

bignum bignumDivRemainder(bignum p,bignum q,bignum *r)
{
	bignum w,v,d;
	LINT n=1;
	if (bignumIsNull(q))
	{
		if (r) *r = bignumFromInt(0);
		return bignumFromInt(0);
	}
	w=bignumCopy(p); if (!w) return NULL;
	v=bignumCopy(q); if (!v) return NULL;
	if (bignumSign(w)) bignumNegFix(w);
	if (bignumSign(v)) bignumNegFix(v);
	while(bignumGreater(w,v))
    {
		v=bignumReplace(v,bignumASL1(v));
		n++;
    }
	d = bignumFromUInt(0); if (!d) return NULL;
	while(n)
    {
		d= bignumReplace(d,bignumASL1(d)); if (!d) return NULL;
		if (bignumGreaterEqual(w,v))
		{
			w= bignumReplace(w,bignumSub(w,v)); if (!w) return NULL;
			bignumSet(d,0,bignumGet(d,0)+1);
		}
		bignumASR1Fix(v);
		n--;
	}
	if (bignumSign(q)) bignumNegFix(d);
	if (bignumSign(p)) bignumNegFix(d);
	if ((bignumSign(p))&&(!bignumIsNull(w)))
	{
		bignum unit=bignumFromUInt(1); if (!unit) return NULL;
		d= bignumReplace(d,bignumSign(q)?bignumAdd(d,unit):bignumSub(d,unit)); if (!d) return NULL;
		if (r && bignumSign(q))
		{
			w= bignumReplace(w,bignumAdd(q,w)); if (!w) return NULL;
			bignumNegFix(w);
		}
		else
		{
			w = bignumReplace(w,bignumSub(q, w)); if (!w) return NULL;
		}
		bignumRelease(unit);
	}
	bignumOptimize(d);
	if (r) *r=w;
	else bignumRelease(w);
	bignumRelease(v);
	return d;
}
bignum bignumDiv(bignum p, bignum q)
{
	return bignumDivRemainder(p, q, NULL);
}

bignum bignumMod(bignum p,bignum q)
{
	bignum w,v,qs,r;
	LINT n=1;
	if ((bignumIsNull(q))||(bignumSign(q))) return bignumFromInt(0);
	if ((!bignumSign(q))&&(!bignumSign(p))&&(bignumGreater(q,p))) return bignumCopy(p);
	w=bignumCopy(p); if (!w) return NULL;
	v=bignumCopy(q); if (!v) return NULL;
	if (bignumSign(w)) bignumNegFix(w);
	qs=bignumCopy(v);  if (!qs) return NULL;
	while(bignumGreater(w,v))
    {
		v= bignumReplace(v,bignumASL1(v)); if (!v) return NULL;
		n++;
    }
	r=bignumCopy(w); if (!r) return NULL;
	while(n)
    {
		if (bignumGreaterEqual(r,v)) r= bignumReplace(r,bignumSub(r,v));
		bignumASR1Fix(v);
		n--;
	}
	if ((bignumSign(p))&&(!bignumIsNull(r))) r= bignumReplace(r,bignumSub(qs,r));
	if (!r) return NULL;
	bignumOptimize(r);
	bignumRelease(w);
	bignumRelease(v);
	bignumRelease(qs);
	return r;
}
bignum bignumAddMod(bignum a, bignum b, bignum n)
{
	bignum v = bignumAddSub(a, b, 0);
	return bignumReplace(v, bignumMod(v, n));
}
bignum bignumSubMod(bignum a, bignum b, bignum n)
{
	bignum v = bignumAddSub(a, b, 1);
	return bignumReplace(v, bignumMod(v, n));
}
bignum bignumMulMod(bignum p,bignum q, bignum n)
{
	bignum r;
	if ((bignumIsNull(n))||(bignumSign(n))) return bignumFromInt(0);
	r=bignumMul(p,q); if (!r) return NULL;
	r= bignumReplace(r,bignumMod(r,n));
	return r;
}

bignum bignumPgcd(bignum p,bignum q)
{
	bignum y;
	bignum x;
	if (bignumIsNull(p)) return bignumCopy(q);
	if (bignumIsNull(q)) return bignumCopy(p);
	x= bignumCopy(p); if (!x) return NULL;
	y=bignumCopy(q); if (!y) return NULL;
	bignumSignSet(x,0);
	bignumSignSet(y,0);
	while(!bignumIsNull(x))
	{
		bignum g=bignumMod(y,x); if (!g) return NULL;
		bignumRelease(y);
		y=x;
		x=g;
	}
	bignumRelease(x);
	return y;
}

int bignumEuclide(bignum p,bignum q,bignum *a,bignum *b,bignum *pgcd) // ap+bq=pgcd
{
	bignum a0,a1,b0,b1,x,y;
	if (bignumIsNull(p)|| bignumIsNull(q))
	{
		if (a) *a = bignumFromInt(0);
		if (b) *b = bignumFromInt(0);
		if (pgcd) *pgcd = bignumFromInt(0);
		return 0;
	}
	x=bignumCopy(q); if (!x) return EXEC_OM;
	y=bignumCopy(p); if (!y) return EXEC_OM;
    a0=a1=b0=b1=NULL;
	if (a)
	{
		a0=bignumFromUInt(1); if (!a0) return EXEC_OM;
		a1=bignumFromUInt(0); if (!a1) return EXEC_OM;
	}
	if (b)
	{
		b0=bignumFromUInt(0); if (!b0) return EXEC_OM;
		b1=bignumFromUInt(1); if (!b1) return EXEC_OM;
	}
	while(!bignumIsNull(x))
	{
		bignum z,r;
		bignum q=bignumDivRemainder(y,x,&r); if (!q) return EXEC_OM;

		if (a)
		{
			z=bignumMul(q,a1); if (!z) return EXEC_OM;
			z= bignumReplace(z,bignumSub(a0,z));  if (!z) return EXEC_OM;
			bignumRelease(a0);
			a0=a1; a1=z;
		}
		if (b)
		{
			z=bignumMul(q,b1); if (!z) return EXEC_OM;
			z= bignumReplace(z,bignumSub(b0,z)); if (!z) return EXEC_OM;
			bignumRelease(b0);
			b0=b1; b1=z;
		}
		bignumRelease(y);
		y=x;
		x=r;
		bignumRelease(q);
	}
	bignumRelease(x);
	if (a) bignumRelease(a1);
	if (b) bignumRelease(b1);
	if (a) *a=a0;
	if (b) *b=b0;
	if (pgcd) *pgcd=y;
	else bignumRelease(y);
	return 0;
}

bignum bignumInv(bignum p,bignum q)
{
	bignum r;
	if ((bignumIsNull(q))||(bignumSign(q))) return bignumFromInt(0);
	if (bignumEuclide(p,q,&r,NULL,NULL)) return NULL;	// only EXEC_OM can happen
	r = bignumReplace(r, bignumMod(r, q)); if (!r) return NULL;
	return r;
}
bignum bignumDivMod(bignum a, bignum b, bignum n)
{
	bignum v = bignumInv(b, n); if (!v) return NULL;
	return bignumReplace(v, bignumMulMod(a, v, n));
}
bignum bignumNegMod(bignum a, bignum n)
{
	bignum v = bignumNeg(a); if (!v) return NULL;
	return bignumReplace(v, bignumMod(v, n));
}

bignum bignumBarrett(bignum n)	// 2^2k / n 
{
	bignum radixk,mu;
	LINT k;
	if ((bignumIsNull(n))||(bignumSign(n))) return bignumFromInt(0);
	k=bignumNbits(n);
	radixk=bignumPower2(2*k); if (!radixk) return bignumFromInt(0);
	mu =bignumDiv(radixk,n);
	bignumRelease(radixk);
	return mu;
}

bignum bignumModBarrett(bignum x, bignum n, bignum mu)
{
	bignum r, xx;
	LINT N, sign;
	if ((bignumIsNull(n)) || (bignumSign(n))) return bignumFromInt(0);
	if ((!bignumSign(n)) && (!bignumSign(x)) && (bignumGreater(n, x))) return bignumCopy(x);

	xx = bignumCopy(x); if (!xx) return NULL;
	sign = bignumSign(x);
	if (sign) bignumNegFix(xx);

	N = bignumNbits(n);
	r=bignumASR(xx, N - 1);
	r = bignumReplace(r,bignumMul(r, mu));
	r = bignumReplace(r, bignumASR(r, N+1));
	r = bignumReplace(r, bignumMul(r, n));
	r = bignumReplace(r, bignumSub(xx, r));

	while(bignumCmp(r, n) >= 0) r = bignumReplace(r, bignumSub(r, n));
	bignumOptimize(r);
	if ((bignumSign(x)) && (!bignumIsNull(r))) r = bignumReplace(r, bignumSub(n, r));
	bignumRelease(xx);
	return r;
}

bignum bignumMulModBarrett(bignum p,bignum q, bignum n,bignum mu)
{
	bignum pp,qq,r;
	if ((bignumIsNull(n))||(bignumSign(n))) return bignumFromInt(0);
	pp=bignumMod(p,n); if (!pp) return NULL;
	qq=bignumMod(q,n); if (!qq) return NULL;
	r=bignumMul(pp,qq); if (!r) return NULL;
	r= bignumReplace(r,bignumModBarrett(r,n,mu));
	bignumRelease(pp);
	bignumRelease(qq);
	return r;
}
bignum bignumDivModBarrett(bignum a, bignum b, bignum n, bignum mu)
{
	bignum v = bignumInv(b, n); if (!v) return NULL;
	return bignumReplace(v, bignumMulModBarrett(a, v, n,mu));
}

bignum bignumExpModBarrett(bignum p,bignum e,bignum n,bignum mu)
{
	LINT i,j,len;
	uint b;
	bignum th,r;

	if ((bignumIsNull(n))||(bignumSign(n))) return bignumFromInt(0);
	th=bignumMod(p,n); if (!th) return NULL;
	r=bignumFromUInt(1); if (!r) return NULL;

	len=bignumLen(e)-1;
	for(i=0;i<=len;i++)
	{
		b=bignumGet(e,i);
		for(j=0;j<32;j++)
		{
			if (b & 1)
			{
//				r = bignumReplace(r, bignumMulMod(r, th, n));
				r = bignumReplace(r, bignumMulModBarrett(r, th, n, mu));
			}
			if (!r) return NULL;

			b>>=1;
			if ((!b)&&(i==len)) 
			{
				if (bignumSign(e)) r= bignumReplace(r,bignumInv(r,n));
				if (!r) return NULL;
				goto cleanup;
			}
//			th=bignumReplace(th,bignumMulMod(th,th,n));
			th=bignumReplace(th,bignumMulModBarrett(th,th,n,mu));
			if (!th) return NULL;
		}
	}
cleanup:
	bignumRelease(th);
	return r;
}

bignum bignumExpMod(bignum p,bignum e,bignum n)
{
	bignum mu,r;
	if (bignumSign(n)|| bignumIsNull(p)) return bignumFromInt(0);
	if (bignumIsNull(n)) return bignumFromInt(0);
	if (bignumIsNull(e)) return bignumFromInt(1);

	mu=bignumBarrett(n); if (!mu) return NULL;
	r=bignumExpModBarrett(p,e,n,mu);
	bignumRelease(mu);
	return r;
}

bignum bignumExpChinese7(bignum a,bignum e,bignum p, bignum q, bignum u, bignum v, bignum pmu, bignum qmu)
{
	bignum x,y,r;
	if (bignumSign(e)) return bignumFromInt(0);
	if (bignumIsNull(a)) return bignumFromInt(0);
	if (bignumIsNull(e)) return bignumFromInt(1);
	if ((bignumIsNull(p))||(bignumSign(p))) return bignumFromInt(0);
	if ((bignumIsNull(q))||(bignumSign(q))) return bignumFromInt(0);
	if ((bignumIsNull(pmu)) || (bignumSign(pmu))) return bignumFromInt(0);
	if ((bignumIsNull(qmu)) || (bignumSign(qmu))) return bignumFromInt(0);

	if ((!bignumSign(e)) && (bignumGreater(p, e)))
	{
		x = bignumCopy(e); if (!x) return NULL;
	}
	else
	{	
		x=bignumDivRemainder(e,p,&r); if (!x) return NULL;
		x= bignumReplace(x,bignumAdd(x,r)); if (!x) return NULL;
		bignumRelease(r);
	}
	x= bignumReplace(x,bignumExpModBarrett(a,x,p,pmu)); if (!x) return NULL;

	if ((!bignumSign(e)) && (!bignumSign(q)) && (bignumGreater(q, e)))
	{
		y = bignumCopy(e); if (!y) return NULL;
	}
	else
	{
		y=bignumDivRemainder(e,q,&r); if (!y) return NULL;
		y= bignumReplace(y,bignumAdd(y,r)); if (!y) return NULL;
		bignumRelease(r);
	}
	y= bignumReplace(y,bignumExpModBarrett(a,y,q,qmu)); if (!y) return NULL;

	x= bignumReplace(x,bignumSub(x,y)); if (!x) return NULL;
	x= bignumReplace(x, bignumModBarrett(x,p,pmu)); if (!x) return NULL;
	x= bignumReplace(x, bignumMulModBarrett(x,v,p,pmu)); if (!x) return NULL;
	x= bignumReplace(x, bignumMul(x,q)); if (!x) return NULL;
	x= bignumReplace(x, bignumAdd(x,y)); if (!x) return NULL;
	bignumRelease(y);
	return x;
}

bignum bignumExpChinese(bignum a,bignum e,bignum p, bignum q)
{
	bignum u,v,pmu,qmu,r;
	if (bignumSign(e)) return bignumFromInt(0);
	if (bignumIsNull(a)) return bignumFromInt(0);
	if (bignumIsNull(e)) return bignumFromInt(1);
	if ((bignumIsNull(p))||(bignumSign(p))) return bignumFromInt(0);
	if ((bignumIsNull(q))||(bignumSign(q))) return bignumFromInt(0);

	pmu=bignumBarrett(p); if (!pmu) return NULL;
	qmu=bignumBarrett(q); if (!qmu) return NULL;
	if (bignumEuclide(p, q, &u, &v, NULL)) return NULL;	// only EXEC_OM may happen
	r=bignumExpChinese7(a,e,p,q,u,v,pmu,qmu);

	bignumRelease(u);
	bignumRelease(v);
	bignumRelease(pmu);
	bignumRelease(qmu);

	return r;
}
bignum bignumExpChinese5(bignum c, bignum p1, bignum p2, bignum e1, bignum e2, bignum coef)
{
	bignum a, b;
	a = bignumExpMod(c, e1, p1);
	b = bignumExpMod(c, e2, p2);
	a = bignumReplace(a, bignumSub(a, b));
	a = bignumReplace(a, bignumMod(a, p1));
	a = bignumReplace(a, bignumMulMod(coef, a, p1));
	a = bignumReplace(a, bignumMul(a, p2));
	a = bignumReplace(a, bignumAdd(b, a));
	bignumRelease(b);
	return a;
}

//----------------------------------------
LB* bigAlloc(Thread* th, bignum b0)
{
	LINT len;
	LB* b;
	if (!b0) return NULL;
	len = sizeof(Bignum) - sizeof(LB) + sizeof(uint) * (((LINT)b0->len) - 1);
	b = memoryAllocBin(th, (char*)&b0->len, len, DBG_B); if (!b) return NULL;
	bignumRelease(b0);
	return b;
}
int bigPush(Thread* th, bignum b0)
{
	LINT len;
	LB* b;
	if (!b0) {
		STACKPUSH(th, NIL);
		return 0;
	}
	len = sizeof(Bignum) - sizeof(LB) + sizeof(uint) * (((LINT)b0->len) - 1);

	NEED_FAST_ALLOC(th)	// maybe not
	b = memoryAllocBin(th, (char*)&b0->len, len, DBG_B); if (!b) return EXEC_OM;
	BigCounter++;
	STACKPUSH_OM(th, PNTTOVAL(b), EXEC_OM);
	bignumRelease(b0);
	return 0;
}

int bigFromBin(Thread* th)
{
	LINT NDROP=1-1;
	LW result=NIL;

	LB* p=VALTOPNT(STACKGET(th,0));
	if (!p) goto cleanup;

	if (bigPush(th,bignumFromBin(STRSTART(p),STRLEN(p)))) return EXEC_OM;
	NDROP++;
	result = STACKGET(th, 0);
cleanup:
	STACKSET(th,NDROP,result);
	STACKDROPN(th,NDROP);
	return 0;
}
int bigToBin(Thread* th)
{
	LB* p;
	LINT size;
	LINT NDROP=2-1;
	LW result=NIL;

	LINT len=VALTOINT(STACKGET(th,0));
	bignum b=_bignumGet(STACKGET(th,1));
	if (!b) goto cleanup;

	size=bignumStringBin(b,len,NULL);
	if (size<0) goto cleanup;
	p = bignumStringAlloc(th, NULL, size); if (!p) return EXEC_OM;
	result=PNTTOVAL(p);
	bignumStringBin(b,len, STRSTART(p));
cleanup:
	STACKSET(th,NDROP,result);
	STACKDROPN(th,NDROP);
	return 0;
}
int bigFromSignedBin(Thread* th)
{
	LINT NDROP = 1 - 1;
	LW result = NIL;

	LB* p = VALTOPNT(STACKGET(th, 0));
	if (!p) goto cleanup;

	if (bigPush(th, bignumFromSignedBin(STRSTART(p), STRLEN(p)))) return EXEC_OM;
	NDROP++;
	result = STACKGET(th, 0);
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}
int bigToSignedBin(Thread* th)
{
	LB* p;
	LINT size;
	LINT NDROP = 1 - 1;
	LW result = NIL;

	bignum b = _bignumGet(STACKGET(th, 0));
	if (!b) goto cleanup;

	size = bignumStringSignedBin(b, NULL);
	if (size < 0) goto cleanup;
	p = bignumStringAlloc(th, NULL, size); if (!p) return EXEC_OM;
	result = PNTTOVAL(p);
	bignumStringSignedBin(b, STRSTART(p));
cleanup:
	STACKSET(th, NDROP, result);
	STACKDROPN(th, NDROP);
	return 0;
}
int bigFromDec(Thread* th)
{
	LINT NDROP=1-1;
	LW result=NIL;

	LB* p=VALTOPNT(STACKGET(th,0));

	if (!p) goto cleanup;

	if (bigPush(th,bignumFromDec(STRSTART(p)))) return EXEC_OM;
	NDROP++;
	result = STACKGET(th, 0);

cleanup:
	STACKSET(th,NDROP,result);
	STACKDROPN(th,NDROP);

	return 0;
}

int bigToDec(Thread* th)
{
	LB* p;
	LINT NDROP=1-1;
	LW result=NIL;

	bignum b=_bignumGet(STACKGET(th,0));
	if (!b) goto cleanup;

	p = bignumStringDec(th, b);	if (!p) return EXEC_OM;
	result=PNTTOVAL(p);
cleanup:
	STACKSET(th,NDROP,result);
	STACKDROPN(th,NDROP);

	return 0;
}
int bigFromHex(Thread* th)
{
	bignum b;
	LINT NDROP=1-1;
	LW result=NIL;

	LB* p=VALTOPNT(STACKGET(th,0));
	if (!p) goto cleanup;
	b = bignumFromHex(th, STRSTART(p)); if (!b) return EXEC_OM;
	if (bigPush(th,b)) return EXEC_OM;
	NDROP++;
	result = STACKGET(th, 0);

cleanup:
	STACKSET(th,NDROP,result);
	STACKDROPN(th,NDROP);
	return 0;
}
int bigToHex(Thread* th)
{
	LB* p;
	LINT size;
	LINT NDROP=1-1;
	LW result=NIL;

	bignum b=_bignumGet(STACKGET(th,0));
	if (!b) goto cleanup;
	size=bignumStringHex(b,NULL);
	if (size<0) goto cleanup;
	p = bignumStringAlloc(th, NULL, size); if (!p) return EXEC_OM;
	result=PNTTOVAL(p);
	bignumStringHex(b, STRSTART(p));
cleanup:
	STACKSET(th,NDROP,result);
	STACKDROPN(th,NDROP);
	return 0;
}

int bigDivRemainder(Thread* th)
{
	bignum q,r;
	LINT NDROP=2-1;
	LW result=NIL;
	bignum b=_bignumGet(STACKGET(th,0));
	bignum a=_bignumGet(STACKGET(th,1));

	if ((!a)||(!b)) goto cleanup;

	q = bignumDivRemainder(a, b, &r); if (!q) return EXEC_OM;
	if (!q) goto cleanup;
	if (bigPush(th,q)) return EXEC_OM;
	if (bigPush(th,r)) return EXEC_OM;
	if (DEFTAB(th,2,DBG_TUPLE)) return EXEC_OM;
	NDROP++;
	result = STACKGET(th, 0);
cleanup:
	STACKSET(th,NDROP,result);
	STACKDROPN(th,NDROP);

	return 0;
}
int bigEuclid(Thread* th)
{
	int k;
	bignum u,v,pgcd;
	LINT NDROP=2-1;
	LW result=NIL;
	bignum b=_bignumGet(STACKGET(th,0));
	bignum a=_bignumGet(STACKGET(th,1));
	if ((!a)||(!b)) goto cleanup;

	k = bignumEuclide(a, b, &u, &v, &pgcd);
	if (k == EXEC_OM) return k;
	if (k) goto cleanup;
	if (bigPush(th,u)) return EXEC_OM;
	if (bigPush(th,v)) return EXEC_OM;
	if (bigPush(th,pgcd)) return EXEC_OM;
	if (DEFTAB(th,3,DBG_TUPLE)) return EXEC_OM;
	NDROP++;
	result = STACKGET(th, 0);
cleanup:
	STACKSET(th,NDROP,result);
	STACKDROPN(th,NDROP);

	return 0;
}

	bigOpeI_B(bigFromInt,bignumFromInt)
	bigOpeI_B(bigPower2,bignumPower2)

	bigOpeIBool_B(bigRand,bignumRand)

	bigOpeB_B(bigASR1,bignumASR1)
	bigOpeB_B(bigASL1,bignumASL1)
	bigOpeB_B(bigAbs,bignumAbs)
	bigOpeB_B(bigNeg,bignumNeg)
	bigOpeB_B(bigBarrett,bignumBarrett)

	bigOpeB_I(bigToInt,bignumToInt)
	bigOpeB_I(bigNbits,bignumNbits)
	bigOpeB_I(bigLowestBit,bignumLowestBit)
	bigOpeB_BOOL(bigPositive,bignumPositive)
	bigOpeB_BOOL(bigIsNull,bignumIsNull)
	bigOpeB_BOOL(bigIsOne,bignumIsOne)
	bigOpeB_BOOL(bigIsEven,bignumIsEven)

	bigOpeBI_B(bigModPower2,bignumModPower2)
	bigOpeBI_B(bigASR,bignumASR)

	bigOpeBI_I(bigBit,bignumBit)

	bigOpeBB_B(bigXor, bignumXor)
	bigOpeBB_B(bigAdd,bignumAdd)
	bigOpeBB_B(bigSub,bignumSub)
	bigOpeBB_B(bigMul,bignumMul)
	bigOpeBB_B(bigExp,bignumExp)
	bigOpeBB_B(bigDiv,bignumDiv)
	bigOpeBB_B(bigMod,bignumMod)
	bigOpeBB_B(bigNegMod,bignumNegMod)
	bigOpeBB_B(bigInv,bignumInv)
	bigOpeBB_B(bigGcd,bignumPgcd)

	bigOpeBB_I(bigCmp,bignumCmp)
	bigOpeBB_BOOL(bigEquals, bignumEquals)
	bigOpeBB_BOOL(bigGreater,bignumGreater)
	bigOpeBB_BOOL(bigGreaterEquals, bignumGreaterEqual)
	bigOpeBB_BOOL(bigLower, bignumLower)
	bigOpeBB_BOOL(bigLowerEquals, bignumLowerEqual)

	bigOpeBBB_B(bigAddMod, bignumAddMod)
	bigOpeBBB_B(bigSubMod, bignumSubMod)
	bigOpeBBB_B(bigDivMod, bignumDivMod)
	bigOpeBBB_B(bigMulMod,bignumMulMod)
	bigOpeBBB_B(bigExpMod,bignumExpMod)
	bigOpeBBB_B(bigModBarrett,bignumModBarrett)

	bigOpeBBBB_B(bigMulModBarrett,bignumMulModBarrett)
	bigOpeBBBB_B(bigDivModBarrett,bignumDivModBarrett)
	bigOpeBBBB_B(bigExpModBarrett,bignumExpModBarrett)
	bigOpeBBBB_B(bigExpChinese,bignumExpChinese)
	bigOpeBBBBBB_B(bigExpChinese5, bignumExpChinese5)

	bigOpeBBBBBBBB_B(bigExpChinese7,bignumExpChinese7)


void coreBignumReset(void)
{
	int i;
	if (BigCount>= BIGREGISTERS) return;
//	PRINTF(LOG_ERR, "Reset bignum registers\n");
	BigList = NULL;
	for (i = 0; i < BIGREGISTERS; i++)
	{
		BigRegisters[i].header.nextBlock = (LB*)BigList;
		BigList = (bignum)&BigRegisters[i];
	}
	BigCount = BIGREGISTERS;
}
int coreBignumInit(Thread* th, Pkg *system)
{
	Type* B = MM.BigNum;
	Type* fun_B_B=typeAlloc(th, TYPECODE_FUN,NULL,2,B,B);
	Type* fun_B_B__B_B=typeAlloc(th, TYPECODE_FUN,NULL,3,B,B,typeAlloc(th, TYPECODE_TUPLE,NULL,2,B,B));
	Type* fun_B_B__B_B_B=typeAlloc(th, TYPECODE_FUN,NULL,3,B,B,typeAlloc(th, TYPECODE_TUPLE,NULL,3,B,B,B));
	Type* fun_B_B_B=typeAlloc(th, TYPECODE_FUN,NULL,3,B,B,B);
	Type* fun_B_B_B_B=typeAlloc(th, TYPECODE_FUN,NULL,4,B,B,B,B);
	Type* fun_B_B_B_B_B=typeAlloc(th, TYPECODE_FUN,NULL,5,B,B,B,B,B);
	Type* fun_B_B_B_B_B_B_B=typeAlloc(th, TYPECODE_FUN,NULL,7,B,B,B,B,B,B,B);
	Type* fun_B_B_B_B_B_B_B_B_B = typeAlloc(th, TYPECODE_FUN, NULL, 9, B, B, B, B, B, B, B, B, B);
	Type* fun_B_B_I=typeAlloc(th, TYPECODE_FUN,NULL,3,B,B,MM.I);
	Type* fun_B_B_Bool = typeAlloc(th, TYPECODE_FUN, NULL, 3, B, B, MM.Boolean);
	Type* fun_B_I=typeAlloc(th, TYPECODE_FUN,NULL,2,B,MM.I);
	Type* fun_B_Bool = typeAlloc(th, TYPECODE_FUN, NULL, 2, B, MM.Boolean);
	Type* fun_B_I_B=typeAlloc(th, TYPECODE_FUN,NULL,3,B,MM.I,B);
	Type* fun_B_I_I=typeAlloc(th, TYPECODE_FUN,NULL,3,B,MM.I,MM.I);
	Type* fun_B_I_S=typeAlloc(th, TYPECODE_FUN,NULL,3,B,MM.I,MM.S);
	Type* fun_B_S=typeAlloc(th, TYPECODE_FUN,NULL,2,B,MM.S);
	Type* fun_I_B=typeAlloc(th, TYPECODE_FUN,NULL,2,MM.I,B);
	Type* fun_I_I_B=typeAlloc(th, TYPECODE_FUN,NULL,3,MM.I,MM.I,B);
	Type* fun_I_Bool_B=typeAlloc(th, TYPECODE_FUN,NULL,3,MM.I,MM.Boolean,B);
	Type* fun_S_B=typeAlloc(th, TYPECODE_FUN,NULL,2,MM.S,B);

	pkgAddFun(th, system,"bigAbs",bigAbs,fun_B_B);
	pkgAddFun(th, system,"bigAdd",bigAdd,fun_B_B_B); MM.bigAdd = system->first;
	pkgAddFun(th, system,"bigXor",bigXor,fun_B_B_B);
	pkgAddFun(th, system,"bigBarrett",bigBarrett,fun_B_B);
	pkgAddFun(th, system,"bigBit",bigBit,fun_B_I_I);
	pkgAddFun(th, system,"bigCmp",bigCmp,fun_B_B_I);
	pkgAddFun(th, system,"bigEquals", bigEquals, fun_B_B_Bool);
	pkgAddFun(th, system, "bigGreater", bigGreater, fun_B_B_Bool); MM.bigGT = system->first;
	pkgAddFun(th, system, "bigGreaterEquals", bigGreaterEquals, fun_B_B_Bool); MM.bigGE = system->first;
	pkgAddFun(th, system, "bigLower", bigLower, fun_B_B_Bool); MM.bigLT = system->first;
	pkgAddFun(th, system, "bigLowerEquals", bigLowerEquals, fun_B_B_Bool); MM.bigLE = system->first;

	pkgAddFun(th, system,"bigDivRemainder", bigDivRemainder,fun_B_B__B_B);
	pkgAddFun(th, system,"bigASR",bigASR,fun_B_I_B);
	pkgAddFun(th, system,"bigEuclid",bigEuclid,fun_B_B__B_B_B);
	pkgAddFun(th, system,"bigExpChinese",bigExpChinese,fun_B_B_B_B_B);
	pkgAddFun(th, system,"bigExpChinese7",bigExpChinese7,fun_B_B_B_B_B_B_B_B_B);
	pkgAddFun(th, system,"bigExpChinese5", bigExpChinese5, fun_B_B_B_B_B_B_B);

	pkgAddFun(th, system,"bigExpMod",bigExpMod,fun_B_B_B_B); MM.bigExpMod = system->first;
	pkgAddFun(th, system,"bigExpModBarrett",bigExpModBarrett,fun_B_B_B_B_B); MM.bigExpModBarrett = system->first;
	pkgAddFun(th, system,"bigFromBin",bigFromBin,fun_S_B);
	pkgAddFun(th, system,"bigFromSignedBin",bigFromSignedBin,fun_S_B);
	pkgAddFun(th, system,"bigFromDec",bigFromDec,fun_S_B);
	pkgAddFun(th, system,"bigFromHex",bigFromHex,fun_S_B);

	pkgAddFun(th, system,"bigFromInt",bigFromInt,fun_I_B);
	pkgAddFun(th, system,"bigInv",bigInv,fun_B_B_B);
	pkgAddFun(th, system,"bigIsEven",bigIsEven, fun_B_Bool);
	pkgAddFun(th, system,"bigIsNull",bigIsNull, fun_B_Bool);
	pkgAddFun(th, system,"bigIsOne",bigIsOne, fun_B_Bool);
	pkgAddFun(th, system,"bigLowestBit",bigLowestBit,fun_B_I);
	pkgAddFun(th, system,"bigNegMod", bigNegMod,fun_B_B_B); MM.bigNegMod = system->first;
	pkgAddFun(th, system,"bigMod",bigMod,fun_B_B_B); MM.bigMod = system->first;
	pkgAddFun(th, system,"bigModBarrett",bigModBarrett,fun_B_B_B_B); MM.bigModBarrett = system->first;
	pkgAddFun(th, system,"bigModPower2",bigModPower2,fun_B_I_B);
	pkgAddFun(th, system,"bigMul",bigMul,fun_B_B_B); MM.bigMul = system->first;
	pkgAddFun(th, system,"bigExp", bigExp,fun_B_B_B); MM.bigExp = system->first;
	pkgAddFun(th, system,"bigDiv", bigDiv, fun_B_B_B); MM.bigDiv = system->first;
	pkgAddFun(th, system,"bigAddMod", bigAddMod,fun_B_B_B_B); MM.bigAddMod = system->first;
	pkgAddFun(th, system,"bigSubMod", bigSubMod,fun_B_B_B_B); MM.bigSubMod = system->first;
	pkgAddFun(th, system,"bigDivMod", bigDivMod,fun_B_B_B_B); MM.bigDivMod = system->first;
	pkgAddFun(th, system,"bigMulMod",bigMulMod,fun_B_B_B_B); MM.bigMulMod = system->first;

	pkgAddFun(th, system,"bigMulModBarrett",bigMulModBarrett,fun_B_B_B_B_B); MM.bigMulModBarrett = system->first;
	pkgAddFun(th, system,"bigDivModBarrett", bigDivModBarrett,fun_B_B_B_B_B); MM.bigDivModBarrett = system->first;
	pkgAddFun(th, system,"bigNbits",bigNbits,fun_B_I);
	pkgAddFun(th, system,"bigNeg",bigNeg,fun_B_B); MM.bigNeg = system->first;
	pkgAddFun(th, system,"bigGcd",bigGcd,fun_B_B_B);
	pkgAddFun(th, system,"bigPositive",bigPositive,fun_B_Bool);
	pkgAddFun(th, system,"bigPower2",bigPower2,fun_I_B);
	pkgAddFun(th, system,"bigRand",bigRand,fun_I_Bool_B);
	pkgAddFun(th, system,"bigASL1",bigASL1,fun_B_B);
	pkgAddFun(th, system,"bigASR1",bigASR1,fun_B_B);
	pkgAddFun(th, system,"bigSub",bigSub,fun_B_B_B); MM.bigSub = system->first;
	pkgAddFun(th, system,"bigToBin",bigToBin,fun_B_I_S);
	pkgAddFun(th, system,"bigToSignedBin", bigToSignedBin,fun_B_S);
	pkgAddFun(th, system,"bigToDec",bigToDec,fun_B_S);
	pkgAddFun(th, system,"bigToHex",bigToHex,fun_B_S);
	pkgAddFun(th, system,"bigToInt",bigToInt,fun_B_I);
	BigCount = 0;
	coreBignumReset();
	return 0;
}
