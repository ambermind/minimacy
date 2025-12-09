// SPDX-License-Identifier: GPL-3.0-only
// Copyright (c) 2022, Sylvain Huet, Ambermind
// Minimacy (r) System
#include <android_native_app_glue.h>

#include <errno.h>
#include <jni.h>
#include <sys/time.h>
#include <time.h>
#include <android/log.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <sys/stat.h>

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <GLES2/gl2ext.h>


#import "../../../../../../src/minimacy.h"
#define  LOG_TAG    "minimacy"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARN,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)

void consoleWrite(int user, char* src, int len)
{
    __android_log_write(ANDROID_LOG_INFO, "Minimacy", src);
}
void consoleVPrint(int user, char* format, va_list arglist)
{
    __android_log_vprint(ANDROID_LOG_INFO, "Minimacy", format, arglist);
}

typedef struct {
	struct android_app* app;
	int started;	// VM started
	int open;	// uiStart done
	LINT type;
	int w;
	int h;
	JNIEnv* env;
	ANativeWindow *window;
	EGLDisplay display;
	EGLConfig config;
	EGLSurface surface;
	EGLContext context;
	int glRefresh;
}UIstruct;
UIstruct UI;

int lastTouchCount=0;

// ----------------------------------------------------------------------
int android_glMakeContext()
{
	UI.display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
	eglInitialize(UI.display, 0, 0);

	/*
	 * Here specify the attributes of the desired configuration.
	 * Below, we select an EGLConfig with at least 8 bits per color
	 * component compatible with on-screen windows
	 */
	const EGLint attribs[] = {EGL_RENDERABLE_TYPE,
							  EGL_OPENGL_ES2_BIT,  // Request opengl ES2.0
							  EGL_SURFACE_TYPE,
							  EGL_WINDOW_BIT,
							  EGL_BLUE_SIZE,
							  8,
							  EGL_GREEN_SIZE,
							  8,
							  EGL_RED_SIZE,
							  8,
							  EGL_DEPTH_SIZE,
							  24,
							  EGL_NONE};

	EGLint num_configs;
	eglChooseConfig(UI.display, attribs, &UI.config, 1, &num_configs);
	if (!num_configs) {
		LOGW("> Unable to retrieve EGL config");
		return -1;
	}
	UI.surface = eglCreateWindowSurface(UI.display, UI.config, UI.app->window, NULL);
	eglQuerySurface(UI.display, UI.surface, EGL_WIDTH, &UI.w);
	eglQuerySurface(UI.display, UI.surface, EGL_HEIGHT, &UI.h);
	const EGLint context_attribs[] = {EGL_CONTEXT_CLIENT_VERSION,
									  2,  // Request opengl ES2.0
									  EGL_NONE};
	UI.context = eglCreateContext(UI.display, UI.config, NULL, context_attribs);

	if (eglMakeCurrent(UI.display, UI.surface, UI.surface, UI.context) == EGL_FALSE) {
		LOGW("> Unable to eglMakeCurrent");
		return -1;
	}

	const char* versionStr = (const char*)glGetString(GL_VERSION);
	LOGI("> glversion=%s\n",versionStr);
	return 0;
}
int android_glSwapBuffers()
{
	if (UI.glRefresh) {	// after app is restored from background state
		UI.glRefresh=0;
		UI.surface = eglCreateWindowSurface(UI.display, UI.config, UI.window, NULL);
		if (eglMakeCurrent(UI.display, UI.surface, UI.surface, UI.context) == EGL_FALSE) {
			LOGW("> Unable to eglMakeCurrent");
		}
	}
	eglSwapBuffers(UI.display, UI.surface);
	return 0;
}

// ----------------------------------------------------------------------

int fun_uiW(Thread* th) { FUN_RETURN_INT(UI.w); }
int fun_uiH(Thread* th) { FUN_RETURN_INT(UI.h); }
int fun_screenW(Thread* th) { FUN_RETURN_INT(UI.w); }
int fun_screenH(Thread* th) { FUN_RETURN_INT(UI.h); }

