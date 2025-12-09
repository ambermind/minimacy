// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

#define BIGREGISTERS 18
BignumRegister BigRegisters[BIGREGISTERS];
bignum BigList=NULL;
LINT BigCount;	// number of available registers, should be BIGREGISTERS when idle


void bignumDebug(bignum a,char* label)
{
	int i;
	if (label) PRINTF(LOG_DEV,"%s", label);
	if (bignumSign(a)) PRINTF(LOG_DEV,"-");
	for (i = bignumLen(a) - 1; i >= 0; i--) PRINTF(LOG_DEV,"%08x", bignumGet(a, i));
	PRINTF(LOG_DEV,"\n");
}
LB* bignumStringAlloc(char* src,LINT len)
{
	return memoryAllocStr(src,len);
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
		PRINTF(LOG_SYS, "> Error: Out of bignum registers\n");
		return NULL;
	}
	
	if (nword<1) nword=1;
	if (nword > BIGNUM_MAXWORDS)
	{
		PRINTF(LOG_SYS, "> Error: Bignum too long (" LSD " bits, max is " LSD ")\n",nword*sizeof(uint)*8, BIGNUM_MAXWORDS *sizeof(uint)*8);
		return NULL;
	}
	b = BigList;
	BigList = (bignum)BigList->header.nextBlock;
	BigCount--;
	b->len=(uint)nword;
	b->sign=0;
	memset((void*)b->data, 0, nword * sizeof(uint));
	return b;
}
bignum bignumCopy(bignum a)
{
	LINT nword = bignumLen(a);
	bignum b = bignumCreate(nword); if (!b) return NULL;
	bignumSignSet(b, bignumSign(a));
	memcpy((void*)b->data, (void*)a->data, nword * sizeof(uint));
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
	LINT x;
#ifdef ATOMIC_32
#else
	if (bignumLen(b)>1)
	{
		LINT y;
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

LINT _bignumhtoc(LINT c)
{
	if ((c>='0')&&(c<='9')) return c-'0';
	else if ((c>='A')&&(c<='F')) return c-'A'+10;
	else if ((c>='a')&&(c<='f')) return c-'a'+10;
	return 0;
}
bignum bignumFromHex(char* src)
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
	p = bignumStringAlloc(NULL, (len + 1) / 2); if (!p) return NULL;
	bin = STR_START(p);
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
	snprintf(buffer,16,"%x",bignumGet(b,bignumLen(b)-1));	// we need this to count the exact number of hex digits
	buffer_len = strlen(buffer);
	len= buffer_len+(bignumLen(b)-1)*8+((bignumSign(b))?1:0);
	if (buffer_len & 1) len++;

	if (!dst) return len;
	if (bignumSign(b)) *(dst++)='-';

	if (buffer_len & 1) *(dst++) = '0';
	snprintf(dst,16,"%x",bignumGet(b,bignumLen(b)-1));
	dst+=strlen(dst);
	
	for(i=1;i<bignumLen(b);i++)
	{
		snprintf(dst,16,"%08x",bignumGet(b,bignumLen(b)-1-i));
		dst+=strlen(dst);
	}
	return len;
}

int bignumDecToBuffer(bignum b, Buffer* buffer)
{
	int k;
	char* buf;
	LINT i, l;
	LINT len = bignumLen(b) * 10;
	LB* p_buf = bignumStringAlloc(NULL, len); if (!p_buf) return EXEC_OM;
	TMP_PUSH(p_buf, EXEC_OM);
	buf = STR_START(p_buf);
	if (bignumSign(b))
	{
		if ((k = bufferAddChar(buffer, '-'))) return k;
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
	while (l >= 0) if ((k = bufferAddChar(buffer, buf[l--] + 48))) return k;
	TMP_PULL();
	return 0;
}

bignum bignumRand(LINT nbits,LB* exact)
{
	bignum b;
	LINT i;
	uint ibit,x;
	LINT len=(nbits+31)>>5;
	if (len<1) len=1;
	b=bignumCreate(len); if (!b) return NULL;

	for(i=0;i<len;i++) hwRandomBytes((char*)&bignumGet(b,i),sizeof(uint));
	x=bignumGet(b,len-1);
	ibit=1<<((nbits-1)&31);
	x=(x&(ibit+ibit-1));
	if (exact==MM._true) x|=ibit;
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
	b= bignumCreate(1 + (n >> 5)); if (!b) return NULL;	// n=4096 => bignumCreate(257)
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
LB* bigAlloc(bignum b0)
{
	LINT len;
	LB* b;
	if (!b0) return NULL;
	len = sizeof(Bignum) - sizeof(LB) + sizeof(uint) * (((LINT)b0->len) - 1);
	b = memoryAllocBin((char*)&b0->len, len, DBG_B); if (!b) return NULL;
	bignumRelease(b0);
	return b;
}
int bigPush(Thread* th, bignum b0)
{
	LINT len;
	LB* b;
	if (!b0) {
		FUN_PUSH_NIL;
		return 0;
	}
	len = sizeof(Bignum) - sizeof(LB) + sizeof(uint) * (((LINT)b0->len) - 1);

	b = memoryAllocBin((char*)&b0->len, len, DBG_B); if (!b) return EXEC_OM;
	FUN_PUSH_PNT(b);
	bignumRelease(b0);
	return 0;
}

int fun_bigFromStr(Thread* th)
{
	LB* p=STACK_PNT(th,0);
	if (!p) FUN_RETURN_NIL;
	FUN_RETURN_BIG(bignumFromBin(STR_START(p),STR_LENGTH(p)));
}
int fun_strFromBig(Thread* th)
{
	LB* p;
	LINT size;

	LINT len=STACK_INT(th,0);
	bignum b=_bignumGet(th,1);
	if (!b) FUN_RETURN_NIL;

	size=bignumStringBin(b,len,NULL);
	if (size<0) FUN_RETURN_NIL;
	p = bignumStringAlloc(NULL, size); if (!p) return EXEC_OM;
	bignumStringBin(b,len, STR_START(p));
	FUN_RETURN_PNT(p);
}
int fun_bigFromSignedStr(Thread* th)
{
	LB* p = STACK_PNT(th, 0);
	if (!p) FUN_RETURN_NIL;
	FUN_RETURN_BIG(bignumFromSignedBin(STR_START(p), STR_LENGTH(p)));
}
int fun_signedStrFromBig(Thread* th)
{
	LB* p;
	LINT size;

	bignum b = _bignumGet(th, 0);
	if (!b) FUN_RETURN_NIL;

	size = bignumStringSignedBin(b, NULL);
	if (size < 0) FUN_RETURN_NIL;
	p = bignumStringAlloc(NULL, size); if (!p) return EXEC_OM;
	bignumStringSignedBin(b, STR_START(p));
	FUN_RETURN_PNT(p);
}
int fun_bigFromDec(Thread* th)
{
	LB* p=STACK_PNT(th,0);
	if (!p) FUN_RETURN_NIL;
	FUN_RETURN_BIG(bignumFromDec(STR_START(p)));
}

int fun_decFromBig(Thread* th)
{
	int k;

	bignum b=_bignumGet(th,0);
	if (!b) FUN_RETURN_NIL;
	bufferReinit(MM.tmpBuffer);
	if ((k = bignumDecToBuffer((bignum)b, MM.tmpBuffer))) return k;
	FUN_RETURN_BUFFER(MM.tmpBuffer);
}

int fun_bigFromHex(Thread* th)
{
	LB* p=STACK_PNT(th,0);
	if (!p) FUN_RETURN_NIL;
	FUN_RETURN_BIG(bignumFromHex(STR_START(p)));
}
int fun_hexFromBig(Thread* th)
{
	LB* p;
	LINT size;

	bignum b=_bignumGet(th,0);
	if (!b) FUN_RETURN_NIL;
	size=bignumStringHex(b,NULL);
	if (size<0) FUN_RETURN_NIL;
	p = bignumStringAlloc(NULL, size); if (!p) return EXEC_OM;
	bignumStringHex(b, STR_START(p));
	FUN_RETURN_PNT(p);
}

int fun_bigDivRemainder(Thread* th)
{
	bignum q,r;

	bignum b=_bignumGet(th,0);
	bignum a=_bignumGet(th,1);
	if ((!a)||(!b)) FUN_RETURN_NIL;
	q = bignumDivRemainder(a, b, &r); if (!q) return EXEC_OM;
	if (!q) FUN_RETURN_NIL;
	if (bigPush(th,q)) return EXEC_OM;
	if (bigPush(th,r)) return EXEC_OM;
	FUN_MAKE_ARRAY(2,DBG_TUPLE);
	return 0;
}
int fun_bigEuclid(Thread* th)
{
	int k;
	bignum u,v,pgcd;

	bignum b=_bignumGet(th,0);
	bignum a=_bignumGet(th,1);
	if ((!a)||(!b)) FUN_RETURN_NIL;
	k = bignumEuclide(a, b, &u, &v, &pgcd);
	if (k == EXEC_OM) return k;
	if (k) FUN_RETURN_NIL;
	if (bigPush(th,u)) return EXEC_OM;
	if (bigPush(th,v)) return EXEC_OM;
	if (bigPush(th,pgcd)) return EXEC_OM;
	FUN_MAKE_ARRAY(3,DBG_TUPLE);
	return 0;
}

	bigOpeI_B(fun_bigFromInt,bignumFromInt)
	bigOpeI_B(fun_bigPower2,bignumPower2)

	bigOpeIBool_B(fun_bigRand,bignumRand)

	bigOpeB_B(fun_bigASR1,bignumASR1)
	bigOpeB_B(fun_bigASL1,bignumASL1)
	bigOpeB_B(fun_bigAbs,bignumAbs)
	bigOpeB_B(fun_bigNeg,bignumNeg)
	bigOpeB_B(fun_bigBarrett,bignumBarrett)

	bigOpeB_I(fun_intFromBig,bignumToInt)
	bigOpeB_I(fun_bigNbits,bignumNbits)
	bigOpeB_I(fun_bigLowestBit,bignumLowestBit)
	bigOpeB_BOOL(fun_bigPositive,bignumPositive)
	bigOpeB_BOOL(fun_bigIsNull,bignumIsNull)
	bigOpeB_BOOL(fun_bigIsOne,bignumIsOne)
	bigOpeB_BOOL(fun_bigIsEven,bignumIsEven)

	bigOpeBI_B(fun_bigModPower2,bignumModPower2)
	bigOpeBI_B(fun_bigASR,bignumASR)

	bigOpeBI_I(fun_bigBit,bignumBit)

	bigOpeBB_B(fun_bigXor, bignumXor)
	bigOpeBB_B(fun_bigAdd,bignumAdd)
	bigOpeBB_B(fun_bigSub,bignumSub)
	bigOpeBB_B(fun_bigMul,bignumMul)
	bigOpeBB_B(fun_bigExp,bignumExp)
	bigOpeBB_B(fun_bigDiv,bignumDiv)
	bigOpeBB_B(fun_bigMod,bignumMod)
	bigOpeBB_B(fun_bigNegMod,bignumNegMod)
	bigOpeBB_B(fun_bigInv,bignumInv)
	bigOpeBB_B(fun_bigGcd,bignumPgcd)

	bigOpeBB_I(fun_bigCmp,bignumCmp)
	bigOpeBB_BOOL(fun_bigEquals, bignumEquals)
	bigOpeBB_BOOL(fun_bigGreater,bignumGreater)
	bigOpeBB_BOOL(fun_bigGreaterEquals, bignumGreaterEqual)
	bigOpeBB_BOOL(fun_bigLower, bignumLower)
	bigOpeBB_BOOL(fun_bigLowerEquals, bignumLowerEqual)

	bigOpeBBB_B(fun_bigAddMod, bignumAddMod)
	bigOpeBBB_B(fun_bigSubMod, bignumSubMod)
	bigOpeBBB_B(fun_bigDivMod, bignumDivMod)
	bigOpeBBB_B(fun_bigMulMod,bignumMulMod)
	bigOpeBBB_B(fun_bigExpMod,bignumExpMod)
	bigOpeBBB_B(fun_bigModBarrett,bignumModBarrett)

	bigOpeBBBB_B(fun_bigMulModBarrett,bignumMulModBarrett)
	bigOpeBBBB_B(fun_bigDivModBarrett,bignumDivModBarrett)
	bigOpeBBBB_B(fun_bigExpModBarrett,bignumExpModBarrett)
	bigOpeBBBB_B(fun_bigExpChinese,bignumExpChinese)
	bigOpeBBBBBB_B(fun_bigExpChinese5, bignumExpChinese5)

	bigOpeBBBBBBBB_B(fun_bigExpChinese7,bignumExpChinese7)


void systemBignumReset(void)
{
	int i;
	if (BigCount>= BIGREGISTERS) return;
//	PRINTF(LOG_DEV, "Reset bignum registers\n");
	BigList = NULL;
	for (i = 0; i < BIGREGISTERS; i++)
	{
		BigRegisters[i].header.nextBlock = (LB*)BigList;
		BigList = (bignum)&BigRegisters[i];
	}
	BigCount = BIGREGISTERS;
}
int systemBignumInit(Pkg *system)
{
	static const Native nativeDefs[] = {
		{ NATIVE_FUN, "bigAbs", fun_bigAbs, "fun BigNum -> BigNum"},
		{ NATIVE_FUN, "bigAdd", fun_bigAdd, "fun BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigXor", fun_bigXor, "fun BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigBarrett", fun_bigBarrett, "fun BigNum -> BigNum" },
		{ NATIVE_FUN, "bigBit", fun_bigBit, "fun BigNum Int -> Int" },
		{ NATIVE_FUN, "bigCmp", fun_bigCmp, "fun BigNum BigNum -> Int" },
		{ NATIVE_FUN, "bigEquals", fun_bigEquals, "fun BigNum BigNum -> Bool" },
		{ NATIVE_FUN, "bigGreater", fun_bigGreater, "fun BigNum BigNum -> Bool" },
		{ NATIVE_FUN, "bigGreaterEquals", fun_bigGreaterEquals, "fun BigNum BigNum -> Bool" },
		{ NATIVE_FUN, "bigLower", fun_bigLower, "fun BigNum BigNum -> Bool" },
		{ NATIVE_FUN, "bigLowerEquals", fun_bigLowerEquals, "fun BigNum BigNum -> Bool" },
		{ NATIVE_FUN, "bigDivRemainder", fun_bigDivRemainder, "fun BigNum BigNum -> [BigNum BigNum]" },
		{ NATIVE_FUN, "bigASR", fun_bigASR, "fun BigNum Int -> BigNum" },
		{ NATIVE_FUN, "bigEuclid", fun_bigEuclid, "fun BigNum BigNum -> [BigNum BigNum BigNum]" },
		{ NATIVE_FUN, "bigExpChinese", fun_bigExpChinese, "fun BigNum BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigExpChinese7", fun_bigExpChinese7, "fun BigNum BigNum BigNum BigNum BigNum BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigExpChinese5", fun_bigExpChinese5, "fun BigNum BigNum BigNum BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigExpMod", fun_bigExpMod, "fun BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigExpModBarrett", fun_bigExpModBarrett, "fun BigNum BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigFromStr", fun_bigFromStr, "fun Str -> BigNum" },
		{ NATIVE_FUN, "bigFromBytes", fun_bigFromStr, "fun Bytes -> BigNum" },
		{ NATIVE_FUN, "bigFromSignedStr", fun_bigFromSignedStr, "fun Str -> BigNum" },
		{ NATIVE_FUN, "bigFromSignedBytes", fun_bigFromSignedStr, "fun Bytes -> BigNum" },
		{ NATIVE_FUN, "bigFromDec", fun_bigFromDec, "fun Str -> BigNum" },
		{ NATIVE_FUN, "bigFromHex", fun_bigFromHex, "fun Str -> BigNum" },
		{ NATIVE_FUN, "bigFromInt", fun_bigFromInt, "fun Int -> BigNum" },
		{ NATIVE_FUN, "bigInv", fun_bigInv, "fun BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigIsEven", fun_bigIsEven, "fun BigNum -> Bool" },
		{ NATIVE_FUN, "bigIsNull", fun_bigIsNull, "fun BigNum -> Bool" },
		{ NATIVE_FUN, "bigIsOne", fun_bigIsOne, "fun BigNum -> Bool" },
		{ NATIVE_FUN, "bigLowestBit", fun_bigLowestBit, "fun BigNum -> Int" },
		{ NATIVE_FUN, "bigNegMod", fun_bigNegMod, "fun BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigMod", fun_bigMod, "fun BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigModBarrett", fun_bigModBarrett, "fun BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigModPower2", fun_bigModPower2, "fun BigNum Int -> BigNum" },
		{ NATIVE_FUN, "bigMul", fun_bigMul, "fun BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigExp", fun_bigExp, "fun BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigDiv", fun_bigDiv, "fun BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigAddMod", fun_bigAddMod, "fun BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigSubMod", fun_bigSubMod, "fun BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigDivMod", fun_bigDivMod, "fun BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigMulMod", fun_bigMulMod, "fun BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigMulModBarrett", fun_bigMulModBarrett, "fun BigNum BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigDivModBarrett", fun_bigDivModBarrett, "fun BigNum BigNum BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigNbits", fun_bigNbits, "fun BigNum -> Int" },
		{ NATIVE_FUN, "bigNeg", fun_bigNeg, "fun BigNum -> BigNum" },
		{ NATIVE_FUN, "bigGcd", fun_bigGcd, "fun BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "bigPositive", fun_bigPositive, "fun BigNum -> Bool" },
		{ NATIVE_FUN, "bigPower2", fun_bigPower2, "fun Int -> BigNum" },
		{ NATIVE_FUN, "bigRand", fun_bigRand, "fun Int Bool -> BigNum" },
		{ NATIVE_FUN, "bigASL1", fun_bigASL1, "fun BigNum -> BigNum" },
		{ NATIVE_FUN, "bigASR1", fun_bigASR1, "fun BigNum -> BigNum" },
		{ NATIVE_FUN, "bigSub", fun_bigSub, "fun BigNum BigNum -> BigNum" },
		{ NATIVE_FUN, "strFromBig", fun_strFromBig, "fun BigNum Int -> Str" },
		{ NATIVE_FUN, "signedStrFromBig", fun_signedStrFromBig, "fun BigNum -> Str" },
		{ NATIVE_FUN, "decFromBig", fun_decFromBig, "fun BigNum -> Str" },
		{ NATIVE_FUN, "hexFromBig", fun_hexFromBig, "fun BigNum -> Str" },
		{ NATIVE_FUN, "intFromBig", fun_intFromBig, "fun BigNum -> Int" },
	};
	NATIVE_DEF(nativeDefs);
	MM.bigAdd = nativeOpcode("bigAdd", 2);
	MM.bigGT = nativeOpcode("bigGreater", 2);
	MM.bigGE = nativeOpcode("bigGreaterEquals", 2);
	MM.bigLT = nativeOpcode("bigLower", 2);
	MM.bigLE = nativeOpcode("bigLowerEquals", 2);

	MM.bigExpMod = nativeOpcode("bigExpMod", 3);
	MM.bigExpModBarrett = nativeOpcode("bigExpModBarrett", 4);
	MM.bigNegMod = nativeOpcode("bigNegMod",2);
	MM.bigMod = nativeOpcode("bigMod",2);
	MM.bigModBarrett = nativeOpcode("bigModBarrett", 3);
	MM.bigMul = nativeOpcode("bigMul", 2);
	MM.bigExp = nativeOpcode("bigExp", 2);
	MM.bigDiv = nativeOpcode("bigDiv", 2);
	MM.bigAddMod = nativeOpcode("bigAddMod", 3);
	MM.bigSubMod = nativeOpcode("bigSubMod", 3);
	MM.bigDivMod = nativeOpcode("bigDivMod", 3);
	MM.bigMulMod = nativeOpcode("bigMulMod", 3);

	MM.bigMulModBarrett = nativeOpcode("bigMulModBarrett", 4);
	MM.bigDivModBarrett = nativeOpcode("bigDivModBarrett", 4);
	MM.bigNeg = nativeOpcode("bigNeg", 2);
	MM.bigSub = nativeOpcode("bigSub", 2);
	BigCount = 0;
	systemBignumReset();
	return 0;
}
