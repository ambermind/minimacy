// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

#ifdef USE_MINMAX_C
#define max(a,b) (((a) > (b)) ? (a) : (b))
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#include "util_2d_const.h"

#define COLORADD(x,y) (ColorClip[256+(x)+(y)])
#define COLORSUB(x,y) (ColorClip[256+(x)-(y)])
#define COLORMUL(x,y) (((x)*(y)>65535)?255:(((x)*(y))>>8))
#define COLORALPHA(a,src,dst) (COLORADD(COLORMUL(a,src),COLORMUL(255-(a),dst)))

// IDCT : from https://membres-ljk.imag.fr/Valerie.Perrier/SiteWeb/node9.html
// C * Ct = I
// Pixels = Ct * DCT * C
// preComputation of C for JPG decoding:
const LFLOAT IDCT88[64] = {
	0.3535533906, 0.3535533906, 0.3535533906, 0.3535533906, 0.3535533906, 0.3535533906, 0.3535533906, 0.3535533906,
	0.4903926402, 0.4157348062, 0.2777851165, 0.09754516101, -0.09754516101, -0.2777851165, -0.4157348062, -0.4903926402,
	0.4619397663, 0.1913417162, -0.1913417162, -0.4619397663, -0.4619397663, -0.1913417162, 0.1913417162, 0.4619397663,
	0.4157348062, -0.09754516101, -0.4903926402, -0.2777851165, 0.2777851165, 0.4903926402, 0.09754516101, -0.4157348062,
	0.3535533906, -0.3535533906, -0.3535533906, 0.3535533906, 0.3535533906, -0.3535533906, -0.3535533906, 0.3535533906,
	0.2777851165, -0.4903926402, 0.09754516101, 0.4157348062, -0.4157348062, -0.09754516101, 0.4903926402, -0.2777851165,
	0.1913417162, -0.4619397663, 0.4619397663, -0.1913417162, -0.1913417162, 0.4619397663, -0.4619397663, 0.1913417162,
	0.09754516101, -0.2777851165, 0.4157348062, -0.4903926402, 0.4903926402, -0.4157348062, 0.2777851165, -0.09754516101
};

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
int _blendDestination(int src, int dst) { return dst; }

