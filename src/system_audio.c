// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include"minimacy.h"

#ifdef WITH_AUDIO

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
#define AUDIO_NB_BUFFERS 2
#define REC_NB_BUFFERS 2
#define AUDIO_ZERO_VALUE 0

#define AUDIO_BUFFER (AUDIO_SLICE*AUDIO_BYTES_PER_SAMPLE)

#ifdef USE_AUDIO_ENGINE
typedef struct
{
	HGLOBAL hWaveHdr;
	HANDLE hData;
	int serial;
}HAudioBuf;
#endif

typedef struct {
	volatile int playing;
	int playSerial;
	int playEventId;
	int playIndex;
	float masterVolume;

	char playBuffer[AUDIO_BUFFER];

	volatile int recording;
	int recSerial;
	int recEventId;
	int recBufferLength;

#ifdef USE_OPENSLES
	SLEngineItf engineEngine;
	SLObjectItf outputMixObject;

	SLObjectItf bqPlayerObject;
	SLVolumeItf bqPlayerVolume;

    SLObjectItf recorderObject;
    char playBuffers[AUDIO_NB_BUFFERS][AUDIO_BUFFER];
    int currentAudioBuffer;
    char recBuffers[REC_NB_BUFFERS][AUDIO_BUFFER];
    int currentRecBuffer;
#endif

#ifdef USE_ALSA
	snd_pcm_t *player;
	snd_pcm_t *rec;
	char recBuffer[AUDIO_BUFFER];
#endif	
#ifdef USE_AUDIOTOOLBOX
	AudioQueueRef playQueue;
	AudioQueueRef recQueue;
#endif
#ifdef USE_AUDIO_ENGINE
	HWAVEOUT hWaveOut;
	HWAVEIN hWaveIn;
	HAudioBuf audioBuffers[AUDIO_NB_BUFFERS];
	HAudioBuf recBuffers[REC_NB_BUFFERS];
#endif
} AudioEngine;
AudioEngine _AE;
AudioEngine* AE=&_AE;
MLOCK AELock;

void haudioAckBuffer(void);
void haudioRecBuffer(char* src);
//--------------------------------------------------------------------------
#ifdef USE_OPENSLES	// ANDROID
int sysPlayVolume()
{
	int x=SL_MILLIBEL_MIN;
	if (AE->masterVolume)
	{
		float y=1000.0f*log10(AE->masterVolume);
		x=(int)y;
	}
//	LOGI("volumeaudio %d -> %d (%d)\n",AE->playVolL,x,SL_MILLIBEL_MIN);
	if (AE->bqPlayerVolume) (*AE->bqPlayerVolume)->SetVolumeLevel(AE->bqPlayerVolume, x);
	return 0;
}

int audioerror(char* str,int x)
{
	LOGI("%s : %d\n",str,x);
	return x;
}

