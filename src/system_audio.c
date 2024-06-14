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

#ifdef WITH_AUDIO
//#define WITH_AUTO_STOP

#ifdef USE_ALSA
#include <alsa/asoundlib.h>
#endif

#ifdef USE_AUDIOTOOLBOX
#include<AudioToolbox/AudioToolbox.h>
#include<AudioToolbox/AudioQueue.h>
#include<CoreFoundation/CFRunLoop.h>
#endif

#ifdef USE_OPENSLES
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_Android.h>
#include <android/log.h>
#define  LOG_TAG    "Minimacy"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#endif

#define AUDIO_FREQ 44100
#define AUDIO_SLICE (AUDIO_FREQ/20)
#define AUDIO_VALUE 2
#define AUDIO_CHANNELS 2
#define AUDIO_BYTES_PER_SAMPLE (AUDIO_VALUE*AUDIO_CHANNELS)
#ifdef ON_RPIOS
#define AUDIO_NB_BUFFERS 4
#else
#define AUDIO_NB_BUFFERS 2
#endif
#define AUDIO_ZERO_VALUE 0

#define AUDIO_BUFFER (AUDIO_SLICE*AUDIO_BYTES_PER_SAMPLE)
#define AUDIO_INTERN_BUFFER (AUDIO_NB_BUFFERS* AUDIO_BUFFER)

int AudioPlaySerial = 0;

typedef struct AudioSound
{
	LB header;
	FORGET forget;
	MARK mark;

	LB* buffer;
	LINT len;	//nb of samples
	LINT i;	// -1:aborted
	int volume;		// 0 255
	LINT loop;	// -1:no loop
	struct AudioSound* next;
}AudioSound;

#ifdef USE_AUDIO_ENGINE
typedef struct
{
	HGLOBAL hWaveHdr;
	HANDLE hData;
	int serial;
}HAudioBuf;
#endif

typedef struct {
	LB header;
	FORGET forget;
	MARK mark;

	MLOCK lock;
	int playing;
	int playTimer;
	int playSerial;
	int masterVolume;

	AudioSound* sounds;
	int noSound;
	char playBuffer[AUDIO_BUFFER];
#ifdef USE_OPENSLES
	SLEngineItf engineEngine;
	SLObjectItf outputMixObject;

	SLObjectItf bqPlayerObject;
	SLVolumeItf bqPlayerVolume;

#endif

#ifdef USE_ALSA
	snd_pcm_t *player;
//	snd_pcm_t *rec;
#endif	
#ifdef USE_AUDIOTOOLBOX
	AudioQueueRef playQueue;
//	AudioQueueRef recQueue;
#endif
#ifdef USE_AUDIO_ENGINE
	HWAVEOUT hWaveOut;
//	HWAVEIN hWaveIn;
	HAudioBuf audioBuffers[AUDIO_NB_BUFFERS];
#endif
}AudioEngine;
AudioEngine* AE=NULL;

void audioSampleMark(LB* user)
{
	AudioSound* as = (AudioSound*)user;
	MEMORY_MARK(user, (LB*)as->buffer);
	MEMORY_MARK(user, (LB*)as->next);
}
int audioSystemForget(LB* user)
{
	AudioEngine* as = (AudioEngine*)user;
	//	PRINTF(LOG_DEV,"audioSystemForget\n");
	lockDelete(&as->lock);
	return 0;
}
void audioSystemMark(LB* user)
{
	AudioEngine* as = (AudioEngine*)user;
	MEMORY_MARK(user, (LB*)as->sounds);
}

int haudioPlayStop(AudioEngine* t);
int haudioPlayNextBuffer(AudioEngine* t, char* output);

//--------------------------------------------------------------------------
#ifdef USE_OPENSLES	// ANDROID
int sysPlayVolume(AudioEngine* t)
{
	int x=SL_MILLIBEL_MIN;
	if (t->masterVolume)
	{
		float y=t->masterVolume;
		y=1000.0f*log10(y/65535.0f);
		x=(int)y;
	}
//	LOGI("volumeaudio %d -> %d (%d)\n",t->playVolL,x,SL_MILLIBEL_MIN);
	if (t->bqPlayerVolume) (*t->bqPlayerVolume)->SetVolumeLevel(t->bqPlayerVolume, x);
	return 0;
}