#define NB_BLEND_FUNCTIONS 11
const BLEND_FUNCTION BlendFunctions[NB_BLEND_FUNCTIONS] =
{
	NULL, _blendXor, _blendOr, _blendAnd,
	_blendMax, _blendMin, _blendAdd, _blendSub,
	_blendMul, _blendAlpha, _blendDestination
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

int _bitmapCopy(LBitmap* b, LINT x, LINT y, LINT w, LINT h, LBitmap** result)
{
	LINT j;
	LBitmap* d;
	int* src, * dst;
	*result = NULL;
	if (!b) return 0;
	if (_clip1D(0, b->w, &x, &w, NULL)) return 0;
	if (_clip1D(0, b->h, &y, &h, NULL)) return 0;
	if ((w <= 0) || (h <= 0)) return 0;
	d = _bitmapCreate(w, h); if (!d) return EXEC_OM;
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

void _bitmapFillRectangle(LBitmap* b, LINT x, LINT y, LINT w, LINT h, int color, BLEND_FUNCTION blend)
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

void _bitmapFillCircle(LBitmap* b, LINT x0, LINT y0, LINT w, LINT h, int color, BLEND_FUNCTION blend)
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

void _bitmapResizeSmooth2(LBitmap* dst, LBitmap* src)	// dst has exactly half the size of src
{
	LINT i, j, k;
	for (j = 0; j < dst->h; j++) {
		for (i = 0; i < dst->w; i++) {
			unsigned char* src2 = (unsigned char*)&src->start8[ j*2* src->next8 + 8 * i];
			unsigned char* dst2 = (unsigned char*)&dst->start8[j * dst->next8 + 4 * i];
			for (k = 0; k < 4; k++) {
				int sum = (src2[0] + src2[4] + src2[src->next8] + src2[src->next8 + 4])>>2;
				src2++;
				*(dst2++) = (lchar)sum;
			}
		}
	}
}

void _bitmapResizeSmooth4(LBitmap* dst, LBitmap* src)	// dst has exactly 1/4 the size of src
{
	LINT i, j, k;
	for (j = 0; j < dst->h; j++) {
		for (i = 0; i < dst->w; i++) {
			unsigned char* src2 = (unsigned char*)&src->start8[j * 4 * src->next8 + 16 * i];
			unsigned char* dst2 = (unsigned char*)&dst->start8[j * dst->next8 + 4 * i];
			for (k = 0; k < 4; k++) {
				int sum = (
					src2[0] + src2[4] + src2[8] + src2[12] +
					src2[src->next8] + src2[src->next8 + 4] + src2[src->next8 + 8] + src2[src->next8 + 12] +
					src2[src->next8 * 2] + src2[src->next8 * 2 + 4] + src2[src->next8 * 2 + 8] + src2[src->next8 * 2 + 12] +
					src2[src->next8 * 3] + src2[src->next8 * 3 + 4] + src2[src->next8 * 3 + 8] + src2[src->next8 * 3 + 12]
					) >> 4;
				src2++;
				*(dst2++) = (lchar)sum;
			}
		}
	}
}

void _bitmapResizeSmooth(LBitmap* dst, LBitmap* src)
{
	lchar* dst0;
	LINT i, j, k, ws, hs, dx, dy, y;
	if ((!dst) || (!src) || 0==dst->w || 0 == dst->h) return;
	if ((src->w == (dst->w) * 2) && (src->h == (dst->h) * 2)) {
		_bitmapResizeSmooth2(dst, src);
		return;
	}
	if ((src->w == (dst->w) * 4) && (src->h == (dst->h) * 4)) {
		_bitmapResizeSmooth4(dst, src);
		return;
	}
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

const int SrcComponent[] = {
	16,8,0,24,0,
	16,8,0,24,0,
};
const int DstInv[] = {
	0,0,0,0,0,
	255,255,255,255,255,
};
const int SrcAnd[] = {
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

#define C_LEFT(z) (w1-(z))
#define C_RIGHT(z) (d->w - w +(z))
#define C_TOP(z) (w1-(z))
#define C_BOTTOM(z) (d->h - w +(z))

#define CORNER_HALF(leftRight,topBottom,x,y) { \
	lchar* p=&d->start8[topBottom(y) * d->next8 + (leftRight(x) << 2) + 3];	\
	*p = COLORMUL(*p,(lchar)alpha);	\
}

#define CORNER(leftRight,topBottom) {\
	CORNER_HALF(leftRight,topBottom,x,y) \
	if (x!=y) CORNER_HALF(leftRight,topBottom,y,x) \
}

void _bitmapCorners(LBitmap* d, LINT w, LINT mask)
{
	LINT x, y, R0, R1, w1;
	if (!d) return;
	if (w > (d->w >> 1)) w = d->w >> 1;
	if (w > (d->h >> 1)) w = d->h >> 1;
	if (w <= 1) return;
	w1 = w - 1;
	R0 = w1*w1;
	R1 = w*w;
	if (!mask) mask = -1;
	for(x=0;x<w;x++)
		for (y = 0; y <= x; y++) {
			LINT r = x * x + y * y;
			LINT alpha = (r <= R0) ? 255 : ((r >= R1) ? 0 : 255 - ((r - R0)<<8) / (R1 - R0));
			if (mask & CORNER_TOP_LEFT) CORNER(C_LEFT,C_TOP)
			if (mask & CORNER_TOP_RIGHT) CORNER(C_RIGHT, C_TOP)
			if (mask & CORNER_BOTTOM_LEFT) CORNER(C_LEFT, C_BOTTOM)
			if (mask & CORNER_BOTTOM_RIGHT) CORNER(C_RIGHT, C_BOTTOM)
		}
}
void _bitmapGradient(LBitmap* d, LINT col00, LINT colw0, LINT col0h)
{
	LINT i, x, y, w1, h1;
	if (!d) return;
	w1 = d->w - 1; if (w1 < 1) w1 = 1;
	h1 = d->h - 1; if (h1 < 1) h1 = 1;
	for (i = 0; i < 4; i++) {
		LINT shift = i * 8;
		LINT b0 = (col00>>shift) & 255;
		LINT bw = ((colw0 >> shift) & 255) - b0;
		LINT bh = ((col0h >> shift) & 255) - b0;
		for (y = 0; y < d->h; y++)
			for (x = 0; x < d->w; x++) {
				LINT val = b0 + bw * x / w1 + bh * y / h1;
				if (val < 0) val = 0;
				else if (val > 255) val = 255;
				d->start8[y * d->next8 + (x << 2) + i] = (char)val;
			}
	}
}
int _rgbFromYCrCb(LINT rgb)
{
	lchar y = (rgb >> 16) & 255;
	lchar u = (rgb) & 255;
	lchar v = (rgb >> 8) & 255;
	lchar b = ColorClip[yuv_bu[u] + y + 256];
	lchar g = ColorClip[yuv_gu[u] + yuv_gv[v] + y + 256];
	lchar r = ColorClip[yuv_rv[v] + y + 256];
	return (r << 16) | (g << 8) | b;
}

int _yCrCbFromRgb(LINT ycrcb)
{
	lchar r = (ycrcb >> 16) & 255;
	lchar g = (ycrcb >> 8) & 255;
	lchar b = (ycrcb) & 255;
	lchar y = yuv_yr[r] + yuv_yg[g] + yuv_yb[b];
	lchar u = yuv_ur[r] + yuv_ug[g] + yuv_ub[b] + 128;
	lchar v = yuv_vr[r] + yuv_vg[g] + yuv_vb[b] + 128;
	return (y << 16) | (v << 8) | u;
}

void _bitmapFromYCrCb(LBitmap* b)
{
	LINT i, j;
	lchar* p0 = b->start8;
	for (j = 0; j < b->h; j++)
	{
		lchar* p = p0;
		for (i = 0; i < b->w; i++)
		{
			lchar y = p[2];
			lchar u = p[0];
			lchar v = p[1];
			p[2] = ColorClip[yuv_rv[v] + y + 256];
			p[1] = ColorClip[yuv_gu[u] + yuv_gv[v] + y + 256];
			p[0] = ColorClip[yuv_bu[u] + y + 256];
			p += 4;
		}
		p0 += b->next8;
	}
}

void _bitmapToYCrCb(LBitmap* b)
{
	LINT i, j;
	lchar* p0 = b->start8;
	for (j = 0; j < b->h; j++)
	{
		lchar* p = p0;
		for (i = 0; i < b->w; i++)
		{
			lchar r = p[2];
			lchar g = p[1];
			lchar b = p[0];
			p[2] = yuv_yr[r] + yuv_yg[g] + yuv_yb[b];
			p[1] = yuv_vr[r] + yuv_vg[g] + yuv_vb[b] + 128;
			p[0] = yuv_ur[r] + yuv_ug[g] + yuv_ub[b] + 128;
			p += 4;
		}
		p0 += b->next8;
	}
}

void idct88(LB* array)
{
	int i, j,k;
	LFLOAT tmp[64];

	for( i = 0; i < 8; i++)
		for (j = 0; j < 8; j++)
		{
			LFLOAT val = 0;
			for (k = 0; k < 8; k++)
			{
				val += ARRAY_FLOAT(array, k + j * 8)* IDCT88[i + k * 8];
			}
			tmp[i + j * 8] = val;
		}
	for (i = 0; i < 8; i++)
		for (j = 0; j < 8; j++)
		{
			LFLOAT val = 0;
			for (k = 0; k < 8; k++)
			{
				val += IDCT88[j + k * 8] * tmp[i + k * 8];
			}
			ARRAY_SET_FLOAT(array,i + j * 8, val);
		}
}
void dct88(LB* array)
{
	int i, j, k;
	LFLOAT tmp[64];

	for (i = 0; i < 8; i++)
		for (j = 0; j < 8; j++)
		{
			LFLOAT val = 0;
			for (k = 0; k < 8; k++)
			{
				val += ARRAY_FLOAT(array, k + j * 8) * IDCT88[k + i * 8];
			}
			tmp[i + j * 8] = val;
		}
	for (i = 0; i < 8; i++)
		for (j = 0; j < 8; j++)
		{
			LFLOAT val = 0;
			for (k = 0; k < 8; k++)
			{
				val += IDCT88[k + j * 8] * tmp[i + k * 8];
			}
			ARRAY_SET_FLOAT(array, i + j * 8, val);
		}
}
void _bitmapExportMono(LBitmap* b, int background, char* dst, LINT len)
{
	LINT i, j;
	LINT wReal = b->w >> 3;
	int* p0 = b->start32;
	lchar* q0 = (lchar*)dst;
	if (b->w & 7) return;	//	bitmap width must be multiple of 8
	if (wReal * b->h > len) return;
	for (j = 0; j < b->h; j++)
	{
		int* p = p0;
		for (i = 0; i < b->w; i++)
			if ((*(p++)) != background) q0[i >> 3] |= 1 << (7 - (i & 7));
			else q0[i >> 3] &= ~(1 << (7 - (i & 7)));
		p0 += b->next32;
		q0 += wReal;
	}
}
void _bitmapImportMono(LBitmap* b, int color, int background, char* src, LINT len)
{
	LINT i, j;
	LINT wReal = b->w >> 3;
	int* p0 = b->start32;
	lchar* q0 = (lchar*)src;
	if (b->w & 7) return;	//	bitmap width must be multiple of 8
	if (wReal * b->h > len) return;
	for (j = 0; j < b->h; j++)
	{
		int* p = p0;
		for (i = 0; i < b->w; i++)
			*(p++) = (q0[i >> 3] & (1 << (7 - (i & 7)))) ? color : background;
		p0 += b->next32;
		q0 += wReal;
	}
}
void _bitmapExportMonoVertical(LBitmap* b, int background, char* dst, LINT len)
{
	LINT i, j;
	int* p0 = b->start32;
	if (b->h & 7) return;	//	bitmap height must be multiple of 8
	if ((b->h >> 3) * b->w > len) return;
	for (j = 0; j < b->h; j++)
	{
		int* p = p0;
		lchar mask=1 << (j & 7);
		lchar* q = ((lchar*)dst) + (j >> 3) * b->w;
		for (i = 0; i < b->w; i++)
			if ((*(p++)) != background) q[i] |= mask;
			else q[i] &= ~mask;
		p0 += b->next32;
	}
}

void _bitmapImportMonoVertical(LBitmap* b, int color, int background, char* src, LINT len)
{
	LINT i, j;
	int* p0 = b->start32;
	if (b->h & 7) return;	//	bitmap height must be multiple of 8
	if ((b->h >> 3) * b->w > len) return;
	for (j = 0; j < b->h; j++)
	{
		int* p = p0;
		lchar mask=1 << (j & 7);
		lchar* q = ((lchar*)src) + (j >> 3) * b->w;
		for (i = 0; i < b->w; i++)
			*(p++)=(q[i]&mask)?color:background;
		p0 += b->next32;
	}
}
