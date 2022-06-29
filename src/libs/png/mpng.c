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
#include"png.h"
#ifndef png_jmpbuf
#  define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#endif

typedef struct {
	void* user;
	void* (*malloc)(void*, int);
	void (*free)(void*, void*);
}mpng_alloc;

static png_voidp mpng_malloc(png_structp png_ptr, png_size_t size)
{
	mpng_alloc* alloc = (mpng_alloc*)png_ptr->mem_ptr;

	png_voidp block = alloc->malloc?(*alloc->malloc)(alloc->user, (int)size):malloc(size);
//	printf(">>>>>>png malloc %llx : %d bytes -> %llx\n", png_ptr->mem_ptr, (int)size,block);
	return block;
}
static void mpng_free(png_structp png_ptr, png_voidp block)
{
	mpng_alloc* alloc = (mpng_alloc*)png_ptr->mem_ptr;
//	printf(">>>>>>png free %llx\n", block);
	if (alloc->free) (*alloc->free)(alloc->user, block);
	else if (!alloc->malloc) free(block);
//	else printf(">>>>>>png free nothing\n");
}

typedef struct{
	char *inbuf;
	unsigned int i;
	unsigned int len;
}mpng_reader;

static void mpng_read_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
	mpng_reader *reader=(mpng_reader*)png_ptr->io_ptr;
	if (length>reader->len-reader->i) length=reader->len-reader->i;
	if (length) memcpy(data,reader->inbuf+reader->i,length);
	reader->i+=length;
}

int* stdloadPngEx(char *inbuffer,int size,int *w,int *h, void* fmalloc, void* ffree, void* user)
{
   png_structp png_ptr;
   png_infop info_ptr;
   png_uint_32 width, height;
   png_bytepp row_pointers;
   int bit_depth, color_type;
   mpng_reader reader;
   mpng_alloc alloc;
   int* bitmap;
   unsigned int i,j;
   
   if ((size<8)||(!png_check_sig(inbuffer, 8))) return NULL;

	alloc.user = user;
	alloc.malloc = fmalloc;
	alloc.free = ffree;

//   png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
   png_ptr = png_create_read_struct_2(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL, &alloc, mpng_malloc, mpng_free);

   if (png_ptr == NULL)
   {
      return NULL;
   }

   // Allocate/initialize the memory for image information.  REQUIRED. 
   info_ptr = png_create_info_struct(png_ptr);
   if (info_ptr == NULL)
   {
      png_destroy_read_struct(&png_ptr, png_infopp_NULL, png_infopp_NULL);
      return NULL;
   }
/*   if (setjmp(png_jmpbuf(png_ptr)))
   {
      png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
      return NULL;
   }
 */  reader.inbuf=inbuffer;
   reader.i=8;
   reader.len=size;

   png_set_read_fn(png_ptr, (void *)&reader, mpng_read_data);
   
   png_set_sig_bytes(png_ptr, 8);
   png_read_png(png_ptr, info_ptr, PNG_TRANSFORM_EXPAND|PNG_TRANSFORM_BGR, png_voidp_NULL);
   png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
      NULL, NULL, NULL);
   if (((color_type!=PNG_COLOR_TYPE_RGB_ALPHA)&&(color_type!=PNG_COLOR_TYPE_RGB))
	   ||(bit_depth!=8))
   {
      png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);
      return NULL;
   }
   *w=width;
   *h=height;

   bitmap= (int*) mpng_malloc(png_ptr, width * height * 4);

   row_pointers = png_get_rows(png_ptr, info_ptr);
   for(i=0;i<height;i++)
   {
	   if (color_type==PNG_COLOR_TYPE_RGB_ALPHA) memcpy(&bitmap[width*i],row_pointers[i],width*4);
	   else for(j=0;j<width;j++)
		   bitmap[width*i+j]=((255&row_pointers[i][j*3]))+((255&row_pointers[i][j*3+1])<<8)+((255&row_pointers[i][j*3+2])<<16)+0xff000000;
   }
   png_destroy_read_struct(&png_ptr, &info_ptr, png_infopp_NULL);

   return bitmap;
}

int* stdloadPng(char* inbuffer, int size, int* w, int* h)
{
	return stdloadPngEx(inbuffer, size, w, h, NULL, NULL, NULL);
}

void stdloadPngRelease(int* content)
{
	if (content) free(content);
}