int sysPlayStop(AudioEngine* t)	// called in the lock section t->lock
{
//PRINTF(LOG_DEV,"sysPlayStop\n");
	if (t->bqPlayerObject != NULL)
	{
		(*t->bqPlayerObject)->Destroy(t->bqPlayerObject);
		t->bqPlayerObject = NULL;
		t->bqPlayerVolume = NULL;
	}
	return 0;
}

int sysAudioInit(AudioEngine* t)
{
	t->engineEngine=NULL;
	t->bqPlayerObject=NULL;
	t->bqPlayerVolume=NULL;
	return 0;
}

int audioerror(char* str,int x)
{
	LOGI("%s : %d\n",str,x);
	return x;
}

int createEngine(AudioEngine* t)
{
    SLresult result;
	SLObjectItf engineObject;

    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
	if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR slCreateEngine",result);

    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
	if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR engineObject->Realize",result);

    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &t->engineEngine);
	if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR GetInterface",result);

    result = (*t->engineEngine)->CreateOutputMix(t->engineEngine, &t->outputMixObject, 0, NULL, NULL);
	if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR CreateOutputMix",result);

    result = (*t->outputMixObject)->Realize(t->outputMixObject, SL_BOOLEAN_FALSE);
	if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR outputMixObject->Realize",result);

	return 0;
}
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *user)
{
	AudioEngine* t=(AudioEngine*)user;

	lockEnter(&t->lock);
	haudioPlayNextBuffer(t,t->playBuffer);
	(*bq)->Enqueue(bq, t->playBuffer, AUDIO_BUFFER);
	lockLeave(&t->lock);
}
int sysPlayStart(AudioEngine* t)
{
	int i;
    SLresult result;
	SLPlayItf bqPlayerPlay;
	SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;

	if (t->engineEngine==NULL) createEngine(t);
	if (t->engineEngine==NULL) return -1;
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, AUDIO_NB_BUFFERS};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, AUDIO_CHANNELS, AUDIO_FREQ*1000,
								   AUDIO_VALUE*8, AUDIO_VALUE*8,
        0, SL_BYTEORDER_LITTLEENDIAN};
    SLDataSource audioSrc = {&loc_bufq, &format_pcm};

    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, t->outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};

    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND, SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    result = (*t->engineEngine)->CreateAudioPlayer(t->engineEngine, &t->bqPlayerObject, &audioSrc, &audioSnk,
            3, ids, req);
	if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR CreateAudioPlayer",result);

    // realize the player
    result = (*t->bqPlayerObject)->Realize(t->bqPlayerObject, SL_BOOLEAN_FALSE);
	if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR bqPlayerObject->Realize",result);

    // get the play interface
    result = (*t->bqPlayerObject)->GetInterface(t->bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
	if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR GetInterface->bqPlayerPlay",result);

    // get the buffer queue interface
    result = (*t->bqPlayerObject)->GetInterface(t->bqPlayerObject, SL_IID_BUFFERQUEUE,&bqPlayerBufferQueue);
	if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR GetInterface->bqPlayerBufferQueue",result);

    // register callback on the buffer queue
    result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, t);
	if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR RegisterCallback",result);

    // get the volume interface
    result = (*t->bqPlayerObject)->GetInterface(t->bqPlayerObject, SL_IID_VOLUME, &t->bqPlayerVolume);
	if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR GetInterface->bqPlayerVolume",result);
	sysPlayVolume(t);

    // set the player's state to playing
    result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
	if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR SetPlayState",result);
	for (i=0;i<AUDIO_NB_BUFFERS;i++) (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, t->playBuffer, AUDIO_BUFFER);

	return 0;
}
#endif	//USE_OPENSLES

//--------------------------------------------------------------------------
#ifdef USE_ALSA	// LINUX
int sysPlayVolume(AudioEngine* t)
{
	// on linux we should apply the master volume on each track before mixing
	return 0;
}
//USE_ALSA
int sysPlayStop(AudioEngine* t)	// called in the lock section t->lock
{
//PRINTF(LOG_DEV,"sysPlayStop\n");
	snd_pcm_close(t->player);
	t->player=NULL;
	return 0;
}

