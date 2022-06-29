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

#ifdef ON_UNIX
LINT min(LINT a,LINT b) { return (a<b)?a:b; }
LINT max(LINT a,LINT b) { return (a>b)?a:b; }
#endif

lchar ColorAdd[256 * 256];
lchar ColorSub[256 * 256];
lchar ColorMul[256 * 256];

#define COLORADD(x,y) (ColorAdd[(x)+((y)<<8)])
#define COLORSUB(x,y) (ColorSub[(x)+((y)<<8)])
#define COLORMUL(x,y) (ColorMul[(x)+((y)<<8)])
#define COLORALPHA(a,src,dst) (COLORADD(COLORMUL(a,src),COLORMUL(255-(a),dst)))

void _colorInit()
{
	int i, j, k;
	for (i = 0; i < 256; i++)
		for (j = 0; j < 256; j++)
		{
			k = i + j;
			ColorAdd[i + (j << 8)] = (k <= 255) ? k : 255;

			k = j - i;
			ColorSub[i + (j << 8)] = (k >=0) ? k : 0;

			k = i * j / 255;
			ColorMul[i + (j << 8)] = k;
		}
}

int _blendXor(int src, int dst) { return src ^ dst; }
int _blendOr(int src, int dst) { return src | dst; }
int _blendAnd(int src, int dst) { return src & dst; }
int _blendMax(int src, int dst) { 
	return max(0xff000000&src, 0xff000000 & dst)|
		max(0xff0000 & src, 0xff0000 & dst) | 
		max(0xff00 & src, 0xff00 & dst) | 
		max(0xff & src, 0xff & dst);
}
int _blendMin(int src, int dst) {
	return min(0xff000000 & src, 0xff000000 & dst) |
		min(0xff0000 & src, 0xff0000 & dst) |
		min(0xff00 & src, 0xff00 & dst) |
		min(0xff & src, 0xff & dst);
}
int _blendAdd(int src, int dst) {
	return (COLORADD((src >> 24) & 255, (dst >> 24) & 255) << 24) |
		(COLORADD((src >> 16) & 255, (dst >> 16) & 255) << 16) |
		(COLORADD((src >> 8) & 255, (dst >> 8) & 255) << 8) |
		(COLORADD((src) & 255, (dst) & 255) );
}
int _blendSub(int src, int dst) {
	return (COLORSUB((src >> 24) & 255, (dst >> 24) & 255) << 24) |
		(COLORSUB((src >> 16) & 255, (dst >> 16) & 255) << 16) |
		(COLORSUB((src >> 8) & 255, (dst >> 8) & 255) << 8) |
		(COLORSUB((src) & 255, (dst) & 255));
}
int _blendMul(int src, int dst) {
	return (COLORMUL((src >> 24) & 255, (dst >> 24) & 255) << 24) |
		(COLORMUL((src >> 16) & 255, (dst >> 16) & 255) << 16) |
		(COLORMUL((src >> 8) & 255, (dst >> 8) & 255) << 8) |
		(COLORMUL((src) & 255, (dst) & 255) );
}

int _blendAlpha(int src, int dst) {
	int a = (src >> 24) & 255;
	return
		//(0xff000000)|
		//(dst&0xff000000)|
		(COLORALPHA(a, (src >> 24) & 255, (dst >> 24) & 255) << 24) |
		(COLORALPHA(a, (src >> 16) & 255, (dst >> 16) & 255) << 16) |
		(COLORALPHA(a, (src >> 8) & 255, (dst >> 8) & 255) << 8) |
		(COLORALPHA(a, (src) & 255, (dst) & 255));
}

#define NB_BLEND_FUNCTIONS 10
BLEND_FUNCTION BlendFunctions[NB_BLEND_FUNCTIONS] =
{
	NULL, _blendXor, _blendOr, _blendAnd,
	_blendMax, _blendMin, _blendAdd, _blendSub,
	_blendMul, _blendAlpha,
};
BLEND_FUNCTION _blendFunction(LINT num)
{
	if (num > 0 && num < NB_BLEND_FUNCTIONS) return BlendFunctions[num];
	return NULL;
}

LINT _clip1D(LINT targetX, LINT targetW, LINT* x, LINT* w, LINT* dx)
{
	if (dx) *dx = 0;
	if (*w <= 0) return -1;

	if (*x < targetX) {
		if (dx) *dx = -(*x);
		*w += *x;  *x = 0;
	}
	if ((*x) + (*w) >= targetX + targetW) *w = targetX + targetW - *x;
	if (*w <= 0) return -1;
	return 0;
}