int fun_uiStart(Thread* th) {
	// we ignore most of the arguments as we have no control on the size of the screen and the ui is always fullscreen
	// at this stage only the type could be useful for a non GL ui, but we need to fix uiUpdate first
	LINT type = STACK_INT(th, 1);
	if (UI.open) FUN_RETURN_NIL;

	UI.type=type;
	UI.open=1;
	FUN_RETURN_PNT(MM._true);
}
int fun_uiStop(Thread* th) FUN_RETURN_NIL
int fun_uiResize(Thread* th) FUN_RETURN_NIL

int fun_uiDrop(Thread* th) FUN_RETURN_NIL

int fun_uiSetTitle(Thread* th) FUN_RETURN_NIL
int fun_keyboardState(Thread* th) FUN_RETURN_NIL

int fun_clipboardCopy(Thread* th) FUN_RETURN_NIL
int fun_clipboardPaste(Thread* th) FUN_RETURN_NIL

int fun_cursorSize(Thread* th) FUN_RETURN_NIL
int fun_cursorCreate(Thread* th) FUN_RETURN_NIL
int fun_cursorShow(Thread* th) FUN_RETURN_NIL

int Java_com_example_Minimacy_log( JNIEnv* env, jobject obj, jstring msg)
{
	LOGI("%s",(*UI.env)->GetStringUTFChars(UI.env, msg, NULL));
	return 0;
}
int Java_com_example_Minimacy_eventBinary( JNIEnv* env, jobject obj, int eventID, jbyteArray data)
{
	jbyte* bufferPtr = (*env)->GetByteArrayElements(env, data, NULL);
	jsize lengthOfArray = (*env)->GetArrayLength(env, data);
	eventBinary(eventID, bufferPtr, lengthOfArray);
	(*env)->ReleaseByteArrayElements(env, data, bufferPtr, 0);
	return 0;
}
int Java_com_example_Minimacy_notifyChar( JNIEnv* env, jobject obj, jint c)
{
    if (c>255) return 0;
    LOGI("Key event: action=%d key=%d",EVENT_KEYDOWN,c);
    eventNotify(EVENT_KEYDOWN, c, 0, 0);
    LOGI("Key event: action=%d key=%d",EVENT_KEYUP,c);
    eventNotify(EVENT_KEYUP, c, 0, 0);
    return 0;
}
int Java_com_example_Minimacy_eventGetNextID(JNIEnv* env,jobject obj)
{
	return eventGetNextID();
}
/*static void jniTestFoobar()
{
	jobject nativeActivity = UI.app->activity->clazz;
	jclass ClassNativeActivity = (*UI.env)->GetObjectClass(UI.env, nativeActivity );
	jmethodID method = (*UI.env)->GetMethodID(UI.env, ClassNativeActivity, "getFoobar", "(Ljava/lang/String;)Ljava/lang/String;" );
	jstring* parameter = (*UI.env)->NewStringUTF(UI.env, "foo");
	jobject result=(*UI.env)->CallObjectMethod(UI.env, nativeActivity, method,parameter);
    char* data=(*UI.env)->GetStringUTFChars(UI.env, result, NULL);
    LOGI("> jniTestresult=%s\n",data);
    (*UI.env)->DeleteLocalRef(UI.env, parameter);
    (*UI.env)->ReleaseStringUTFChars(UI.env, result, data);
    (*UI.env)->DeleteLocalRef(UI.env, result);
}
*/
void _keyboardToggle(int show)
{
	jobject nativeActivity = UI.app->activity->clazz;
	jclass ClassNativeActivity = (*UI.env)->GetObjectClass(UI.env, nativeActivity );
	jmethodID MethodShowKeyboard = (*UI.env)->GetMethodID(UI.env, ClassNativeActivity, show?"showKeyboard":"hideKeyboard", "()V" );
	(*UI.env)->CallVoidMethod(UI.env, nativeActivity, MethodShowKeyboard );
}
int fun_keyboardShow(Thread* th) {
	_keyboardToggle(1);
	FUN_RETURN_TRUE;
}
int fun_keyboardHide(Thread* th) {
	_keyboardToggle(0);
	FUN_RETURN_TRUE;
}
int fun_keyboardHeight(Thread* th) FUN_RETURN_NIL
int fun_orientationGet(Thread* th) FUN_RETURN_NIL
int fun_accelerometerX(Thread* th)
{
    jobject nativeActivity = UI.app->activity->clazz;
    jclass ClassNativeActivity = (*UI.env)->GetObjectClass(UI.env, nativeActivity );
    jmethodID method = (*UI.env)->GetMethodID(UI.env, ClassNativeActivity, "accGetX", "()F" );
    LFLOAT result=(*UI.env)->CallFloatMethod(UI.env, nativeActivity, method);
    FUN_RETURN_FLOAT(result);
}

