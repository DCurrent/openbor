#include <string.h>
#include <jni.h>

#include "SDL.h"
#include "globals.h"
#include "borendian.h"
#include "sdlport.h"
#include "control.h"
#include "openbor.h"

extern float bx[MAXTOUCHB];
extern float by[MAXTOUCHB];
extern float br[MAXTOUCHB];
extern int nativeWidth;
extern int nativeHeight;

int is_touch_area(float x, float y)
{
	int j;
	float tx, ty, tr;
	float r[MAXTOUCHB];
	float dirx, diry, circlea, circleb, tan;

	for(j=0; j<MAXTOUCHB; j++)
	{
		r[j] = br[j]*br[j]*(1.5*1.5);
	}
	dirx = (bx[SDID_MOVERIGHT]+bx[SDID_MOVELEFT])/2.0;
	diry = (by[SDID_MOVEUP]+by[SDID_MOVEDOWN])/2.0;
	circlea = bx[SDID_MOVERIGHT]-dirx-br[SDID_MOVEUP];
	circleb = bx[SDID_MOVERIGHT]-dirx+br[SDID_MOVEUP]*1.5;
	circlea *= circlea;
	circleb *= circleb;
	#define tana 0.577350f
	#define tanb 1.732051f
    tx = x-dirx;
    ty = y-diry;
    tr = tx*tx + ty*ty;
    //direction button logic is different, check a ring instead of individual buttons
    if(tr>circlea && tr<=circleb)
    {
        if(tx<0)
        {
            tan = ty/tx;
            if(tan>=-tana && tan<=tana)
            {
                return 1;
            }
            else if(tan<-tanb)
            {
                return 1;
            }
            else if(tan>tanb)
            {
                return 1;
            }
            else if(ty<0)
            {
                return 1;
            }
            else
            {
                return 1;
            }
        }
        else if(tx>0)
        {
            tan = ty/tx;
            if(tan>=-tana && tan<=tana)
            {
                return 1;
            }
            else if(tan<-tanb)
            {
                return 1;
            }
            else if(tan>tanb)
            {
                return 1;
            }
            else if(ty<0)
            {
                return 1;
            }
            else
            {
                return 1;
            }
        }
        else
        {
            if(ty>0)
            {
                return 1;
            }
            else
            {
                return 1;
            }
        }
    }
    //rest buttons
    for(j=0; j<MAXTOUCHB; j++)
    {
        if(j==SDID_MOVERIGHT || j==SDID_MOVEUP ||
            j==SDID_MOVELEFT || j==SDID_MOVEDOWN)
            continue;
        tx = x-bx[j];
        ty = y-by[j];
        tr = tx*tx + ty*ty;
        if(tr<=r[j])
        {
            return 1;
        }
    }
	#undef tana
	#undef tanb

	return 0;
}

extern "C" JNIEXPORT jint JNICALL Java_org_libsdl_app_SDLActivity_getTouchVibration(JNIEnv* env, jobject obj, jfloat x, jfloat y)
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