int _bitmapCopy(Thread* th, LBitmap* b, LINT x, LINT y, LINT w, LINT h, LBitmap** result)
{
	LINT j;
	LBitmap* d;
	int* src, * dst;
	*result = NULL;
	if (!b) return 0;
	if (_clip1D(0, b->w, &x, &w, NULL)) return 0;
	if (_clip1D(0, b->h, &y, &h, NULL)) return 0;
	d = _bitmapCreate(th, w, h); if (!d) return EXEC_OM;
	*result = d;
	dst = d->start32;
	src = &b->start32[y * b->next32 + x];

	for (j = 0; j < h; j++)
	{
		memcpy((char*)dst, (char*)src, w * 4);
		dst += d->next32;
		src += b->next32;
	}
	return 0;
}

void _bitmapFill(LBitmap* b, int color, BLEND_FUNCTION blend)
{
	int* p0;
	LINT i, j;
	if (!b) return;
	if ((color == 0) && (blend==NULL))
	{
		memset(b->start8, 0, b->h * b->next8);
		return;
	}
	p0 = b->start32;
	for (j = 0; j < b->h; j++)
	{
		int* p = p0;
		if (blend) for (i = 0; i < b->w; i++) p[i] = (*blend)(color, p[i]);
		else for (i = 0; i < b->w; i++) p[i] = color;
		p0 += b->next32;
	}
}
void _bitmapPlot(LBitmap* b, LINT x, LINT y, int color, BLEND_FUNCTION blend)
{
	int* p;
	if ((!b) || (x < 0) || (x >= b->w) || (y < 0) || (y >= b->h)) return;
	p = &b->start32[x + y * b->next32];
	*p = blend ? (*blend)(color, *p) : color;
}

void _bitmapMakeColorTransparent(LBitmap* b, int color)
{
	int* p0;
	LINT i, j;
	if (!b) return;
	p0 = b->start32;
	for (j = 0; j < b->h; j++)
	{
		int* p = p0;
		for (i = 0; i < b->w; i++)
			if ((0xffffff&p[i]) != color) p[i] |= 0xff000000;
			else p[i] &= 0xffffff;
		p0 += b->next32;
	}
}

void _bitmapToBitmap(LBitmap* b, LINT xdst, LINT ydst, LBitmap* a, LINT xsrc, LINT ysrc, LINT w, LINT h, BLEND_FUNCTION blend)
{
	int* p0, * q0;
	LINT dx, dy, i, j;
	if ((!b) || !a) return;
	if (_clip1D(0, a->w, &xsrc, &w, &dx)) return;
	if (_clip1D(0, a->h, &ysrc, &h, &dy)) return;
	xdst += dx; ydst += dy;
	if (_clip1D(0, b->w, &xdst, &w, &dx)) return;
	if (_clip1D(0, b->h, &ydst, &h, &dy)) return;
	xsrc += dx; ysrc += dy;
	p0 = &a->start32[ysrc * a->next32 + xsrc];
	q0 = &b->start32[ydst * b->next32 + xdst];
	for (j = 0; j < h; j++)
	{
		int* p = p0;
		int* q = q0;
		for (i = 0; i < w; i++)
		{
			int color = *(p++);
			if (blend) color=(*blend)(color, *q);
			*(q++) = color;
		}
		p0 += a->next32;
		q0 += b->next32;
	}
}

void _bitmapToBitmapColored(LBitmap* b, LINT xdst, LINT ydst, LBitmap* a, LINT xsrc, LINT ysrc, LINT w, LINT h, BLEND_FUNCTION blend,int colored, BLEND_FUNCTION coloredBlend)
{
	int* p0, * q0;
	LINT dx, dy, i, j;
	if ((!b) || !a) return;
	if (_clip1D(0, a->w, &xsrc, &w, &dx)) return;
	if (_clip1D(0, a->h, &ysrc, &h, &dy)) return;
	xdst += dx; ydst += dy;
	if (_clip1D(0, b->w, &xdst, &w, &dx)) return;
	if (_clip1D(0, b->h, &ydst, &h, &dy)) return;
	xsrc += dx; ysrc += dy;
	p0 = &a->start32[ysrc * a->next32 + xsrc];
	q0 = &b->start32[ydst * b->next32 + xdst];
	for (j = 0; j < h; j++)
	{
		int* p = p0;
		int* q = q0;
		for (i = 0; i < w; i++)
		{
			int color = *(p++);
			if (coloredBlend) color = (*coloredBlend)(color, colored);
			if (blend) color=(*blend)(color, *q);
			*(q++) = color;
		}
		p0 += a->next32;
		q0 += b->next32;
	}
}