int fun_accelerometerY(Thread* th)
{
    jobject nativeActivity = UI.app->activity->clazz;
    jclass ClassNativeActivity = (*UI.env)->GetObjectClass(UI.env, nativeActivity );
    jmethodID method = (*UI.env)->GetMethodID(UI.env, ClassNativeActivity, "accGetY", "()F" );
    LFLOAT result=(*UI.env)->CallFloatMethod(UI.env, nativeActivity, method);
    FUN_RETURN_FLOAT(result);
}

int fun_accelerometerZ(Thread* th)
{
    jobject nativeActivity = UI.app->activity->clazz;
    jclass ClassNativeActivity = (*UI.env)->GetObjectClass(UI.env, nativeActivity );
    jmethodID method = (*UI.env)->GetMethodID(UI.env, ClassNativeActivity, "accGetZ", "()F" );
    LFLOAT result=(*UI.env)->CallFloatMethod(UI.env, nativeActivity, method);
    FUN_RETURN_FLOAT(result);
}

int fun_accelerometerInit(Thread* th) {
    LFLOAT f = STACK_FLOAT(th,0);
    jobject nativeActivity = UI.app->activity->clazz;
    jclass ClassNativeActivity = (*UI.env)->GetObjectClass(UI.env, nativeActivity );
    jmethodID MethodSetAccelerometer = (*UI.env)->GetMethodID(UI.env, ClassNativeActivity, "setAccelerometer", "(F)I" );
    (*UI.env)->CallIntMethod(UI.env, nativeActivity, MethodSetAccelerometer, f );
    return 0;
}

int fun_glMakeContext(Thread* th)
{
    GLinstance++;
    FUN_RETURN_INT(android_glMakeContext());
}
int fun_glRefreshContext(Thread* th) FUN_RETURN_NIL
int fun_glSwapBuffers(Thread* th)
{
	FUN_RETURN_INT(android_glSwapBuffers());
}
int viewPortScale(int u) { return u; }

