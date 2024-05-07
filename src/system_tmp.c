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




#ifdef WITH_DECODE_MP3
#define MINIMP3_IMPLEMENTATION
#include"../tmpSrc/minimp3.h"

MTHREAD_START _mp3Decode(Thread* th)
{
	unsigned char* input_buf;
	int buf_size = -1;
	static mp3dec_t mp3d;
	mp3dec_frame_info_t info;
	int samples;
	short pcm[MINIMP3_MAX_SAMPLES_PER_FRAME];

	LB* src = STACK_PNT(th, 0);
	Buffer* out = (Buffer*)STACK_PNT(th, 1);
	if ((!src) || (!out)) return workerDonePnt(th, MM._false);
	input_buf = STR_START(src);
	buf_size = (int)STR_LENGTH(src);
	mp3dec_init(&mp3d);
	do {
		samples = mp3dec_decode_frame(&mp3d, input_buf, buf_size, pcm, &info);
//		printf("frame: samples=%d frame_bytes=%d\n", samples, info.frame_bytes);
		input_buf += info.frame_bytes;
		buf_size -= info.frame_bytes;
		if (samples) {
			if (bufferAddBin(th, out, (char*)pcm, 2 * info.channels * samples)) return workerDoneNil(th);
		}
	} while (samples || info.frame_bytes);

	return workerDonePnt(th, MM._true);
}

int fun_mp3Decode(Thread* th) { return workerStart(th, 2, _mp3Decode); }
int usrMp3Init(Thread* th, Pkg* system)
{
	pkgAddFun(th, system, "_mp3Decode", fun_mp3Decode, typeAlloc(th, TYPECODE_FUN, NULL, 3, MM.Buffer, MM.Str, MM.Boolean));
	return 0;
}
#else
int usrMp3Init(Thread* th, Pkg* system)
{
	return 0;
}
#endif


#ifdef WITH_SECTOR_STORAGE
int fun_storageRead(Thread* th)
{
	LINT len;
	
	LINT nb = STACK_INT(th, 0);
	LINT start = STACK_INT(th, 1);
	LINT offset = STACK_INT(th, 2);
	LB* bytes = STACK_PNT(th, 3);
	LINT index = STACK_INT(th, 4);	// index of storage device
	if ((!bytes)||(offset<0)||(start < 0)||(nb<=0)) FUN_RETURN_NIL;
	if (offset+512*nb>STR_LENGTH(bytes)) FUN_RETURN_NIL;
	len = storageRead(index, STR_START(bytes)+offset, (int)start, (int)nb);
	if (len < 0) FUN_RETURN_NIL;
	FUN_RETURN_INT(len);
}

int fun_storageWrite(Thread* th)
{
	LINT len;

	LINT nb = STACK_INT(th, 0);
	LINT start = STACK_INT(th, 1);
	LINT offset = STACK_INT(th, 2);
	LB* bytes = STACK_PNT(th, 3);
	LINT index = STACK_INT(th, 4);	// index of storage device
	if ((!bytes)||(offset<0)||(start < 0)||(nb<=0)) FUN_RETURN_NIL;
	if (offset+512*nb>STR_LENGTH(bytes)) FUN_RETURN_NIL;
	len = storageWrite(index, STR_START(bytes)+offset, (int)start, (int)nb);
	if (len < 0) FUN_RETURN_NIL;
	FUN_RETURN_INT(len);
}

int usrSdInit(Thread* th, Pkg* system)
{
	pkgAddFun(th, system, "storageRead", fun_storageRead, typeAlloc(th, TYPECODE_FUN, NULL, 6, MM.Int, MM.Bytes, MM.Int,  MM.Int, MM.Int, MM.Int));
	pkgAddFun(th, system, "storageWrite", fun_storageWrite, typeAlloc(th, TYPECODE_FUN, NULL, 6, MM.Int, MM.Bytes, MM.Int, MM.Int, MM.Int, MM.Int));
	return 0;
}
#else
int usrSdInit(Thread* th, Pkg* system){ return 0; }
#endif

#ifdef WITH_ACTIVITY_LED
int fun_activityLed(Thread* th)
{
	LB* val=STACK_PNT(th, 0);
	hwActivityLedSet((val==MM._true)?1:0);
	return 0;
}
#else
int fun_activityLed(Thread* th) FUN_RETURN_NIL;
#endif

int usrActivityLedInit(Thread* th, Pkg* system)
{
	pkgAddFun(th, system, "activityLed", fun_activityLed, typeAlloc(th, TYPECODE_FUN, NULL, 2, MM.Boolean, MM.Boolean));
	return 0;
}

int usrUefiInit(Thread* th, Pkg* system);
int usrRpiInit(Thread* th, Pkg* system);

int tmpInit(Thread* th, Pkg* system)
{
	usrSdInit(th, system);
	usrActivityLedInit(th, system);
	usrMp3Init(th, system);
#ifdef ON_UEFI
	usrUefiInit(th, system);
#endif
#ifdef GROUP_RPI
	usrRpiInit(th, system);
#endif
	return 0;
}