// never called directly, assume x and x+w into [0,b->w[, and y into [0,b->h[
void _bitmapHorizontalLine(LBitmap* b, LINT x, LINT y, LINT w, int color, BLEND_FUNCTION blend)
{
	LINT i;
	int *p = &b->start32[y * b->next32 + x];
	if (blend) for (i = 0; i < w; i++) p[i] = (*blend)(color,p[i]);
	else for (i = 0; i < w; i++) p[i] = color;
}

// never called directly, assume y and y+h both into [0,b->h[, and x into [0,b->w[
void _bitmapVerticalLine(LBitmap* b, LINT x, LINT y, LINT h, int color, BLEND_FUNCTION blend)
{
	LINT i;
	int* p = &b->start32[y * b->next32 + x];
	if (blend) for (i = 0; i < h; i++, p += b->next32) *p = (*blend)(color, p[i]);
	else for (i = 0; i < h; i++, p += b->next32) *p = color;
}

void _bitmapFilledRectangle(LBitmap* b, LINT x, LINT y, LINT w, LINT h, int color, BLEND_FUNCTION blend)
{
	LINT i;
	if (!b) return;
	if (_clip1D(0, b->w, &x, &w, NULL)) return;
	if (_clip1D(0, b->h, &y, &h, NULL)) return;
	for (i = 0; i < h; i++) _bitmapHorizontalLine(b, x, y + i, w, color, blend);
}

void _bitmapRectangle(LBitmap* b, LINT x, LINT y, LINT w, LINT h, int color, BLEND_FUNCTION blend)
{
	if (!b) return;
	if ((w <= 0) || (h <= 0)) return;

	if ((y>=0)&&(y<b->h)) _bitmapHorizontalLine(b, max(x, 0), y, min(x+w,b->w-1) - max(x, 0), color, blend);
	if (((y+h-1)>= 0) && ((y+h-1) < b->h)) _bitmapHorizontalLine(b, max(x, 0), y+h-1, min(x + w, b->w - 1) - max(x, 0), color, blend);

	if ((x >= 0) && (x < b->w)) _bitmapVerticalLine(b,  x, max(y, 0), min(y + h, b->h- 1) - max(y, 0), color, blend);
	if ((x + w - 1 >= 0) && (x + w - 1 < b->w)) _bitmapVerticalLine(b, x + w - 1, max(y, 0), min(y + h, b->h - 1) - max(y, 0), color, blend);
}

void _bitmapScanline(LBitmap* b, LINT x1, LINT x2, LINT y, int color, BLEND_FUNCTION blend)
{
	if (!b) return;
	if ((x1>x2) || (y < 0) || (y>=b->h) || (x1>=b->w) ||(x2<0)) return;
	if (x1 < 0) x1 = 0;
	if (x2 >= b->w) x2 = b->w - 1;

	_bitmapHorizontalLine(b, x1,y,x2-x1+1, color, blend);
}