int fun_bleSerialStart(Thread* th)
{
    LB* tx = STACK_PNT(th, 0);
    LB* rx = STACK_PNT(th, 1);
    LB* service = STACK_PNT(th, 2);
    if (!rx || !tx || !service) FUN_RETURN_NIL;
	jobject nativeActivity = UI.app->activity->clazz;
	jclass ClassNativeActivity = (*UI.env)->GetObjectClass(UI.env, nativeActivity );

    jstring* serviceUUID = (*UI.env)->NewStringUTF(UI.env, STR_START(service));
    jstring* rxUUID = (*UI.env)->NewStringUTF(UI.env, STR_START(rx));
    jstring* txUUID = (*UI.env)->NewStringUTF(UI.env, STR_START(tx));

	jmethodID method = (*UI.env)->GetMethodID(UI.env, ClassNativeActivity, "bleSerialStart", "(Ljava/lang/String;Ljava/lang/String;Ljava/lang/String;)I" );
	LINT event=(*UI.env)->CallIntMethod(UI.env, nativeActivity, method,serviceUUID,rxUUID,txUUID);
    (*UI.env)->DeleteLocalRef(UI.env, serviceUUID);
    (*UI.env)->DeleteLocalRef(UI.env, rxUUID);
    (*UI.env)->DeleteLocalRef(UI.env, txUUID);

    if (!event) FUN_RETURN_NIL;
    FUN_RETURN_INT(event);
}
int fun_bleSerialStop(Thread* th)
{
	jobject nativeActivity = UI.app->activity->clazz;
	jclass ClassNativeActivity = (*UI.env)->GetObjectClass(UI.env, nativeActivity );
	jmethodID method = (*UI.env)->GetMethodID(UI.env, ClassNativeActivity, "bleSerialStop", "()I" );
	int result=(*UI.env)->CallIntMethod(UI.env, nativeActivity, method);
    FUN_RETURN_BOOL(1);
}
int fun_bleSerialIsConnected(Thread* th)
{
	jobject nativeActivity = UI.app->activity->clazz;
	jclass ClassNativeActivity = (*UI.env)->GetObjectClass(UI.env, nativeActivity );
	jmethodID method = (*UI.env)->GetMethodID(UI.env, ClassNativeActivity, "bleSerialIsConnected", "()I" );
	int result=(*UI.env)->CallIntMethod(UI.env, nativeActivity, method);
	FUN_RETURN_BOOL(result);
}
int fun_bleSerialName(Thread* th)
{
	jobject nativeActivity = UI.app->activity->clazz;
	jclass ClassNativeActivity = (*UI.env)->GetObjectClass(UI.env, nativeActivity );
	jmethodID method = (*UI.env)->GetMethodID(UI.env, ClassNativeActivity, "bleSerialName", "()Ljava/lang/String;" );
	jobject result=(*UI.env)->CallObjectMethod(UI.env, nativeActivity, method);
	if (!result) FUN_RETURN_NIL;
	char* data=(*UI.env)->GetStringUTFChars(UI.env, result, NULL);
	STACK_PUSH_STR_ERR(th,data,-1,EXEC_OM);
//	FUN_RETURN_STR(data,-1);
	(*UI.env)->ReleaseStringUTFChars(UI.env, result, data);
	(*UI.env)->DeleteLocalRef(UI.env, result);
	return 0;
}
int fun_bleSerialWrite(Thread* th)
{
    LINT len=0;

    LINT start = STACK_INT(th, 0);
    LB* src = STACK_PNT(th, 1);
    FUN_SUBSTR(src, start, len, 1, STR_LENGTH(src));
    if (len==0) FUN_RETURN_INT(start);

	jobject nativeActivity = UI.app->activity->clazz;
	jclass ClassNativeActivity = (*UI.env)->GetObjectClass(UI.env, nativeActivity );
	jmethodID method = (*UI.env)->GetMethodID(UI.env, ClassNativeActivity, "bleSerialWrite", "([B)I" );

	jbyteArray parameter = (*UI.env)->NewByteArray(UI.env, len);
	jbyte *bytes = (*UI.env)->GetByteArrayElements(UI.env, parameter, 0);
	char* p= STR_START(src)+start;
	for(int i=0;i<len;i++) bytes[i]=p[i];
	(*UI.env)->SetByteArrayRegion(UI.env,parameter, 0, len, bytes );

	len=(*UI.env)->CallIntMethod(UI.env, nativeActivity, method,parameter);
	(*UI.env)->DeleteLocalRef(UI.env, parameter);
	FUN_RETURN_INT(start + len);
}
int systemBleInit(Pkg* system) {
    static const Native nativeDefs[] = {
            {NATIVE_FUN, "bleSerialStart",       fun_bleSerialStart,       "fun Str Str Str-> Int"},
            {NATIVE_FUN, "bleSerialStop",        fun_bleSerialStop,        "fun -> Bool"},
            {NATIVE_FUN, "bleSerialIsConnected", fun_bleSerialIsConnected, "fun -> Bool"},
            {NATIVE_FUN, "bleSerialName",        fun_bleSerialName,        "fun -> Str"},
            {NATIVE_FUN, "bleSerialWrite",       fun_bleSerialWrite,       "fun Str Int -> Int"},
    };
    NATIVE_DEF(nativeDefs);
    return 0;
}


void Java_com_example_Minimacy_notifyLocation( JNIEnv* env, jobject obj, jint eventID, jdouble x, jdouble y, jdouble z)
{
	double coords[3];
	coords[0]=x;
	coords[1]=y;
	coords[2]=z;
	LOGI("notifyLocation %d %f %f %f %d",eventID,coords[0],coords[1],coords[2],sizeof(coords));
	eventBinary(eventID, (char*)&coords[0], sizeof(coords));
}

