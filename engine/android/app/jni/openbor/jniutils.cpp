#include "jniutils.h"
#include <jni.h>
#include "SDL.h"

#define ACTIVITY_CLS_NAME "GameActivity"

void jniutils_vibrate_device()
{
  // retrieve the JNI environment
  JNIEnv  *env = (JNIEnv*)SDL_AndroidGetJNIEnv();

  // retrieve the Java instance of the GameActivity
  jobject activity = (jobject)SDL_AndroidGetActivity();

  // find the Java class of the activity. It should be GameActivity.
  jclass cls = env->GetObjectClass(activity);

  // find the identifier of the method to call
  jmethodID method_id = env->GetStaticMethodID(cls, "jni_vibrate", "()V");

  // effectively call the Java method
  env->CallStaticVoidMethod(cls, method_id);

  // clean up the local references
  env->DeleteLocalRef(cls);
  env->DeleteLocalRef(activity);
}