// where=1  -> clip left part
// where=-1 -> clip right part
int _borderCross(LINT *x1, LINT *y1, LINT *x2, LINT *y2,LINT xb, LINT where)
{
	float k;
	if (where * (xb - *x1) > 0)
	{
		if (where *(xb-*x2)>0) return -1;
		k = (float)(xb - *x1) * (float)(*y2 - *y1) / (*x2 - *x1);
		*y1 += (LINT)k;
		*x1 = xb;
	}
	else if (where * (xb - *x2) > 0)
	{
		k = (float)(xb - *x2) * (float)(*y1 - *y2) / (*x1 - *x2);
		*y2 += (LINT)k;
		*x2 = xb;
	}
	return 0;
}
void _bitmapLine(LBitmap* b, LINT x1, LINT y1, LINT x2, LINT y2, int color, BLEND_FUNCTION blend)
{
	int* p;
	LINT ax, ay, bx, by, dx, dy, kx, ky, k, i;

	if (_borderCross(&x1, &y1, &x2, &y2, 0, 1)) return;
	if (_borderCross(&x1, &y1, &x2, &y2, b->w-1, -1)) return;
	if (_borderCross(&y1, &x1, &y2, &x2, 0, 1)) return;
	if (_borderCross(&y1, &x1, &y2, &x2, b->h-1, -1)) return;

	dx = abs((int)(x2 - x1)); dy = abs((int)(y2 - y1));
	if (dx >= dy)
	{
		if (x2 > x1) { ax = x1; ay = y1; bx = x2; by = y2; }
		else { ax = x2; ay = y2; bx = x1; by = y1; }

		kx = bx - ax; ky = by - ay;
		dx = 1; dy = b->next32;
		p = b->start32+ ax + ay * b->next32;

		if (ky < 0) { dy = -dy; ky = -ky;}
		k = kx >> 1;
		for (i = ax; i <= bx; i++)
		{
			*p = blend?(*blend)(color,*p):color;
			k += ky;
			if (k > kx) { k -= kx; p += dy; }
			p += dx;
		}
	}
	else
	{
		if (y2 > y1) { ax = x1; ay = y1; bx = x2; by = y2; }
		else { ax = x2; ay = y2; bx = x1; by = y1; }

		kx = bx - ax; ky = by - ay;
		dx = 1; dy = b->next32;
		p = b->start32 + ax + ay * b->next32;

		if (kx < 0) { dx = -dx; kx = -kx; }
		k = ky >> 1;
		for (i = ay; i <= by; i++)
		{
			*p = blend ? (*blend)(color, *p) : color;
			k += kx;
			if (k > ky) { k -= ky; p += dx; }
			p += dy;
		}
	}
}

void _bitmapFilledCircle(LBitmap* b, LINT x0, LINT y0, LINT w, LINT h, int color, BLEND_FUNCTION blend)
{
	LINT i,x;
	double fox, foy;
	if ((w <= 0) || (h <= 0)) return;

	fox = (double)w;
	foy = (double)h;
	fox -= 0.1; foy -= 0.1;
	fox /= 2; foy /= 2;

	for (i = max(0,y0); i < min(y0+h,b->h); i++)
	{
		double x1, x2, dx, y;
		y = (double)(i-y0);
		dx = fox * sqrt(1 - (foy - y - .5) * (foy - y - .5) / (foy * foy));
		x1 = fox - dx;
		x2 = fox + dx;
		x = x0 + (LINT)x1;
		_bitmapHorizontalLine(b, max(x,0), i, min(x0 + (LINT)x2,b->w-1)-max(x,0), color, blend);
	}
}

void _bitmapCircle4Plot(LBitmap* b, double x0, double y0, double dx, double dy, int color, BLEND_FUNCTION blend)
{
	_bitmapPlot(b, (LINT)floor(x0+dx), (LINT)floor(y0+dy), color, blend);
	_bitmapPlot(b, (LINT)floor(x0-dx), (LINT)floor(y0+dy), color, blend);
	_bitmapPlot(b, (LINT)floor(x0 + dx), (LINT)floor(y0 - dy), color, blend);
	_bitmapPlot(b, (LINT)floor(x0 - dx), (LINT)floor(y0 - dy), color, blend);
}
void _bitmapCircle(LBitmap* b, LINT x0, LINT y0, LINT w0, LINT h0, int color, BLEND_FUNCTION blend)
{
	LINT last = -1;
	double xc = ((double)w0) / 2 + x0;
	double yc = ((double)h0) / 2 + y0;
	double w = ((double)w0) - 1;
	double h = ((double)h0) - 1;
	double x, y, h2, w2, hw2, ny, c1, c2, c3;

	if ((w <= 0) || (h <= 0)) return;
	w2 = w * w / 4;	h2 = h * h / 4;	hw2 = w2 * h2;

	x = w * 0.5; y = (h0 & 1)?0:0.5;
	do
	{
		_bitmapCircle4Plot(b, xc, yc, x, y, color, blend);
		ny = (y + 1) * (y + 1) * w2;
		c1 = x * x * h2 + ny - hw2; if (c1 < 0) c1 = -c1;
		c2 = (x-1) * (x-1) * h2 + ny - hw2; if (c2 < 0) c2 = -c2;
		c3 = (x-1) * (x-1) * h2 + y*y*w2 - hw2; if (c3 < 0) c3 = -c3;
		if (c1 < c2 && c1 < c3 && last!=3) {
			last = 1; y += 1;
		}
		else if (c3<c2 && last!=1) {
			last = 3;  x -= 1;
		}
		else {
			last = 2; x -= 1; y += 1;
		}
	} while (x>=0);
}