int fun_locationStart(Thread* th)
{
    jobject nativeActivity = UI.app->activity->clazz;
    jclass ClassNativeActivity = (*UI.env)->GetObjectClass(UI.env, nativeActivity );
    jmethodID method = (*UI.env)->GetMethodID(UI.env, ClassNativeActivity, "locationStart", "()I" );
    int eventID=(*UI.env)->CallIntMethod(UI.env, nativeActivity, method);
    if (!eventID) FUN_RETURN_NIL;
    FUN_RETURN_INT(eventID);
}
int fun_locationStop(Thread* th)
{
	jobject nativeActivity = UI.app->activity->clazz;
	jclass ClassNativeActivity = (*UI.env)->GetObjectClass(UI.env, nativeActivity );
	jmethodID method = (*UI.env)->GetMethodID(UI.env, ClassNativeActivity, "locationStop", "()V" );
	(*UI.env)->CallVoidMethod(UI.env, nativeActivity, method);
    FUN_RETURN_BOOL(1);
}
int fun_locationIsStarted(Thread* th)
{
	jobject nativeActivity = UI.app->activity->clazz;
	jclass ClassNativeActivity = (*UI.env)->GetObjectClass(UI.env, nativeActivity );
	jmethodID method = (*UI.env)->GetMethodID(UI.env, ClassNativeActivity, "locationIsStarted", "()I" );
	int isStarted=(*UI.env)->CallIntMethod(UI.env, nativeActivity, method);
    FUN_RETURN_BOOL(isStarted);
}

int systemLocationInit(Pkg* system) {
    static const Native nativeDefs[] = {
            { NATIVE_FUN, "locationStart", fun_locationStart, "fun -> Int" },
            { NATIVE_FUN, "locationStop", fun_locationStop, "fun -> Bool" },
            { NATIVE_FUN, "locationIsStarted", fun_locationIsStarted, "fun -> Bool" },
    };
    NATIVE_DEF(nativeDefs);
    return 0;
}

int systemUiHwInit(Pkg* system) {
	JavaVM *vm=UI.app->activity->vm;
	(*vm)->AttachCurrentThread(vm, &UI.env, NULL);

    systemBleInit(system);
    systemLocationInit(system);

    return 0;
}

static void engine_term_display(UIstruct* ui) {
}