int createEngine()
{
    SLresult result;
	SLObjectItf engineObject;

    result = slCreateEngine(&engineObject, 0, NULL, 0, NULL, NULL);
	if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR slCreateEngine",result);

    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
	if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR engineObject->Realize",result);

    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE, &((AudioEngine*)AE)->engineEngine);
	if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR GetInterface",result);

    result = (*AE->engineEngine)->CreateOutputMix(AE->engineEngine, &((AudioEngine*)AE)->outputMixObject, 0, NULL, NULL);
	if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR CreateOutputMix",result);

    result = (*AE->outputMixObject)->Realize(AE->outputMixObject, SL_BOOLEAN_FALSE);
	if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR outputMixObject->Realize",result);

	return 0;
}
void bqPlayerCallback(SLAndroidSimpleBufferQueueItf bq, void *user)
{
	if (!AE->playing) return;
	lockEnter(&AELock);
    memcpy(AE->playBuffers[AE->currentAudioBuffer],AE->playBuffer,AUDIO_BUFFER);
	(*bq)->Enqueue(bq, AE->playBuffers[AE->currentAudioBuffer++], AUDIO_BUFFER);
    if (AE->currentAudioBuffer>=AUDIO_NB_BUFFERS) AE->currentAudioBuffer=0;
	haudioAckBuffer();
	lockLeave(&AELock);
}
int sysPlayStart()
{
	int i;
    SLresult result;
	SLPlayItf bqPlayerPlay;
	SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue;

	if (AE->engineEngine==NULL) createEngine();
	if (AE->engineEngine==NULL) return -1;
    SLDataLocator_AndroidSimpleBufferQueue loc_bufq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, AUDIO_NB_BUFFERS};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, AUDIO_CHANNELS, AUDIO_FREQ*1000,
								   AUDIO_VALUE*8, AUDIO_VALUE*8,
        0, SL_BYTEORDER_LITTLEENDIAN};
    SLDataSource audioSrc = {&loc_bufq, &format_pcm};

    SLDataLocator_OutputMix loc_outmix = {SL_DATALOCATOR_OUTPUTMIX, AE->outputMixObject};
    SLDataSink audioSnk = {&loc_outmix, NULL};

    const SLInterfaceID ids[3] = {SL_IID_BUFFERQUEUE, SL_IID_EFFECTSEND, SL_IID_VOLUME};
    const SLboolean req[3] = {SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE, SL_BOOLEAN_TRUE};

    result = (*AE->engineEngine)->CreateAudioPlayer(AE->engineEngine, &((AudioEngine*)AE)->bqPlayerObject, &audioSrc, &audioSnk,
            3, ids, req);
	if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR CreateAudioPlayer",result);

    // realize the player
    result = (*AE->bqPlayerObject)->Realize(AE->bqPlayerObject, SL_BOOLEAN_FALSE);
	if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR bqPlayerObject->Realize",result);

    // get the play interface
    result = (*AE->bqPlayerObject)->GetInterface(AE->bqPlayerObject, SL_IID_PLAY, &bqPlayerPlay);
	if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR GetInterface->bqPlayerPlay",result);

    // get the buffer queue interface
    result = (*AE->bqPlayerObject)->GetInterface(AE->bqPlayerObject, SL_IID_BUFFERQUEUE,&bqPlayerBufferQueue);
	if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR GetInterface->bqPlayerBufferQueue",result);

    // register callback on the buffer queue
    result = (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, NULL);
	if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR RegisterCallback",result);

    // get the volume interface
    result = (*AE->bqPlayerObject)->GetInterface(AE->bqPlayerObject, SL_IID_VOLUME, &((AudioEngine*)AE)->bqPlayerVolume);
	if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR GetInterface->bqPlayerVolume",result);
	sysPlayVolume();

    // set the player's state to playing
    result = (*bqPlayerPlay)->SetPlayState(bqPlayerPlay, SL_PLAYSTATE_PLAYING);
	if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR SetPlayState",result);
    AE->currentAudioBuffer=0;
	for (i=0;i<AUDIO_NB_BUFFERS;i++) {
        memset(AE->playBuffers[i], AUDIO_ZERO_VALUE, AUDIO_BUFFER);
        (*bqPlayerBufferQueue)->Enqueue(bqPlayerBufferQueue, AE->playBuffers[i], AUDIO_BUFFER);
    }
	return 0;
}
int sysPlayStop()	// called in the lock section AE->lock
{
	//PRINTF(LOG_DEV,"sysPlayStop\n");
	if (AE->bqPlayerObject != NULL)
	{
		(*AE->bqPlayerObject)->Destroy(AE->bqPlayerObject);
		AE->bqPlayerObject = NULL;
		AE->bqPlayerVolume = NULL;
	}
	return 0;
}

void bqRecorderCallback(SLAndroidSimpleBufferQueueItf bq, void *user)
{
    if (!AE->recording) return;
    haudioRecBuffer(AE->recBuffers[AE->currentRecBuffer]);
    (*bq)->Enqueue(bq,  AE->recBuffers[AE->currentRecBuffer++], AE->recBufferLength);
    if (AE->currentRecBuffer>=REC_NB_BUFFERS) AE->currentRecBuffer=0;
}
int sysRecStart(int nChannels)
{
    int i;
    SLresult result;
    SLRecordItf recorderRecord;
    SLAndroidSimpleBufferQueueItf recorderBufferQueue;

    if (AE->engineEngine==NULL) createEngine();
    if (AE->engineEngine==NULL) return -1;

    SLDataLocator_IODevice loc_dev = {SL_DATALOCATOR_IODEVICE, SL_IODEVICE_AUDIOINPUT,
                                      SL_DEFAULTDEVICEID_AUDIOINPUT, NULL};
    SLDataSource audioSrc = {&loc_dev, NULL};

    // configure audio sink
    SLDataLocator_AndroidSimpleBufferQueue loc_bq = {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, REC_NB_BUFFERS};
    SLDataFormat_PCM format_pcm = {SL_DATAFORMAT_PCM, nChannels, AUDIO_FREQ*1000,
                                   AUDIO_VALUE*8, AUDIO_VALUE*8,
                                   (nChannels==1)?SL_SPEAKER_FRONT_CENTER:0, SL_BYTEORDER_LITTLEENDIAN};
    SLDataSink audioSnk = {&loc_bq, &format_pcm};

    // create audio recorder
    // (requires the RECORD_AUDIO permission)
    const SLInterfaceID id[1] = {SL_IID_ANDROIDSIMPLEBUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};
    result = (*AE->engineEngine)->CreateAudioRecorder(AE->engineEngine, &AE->recorderObject, &audioSrc,
                                                     &audioSnk, 1, id, req);
    if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR CreateAudioRecorder",result);

    // realize the audio recorder
    result = (*AE->recorderObject)->Realize(AE->recorderObject, SL_BOOLEAN_FALSE);
    if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR Realize",result);

    // get the record interface
    result = (*AE->recorderObject)->GetInterface(AE->recorderObject, SL_IID_RECORD, &recorderRecord);
    if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR GetInterface",result);

    // get the buffer queue interface
    result = (*AE->recorderObject)->GetInterface(AE->recorderObject, SL_IID_ANDROIDSIMPLEBUFFERQUEUE,
                                                &recorderBufferQueue);
    if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR GetInterface",result);

    // register callback on the buffer queue
    result = (*recorderBufferQueue)->RegisterCallback(recorderBufferQueue, bqRecorderCallback, NULL);
    if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR RegisterCallback",result);

    result = (*recorderRecord)->SetRecordState(recorderRecord, SL_RECORDSTATE_RECORDING);
    if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR GetInterface",result);

    AE->currentRecBuffer=0;
    for (i=0;i<REC_NB_BUFFERS;i++) {
        memset(AE->recBuffers[i], AUDIO_ZERO_VALUE, AE->recBufferLength);
        result=(*recorderBufferQueue)->Enqueue(recorderBufferQueue, AE->recBuffers[i], AE->recBufferLength);
        if (result!=SL_RESULT_SUCCESS) return audioerror("#ERR Enqueue",result);
    }

    return 0;
}
int sysRecStop(void)
{
    if (AE->recorderObject != NULL) {
        (*AE->recorderObject)->Destroy(AE->recorderObject);
        AE->recorderObject = NULL;
    }
}