LINT _pixelh(lchar* p0, LINT x, LINT dx)
{
	LINT x2 = x + dx;
	LINT v = 0;
	while (x2 >= 65536)
	{
		v += ((*p0) & 255) * (65536 - x);
		p0 += 4;
		x2 -= 65536;
		x = 0;
	}
	if (x2 != x) v += ((*p0) & 255) * (x2 - x);
	if (dx) v /= dx;
	return v;
}

lchar _pixelv(lchar* p0, LINT y, LINT dy, LINT next, LINT x, LINT dx)
{
	LINT y2 = y + dy;
	LINT v = 0;
	while (y2 >= 65536)
	{
		v += _pixelh(p0, x, dx) * (65536 - y);
		p0 += next;
		y2 -= 65536;
		y = 0;
	}
	if (y2 != y) v += _pixelh(p0, x, dx) * (y2 - y);
	if (dy) v /= dy;
	return (lchar)v;
}

void _bitmapResizeSmooth(LBitmap* dst, LBitmap* src)
{
	lchar* dst0;
	LINT i, j, k, ws, hs, dx, dy, y;
	if ((!dst) || (!src) || 0==dst->w || 0 == dst->h) return;

	ws = src->w;
	hs = src->h;

	dx = (ws << 16) / dst->w;
	dy = (hs << 16) / dst->h;

	dst0 = dst->start8;
	y = 0;
	for (j = 0; j < dst->h; j++)
	{
		lchar* dst1 = dst0;
		LINT x = 0;
		for (i = 0; i < dst->w; i++)
		{
			lchar* src2 = &src->start8[(y >> 16) * src->next8 + 4 * (x >> 16)];
			for(k=0;k<4;k++)
				dst1[k] = _pixelv(src2 + k, y & 65535, dy, src->next8, x & 65535, dx);
			dst1 += 4;
			x += dx;
		}
		dst0 += dst->next8;
		y += dy;
	}
}

void _bitmapResizeNearest(LBitmap* dst, LBitmap* src)
{
	int* dst0;
	double i, j, kx, ky, w, h;
	if ((!dst) || (!src) || 0 == dst->w || 0 == dst->h) return;
	w = (double)dst->w;
	h = (double)dst->h;

	kx = ((double)src->w)/w;
	ky = ((double)src->h)/h;
	dst0 = dst->start32;
	for (j = 0; j < h; j++)
	{
		int* dst1 = dst0;
		for (i = 0; i < w; i++)
		{
			LINT x = (LINT)(i * kx);
			LINT y = (LINT)(j * ky);
			*(dst1++)= src->start32[y * src->next32 + x];
		}
		dst0 += dst->next32;
	}
}

int SrcComponent[] = {
	16,8,0,24,0,
	16,8,0,24,0,
};
int DstInv[] = {
	0,0,0,0,0,
	255,255,255,255,255,
};
int SrcAnd[] = {
	255,255,255,255,0,
	255,255,255,255,0,
};
void _bitmapComponents(LBitmap* d, LINT r, LINT g, LINT b, LINT a)
{
	int* p0;
	LINT i, j;
	int cr = SrcComponent[r];
	int cg = SrcComponent[g];
	int cb = SrcComponent[b];
	int ca = SrcComponent[a];
	int and = (SrcAnd[a] << 24) | (SrcAnd[r] << 16) | (SrcAnd[g] << 8) | (SrcAnd[b]);
	int mask = (DstInv[a] << 24) | (DstInv[r] << 16) | (DstInv[g] << 8) | (DstInv[b]);

	if (!d) return;
	p0 = d->start32;
	for (j = 0; j < d->h; j++)
	{
		int* p = p0;
		for (i = 0; i < d->w; i++)
		{
			int color = p[i];
			int v= mask ^ (and & ((((color >> ca) & 255) << 24) | (((color >> cr) & 255) << 16) | (((color >> cg) & 255) << 8) | (((color >> cb) & 255))));
			p[i] = v;
		}
		p0 += d->next32;
	}
}
