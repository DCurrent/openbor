
#include "SDL_config.h"

#ifdef __ANDROID__

/* Include the SDL main definition header */
#include "SDL_main.h"

/*******************************************************************************
                 Functions called by JNI
*******************************************************************************/
#include <jni.h>

// Called before SDL_main() to initialize JNI bindings in SDL library
extern "C" void SDL_Android_Init(JNIEnv* env, jclass cls);

// Library init
extern "C" jint JNI_OnLoad(JavaVM* vm, void* reserved);

// Start up the SDL app
extern "C" void Java_org_libsdl_app_SDLActivity_nativeInit(JNIEnv* env, jclass cls, jobject obj)
{
    /* This interface could expand with ABI negotiation, calbacks, etc. */
    SDL_Android_Init(env, cls);

    /* Run the application code! */
    int status;
    char *argv[2];
    argv[0] = strdup("SDL_app");
    argv[1] = NULL;
    status = SDL_main(1, argv);
	
    /* Do not issue an exit or the whole application will terminate instead of just the SDL thread */
    //exit(status);
}

extern "C" void control_update_android_touch(float* px, float* py, int* pid, int maxp);
extern "C" void Java_org_libsdl_app_SDLActivity_nativeUpdateTouchStates(JNIEnv* env, jclass cls, jfloatArray px, jfloatArray py, jintArray pid, jint maxp)
{
	jfloat* pxs = env->GetFloatArrayElements(px,0);
	jfloat* pys = env->GetFloatArrayElements(py,0);
	jint* pids = env->GetIntArrayElements(pid,0);
	static float pxbuf[64]; //enough
	static float pybuf[64];
	static int pidbuf[64];
	for(int i=0; i<maxp; i++)
	{
		pxbuf[i] = pxs[i];
		pybuf[i] = pys[i];
		pidbuf[i] = pids[i];
	}
	control_update_android_touch(pxbuf, pybuf, pidbuf, maxp);
	env->ReleaseFloatArrayElements(px, pxs, 0);
	env->ReleaseFloatArrayElements(py, pys, 0);
	env->ReleaseIntArrayElements(pid, pids, 0);
}


/*
extern "C" Uint8* SDL_GetKeyboardState(int *numkeys);
extern "C" void writeToLogFile(const char* fmt, ...);
//key events hanlding
extern "C" void Java_org_openbor_engine_SDLMainActivity_onNativeKeyDown(JNIEnv* env, jclass cls, int keycode)
{
	int nk;
	Uint8* keystate = SDL_GetKeyboardState(&nk);
	writeToLogFile("\nkey down %d / %d\n", keycode, nk);
	if(keycode>0 && keycode<nk)
		keystate[keycode] = 1;
}

extern "C" void Java_org_openbor_engine_SDLMainActivity_onNativeKeyUp(JNIEnv* env, jclass cls, int keycode)
{
	int nk;
	Uint8* keystate = SDL_GetKeyboardState(&nk);
	writeToLogFile("\nkey up %d / %d\n", keycode, nk);
	if(keycode>0 && keycode<nk)
		keystate[keycode] = 0;
}
*/
#endif /* __ANDROID__ */

/* vi: set ts=4 sw=4 expandtab: */
