// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"




#ifdef WITH_DECODE_MP3
#define MINIMP3_IMPLEMENTATION
#include"../tmpSrc/minimp3.h"

WORKER_START _mp3Decode(volatile Thread* th)
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
	bufferSetWorkerThread(&out, &th);
	input_buf = STR_START(src);
	buf_size = (int)STR_LENGTH(src);
	mp3dec_init(&mp3d);
	do {
		samples = mp3dec_decode_frame(&mp3d, input_buf, buf_size, pcm, &info);
//		printf("frame: samples=%d frame_bytes=%d\n", samples, info.frame_bytes);
		input_buf += info.frame_bytes;
		buf_size -= info.frame_bytes;
		if (samples) {
			if (bufferAddBinWorker(&out, (char*)pcm, 2 * info.channels * samples)) {
				bufferUnsetWorkerThread(&out, &th);
				return workerDoneNil(th);
			}
		}
	} while (samples || info.frame_bytes);
	bufferUnsetWorkerThread(&out, &th);
	return workerDonePnt(th, MM._true);
}

int fun_mp3Decode(Thread* th) { return workerStart(th, 2, _mp3Decode); }
int usrMp3Init(Pkg* system)
{
	static const Native nativeDefs[] = {
		{ NATIVE_FUN, "_mp3Decode", fun_mp3Decode, "fun Buffer Str -> Bool"},
	};
	NATIVE_DEF(nativeDefs);

	return 0;
}
#else
int usrMp3Init(Pkg* system)
{
	return 0;
}
#endif


#ifdef WITH_SECTOR_STORAGE
int fun_storageRead(Thread* th)
{
	LINT len;
	LB* bytes;
	
	LINT nb = STACK_INT(th, 0);
	LINT start = STACK_INT(th, 1);
	LINT index = STACK_INT(th, 2);	// index of storage device
	if ((start < 0)||(nb<=0)) FUN_RETURN_NIL;
	if (index<0 || index>=storageCount()) FUN_RETURN_NIL;
	FUN_PUSH_STR(NULL, nb*storageSectorSize(index));

	bytes = STACK_PNT(th,0);
	len = storageRead(index, STR_START(bytes), (int)start, (int)nb);
	if (len < nb*storageSectorSize(index)) FUN_RETURN_NIL;
	FUN_RETURN_PNT(bytes);
}

int fun_storageWrite(Thread* th)
{
	LINT len;

	LB* bytes = STACK_PNT(th, 0);
	LINT start = STACK_INT(th, 1);
	LINT index = STACK_INT(th, 2);	// index of storage device
	if ((!bytes)||(start < 0)) FUN_RETURN_NIL;
	if (index<0 || index>=storageCount()) FUN_RETURN_NIL;
	len = storageWrite(index, STR_START(bytes), (int)start, STR_LENGTH(bytes));
	if (len < 0) FUN_RETURN_NIL;
	FUN_RETURN_INT(len);
}
int fun_storageNbSectors(Thread* th)
{
	LINT index = STACK_INT(th, 0);	// index of storage device
	LINT value=storageNbSectors(index);
	if (value<0) FUN_RETURN_NIL;
	FUN_RETURN_INT(value);
}
int fun_storageSectorSize(Thread* th)
{
	LINT index = STACK_INT(th, 0);	// index of storage device
	LINT value=storageSectorSize(index);
	if (value<0) FUN_RETURN_NIL;
	FUN_RETURN_INT(value);
}
int fun_storageWritable(Thread* th)
{
	LINT index = STACK_INT(th, 0);	// index of storage device
	LINT value=storageWritable(index);
	if (value<0) FUN_RETURN_NIL;
	FUN_RETURN_BOOL(value);
}
int fun_storageCount(Thread* th)
{
	FUN_RETURN_INT(storageCount());
}

int usrSdInit(Pkg* system)
{
	static const Native nativeDefs[] = {
		{ NATIVE_FUN, "storageCount", fun_storageCount, "fun -> Int"},
		{ NATIVE_FUN, "storageWritable", fun_storageWritable, "fun Int -> Bool"},
		{ NATIVE_FUN, "storageNbSectors", fun_storageNbSectors, "fun Int -> Int"},
		{ NATIVE_FUN, "storageSectorSize", fun_storageSectorSize, "fun Int -> Int"},
		{ NATIVE_FUN, "storageRead", fun_storageRead, "fun Int Int Int -> Bytes"},
		{ NATIVE_FUN, "storageWrite", fun_storageWrite, "fun Int Int Bytes -> Int"},
	};
	NATIVE_DEF(nativeDefs);
	return 0;
}
#else
int usrSdInit(Pkg* system){ return 0; }
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

int usrActivityLedInit(Pkg* system)
{
	static const Native nativeDefs[] = {
		{ NATIVE_FUN, "activityLed", fun_activityLed, "fun Bool -> Bool"},
	};
	NATIVE_DEF(nativeDefs);

	return 0;
}

int systemTmpInit(Pkg* system)
{
	usrSdInit(system);
	usrActivityLedInit(system);
	usrMp3Init(system);
#ifdef USE_HOST_ONLY_FUNCTIONS
	hostOnlyFunctionsInit(system);
#endif
	return 0;
}