static unsigned char key_ascii(struct android_app* app, AInputEvent* event) {
	int32_t code = AKeyEvent_getKeyCode(event);

	/* Handle a few special cases: */
	switch (code) {
		case AKEYCODE_DEL:
			return 8;
		case AKEYCODE_FORWARD_DEL:
			return 127;
		case AKEYCODE_ESCAPE:
			return 27;
	}

	/* Get usable JNI context */
	JNIEnv* env = app->activity->env;
	JavaVM* vm = app->activity->vm;
	(*vm)->AttachCurrentThread(vm, &env, NULL);

	jclass KeyEventClass = (*env)->FindClass(env, "android/view/KeyEvent");
	jmethodID KeyEventConstructor = (*env)->GetMethodID(env, KeyEventClass, "<init>", "(II)V");
	jobject keyEvent = (*env)->NewObject(env, KeyEventClass, KeyEventConstructor,
										 AKeyEvent_getAction(event), AKeyEvent_getKeyCode(event));
	jmethodID KeyEvent_getUnicodeChar = (*env)->GetMethodID(env, KeyEventClass, "getUnicodeChar", "(I)I");
	int ascii = (*env)->CallIntMethod(env, keyEvent, KeyEvent_getUnicodeChar, AKeyEvent_getMetaState(event));

	LOGI("getUnicodeChar(%d) = %d", code, ascii);

	return ascii;
}
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
	if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
		int c=-1;

		float sx=0;
		float sy=0;
		int touchCount=0;
		int anyUp;

		int mAction=AMotionEvent_getAction(event);
		int mIndex=(mAction & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK)>> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;

		mAction&=AMOTION_EVENT_ACTION_MASK;
		if (mAction==AMOTION_EVENT_ACTION_POINTER_DOWN) c=EVENT_CLICK;
		if (mAction==AMOTION_EVENT_ACTION_DOWN) c=EVENT_CLICK;
		if (mAction==AMOTION_EVENT_ACTION_POINTER_UP) c=EVENT_UNCLICK;
		if (mAction==AMOTION_EVENT_ACTION_UP) c=EVENT_UNCLICK;
		if (mAction==AMOTION_EVENT_ACTION_MOVE) c=EVENT_MOUSEMOVE;

		int nb=AMotionEvent_getPointerCount(event);
		if (!nb) return 1;

		for(int i=0;i<nb;i++) {
			float x= AMotionEvent_getX(event, i);
			float y= AMotionEvent_getY(event, i);
//			LOGI("----touch %fx%f up=%d", x, y, ((c==EVENT_UNCLICK)&&(i==mIndex))?1:0);
			if ((c==EVENT_UNCLICK)&&(i==mIndex)) {
				anyUp=i;
			}
			else {
				touchCount++;
				sx+=x; sy+=y;
				eventNotify(EVENT_MULTITOUCH, (int)x, (int)y, 0);
			}
		}
		eventNotify(EVENT_MULTITOUCH, 0, 0, 1);
		if (!touchCount) {
			float x= AMotionEvent_getX(event, anyUp);
			float y= AMotionEvent_getY(event, anyUp);
//			LOGI("unclick %fx%f",x,y);
			eventNotify(EVENT_UNCLICK,(int)x,(int)y,1);
		}
		else {
			sx/=touchCount;
			sy/=touchCount;
			if (lastTouchCount) {
//			LOGI("move %fx%f",sx,sy);
				eventNotify(EVENT_MOUSEMOVE,(int)sx,(int)sy,1);
			}
			else {
			LOGI("click %fx%f",sx,sy);
				eventNotify(EVENT_CLICK,(int)sx,(int)sy,1);
//				jniTestFoobar();
			}
		}
		lastTouchCount=touchCount;
		return 1;
	} else if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_KEY) {
		int mAction=AKeyEvent_getAction(event);
		int keycode=AKeyEvent_getKeyCode(event);
		int key=key_ascii(UI.app,event);
		int metaState=AKeyEvent_getMetaState(event);
		int action=-1;
		if (mAction==AKEY_EVENT_ACTION_DOWN) action=EVENT_KEYDOWN;
		if (mAction==AKEY_EVENT_ACTION_UP) action=EVENT_KEYUP;

		LOGI("Key event: action=%d keyCode=%d key=%d metaState=0x%x",mAction,keycode,key,metaState);
		if (action>=0 && key>0) {
			eventNotify(action, key, 0, 0);
			return 1;
		}
	}
	else LOGI("unknown event %x",AInputEvent_getType(event));

	return 0;
}

static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
	static int32_t format = WINDOW_FORMAT_RGBA_8888;
	UIstruct* ui = (UIstruct*)app->userData;
	LOGI("engine_handle_cmd %d",cmd);
	switch (cmd) {
		case APP_CMD_INIT_WINDOW:
			LOGI("==========APP_CMD_INIT_WINDOW");
			if (ui->app->window != NULL) {
				format = ANativeWindow_getFormat(app->window);
				ui->w=ANativeWindow_getWidth(app->window);
				ui->h=ANativeWindow_getHeight(app->window);
				if (ui->started) {
					if(app->window != UI.window)
					{
						LOGI("--------NEW WINDOW");
						UI.glRefresh=1;
						UI.window=app->window;
					}
					eventNotify(EVENT_RESUME,0,0,0);
                    eventNotify(EVENT_PAINT,0,0,0);
				}
				else
				{
					ANativeWindow_setBuffersGeometry(UI.app->window,
													 UI.w,UI.h,
													 WINDOW_FORMAT_RGBA_8888);
					startInThread(0, NULL);    // start Minimacy VM
					ui->started = 1;
					ui->window=app->window;
				}
			}
			break;
		case APP_CMD_TERM_WINDOW:
			LOGI("==========APP_CMD_TERM_WINDOW");
			engine_term_display(ui);
				ANativeWindow_setBuffersGeometry(app->window,
						  ANativeWindow_getWidth(app->window),
						  ANativeWindow_getHeight(app->window),
						  format);
			UI.w=UI.h=0;
			eventNotify(EVENT_SUSPEND,0,0,0);
			break;
		case APP_CMD_LOST_FOCUS:
			LOGI("==========APP_CMD_LOST_FOCUS");
			break;
		case APP_CMD_GAINED_FOCUS:
			LOGI("==========APP_CMD_GAINED_FOCUS");
			break;
		case APP_CMD_WINDOW_RESIZED:	//3
			LOGI("window resized %d x %d",ANativeWindow_getWidth(app->window),ANativeWindow_getHeight(app->window));
			ui->w=ANativeWindow_getWidth(app->window);
			ui->h=ANativeWindow_getHeight(app->window);

			ANativeWindow_setBuffersGeometry(app->window,
											 ui->w,
											 ui->h,
											 WINDOW_FORMAT_RGBA_8888);
			eventNotify(EVENT_SIZE,ui->w,ui->h,0);
			break;
		case APP_CMD_WINDOW_REDRAW_NEEDED:	//4
			LOGI("redraw needed %d x %d",ANativeWindow_getWidth(app->window),ANativeWindow_getHeight(app->window));
			eventNotify(EVENT_PAINT,0,0,0);
			break;
		case APP_CMD_CONTENT_RECT_CHANGED:	//5
			LOGI("rect changed %d x %d",ANativeWindow_getWidth(app->window),ANativeWindow_getHeight(app->window));
			break;
	}
}
void systemMainDir(char* path, int len, const char* argv0)
{
	if (UI.app) sprintf(path,"%s/",UI.app->activity->internalDataPath);
	else strcpy(path, "");
//	LOGI("minimacy dir=%s\n%d\n",path,strlen(path));
}
void systemCurrentDir(char* path, int len) { path[0] = 0; }
void systemUserDir(char* path, int len) { path[0] = 0; }

