#include <string.h>
#include <jni.h>


extern "C" void Java_org_libsdl_app_SDLActivity_setRootDir(JNIEnv* env, jclass cls, jstring dir)
{
    const char *rootdir = env->GetStringUTFChars(dir, 0);
    extern char rootDir[128];
    strcpy(rootDir, rootdir);

    env->ReleaseStringUTFChars(dir, rootdir);
}