int sysAudioInit()
{
	AE->engineEngine = NULL;
	AE->bqPlayerObject = NULL;
	AE->bqPlayerVolume = NULL;
	return 0;
}
void sysAudioClose(void)
{
}

#endif	//USE_OPENSLES

//--------------------------------------------------------------------------
#ifdef USE_ALSA	// LINUX
int sysPlayVolume()
{
	// on linux we apply the master volume on each track before mixing
	return 0;
}

void* sysPlayAlsaThread(void* param)
{
	int err,k;
	while(1)
	{
		if (!AE->playing) break;
		if (!AE->player) break;
		err = snd_pcm_wait(AE->player, -1);
		if (!AE->playing) break;
		if (err < 0) snd_pcm_prepare(AE->player);
		k=snd_pcm_avail(AE->player);
		if (k<0) snd_pcm_prepare(AE->player);
		else if (k>=AUDIO_SLICE)
		{
			int i;
			int n=AUDIO_SLICE*AUDIO_CHANNELS;
			signed short* p=(signed short*)AE->playBuffer;
			float vol = AE->masterVolume * 256.0;
			int masterVolume = vol;
			lockEnter(&AELock);
			for(i=0;i<n;i++) {
				int v=p[i];
				v=(v*masterVolume)>>8;
				p[i]=v;
			}
			if (AE->player) snd_pcm_writei(AE->player, AE->playBuffer, AUDIO_SLICE);
			haudioAckBuffer();
			lockLeave(&AELock);
		}
	}
	snd_pcm_close(AE->player);
	AE->player=NULL;
	return NULL;
}

int sysPlayStart()
{
	int err,exact_rate;
	snd_pcm_uframes_t exact_buffersize;
	snd_pcm_t *handle=NULL;
	snd_pcm_hw_params_t *hw_params=NULL;
	snd_pcm_sw_params_t *sw_params=NULL;
	char* device="default";
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
	AE->player=handle;
	hwThreadCreate(sysPlayAlsaThread,NULL);
	return 0;
cleanup:
	if (hw_params) snd_pcm_hw_params_free (hw_params);
	if (handle) snd_pcm_close(handle);
	return -1;
}
int sysPlayStop()
{
	// on linux the thread stops the player when AE->playing gets zero
	return 0;
}

void* sysRecAlsaThread(void* param)
{
	int err,k;
	while(1)
	{
		if (!AE->recording) break;
		if (!AE->rec) break;
		k = snd_pcm_avail(AE->rec);
		if (k>=AUDIO_SLICE) {
			err = snd_pcm_readi(AE->rec, AE->recBuffer, AUDIO_SLICE);
			if (err>0) {
				haudioRecBuffer(AE->recBuffer);
			}
			else snd_pcm_prepare(AE->rec);
		}
		else hwSleepMs(1);
	}
	snd_pcm_close(AE->rec);
	AE->rec=NULL;
	return NULL;
}

