#include <string.h>
#include <jni.h>

#include "SDL.h"
#include "globals.h"
#include "borendian.h"
#include "sdlport.h"
extern "C"
{
    #include "control.h"
}

extern float bx[MAXTOUCHB];
extern float by[MAXTOUCHB];
extern float br[MAXTOUCHB];
extern int nativeWidth;
extern int nativeHeight;

extern "C" JNIEXPORT jint JNICALL Java_org_libsdl_app_SDLActivity_isNativeVibrationEnabled(JNIEnv* env, jobject obj)
{
    return is_touchpad_vibration_enabled();
}


extern "C" JNIEXPORT jint JNICALL Java_org_libsdl_app_SDLActivity_isTouchArea(JNIEnv* env, jobject obj, jfloat x, jfloat y)
{
    return is_touch_area(x*nativeWidth, y*nativeHeight);
}

/*extern "C" void Java_org_libsdl_app_SDLActivity_setRootDir(JNIEnv* env, jclass cls, jstring dir)
{
    const char *rootdir = env->GetStringUTFChars(dir, 0);
    extern char rootDir[MAX_BUFFER_LEN];
    strcpy(rootDir, rootdir);

    env->ReleaseStringUTFChars(dir, rootdir);
}*/