void assetDirCopy(struct android_app* app, char* dir,char* volume )
{
	const char* filename;
	char dirPath[1024];
	AAssetManager * mgr = app->activity->assetManager;
	const char* systemPath=app->activity->internalDataPath;

	AAssetDir* assetDir = AAssetManager_openDir(mgr, dir);

	sprintf(dirPath,"%s/%s",systemPath,volume);
	mkdir(dirPath,-1);

	if (strlen(dir)) {
		sprintf(dirPath,"%s/%s/%s",systemPath,volume,dir);
		mkdir(dirPath,-1);
	}
	while ((filename = AAssetDir_getNextFileName(assetDir)) != NULL) {
		char srcPath[1024];
		char filePath[1024];
		if (strlen(dir)) sprintf(srcPath,"%s/%s",dir,filename);
		else sprintf(srcPath,"%s",filename);

		sprintf(filePath,"%s/%s",dirPath,filename);
//        LOGI("========asset: %s -> %s",srcPath,filePath);

		AAsset* asset = AAssetManager_open(mgr, srcPath, AASSET_MODE_STREAMING);
		if (!asset) continue;

		FILE* out = fopen(filePath, "w");
		if (out) {
			char buf[BUFSIZ];
			int nb_read;
			while ((nb_read = AAsset_read(asset, buf, BUFSIZ)) > 0) {
				fwrite(buf, nb_read, 1, out);
			}
			fclose(out);
		}
		else LOGI("========== fopen %s returns null",filePath);
		AAsset_close(asset);
	}
	AAssetDir_close(assetDir);
}
void android_main(struct android_app* app)
{
	memset(&UI, 0, sizeof(UI));
	UI.app=app;
	app->userData = &UI;
	app->onAppCmd = engine_handle_cmd;
	app->onInputEvent = engine_handle_input;

	assetDirCopy(app,"bios","rom");
	assetDirCopy(app,"core","rom");
	assetDirCopy(app,"rsc","rom");
	assetDirCopy(app,"rsc/cursors","rom");
	assetDirCopy(app,"rsc/fonts","rom");
	assetDirCopy(app,"","programs");
	assetDirCopy(app,"demo","programs");
	assetDirCopy(app,"usr","programs");
	assetDirCopy(app,"test","programs");
	while (1) {
		// Read all pending events.
		int ident;
		int events;
		struct android_poll_source* source;

		while ((ident=ALooper_pollAll( 100 , NULL, &events,
				(void**)&source)) >= 0) {

			// Process this event.
			if (source != NULL) {
				source->process(app, source);
			}

			// Check if we are exiting.
			if (app->destroyRequested != 0) {
				LOGI("Engine thread destroy requested!");
				engine_term_display(&UI);
				return;
			}
		}
	}
}