void* sysPlayAlsaThread(void* param)
{
	int err,k;
	AudioEngine* t=(AudioEngine*)param;
//	PRINTF(LOG_DEV,"nsamples=%d\n",nsamples);
	while(t->player)
	{
		if ((err = snd_pcm_wait (t->player, -1)) < 0) {
//			PRINTF(LOG_DEV, "poll failed (%s)\n", snd_strerror (err));
			snd_pcm_prepare(t->player);
		}
		if (!t->player) return NULL;	//possible if sysPlayStop occurs in another thread
		k=snd_pcm_avail(t->player);
//			PRINTF(LOG_DEV,"k=%d\n",k);
		if (k<0)
		{
//			PRINTF(LOG_DEV,"err %s\n",snd_strerror (k));
			snd_pcm_prepare(t->player);
		}
		else if (k>=AUDIO_SLICE)
		{
			lockEnter(&t->lock);
			haudioPlayNextBuffer(t,t->playBuffer);
			if (t->player) snd_pcm_writei(t->player, t->playBuffer, AUDIO_SLICE);
			lockLeave(&t->lock);
		}
//		PRINTF(LOG_DEV,"cb\n");
	}
	return NULL;
}

int sysPlayStart(AudioEngine* t)
{
	int err,exact_rate;
	snd_pcm_uframes_t exact_buffersize;
	snd_pcm_t *handle=NULL;
	snd_pcm_hw_params_t *hw_params=NULL;
	snd_pcm_sw_params_t *sw_params=NULL;
	char* device="plughw:0,0";

	if ((err = snd_pcm_open (&handle, device, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
		PRINTF(LOG_SYS,"> ALSA: cannot open audio device %s (%s)\n", 
			device,
			snd_strerror (err));
		goto cleanup;
	}
	if ((err = snd_pcm_hw_params_malloc (&hw_params)) < 0) {
		PRINTF(LOG_SYS,"> ALSA: cannot allocate hardware parameter structure (%s)\n",
			snd_strerror (err));
		goto cleanup;
	}
			 
	if ((err = snd_pcm_hw_params_any (handle, hw_params)) < 0) {
		PRINTF(LOG_SYS,"> ALSA: cannot initialize hardware parameter structure (%s)\n",
			snd_strerror (err));
		goto cleanup;
	}

	if ((err = snd_pcm_hw_params_set_access (handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		PRINTF(LOG_SYS,"> ALSA: cannot set access type (%s)\n",
			snd_strerror (err));
		goto cleanup;
	}

	if ((err = snd_pcm_hw_params_set_format (handle, hw_params,SND_PCM_FORMAT_S16_LE)) < 0) {
		PRINTF(LOG_SYS,"> ALSA: cannot set sample format (%s)\n",
			snd_strerror (err));
		goto cleanup;
	}
	exact_rate=AUDIO_FREQ;
	if ((err = snd_pcm_hw_params_set_rate_near (handle, hw_params, &exact_rate, 0)) < 0) {
		PRINTF(LOG_SYS,"> ALSA: cannot set sample rate (%s)\n",
			snd_strerror (err));
		goto cleanup;
	}
//	PRINTF(LOG_SYS,"> ALSA: exact_rate=%d\n",exact_rate);

	if ((err = snd_pcm_hw_params_set_channels (handle, hw_params, AUDIO_CHANNELS)) < 0) {
		PRINTF(LOG_SYS,"> ALSA: cannot set channel count (%s)\n",
			snd_strerror (err));
		goto cleanup;
	}
	if ((err = snd_pcm_hw_params_set_periods (handle, hw_params, AUDIO_NB_BUFFERS, 0)) < 0) {
		PRINTF(LOG_SYS,"> ALSA: cannot set periods (%s)\n",
			snd_strerror (err));
		goto cleanup;
	}
	exact_buffersize=AUDIO_NB_BUFFERS*AUDIO_SLICE;
	//PRINTF(LOG_SYS,"> ALSA: expected buffersize=%d %d %d %d %d\n",(int)exact_buffersize,nbBuffers,channelSize,nChannels,resolution);
	if ((err = snd_pcm_hw_params_set_buffer_size_near (handle, hw_params, &exact_buffersize)) < 0) {
		PRINTF(LOG_SYS,"> ALSA: cannot set buffer size (%s)\n",
			snd_strerror (err));
		goto cleanup;
	}
//	PRINTF(LOG_SYS,"> ALSA: exact_buffersize=%d\n",(int)exact_buffersize);

	if ((err = snd_pcm_hw_params (handle, hw_params)) < 0) {
		PRINTF(LOG_SYS,"> ALSA: cannot set parameters (%s)\n",
			snd_strerror (err));
		goto cleanup;
	}

	snd_pcm_hw_params_free (hw_params);
	hw_params=NULL;

	if ((err = snd_pcm_sw_params_malloc (&sw_params)) < 0) {
		PRINTF(LOG_SYS,"> ALSA: cannot allocate software parameter structure (%s)\n",
			snd_strerror (err));
		goto cleanup;
	}
	if ((err = snd_pcm_sw_params_current (handle, sw_params)) < 0) {
		PRINTF(LOG_SYS,"> ALSA: cannot current software parameter structure (%s)\n",
			snd_strerror (err));
		goto cleanup;
	}
	if ((err = snd_pcm_sw_params_set_start_threshold (handle, sw_params,0)) < 0) {
		PRINTF(LOG_SYS,"> ALSA: cannot initialize software start threshold (%s)\n",
			snd_strerror (err));
		goto cleanup;
	}
	if ((err = snd_pcm_sw_params_set_avail_min (handle, sw_params,AUDIO_SLICE)) < 0) {
		PRINTF(LOG_SYS,"> ALSA: cannot initialize software available min (%s)\n",
			snd_strerror (err));
		goto cleanup;
	}
	if ((err = snd_pcm_sw_params(handle, sw_params)) < 0) {
		PRINTF(LOG_SYS,"> ALSA: cannot initialize software parameter structure (%s)\n",
			snd_strerror (err));
		goto cleanup;
	}
	snd_pcm_sw_params_free (sw_params);
	sw_params=NULL;

	if ((err = snd_pcm_prepare (handle)) < 0) {
		PRINTF(LOG_SYS,"> ALSA: cannot prepare audio interface for use (%s)\n",
			snd_strerror (err));
		goto cleanup;
	}
	t->player=handle;
	hwThreadCreate(sysPlayAlsaThread,t);
	return 0;
cleanup:
	if (hw_params) snd_pcm_hw_params_free (hw_params);
	if (handle) snd_pcm_close(handle);
	return -1;
}
int sysAudioInit(AudioEngine* t)
{
	return 0;
}
#endif	//USE_ALSA
//--------------------------------------------------------------------------
#ifdef USE_AUDIOTOOLBOX	// MACOS, IOS
int sysPlayVolume(AudioEngine* t)
{
	float v=t->masterVolume;
	if (t->playQueue) AudioQueueSetParameter(t->playQueue,kAudioQueueParam_Volume,v/65535.0f);
	return 0;
}

int sysPlayStop(AudioEngine* t)	// called in the lock section t->lock
{
    	AudioQueueStop(t->playQueue,true);
	AudioQueueDispose(t->playQueue,true);
	return 0;
}

static void sysPlayCb(void *user,AudioQueueRef queue,AudioQueueBufferRef buffer)
{
	AudioEngine* t=AE;
	lockEnter(&t->lock);
	haudioPlayNextBuffer(t,buffer->mAudioData);
	buffer->mAudioDataByteSize = AUDIO_BUFFER;
	AudioQueueEnqueueBuffer(queue,buffer,0,NULL);
	lockLeave(&t->lock);
}

int sysPlayStart(AudioEngine* t)
{
	int i;
	AudioStreamBasicDescription audioFormat;
	AudioQueueBufferRef buffer;
	
	memset(&audioFormat,0,sizeof(audioFormat));
	audioFormat.mFormatID=kAudioFormatLinearPCM;
	audioFormat.mSampleRate=AUDIO_FREQ;
	audioFormat.mFormatFlags=kLinearPCMFormatFlagIsSignedInteger;
	audioFormat.mBytesPerPacket=AUDIO_BYTES_PER_SAMPLE;
	audioFormat.mFramesPerPacket=1;
	audioFormat.mBytesPerFrame=AUDIO_BYTES_PER_SAMPLE;
	audioFormat.mChannelsPerFrame=AUDIO_CHANNELS;
	audioFormat.mBitsPerChannel=AUDIO_VALUE*8;
	
	AudioQueueNewOutput(&audioFormat,sysPlayCb,t,NULL,kCFRunLoopCommonModes,0,&t->playQueue);
	sysPlayVolume(t);
//	PRINTF(LOG_DEV,"sysPlayStart masterVolume=%x\n", t->masterVolume);

	for(i=0;i<AUDIO_NB_BUFFERS;i++)
	{
		AudioQueueAllocateBuffer(t->playQueue,AUDIO_BUFFER,&buffer);
		memset(buffer->mAudioData,AUDIO_ZERO_VALUE,AUDIO_BUFFER);
		buffer->mAudioDataByteSize = AUDIO_BUFFER;
		AudioQueueEnqueueBuffer(t->playQueue,buffer,0,NULL);
	}
	AudioQueueStart(t->playQueue,NULL);
	return 0;
}
int sysAudioInit(AudioEngine* t)
{
	return 0;
}
#endif	// USE_AUDIOTOOLBOX
//--------------------------------------------------------------------------
#ifdef USE_AUDIO_ENGINE	// WINDOWS
int sysPlayVolume(AudioEngine* t)
{
	int masterVolume= (t->masterVolume << 16) + t->masterVolume;
	waveOutSetVolume(t->hWaveOut,masterVolume);
	return 0;
}

int sysPlayStop(AudioEngine* t)	// called in the lock section t->lock
{
	//	PRINTF(LOG_DEV,"sysPlayStop\n");
	AudioPlaySerial++;
	waveOutClose(t->hWaveOut);
	return 0;
}

void CALLBACK waveOutProc(HWAVEOUT m_hWO, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	AudioEngine* t = (AudioEngine*)dwInstance;
	if (uMsg == WOM_DONE)
	{
		LPWAVEHDR lpWaveHdr = (LPWAVEHDR)dwParam1;
		HAudioBuf* ha = (HAudioBuf*)lpWaveHdr->dwUser;
		//	PRINTF(LOG_DEV,"###audioEventPlayData\n");
		if (ha->serial != AudioPlaySerial)
		{
//			PRINTF(LOG_DEV,"###release audio buffer\n");
			GlobalUnlock(ha->hData);
			GlobalFree(ha->hData);
			GlobalUnlock(ha->hWaveHdr);
			GlobalFree(ha->hWaveHdr);
			return;
		}
		lockEnter(&t->lock);
		haudioPlayNextBuffer(t, lpWaveHdr->lpData);
		waveOutWrite(t->hWaveOut, lpWaveHdr, sizeof(WAVEHDR));
		lockLeave(&t->lock);
	}
}
int sysPlayStart(AudioEngine* t)
{
	UINT wResult;
	PCMWAVEFORMAT pcmWaveFormat;
	int i;

	pcmWaveFormat.wf.wFormatTag = WAVE_FORMAT_PCM;
	pcmWaveFormat.wf.nChannels = AUDIO_CHANNELS;
	pcmWaveFormat.wf.nSamplesPerSec = AUDIO_FREQ;
	pcmWaveFormat.wf.nAvgBytesPerSec = AUDIO_FREQ * AUDIO_BYTES_PER_SAMPLE;
	pcmWaveFormat.wf.nBlockAlign = AUDIO_BYTES_PER_SAMPLE;
	pcmWaveFormat.wBitsPerSample = AUDIO_VALUE*8;

	wResult = waveOutOpen((LPHWAVEOUT)&t->hWaveOut, WAVE_MAPPER, (LPWAVEFORMATEX)&pcmWaveFormat, (DWORD_PTR)waveOutProc, (DWORD_PTR)t, CALLBACK_FUNCTION);
	if (wResult != 0)	return -1;
	sysPlayVolume(t);
//	PRINTF(LOG_DEV,"sysPlayStart masterVolume=%x\n", t->masterVolume);
	for (i = 0; i < AUDIO_NB_BUFFERS; i++)
	{
		HPSTR lpData;
		LPWAVEHDR lpWaveHdr;
		HAudioBuf* ha = &t->audioBuffers[i];
		ha->serial = AudioPlaySerial;
		ha->hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, AUDIO_BUFFER);
		if (!ha->hData) return -1;
		lpData = (char*)GlobalLock(ha->hData);
		if (!lpData) return -1;
		ha->hWaveHdr = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, (DWORD)sizeof(WAVEHDR));
		if (ha->hWaveHdr == NULL) return -1;

		lpWaveHdr = (LPWAVEHDR)GlobalLock(ha->hWaveHdr);
		if (!lpWaveHdr) return -1;

		lpWaveHdr->lpData = lpData;
		lpWaveHdr->dwBufferLength = AUDIO_BUFFER;
		lpWaveHdr->dwUser = (DWORD_PTR)ha;
		lpWaveHdr->dwFlags = 0L;
		lpWaveHdr->dwLoops = 1L;
		wResult = waveOutPrepareHeader(t->hWaveOut, lpWaveHdr, sizeof(WAVEHDR));
		if (wResult != 0) return -1;
		memset(lpData, AUDIO_ZERO_VALUE, AUDIO_BUFFER);
		wResult = waveOutWrite(t->hWaveOut, lpWaveHdr, sizeof(WAVEHDR));
	}
	return 0;
}

int sysAudioInit(AudioEngine* t)
{
	return 0;
}
#endif	// USE_AUDIO_ENGINE
//--------------------------------------------------------------------------

// haudioPlayStop is only called by haudioPlayNextBuffer, and therefore in the lock section t->lock
int haudioPlayStop(AudioEngine* t)
{
	//LOGI("haudioPlayStop %d\n",t->playing);
	//PRINTF(LOG_DEV,"#################playstop\n");
	if (t->playing)
	{
		t->playing = 0;
		sysPlayStop(t);
		internalPoke();
	}
	return 0;
}

void haudioMix(short* out, short* in, int vol, int n)
{
	int i;
	//	PRINTF(LOG_DEV,"mix %d %d ", vol, n);
	for (i = 0; i < n; i++)
	{
		int v = out[i];
		int s = in[i];
		s = (s * vol) >> 8;
		v += s;
		if (v > 0x7ffff) v = 0x7fff;
		if (v < -0x7ffff) v = -0x7fff;
		out[i] = v;
	}
}

int haudioPlayNextBuffer(AudioEngine* t, char* output)
{
	short* out = (short*)output;
	AudioSound* snd;

	memset(output, 0, AUDIO_BUFFER);
//	PRINTF(LOG_DEV,"\nplay %d ", AUDIO_SLICE);

	snd = t->sounds;
#ifdef WITH_AUTO_STOP
    // audio device startup may be slow and slightly freezing
	if (!snd)
	{
		t->noSound++;
		//		PRINTF(LOG_DEV,"nosound %d",t->noSound);
#ifndef USE_OPENSLES
		if (t->noSound > 20) haudioPlayStop(t);	// we close only after a certain time of inactivity
#endif
		return 0;
	}
	t->noSound = 0;
#endif
    while (snd)
	{
		//		PRINTF(LOG_DEV,"sound %d ", snd->volume);
		if (snd->i >= 0)
		{
			short* buffer = (short*)STR_START(snd->buffer);
			int nbToPlay = (int)(snd->len - snd->i);
			if (nbToPlay > AUDIO_SLICE) nbToPlay = AUDIO_SLICE;
			//			printf("mix %llx %d %d %d\n", snd->buffer, snd->i, snd->len, nbToPlay);
			haudioMix(out, &buffer[snd->i * AUDIO_CHANNELS], snd->volume, nbToPlay * AUDIO_CHANNELS);
			snd->i += nbToPlay;
			if ((snd->i >= snd->len) && (snd->loop >= 0))
			{
				int nbToPlay2 = AUDIO_SLICE - nbToPlay;
				snd->i = snd->loop;
				haudioMix(&out[nbToPlay * AUDIO_CHANNELS], &buffer[snd->i * AUDIO_CHANNELS], snd->volume, nbToPlay2 * AUDIO_CHANNELS);
				snd->i += nbToPlay2;
			}
		}
		snd = snd->next;
	}

	snd = t->sounds;
	t->sounds = NULL;
	while (snd)
	{
		AudioSound* next = snd->next;
		if ((snd->i < snd->len) && (snd->i >= 0))
		{
			snd->next = t->sounds;
			t->sounds = snd;
		}
		snd = next;
	}
	return 0;
}

int haudioPlayStart(AudioEngine* t)
{
//    printf("haudioPlayStart\n");
	memset(t->playBuffer, AUDIO_ZERO_VALUE, AUDIO_BUFFER);

	t->noSound = 0;
	if (sysPlayStart(t)) return -1;

	t->playing = 1;
	t->playTimer = -1;
	return 0;
}
int fun_soundStart(Thread* th)
{
	int startNow;
	AudioSound* as;
	LINT len;

	int loopIsNil = STACK_IS_NIL(th,0);
	LINT loop = STACK_INT(th, 0);
	LFLOAT volume = STACK_FLOAT(th, 1);
	LB* src = STACK_PNT(th, 2);
	if (!src) FUN_RETURN_NIL;
	len = STR_LENGTH(src);
	if (len & 3) FUN_RETURN_NIL;

	volume *= 255;
	if (volume < 0) volume = 0;
	if (volume >255) volume = 255;
	as = (AudioSound*)memoryAllocExt(th, sizeof(AudioSound), DBG_BIN, NULL, audioSampleMark); if (!as) return EXEC_OM;
	as->buffer = src;
	as->len = len / 4;
	as->volume = (int)volume;
	as->i = 0;
	as->loop = loopIsNil ? -1 : loop;
	if (as->loop > as->len) as->loop = -1;
	MEMORY_MARK(NULL, (LB*)as->buffer);
	lockEnter(&AE->lock);
    startNow = AE->playing?0:1;
	MEMORY_MARK(NULL, (LB*)AE->sounds);
	as->next = AE->sounds;
	AE->sounds = as;
	lockLeave(&AE->lock);

	if (startNow) haudioPlayStart(AE);
	FUN_RETURN_PNT((LB*)as)
}
int fun_soundAbort(Thread* th)
{
	AudioSound* as= (AudioSound*)STACK_PNT(th, 0);
	if (as) as->i = -1;
	return 0;
}

int fun_soundPosition(Thread* th)
{
	AudioSound* as = (AudioSound*)STACK_PNT(th, 0);
	if ((!as) || (as->i < 0)) FUN_RETURN_NIL;
	FUN_RETURN_INT(as->i);
}

int fun_soundSetVolume(Thread* th)
{
	LFLOAT volume = STACK_PULL_FLOAT(th);
	AudioSound* as = (AudioSound*)STACK_PNT(th, 0);
	if (!as) return 0;

	volume *= 255;
	if (volume < 0) volume = 0;
	if (volume > 255) volume = 255;
	as->volume = (int)volume;
	return 0;
}

int fun_soundPlaying(Thread* th)
{
	FUN_RETURN_BOOL(AE->sounds);
}

#else	//WITH_AUDIO
int fun_soundStart(Thread* th) FUN_RETURN_NIL
int fun_soundAbort(Thread* th) FUN_RETURN_NIL
int fun_soundPosition(Thread* th) FUN_RETURN_NIL
int fun_soundSetVolume(Thread* th) FUN_RETURN_NIL
int fun_soundPlaying(Thread* th) FUN_RETURN_NIL
#endif	//WITH_AUDIO
int coreAudioInit(Thread* th, Pkg *system)
{
	Def* Sample= pkgAddType(th, system, "Sample");
	Type* fun_S_F_I_Sample = typeAlloc(th, TYPECODE_FUN, NULL, 4, MM.Str, MM.Float, MM.Int, Sample->type);
	Type* fun_Sample_Sample = typeAlloc(th, TYPECODE_FUN, NULL, 2, Sample->type, Sample->type);
	Type* fun_Bool = typeAlloc(th, TYPECODE_FUN, NULL, 1, MM.Boolean);
	Type* fun_Sample_I = typeAlloc(th, TYPECODE_FUN, NULL, 2, Sample->type, MM.Int);
	Type* fun_Sample_F_Sample = typeAlloc(th, TYPECODE_FUN, NULL, 3, Sample->type, MM.Float, Sample->type);

#ifdef WITH_AUDIO
	AE= (AudioEngine*)memoryAllocExt(th, sizeof(AudioEngine), DBG_BIN, audioSystemForget, audioSystemMark);
	AE->sounds = NULL;
	AE->noSound = 0;

	AE->masterVolume = 0x8000;
	AE->playing = 0;
	lockCreate(&AE->lock);
	memoryAddRoot((LB*)AE);
	sysAudioInit(AE);
#endif
	pkgAddFun(th, system, "soundStart", fun_soundStart, fun_S_F_I_Sample);
	pkgAddFun(th, system, "soundAbort", fun_soundAbort, fun_Sample_Sample);
	pkgAddFun(th, system, "soundPosition", fun_soundPosition, fun_Sample_I);
	pkgAddFun(th, system, "soundSetVolume", fun_soundSetVolume, fun_Sample_F_Sample);
	pkgAddFun(th, system, "soundPlaying", fun_soundPlaying, fun_Bool);

	return 0;
}