int sysRecStart(int nChannels)
{
	int err;
	int exact_rate;
	snd_pcm_uframes_t exact_buffersize;
	snd_pcm_t* handle = NULL;
	snd_pcm_hw_params_t* hw_params = NULL;
	char* device = "default";

	if ((err = snd_pcm_open(&handle, device, SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK)) < 0) {
		PRINTF(LOG_SYS, "> ALSA: cannot open capture device %s (%s)\n",
			device,
			snd_strerror(err));
		goto cleanup;
	}
	if ((err = snd_pcm_hw_params_malloc(&hw_params)) < 0) {
		PRINTF(LOG_SYS, "> ALSA: cannot allocate hardware parameter structure (%s)\n",
			snd_strerror(err));
		goto cleanup;
	}

	if ((err = snd_pcm_hw_params_any(handle, hw_params)) < 0) {
		PRINTF(LOG_SYS, "> ALSA: cannot initialize hardware parameter structure (%s)\n",
			snd_strerror(err));
		goto cleanup;
	}

	if ((err = snd_pcm_hw_params_set_access(handle, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED)) < 0) {
		PRINTF(LOG_SYS, "> ALSA: cannot set access type (%s)\n",
			snd_strerror(err));
		goto cleanup;
	}

	if ((err = snd_pcm_hw_params_set_format(handle, hw_params, SND_PCM_FORMAT_S16_LE)) < 0) {
		PRINTF(LOG_SYS, "> ALSA: cannot set sample format (%s)\n",
			snd_strerror(err));
		goto cleanup;
	}
	exact_rate = AUDIO_FREQ;
	if ((err = snd_pcm_hw_params_set_rate_near(handle, hw_params, &exact_rate, 0)) < 0) {
		PRINTF(LOG_SYS, "> ALSA: cannot set sample rate (%s)\n",
			snd_strerror(err));
		goto cleanup;
	}
//	PRINTF(LOG_SYS, "> ALSA: exact_rate=%d\n", exact_rate);

	if ((err = snd_pcm_hw_params_set_channels(handle, hw_params, nChannels)) < 0) {
		PRINTF(LOG_SYS, "> ALSA: cannot set channel count (%s)\n",
			snd_strerror(err));
		goto cleanup;
	}
	if ((err = snd_pcm_hw_params_set_periods(handle, hw_params, REC_NB_BUFFERS, 0)) < 0) {
		PRINTF(LOG_SYS, "> ALSA: cannot set periods (%s)\n",
			snd_strerror(err));
		goto cleanup;
	}
	exact_buffersize = AE->recBufferLength;
	if ((err = snd_pcm_hw_params_set_buffer_size_near(handle, hw_params, &exact_buffersize)) < 0) {
		PRINTF(LOG_SYS, "> ALSA: cannot set buffer size (%s)\n",
			snd_strerror(err));
		goto cleanup;
	}
//	PRINTF(LOG_SYS, "> ALSA: exact_buffersize=%d\n", (int)exact_buffersize);

	if ((err = snd_pcm_hw_params(handle, hw_params)) < 0) {
		PRINTF(LOG_SYS, "> ALSA: cannot set parameters (%s)\n",
			snd_strerror(err));
		goto cleanup;
	}
	snd_pcm_hw_params_free(hw_params);
	hw_params = NULL;

	if ((err = snd_pcm_prepare(handle)) < 0) {
		PRINTF(LOG_SYS, "cannot prepare audio interface for use (%s)\n",
			snd_strerror(err));
		goto cleanup;
	}
	if ((err = snd_pcm_start(handle)) < 0) {
		PRINTF(LOG_SYS, "Go error: %s\n", snd_strerror(err));
		goto cleanup;
	}
	AE->rec = handle;
	hwThreadCreate(sysRecAlsaThread,NULL);
	return 0;
cleanup:
	if (hw_params) snd_pcm_hw_params_free(hw_params);
	if (handle) snd_pcm_close(handle);
	return -1;
}
int sysRecStop()
{
	// on linux the thread stops the recording when AE->recording gets zero
	return 0;
}

int sysAudioInit()
{
	return 0;
}
void sysAudioClose(void)
{
}
#endif	//USE_ALSA
//--------------------------------------------------------------------------
#ifdef USE_AUDIOTOOLBOX	// MACOS, IOS
int sysPlayVolume(void)
{
	if (AE->playQueue) AudioQueueSetParameter(AE->playQueue,kAudioQueueParam_Volume, AE->masterVolume);
	return 0;
}

int sysPlayStop(void)	// called in the lock section AE->lock
{
	AudioQueueStop(AE->playQueue,true);
	AudioQueueDispose(AE->playQueue,true);
	return 0;
}

static void sysPlayCb(void *user,AudioQueueRef queue,AudioQueueBufferRef buffer)
{
	if (!AE->playing) return;
	lockEnter(&AELock);
	memcpy(buffer->mAudioData, AE->playBuffer, AUDIO_BUFFER);
	haudioAckBuffer();
	buffer->mAudioDataByteSize = AUDIO_BUFFER;
	AudioQueueEnqueueBuffer(queue,buffer,0,NULL);
	lockLeave(&AELock);
}

int sysPlayStart(void)
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
	
	AudioQueueNewOutput(&audioFormat,sysPlayCb,NULL,NULL,kCFRunLoopCommonModes,0,(AudioQueueRef*)&AE->playQueue);
	sysPlayVolume();
//	PRINTF(LOG_DEV,"sysPlayStart masterVolume=%f\n", AE->masterVolume);

	for(i=0;i<AUDIO_NB_BUFFERS;i++)
	{
		AudioQueueAllocateBuffer(AE->playQueue,AUDIO_BUFFER,&buffer);
		memset(buffer->mAudioData,AUDIO_ZERO_VALUE,AUDIO_BUFFER);
		buffer->mAudioDataByteSize = AUDIO_BUFFER;
		AudioQueueEnqueueBuffer(AE->playQueue,buffer,0,NULL);
	}
	AudioQueueStart(AE->playQueue,NULL);
	return 0;
}