typedef struct mpng_writer{
	char *outbuf;
	unsigned int i;
	unsigned int len;
}mpng_writer;

static void mpng_write_data(png_structp png_ptr, png_bytep data, png_size_t length)
{
	mpng_writer *writer=(mpng_writer*)png_ptr->io_ptr;
	if (length>writer->len-writer->i)
	{
		char* newbuf;
		writer->len=2*writer->len;
		if (writer->len<writer->i+length) writer->len=writer->i+length;
		newbuf=(char*)mpng_malloc(png_ptr,writer->len);
		if (writer->i) memcpy(newbuf,writer->outbuf,writer->i);
		mpng_free(png_ptr,writer->outbuf);
		writer->outbuf=newbuf;
	}
	memcpy(writer->outbuf+writer->i,data,length);
	writer->i+=length;
}

void mpng_flush_data(png_structp png_ptr)
{
	int x=1;
	x+=213;
}

char* stdmakePngEx(char* pixels, int* size, int w, int h, int alphaok, int mode, void* fmalloc, void* ffree, void* user)
{
	png_structp png_ptr;
	png_infop info_ptr;
	png_bytepp row_pointers;
	png_bytep row_data;
	png_bytep p;
	mpng_writer writer;
	mpng_alloc alloc;
	int color_type,bit_depth,i,j,bytes;

	alloc.user = user;
	alloc.malloc = fmalloc;
	alloc.free = ffree;

	png_ptr = png_create_write_struct_2(PNG_LIBPNG_VER_STRING, (png_voidp)NULL, NULL, NULL, &alloc, mpng_malloc, mpng_free);
	if (!png_ptr) return NULL;
	
	info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_write_struct(&png_ptr,(png_infopp)NULL);
		return NULL;
	}
/*	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_write_struct(&png_ptr, &info_ptr);
		return NULL;
	}
*/	
	writer.len = 16384;
	writer.outbuf = (char*)mpng_malloc(png_ptr, writer.len);
	writer.i= 0; 

	png_set_write_fn(png_ptr,(voidp)&writer,(png_rw_ptr)mpng_write_data,
                    (png_flush_ptr)mpng_flush_data);
	bit_depth=8;
	color_type=alphaok?PNG_COLOR_TYPE_RGB_ALPHA:PNG_COLOR_TYPE_RGB;

	png_set_IHDR(png_ptr, info_ptr, w, h,
		bit_depth, color_type, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);


	bytes=alphaok?4:3;
	row_pointers = (png_bytepp)mpng_malloc(png_ptr, h * sizeof(png_bytep));
	row_data = (png_bytep)mpng_malloc(png_ptr, h * w * bytes);
	
	p = row_data;
    for (j=0;j<h;j++)
	{
		row_pointers[j]=p;
		if (alphaok) 
		{
			if (mode) 
			{
				for(i=0;i<w;i++)
				{
					p[i*4]=pixels[i*4+2];
					p[i*4+1]=pixels[i*4+1];
					p[i*4+2]=pixels[i*4];
					p[i*4+3]=pixels[i*4+3];
				}
			}
			else memcpy(p,pixels,w*4);
		}
		else for(i=0;i<w;i++)
		{
			if (mode)
			{
				p[i*3]=pixels[i*4+2];
				p[i*3+1]=pixels[i*4+1];
				p[i*3+2]=pixels[i*4];
			}
			else
			{
				p[i*3]=pixels[i*4];
				p[i*3+1]=pixels[i*4+1];
				p[i*3+2]=pixels[i*4+2];
			}
		}
		pixels+=4*w;
		p += w * bytes;
	}
    png_set_rows(png_ptr, info_ptr, row_pointers);

    png_write_png(png_ptr, info_ptr, PNG_TRANSFORM_BGR, NULL);

	mpng_free(png_ptr, row_data);
	mpng_free(png_ptr, row_pointers);
	png_destroy_write_struct(&png_ptr, &info_ptr);
	if (size) *size=writer.i;
    return writer.outbuf;
}
char* stdmakePng(char* pixels, int* size, int w, int h, int alphaok, int mode)
{
	return stdmakePngEx(pixels, size, w, h, alphaok, mode, NULL, NULL, NULL);
}
void stdmakePngRelease(char* content)
{
	if (content) free(content);
}
