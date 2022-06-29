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
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include "jpeglib.h"
#include <setjmp.h>

#define MNEW(type,n) (type*)malloc((n)*sizeof(type))
#define MDELETE(f) if (f) free(f)

typedef struct {
	void* user;
	void* (*malloc)(void*, int);
	void (*free)(void*, void*);
}mjpg_alloc;

static void* mjpg_malloc(mjpg_alloc* alloc, size_t size)
{
	void* block = alloc->malloc ? (*alloc->malloc)(alloc->user, (int)size) : malloc(size);
//	printf(">>>>>>jpg malloc %llx : %d bytes -> %llx\n", alloc, (int)size,block);
	return block;
}
static void mjpg_free(mjpg_alloc* alloc, void* block)
{
//	printf(">>>>>>png free %llx\n", block);
	if (alloc->free) (*alloc->free)(alloc->user, block);
	else if (!alloc->malloc) free(block);
//	else printf(">>>>>>jpg free nothing\n");
}

/*
 * Memory allocation and freeing are controlled by the regular library
 * routines malloc() and free().
 */

GLOBAL(void*)
jpeg_get_small(j_common_ptr cinfo, size_t sizeofobject)
{
	return mjpg_malloc((mjpg_alloc*)cinfo->user, sizeofobject);
}

GLOBAL(void)
jpeg_free_small(j_common_ptr cinfo, void* object, size_t sizeofobject)
{
	mjpg_free((mjpg_alloc*)cinfo->user, object);
}


/*
 * "Large" objects are treated the same as "small" ones.
 * NB: although we include FAR keywords in the routine declarations,
 * this file won't actually work in 80x86 small/medium model; at least,
 * you probably won't be able to process useful-size images in only 64KB.
 */

GLOBAL(void FAR*)
jpeg_get_large(j_common_ptr cinfo, size_t sizeofobject)
{
	return mjpg_malloc((mjpg_alloc*)cinfo->user, sizeofobject);
}

GLOBAL(void)
jpeg_free_large(j_common_ptr cinfo, void FAR * object, size_t sizeofobject)
{
	mjpg_free((mjpg_alloc*)cinfo->user, object);
}



typedef struct JpgDst
{
	mjpg_alloc *alloc;
	char* outbuffer;
	int len;
	int index;
}JpgDst;

int	convert24to32(int *dst, unsigned char *buffer,int width)
{
	int i;
	unsigned char* p=(unsigned char*)dst;	
	for(i=0;i<width;i++)
	{
		p[3]=255;
		p[2]=(*(buffer++));
		p[1]=(*(buffer++));
		p[0]=(*(buffer++));
		p+=4;
	}
	return 0;
}

int	convert8to32(int *dst, unsigned char *buffer,int width)
{
	int i;
	unsigned char* p=(unsigned char*)dst;	
	for(i=0;i<width;i++)
	{
		p[3]=255;
		p[2]=(*(buffer));
		p[1]=(*(buffer));
		p[0]=(*(buffer++));
		p+=4;
	}
	return 0;
}

struct jpg_error_mgr {
	struct jpeg_error_mgr pub;	/* "public" fields */
	jmp_buf setjmp_buffer;	/* for return to caller */
};
typedef struct jpg_error_mgr * jpg_error_ptr;

METHODDEF(void) jpg_error_exit (j_common_ptr cinfo)
{
	jpg_error_ptr jpgerr = (jpg_error_ptr) cinfo->err;
//fprintf(stderr,"jpg_error_exit\n");
	
	/*  (*cinfo->err->output_message) (cinfo);*/
	longjmp(jpgerr->setjmp_buffer, 1);
}

int *stdloadJpgEx(char *inbuffer,int size,int *w,int *h, void* fmalloc, void* ffree, void* user)
{
	struct jpeg_decompress_struct cinfo;
	struct jpg_error_mgr jerr;
	
	JSAMPARRAY buffer;		/* Output row buffer */
	int row_stride;		/* physical row width in output buffer */
	int *dst;
	int *bitmap;
	mjpg_alloc alloc;

	int i,j;
	
	alloc.user = user;
	alloc.malloc = fmalloc;
	alloc.free = ffree;

	cinfo.err = jpeg_std_error(&jerr.pub);
	cinfo.user = &alloc;
	jerr.pub.error_exit = jpg_error_exit;
	
	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_decompress(&cinfo);
		return NULL;
	}
	
	jpeg_create_decompress(&cinfo);
	
	jpeg_stdio_src(&cinfo, inbuffer,size);
	
	jpeg_read_header(&cinfo, TRUE);
	jpeg_start_decompress(&cinfo);
	
	i=cinfo.output_width;
	j=cinfo.output_height;