static void sysRecCb(void* user, AudioQueueRef queue, AudioQueueBufferRef buffer, const AudioTimeStamp* startTime,
	UInt32 numPackets, const AudioStreamPacketDescription* packetDesc)
{
	if (!AE->recording) return;
	lockEnter(&AELock);
	haudioRecBuffer((char*)buffer->mAudioData);
	AudioQueueEnqueueBuffer(queue, buffer, 0, NULL);
	lockLeave(&AELock);
}


int sysRecStart(int nChannels)
{
	int i;
	int k = 3 - nChannels;
	AudioStreamBasicDescription audioFormat;
	AudioQueueBufferRef buffer;

	memset(&audioFormat, 0, sizeof(audioFormat));
	audioFormat.mFormatID = kAudioFormatLinearPCM;
	audioFormat.mSampleRate = AUDIO_FREQ;
	audioFormat.mFormatFlags = kLinearPCMFormatFlagIsSignedInteger;
	audioFormat.mBytesPerPacket = AUDIO_BYTES_PER_SAMPLE/k;
	audioFormat.mFramesPerPacket = 1;
	audioFormat.mBytesPerFrame = AUDIO_BYTES_PER_SAMPLE/k;
	audioFormat.mChannelsPerFrame = nChannels;
	audioFormat.mBitsPerChannel = 16;

	AudioQueueNewInput(&audioFormat, sysRecCb, NULL, NULL, kCFRunLoopCommonModes, 0, &AE->recQueue);

	for (i = 0; i < REC_NB_BUFFERS; i++)
	{
		AudioQueueAllocateBuffer(AE->recQueue, AE->recBufferLength, &buffer);
		AudioQueueEnqueueBuffer(AE->recQueue, buffer, 0, NULL);
	}
	AudioQueueStart(AE->recQueue, NULL);
	return 0;
}
int sysRecStop()
{
	AudioQueueStop(AE->recQueue, true);
	AudioQueueDispose(AE->recQueue, true);
	return 0;
}

int sysAudioInit(void)
{
	return 0;
}
void sysAudioClose(void)
{
}
#endif	// USE_AUDIOTOOLBOX
//--------------------------------------------------------------------------
#ifdef USE_AUDIO_ENGINE	// WINDOWS
int sysPlayVolume()
{
	float v = AE->masterVolume * 65535;
	int masterVolume = ((int)v) * 0x10001;
	waveOutSetVolume(AE->hWaveOut,masterVolume);
	return 0;
}

void _audioReleaseBuffer(HAudioBuf* ha)
{
	if (ha->hData) {
		GlobalUnlock(ha->hData);
		GlobalFree(ha->hData);
		ha->hData = NULL;
	}
	if (ha->hWaveHdr) {
		GlobalUnlock(ha->hWaveHdr);
		GlobalFree(ha->hWaveHdr);
		ha->hWaveHdr = NULL;
	}
}
void _cleanPlayBuffer()
{
	int i;
	for (i = 0; i < AUDIO_NB_BUFFERS; i++) _audioReleaseBuffer(&AE->audioBuffers[i]);
}
void _cleanRecBuffer()
{
	int i;
	for (i = 0; i < REC_NB_BUFFERS; i++) _audioReleaseBuffer(&AE->recBuffers[i]);
}

void CALLBACK waveOutProc(HWAVEOUT m_hWO, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	if (uMsg == WOM_DONE)
	{
		LPWAVEHDR lpWaveHdr = (LPWAVEHDR)dwParam1;
		HAudioBuf* ha = (HAudioBuf*)lpWaveHdr->dwUser;
		lockEnter(&AELock);
		if (ha->serial == AE->playSerial) {
			memcpy(lpWaveHdr->lpData, AE->playBuffer, AUDIO_BUFFER);
			haudioAckBuffer();
			waveOutWrite(AE->hWaveOut, lpWaveHdr, sizeof(WAVEHDR));
		}
		lockLeave(&AELock);
	}
}
int sysPlayStart()
{
	UINT wResult;
	PCMWAVEFORMAT pcmWaveFormat;
	int i;

	_cleanPlayBuffer();
	pcmWaveFormat.wf.wFormatTag = WAVE_FORMAT_PCM;
	pcmWaveFormat.wf.nChannels = AUDIO_CHANNELS;
	pcmWaveFormat.wf.nSamplesPerSec = AUDIO_FREQ;
	pcmWaveFormat.wf.nAvgBytesPerSec = AUDIO_FREQ * AUDIO_BYTES_PER_SAMPLE;
	pcmWaveFormat.wf.nBlockAlign = AUDIO_BYTES_PER_SAMPLE;
	pcmWaveFormat.wBitsPerSample = AUDIO_VALUE*8;

	wResult = waveOutOpen((LPHWAVEOUT)&AE->hWaveOut, WAVE_MAPPER, (LPWAVEFORMATEX)&pcmWaveFormat, (DWORD_PTR)waveOutProc, (DWORD_PTR)NULL, CALLBACK_FUNCTION);
	if (wResult != 0)	return -1;
	sysPlayVolume();
//	PRINTF(LOG_DEV,"sysPlayStart masterVolume=%f\n", AE->masterVolume);
	for (i = 0; i < AUDIO_NB_BUFFERS; i++)
	{
		HPSTR lpData;
		LPWAVEHDR lpWaveHdr;
		HAudioBuf* ha = &AE->audioBuffers[i];
		ha->serial = AE->playSerial;
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
		wResult = waveOutPrepareHeader(AE->hWaveOut, lpWaveHdr, sizeof(WAVEHDR));
		if (wResult != 0) return -1;
		memset(lpData, AUDIO_ZERO_VALUE, AUDIO_BUFFER);
		wResult = waveOutWrite(AE->hWaveOut, lpWaveHdr, sizeof(WAVEHDR));
	}
	return 0;
}
int sysPlayStop()	// called in the lock section AE->lock
{
	AE->playSerial++;
	waveOutClose(AE->hWaveOut);
	return 0;
}

