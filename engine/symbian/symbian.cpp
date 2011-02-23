// optim
#include <eikapp.h>
#include <e32base.h>
#include <sdlapp.h>

#include <unistd.h>
extern "C"
{
#include "vsnprintf.h"
}

FILE* mystdout = NULL;
FILE* mystderr = NULL;
class CSymbianApp : public CSDLApp {
public:
	CSymbianApp();
	~CSymbianApp();
#if defined (UIQ3)
	/**
	* Returns the resource id to be used to declare the views supported by this UIQ3 app
	* @return TInt, resource id
	*/
	TInt ViewResourceId();
#endif
	void PreInitializeAppL()
	{
		int drive = 0;
		char resourcedir[256];
		strcpy(resourcedir,"d:\\openbor");
		for(drive = 'D';drive<'Z';drive++)
		{
			resourcedir[0] = drive;
			if(access(resourcedir, F_OK | R_OK) == 0)
			{
				break;
			}          
		}

		if(drive == 'Z')
		{
#ifdef UIQ3
		strcpy(resourcedir, "c:\\shared\\openbor");
#else
		strcpy(resourcedir, "c:\\data\\openbor");
#endif
		}
		chdir(resourcedir);
#ifdef UIQ3
		mystdout = fopen("c:\\shared\\openbor\\stdout.txt","w");
		mystderr = fopen("c:\\shared\\openbor\\stderr.txt","w");
#else
		mystdout = fopen("c:\\data\\openbor\\stdout.txt","w");
		mystderr = fopen("c:\\data\\openbor\\stderr.txt","w");
#endif
		*stdout = *mystdout;
		*stderr = *mystderr;
	}

	TUid AppDllUid() const;

};

#ifdef EPOC_AS_APP
// this function is called automatically by the SymbianOS to deliver the new CApaApplication object
#if !defined (UIQ3) && !defined (S60V3)
EXPORT_C 
#endif
CApaApplication* NewApplication() {
	return new CSymbianApp;
}

#if defined (UIQ3) || defined (S60V3)
#include <eikstart.h>
// E32Main() contains the program's start up code, the entry point for an EXE.
GLDEF_C TInt E32Main() {
	return EikStart::RunApplication(NewApplication);
}
#endif

#endif // EPOC_AS_APP

#if !defined (UIQ3) && !defined (S60V3)
GLDEF_C  TInt E32Dll(TDllReason) {
	return KErrNone;
}
#endif

CSymbianApp::CSymbianApp() 
{
}

CSymbianApp::~CSymbianApp() 
{
}

#if defined (UIQ3)
#include <openbor.rsg>
/**
* Returns the resource id to be used to declare the views supported by this UIQ3 app
* @return TInt, resource id
*/
TInt CSymbianApp::ViewResourceId() {
	return R_SDL_VIEW_UI_CONFIGURATIONS;
}
#endif

/**
*   Responsible for returning the unique UID of this application
* @return unique UID for this application in a TUid
**/
TUid CSymbianApp::AppDllUid() const {
	return TUid::Uid(0xA000E813);
}

extern "C"
	{
	int GetFreeAmount()
		{
		TInt biggestBlock;
		TInt avail = User::Available(biggestBlock);
		TMemoryInfoV1Buf info;
		UserHal::MemoryInfo(info);
		return avail+info().iFreeRamInBytes; 
		}
	
	int GetTotalAmount()
		{
		TMemoryInfoV1Buf info;
		UserHal::MemoryInfo(info);
		TInt maxLength = User::Heap().MaxLength();
		if(info().iTotalRamInBytes < maxLength)
			{
			return info().iTotalRamInBytes;			
			}
		else
			{
			return maxLength;
			}		
		}
}