/*	printf("out_color_components=%d\n",cinfo.out_color_components);
	printf("output_components=%d\n",cinfo.output_components);
	printf("colormap=%d\n",cinfo.colormap);
	printf("actual_number_of_colors=%d\n",cinfo.actual_number_of_colors);
*/	
	*w=i;
	*h=j;
	
	bitmap=(int*)mjpg_malloc(&alloc,4*i*j);
	if (bitmap==NULL) return NULL;
	
	row_stride = cinfo.output_width * cinfo.output_components;
	
	buffer = (*cinfo.mem->alloc_sarray)
		((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);
	
	dst=(int*)bitmap;
	
	while (cinfo.output_scanline < cinfo.output_height)
	{
		jpeg_read_scanlines(&cinfo, buffer, 1);
		if (cinfo.output_components==1) convert8to32(dst,buffer[0],cinfo.output_width);
		else convert24to32(dst,buffer[0],cinfo.output_width);
		dst+=i;
	}
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);
	
	return bitmap;
}
int* stdloadJpg(char* inbuffer, int size, int* w, int* h)
{
	return stdloadJpgEx(inbuffer, size, w, h, NULL, NULL, NULL);
}

void stdloadJpgRelease(int* content)
{
	if (content) free(content);
}

void jpgDstWrite(void* user, JOCTET * buffer, size_t datacount)
{
	JpgDst* gd = (JpgDst*)user;
	char* src = (char*)buffer;
	int size = datacount;

	while (size + gd->index > gd->len)
	{
		char* newout = (char*)mjpg_malloc(gd->alloc, gd->len * 2);
		memcpy(newout, gd->outbuffer, gd->index);
		mjpg_free(gd->alloc, gd->outbuffer);
		gd->outbuffer = newout;
		gd->len *= 2;
	}
	memcpy(gd->outbuffer + gd->index, src, size);
	gd->index += size;
}

char* stdmakeJpgEx(unsigned char *start,int w,int h,int quality,int* size,int R,int G,int B, int INC, void* fmalloc, void* ffree, void* user)
{
	struct jpeg_compress_struct cinfo;
	struct jpg_error_mgr jerr;
	JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
	int row_stride; /* physical row width in output buffer */
	unsigned char *buf=NULL;
	int i,j;
	char* result;
	JpgDst jpgdst;
	mjpg_alloc alloc;

	alloc.user = user;
	alloc.malloc = fmalloc;
	alloc.free = ffree;

	cinfo.err = jpeg_std_error(&jerr.pub);
	cinfo.user = &alloc;

	jpgdst.alloc = &alloc;
	jpgdst.len = 4096;
	jpgdst.outbuffer = mjpg_malloc(&alloc, jpgdst.len);
	jpgdst.index = 0;

	jerr.pub.error_exit = jpg_error_exit;
	
	if (setjmp(jerr.setjmp_buffer)) {
		jpeg_destroy_compress(&cinfo);
		if (buf) free(buf);
		return NULL;
	}

	jpeg_create_compress(&cinfo);
	jpeg_stdio_dest(&cinfo, jpgDstWrite,(void*)&jpgdst);

	cinfo.image_width = w; 	/* image width and height, in pixels */
	cinfo.image_height = h;
	cinfo.input_components = 3;		/* # of color components per pixel */
	cinfo.in_color_space = JCS_RGB; 	/* colorspace of input image */
	jpeg_set_defaults(&cinfo);
	jpeg_set_quality(&cinfo, quality, TRUE /* limit to baseline-JPEG values */);

	jpeg_start_compress(&cinfo, TRUE);
	row_stride = w * 3;	/* JSAMPLEs per row in image_buffer */

	buf=(unsigned char*)mjpg_malloc(&alloc, row_stride);
	if (buf)
	{
		while (cinfo.next_scanline < cinfo.image_height)
		{
			j=0;
			for(i=0;i<row_stride;i+=3)
			{
				buf[i]=start[j+R];
				buf[i+1]=start[j+G];
				buf[i+2]=start[j+B];
				j+=INC;
			}

			row_pointer[0] = buf;
			jpeg_write_scanlines(&cinfo, row_pointer, 1);
			start+=w*INC;
		}
		jpeg_finish_compress(&cinfo);
		mjpg_free(&alloc,buf);
	}
	jpeg_destroy_compress(&cinfo);
	*size=jpgdst.index;
	result= jpgdst.outbuffer;
	return result;
}

char* stdmakeJpg(unsigned char* start, int w, int h, int quality, int* size, int R, int G, int B, int INC)
{
	return stdmakeJpgEx(start, w, h, quality, size, R, G, B, INC, NULL, NULL, NULL);
}

void stdmakeJpgRelease(char* content)
{
	MDELETE(content);
}