void CALLBACK waveInProc(HWAVEOUT m_hWO, UINT uMsg, DWORD_PTR dwInstance, DWORD_PTR dwParam1, DWORD_PTR dwParam2)
{
	if (uMsg == WIM_DATA)
	{
		LPWAVEHDR lpWaveHdr = (LPWAVEHDR)dwParam1;
		HAudioBuf* ha = (HAudioBuf*)lpWaveHdr->dwUser;
		lockEnter(&AELock);
		if (ha->serial == AE->recSerial) {
			haudioRecBuffer((char*)lpWaveHdr->lpData);
			waveInAddBuffer(AE->hWaveIn, lpWaveHdr, sizeof(WAVEHDR));
		}
		lockLeave(&AELock);
	}
}
int sysRecStart(int nChannels)
{
	UINT wResult;
	PCMWAVEFORMAT pcmWaveFormat;
	int i;
	int k = 3 - nChannels;
	_cleanRecBuffer();
	pcmWaveFormat.wf.wFormatTag = WAVE_FORMAT_PCM;
	pcmWaveFormat.wf.nChannels = nChannels;
	pcmWaveFormat.wf.nSamplesPerSec = AUDIO_FREQ;
	pcmWaveFormat.wf.nAvgBytesPerSec = AUDIO_FREQ * AUDIO_BYTES_PER_SAMPLE/k;
	pcmWaveFormat.wf.nBlockAlign = AUDIO_BYTES_PER_SAMPLE/k;
	pcmWaveFormat.wBitsPerSample = AUDIO_VALUE * 8;

	wResult = waveInOpen((LPHWAVEIN)&AE->hWaveIn, WAVE_MAPPER,
		(LPWAVEFORMATEX)&pcmWaveFormat, (DWORD_PTR)waveInProc, (DWORD_PTR)NULL, CALLBACK_FUNCTION);
	if (wResult != 0)	return -1;

	for (i = 0; i < REC_NB_BUFFERS; i++)
	{
		HPSTR lpData;
		LPWAVEHDR lpWaveHdr;
		HAudioBuf* ha = &AE->recBuffers[i];
		ha->serial = AE->recSerial;
		ha->hData = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, AE->recBufferLength);
		if (!ha->hData) return -1;
		lpData = (char*)GlobalLock(ha->hData);
		if (!lpData) return -1;
		ha->hWaveHdr = GlobalAlloc(GMEM_MOVEABLE | GMEM_SHARE, (DWORD)sizeof(WAVEHDR));
		if (ha->hWaveHdr == NULL) return -1;

		lpWaveHdr = (LPWAVEHDR)GlobalLock(ha->hWaveHdr);
		if (!lpWaveHdr) return -1;

		lpWaveHdr->lpData = lpData;
		lpWaveHdr->dwBufferLength = AE->recBufferLength;
		lpWaveHdr->dwUser = (DWORD_PTR)ha;
		lpWaveHdr->dwFlags = 0L;
		lpWaveHdr->dwLoops = 1L;
		wResult = waveInPrepareHeader(AE->hWaveIn, lpWaveHdr, sizeof(WAVEHDR));
		if (wResult != 0) return -1;
		wResult = waveInAddBuffer(AE->hWaveIn, lpWaveHdr, sizeof(WAVEHDR));
	}
	waveInStart(AE->hWaveIn);
	return 0;
}
int sysRecStop(void)
{
	AE->recSerial++;
	waveInClose(AE->hWaveIn);
	return 0;
}

int sysAudioInit(void)
{
	return 0;
}
void sysAudioClose(void)
{
	_cleanPlayBuffer();
	_cleanRecBuffer();
}
#endif	// USE_AUDIO_ENGINE
//--------------------------------------------------------------------------

void haudioAckBuffer(void)
{
	int index = LSBL(AE->playIndex);
	memset(AE->playBuffer, AUDIO_ZERO_VALUE, AUDIO_BUFFER);
	eventBinary(AE->playEventId, (char*) & index, 4);
	AE->playIndex += AUDIO_SLICE;
}

void haudioRecBuffer(char* src)
{
	eventBinary(AE->recEventId, src, AE->recBufferLength);
}

//--------------------------------------------------------------------------

void _mixStereo16(short* out, short* in, int n, int volume)
{
	int i;
	for (i = 0; i < n; i++) {
		int v = out[i];
		int s = in[i];
		s = (s * volume) >> 8;
		v += s;
		if (v > 0x7ffff) v = 0x7ffff;
		if (v < -0x7ffff) v = -0x7ffff;
		out[i] = v;
	}
}
void _mixMono16(short* out, short* in, int n, int volume)
{
	int i;
	for (i = 0; i < n; i++) {
		int v = out[i];
		int s = in[i>>1];
		s = (s * volume) >> 8;
		v += s;
		if (v > 0x7ffff) v = 0x7ffff;
		if (v < -0x7ffff) v = -0x7ffff;
		out[i] = v;
	}
}

int fun_audioFeedStereo16(Thread* th)
{
	LINT len;
	char *src, *dst;
	LFLOAT volume = 255.0 * STACK_FLOAT(th, 0);
	LINT offset = STACK_INT(th, 1);
	LB* sample = STACK_PNT(th, 2);
	LINT result = offset + AUDIO_BUFFER;
	if (!sample || offset&3 || STR_LENGTH(sample)&3) FUN_RETURN_NIL;
	if (offset < 0) {
		src = STR_START(sample);
		dst = AE->playBuffer - offset;
		len = AUDIO_BUFFER + offset;
		if (len > STR_LENGTH(sample)) len = STR_LENGTH(sample);
	}
	else {
		src = STR_START(sample)+offset;
		dst = AE->playBuffer;
		len = STR_LENGTH(sample) - offset;
		if (len > AUDIO_BUFFER) len = AUDIO_BUFFER;
	}
	if (len <= 0) FUN_RETURN_INT(result);
	volume = (volume < 0) ? 0 : ((volume > 256) ? 256 : volume);

	lockEnter(&AELock);
	_mixStereo16((short*)dst, (short*)src, (int)(len >> 1), (int)volume);
	lockLeave(&AELock);
	FUN_RETURN_INT(result);
}
int fun_audioFeedMono16(Thread* th)
{
	LINT len;
	char* src, * dst;
	LFLOAT volume = 255.0 * STACK_FLOAT(th, 0);
	LINT offset = STACK_INT(th, 1);
	LB* sample = STACK_PNT(th, 2);
	LINT result = offset + (AUDIO_BUFFER>>1);
	if (!sample || offset&1 || STR_LENGTH(sample)&1) FUN_RETURN_NIL;
	if (offset < 0) {
		src = STR_START(sample);
		dst = AE->playBuffer - (offset<<1);
		len = AUDIO_BUFFER + (offset<<1);
		if (len > STR_LENGTH(sample)) len = STR_LENGTH(sample);
	}
	else {
		src = STR_START(sample) + offset;
		dst = AE->playBuffer;
		len = STR_LENGTH(sample) - (offset<<1);
		if (len > AUDIO_BUFFER) len = AUDIO_BUFFER;
	}
	if (len<=0) FUN_RETURN_INT(result);
	volume = (volume < 0) ? 0 : ((volume > 256) ? 256 : volume);

	lockEnter(&AELock);
	_mixMono16((short*)dst, (short*)src, (int)len, (int)volume);
	lockLeave(&AELock);
	FUN_RETURN_INT(result);
}

int fun_audioSetVolume(Thread* th)
{
	LFLOAT volume = STACK_FLOAT(th, 0);
	if (volume < 0.0) volume = 0.0;
	if (volume > 1.0) volume = 1.0;
	AE->masterVolume = (float)volume;
	lockEnter(&AELock);
	sysPlayVolume();
	lockLeave(&AELock);
	return 0;
}

int fun_audioSamplesPerSlice(Thread* th)
{
	FUN_RETURN_INT(AUDIO_SLICE);
}

int fun_audioPlaying(Thread* th)
{
	FUN_RETURN_BOOL(AE->playing);
}

int fun_audioRecording(Thread* th)
{
	FUN_RETURN_BOOL(AE->recording);
}

int fun_audioStart(Thread* th)
{
	if (AE->playing) FUN_RETURN_NIL;
	AE->playIndex = 0;
	if (sysPlayStart()) FUN_RETURN_NIL;
	AE->playing = 1;
	FUN_RETURN_INT(AE->playEventId);
}

int fun_audioStop(Thread* th)
{
	if (!AE->playing) FUN_RETURN_NIL;
	AE->playing = 0;
	lockEnter(&AELock);
	sysPlayStop();
	lockLeave(&AELock);
	FUN_RETURN_BOOL(1);
}

int audioAmplitude16(short* src, int offset, int delta, int n)
{
	int mn = 0;
	int mx = 0;
	int i = 0;
	for (i = offset; i < n; i += delta) {
		short s = LSBW(src[i]);
		if (s > mx) mx = s;
		if (s < mn) mn = s;
	}
	if (mx < -mn) mx = -mn;
	return mx;
}
int fun_audioAmplitude16(Thread* th)
{
	int amplitude;
	int offset = 0;
	int step = 2;
	LB* channel = STACK_PNT(th, 0);
	LB* src = STACK_PNT(th, 1);
	if ((!src)||(STR_LENGTH(src)&1)) FUN_RETURN_NIL;
	if (!channel) step = 1;
	else if (channel == MM._false) offset = 1;
	amplitude = audioAmplitude16((short*)STR_START(src), offset, step, (int)STR_LENGTH(src) >> 1);
	FUN_RETURN_INT(amplitude);
}
int fun_audioRecStart(Thread* th)
{
	int nbChannels = 1;
	LB* stereo = STACK_PNT(th, 0);
	if (AE->recording) FUN_RETURN_NIL;
	if (stereo == MM._true) nbChannels = 2;
	AE->recBufferLength = AUDIO_BUFFER / (3 - nbChannels);
	if (sysRecStart(nbChannels)) FUN_RETURN_NIL;
	AE->recording = 1;
	FUN_RETURN_INT(AE->recEventId);
}
int fun_audioRecStop(Thread* th)
{
	if (!AE->recording) FUN_RETURN_NIL;
	AE->recording = 0;
	lockEnter(&AELock);
	sysRecStop();
	lockLeave(&AELock);
	FUN_RETURN_BOOL(1);
}
void audioInit(void)
{
	AE = &_AE;
	memset(AE, 0, sizeof(AudioEngine));
	AE->masterVolume = 1.0;
	AE->playEventId = eventGetNextID();
	AE->recEventId = eventGetNextID();
	lockCreate(&AELock);
	sysAudioInit();
}
void audioClose(void)
{
	sysAudioClose();
}
#else	//WITH_AUDIO
int fun_audioStart(Thread* th) FUN_RETURN_NIL
int fun_audioStop(Thread* th) FUN_RETURN_NIL
int fun_audioSamplesPerSlice(Thread* th) FUN_RETURN_NIL
int fun_audioAvailableSpace(Thread* th) FUN_RETURN_NIL
int fun_audioFeedStereo16(Thread* th) FUN_RETURN_NIL
int fun_audioFeedMono16(Thread* th) FUN_RETURN_NIL
int fun_audioSetVolume(Thread* th) FUN_RETURN_NIL
int fun_audioPlaying(Thread* th) FUN_RETURN_NIL
int fun_audioRecording(Thread* th) FUN_RETURN_NIL
int fun_audioAmplitude16(Thread* th) FUN_RETURN_NIL
int fun_audioRecStart(Thread* th) FUN_RETURN_NIL
int fun_audioRecStop(Thread* th) FUN_RETURN_NIL
void audioInit(void) {}
void audioClose(void) {}
#endif	//WITH_AUDIO
int systemAudioInit(Pkg *system)
{
	static const Native nativeDefs[] = {
		{ NATIVE_FUN, "_audioStart", fun_audioStart, "fun -> Int"},
		{ NATIVE_FUN, "_audioStop", fun_audioStop, "fun -> Bool"},
		{ NATIVE_FUN, "audioSamplesPerSlice", fun_audioSamplesPerSlice, "fun -> Int"},
		{ NATIVE_FUN, "audioFeedStereo16", fun_audioFeedStereo16, "fun Str Int Float -> Int"},
		{ NATIVE_FUN, "audioFeedStereo16Bytes", fun_audioFeedStereo16, "fun Bytes Int Float -> Int"},
		{ NATIVE_FUN, "audioFeedMono16", fun_audioFeedMono16, "fun Str Int Float -> Int"},
		{ NATIVE_FUN, "audioFeedMono16Bytes", fun_audioFeedMono16, "fun Bytes Int Float -> Int"},
		{ NATIVE_FUN, "audioSetVolume", fun_audioSetVolume, "fun Float -> Float" },
		{ NATIVE_FUN, "audioPlaying", fun_audioPlaying, "fun -> Bool" },
		{ NATIVE_FUN, "audioAmplitude16", fun_audioAmplitude16, "fun Str Bool -> Int" },
		{ NATIVE_FUN, "audioAmplitude16Bytes", fun_audioAmplitude16, "fun Bytes Bool -> Int" },
		{ NATIVE_FUN, "_audioRecStart", fun_audioRecStart, "fun Bool -> Int" },
		{ NATIVE_FUN, "_audioRecStop", fun_audioRecStop, "fun -> Bool" },
		{ NATIVE_FUN, "audioRecording", fun_audioRecording, "fun -> Bool" },
	};
	NATIVE_DEF(nativeDefs);
	audioInit();
	return 0;
}
void systemAudioClose(void)
{
	audioClose();
}
