/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

#include <XBApp.h>
#include <XBFont.h>
#include <XBHelp.h>
#include <XBSound.h>
#include <xgraphics.h>
#include <assert.h>
#include <d3d8perf.h>
#include "SndXBOX.hxx"
#include <stdio.h>
#include <vector>
#include "undocumented.h"
#include "iosupport.h"
#include "panel.h"
#include "custom_launch_params.h"
#include "keyboard_api.h"
#include "gamescreen.h"


DWORD g_dwSoundThreadId ;
HANDLE g_hSoundThread ;

#ifdef __cplusplus
extern "C" {
#endif

#include "xboxport.h"
#include "packfile.h"

#define uint8 unsigned char
#define uint32 unsigned int

void xbox_init(void);
void xbox_return(void);

void _2xSaI(uint8 *srcPtr, uint32 srcPitch,
		 uint8 *deltaPtr,
		 uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode);
void _2xSaIScanline(uint8 *srcPtr, uint32 srcPitch,
		 uint8 *deltaPtr,
		 uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode);
void SuperEagle(uint8 *srcPtr, uint32 srcPitch,
		uint8 *deltaPtr,
		 uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode);
void SuperEagleScanline(uint8 *srcPtr, uint32 srcPitch,
		uint8 *deltaPtr,
		 uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode);
void Super2xSaI(uint8 *srcPtr, uint32 srcPitch,
		 uint8 *deltaPtr,
		 uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode);
void Super2xSaIScanline(uint8 *srcPtr, uint32 srcPitch,
		 uint8 *deltaPtr,
		 uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode);
void Scale2x(uint8 *srcPtr, uint32 srcPitch,
	     uint8 *deltaPtr,
	     uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode);
void SuperScale(uint8 *srcPtr, uint32 srcPitch,
	     uint8 *deltaPtr,
	     uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode);
void SuperScaleScanline(uint8 *srcPtr, uint32 srcPitch,
	     uint8 *deltaPtr,
	     uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode);
void Eagle(uint8 *srcPtr, uint32 srcPitch,
	     uint8 *deltaPtr,
	     uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode);
void EagleScanline(uint8 *srcPtr, uint32 srcPitch,
	     uint8 *deltaPtr,
	     uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode);

void Simple2x(unsigned char *srcPtr, uint32 srcPitch, unsigned char * /* deltaPtr */,
              unsigned char *dstPtr, uint32 dstPitch, int width, int height, int scanlines) ;
void AdMame2x(unsigned char *srcPtr, uint32 srcPitch, unsigned char * /* deltaPtr */,
              unsigned char *dstPtr, uint32 dstPitch, int width, int height, int scanline) ;

char packfile[128] = {"d:\\Paks\\menu.pak"};

#ifdef __cplusplus
}
#endif

#pragma warning(disable:4244)
#pragma warning(disable:4018)
#pragma warning(disable:4101)

#define PLATFORM_SAV L"OPENBORSAV"
#define PLATFORM_INI "OpenBoR.ini"
#define DEFAULT_SAVE_PATH "d:\\Saves"

//-----------------------------------------------------------------------------
// Name: class CXBoxSample
// Desc: Main class to run this application. Most functionality is inherited
//       from the CXBApplication base class.
//-----------------------------------------------------------------------------
class CXBoxSample : public CXBApplication
{
public:
    CXBoxSample();

    virtual HRESULT Initialize();
    virtual HRESULT InitializeWithScreen();
    virtual HRESULT FrameMove();
	virtual void    initConsole(UINT32 idx, int isFavorite, int forceConfig);
	virtual int     init_texture();
	virtual void    doScreenSize();
	virtual BOOL    SetRefreshRate(INT iRefreshRate);
	virtual int		render_to_texture(int src_w, int src_h, s_screen* source);
	virtual void    saveSettings(char *filename);
	virtual int     loadSettings(char *filename);
	void            setPalette(unsigned char *palette);
	virtual void	pollXBoxControllers(void);
	virtual void    xboxChangeFilter();
	void            ClearScreen();
	void            ResetResolution();
	virtual void    fillPresentationParams();



	D3DCOLOR m_color_palette[256];
	DWORD m_startTime ;
	DWORD m_joypad ;

	D3DCOLOR			color_palette[256];


	unsigned long m_xboxControllers[4] ;

	D3DXVECTOR2 m_gameVecScale ;
	D3DXVECTOR2 m_gameVecTranslate ;
	RECT m_gameRectSource ;
	int m_throttle ;

	int m_xboxSFilter ;
	unsigned int m_pitch ;


	D3DPRESENT_PARAMETERS	m_origPP ;

	CPanel m_pnlBackgroundMain ;
	CPanel m_pnlBackgroundSelect ;
	CPanel m_pnlBackgroundOther ;
	CPanel m_pnlSplashEmu ;
	CPanel m_pnlSplashGame ;
	CPanel m_pnlPopup ;
	CPanel m_pnlGameScreen ;
	CPanel m_pnlGameScreen2 ;


	CIoSupport m_io ;



	LPDIRECT3DTEXTURE8	Texture;
	LPDIRECT3DTEXTURE8	Texture2;
	LPDIRECT3DTEXTURE8	WhiteTexture;
	LPD3DXSPRITE			Sprite;
	LPD3DXSPRITE			MenuSprite;

	byte*				g_pBlitBuff ;
	byte*				g_pDeltaBuff ;

	WCHAR      m_strMessage[80];
	UINT32              m_msgDelay ;


	SoundXBOX			m_sound;
	char				g_savePath[500] ;
	char				g_saveprefix[500] ;


	// Indicates the width and height of the screen
	UINT32 theWidth;
	UINT32 theHeight;
    RECT SrcRect;
    RECT DestRect;

	int					m_nScreenX, m_nScreenY, m_nScreenMaxX, m_nScreenMaxY ;


};

#ifdef __cplusplus
extern "C" {
#endif


void hq2x_16(unsigned char*, unsigned char*, DWORD, DWORD, DWORD, DWORD, DWORD);
unsigned int   LUT16to32[65536];
unsigned int   RGBtoYUV[65536];

#ifdef __cplusplus
}
#endif

void hq2x_16_stub(uint8 *srcPtr, uint32 srcPitch, uint8 *deltaPtr, uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode )
{
    hq2x_16( srcPtr, dstPtr, width, height, dstPitch, srcPitch - (width<<1), dstPitch - (width<<2));

}

void dummy_blitter(uint8 *srcPtr, uint32 srcPitch, uint8 *deltaPtr, uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode )
{
}

struct blitters
{
	void (*blitfunc)(uint8 *srcPtr, uint32 srcPitch, uint8 *deltaPtr, uint8 *dstPtr, uint32 dstPitch, int width, int height, int scanmode ) ;
	char name[100] ;
	float  multiplier ;
} SOFTWARE_FILTERS[] =
{
	{ dummy_blitter, "None", 1 },
	{ _2xSaI, "2xSai", 2},
	{ Super2xSaI, "Super 2xSai", 2},
	{ hq2x_16_stub, "HQ2X", 2},
	{ Eagle, "Eagle2x", 2},
	{ SuperEagle, "Super Eagle 2x", 2},
	{ SuperScale, "SuperScale 2x", 2},
	{ AdMame2x, "AdvanceMame 2x", 2},
	{ Simple2x, "Simple 2x", 2},
	{ _2xSaIScanline, "2xSai Scanline", 2},
	{ Super2xSaIScanline, "Super 2xSai Scanline", 2},
	{ EagleScanline, "Eagle2x Scanline", 2},
	{ SuperEagleScanline, "Super Eagle2x Scanline", 2},
	{ SuperScaleScanline, "SuperScale 2x Scanline", 2},
} ;

#define HQFILTERNUM 1

#define NUM_SOFTWARE_FILTERS 14

void InitLUTs(void)
{
  int i, j, k, r, g, b, Y, u, v;

  for (i=0; i<65536; i++)
    LUT16to32[i] = ((i & 0xF800) << 8) + ((i & 0x07E0) << 5) + ((i & 0x001F) << 3);

  for (i=0; i<32; i++)
  for (j=0; j<64; j++)
  for (k=0; k<32; k++)
  {
    r = i << 3;
    g = j << 2;
    b = k << 3;
    Y = (r + g + b) >> 2;
    u = 128 + ((r - b) >> 2);
    v = 128 + ((-r + 2*g -b)>>3);
    RGBtoYUV[ (i << 11) + (j << 5) + k ] = (Y<<16) + (u<<8) + v;
  }

}



CXBoxSample *g_app ;
char global_error_message[1024] ;
unsigned int m_performanceFreq[2] ;
unsigned int m_performancePrev[2] ;


int recreate( D3DPRESENT_PARAMETERS *pparams )
{
	char tmpfilename[500] ;

	SAFE_RELEASE( g_app->Sprite ) ;
	SAFE_RELEASE( g_app->MenuSprite ) ;

				g_app->m_pd3dDevice->Release();

				//g_app->fillPresentationParams() ;

				if( FAILED( g_app->m_pD3D->CreateDevice( 0, D3DDEVTYPE_HAL, NULL,
													   D3DCREATE_HARDWARE_VERTEXPROCESSING,
													   pparams, &g_app->m_pd3dDevice ) ) )
				{
					return 0 ;
				}
				else
				{
					g_pd3dDevice = g_app->m_pd3dDevice ;

					g_app->m_pnlBackgroundMain.Recreate( g_app->m_pd3dDevice );
					g_app->m_pnlBackgroundSelect.Recreate( g_app->m_pd3dDevice );
					g_app->m_pnlBackgroundOther.Recreate( g_app->m_pd3dDevice );
					g_app->m_pnlSplashEmu.Recreate( g_app->m_pd3dDevice );
					g_app->m_pnlSplashGame.Recreate( g_app->m_pd3dDevice );
					g_app->m_pnlPopup.Recreate( g_app->m_pd3dDevice );
					g_app->m_pnlGameScreen.Recreate( g_app->m_pd3dDevice );

					return 1 ;

				}
}

void xbox_set_refreshrate( int rate )
{
	int nativeRate ;
	D3DPRESENT_PARAMETERS holdpp ;

	if(XGetVideoStandard() == XC_VIDEO_STANDARD_PAL_I)
	{
		//get supported video flags
		DWORD videoFlags = XGetVideoFlags();

		//set pal60 if available.
		if(videoFlags & XC_VIDEO_FLAGS_PAL_60Hz)
			nativeRate = 60 ;
		else
			nativeRate = 50 ;
	}
	else
		nativeRate = 60 ;

	if ( rate != g_app->m_d3dpp.FullScreen_RefreshRateInHz )
	{
		g_app->m_d3dpp.FullScreen_RefreshRateInHz = rate ;

		if ( nativeRate != rate )
		{
			g_app->m_d3dpp.Flags |= D3DPRESENTFLAG_EMULATE_REFRESH_RATE ;
		}
		else
		{
			g_app->m_d3dpp.Flags &= ~D3DPRESENTFLAG_EMULATE_REFRESH_RATE ;
		}
		memcpy( &holdpp, &g_app->m_d3dpp, sizeof(D3DPRESENT_PARAMETERS) ) ;

		recreate( &g_app->m_d3dpp ) ;

		memcpy( &g_app->m_d3dpp, &holdpp, sizeof(D3DPRESENT_PARAMETERS) ) ;

		g_app->m_pd3dDevice->Reset(&g_app->m_d3dpp);

		memcpy( &g_app->m_d3dpp, &holdpp, sizeof(D3DPRESENT_PARAMETERS) ) ;
	}
}



int CXBoxSample::init_texture()
{
	D3DCOLOR *palette ;

	// Release any previous texture
	if (Texture)
	{
		return 1 ;
		Texture->BlockUntilNotBusy() ;
		Texture->Release();
		Texture = NULL;
	}

	theWidth = 320*4 ;
	theHeight = 240*4 ;



	// Create the texture
	if (D3DXCreateTextureFromFileInMemoryEx(m_pd3dDevice, GAMESCREEN_FILE, sizeof(GAMESCREEN_FILE),
		 theWidth, theHeight, 1, 0, D3DFMT_LIN_R5G6B5, D3DPOOL_MANAGED,
		 //D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED,
		 D3DX_FILTER_NONE , D3DX_FILTER_NONE, 0, NULL, NULL, &Texture)==D3D_OK)
	{
		m_pnlGameScreen.m_pTexture = NULL ;
		if ( FAILED(m_pnlGameScreen.Create(m_pd3dDevice, Texture, FALSE, theWidth, theHeight)) )
		{
			//popupMsg( "no create panel", &m_pnlBackgroundOther ) ;
		}
	}
	else
	{
		//popupMsg( "no load texture", &m_pnlBackgroundOther ) ;
	}

	D3DSURFACE_DESC desc;
    Texture->GetLevelDesc(0, &desc);


	if (g_pBlitBuff != NULL)
	{
		delete [] g_pBlitBuff;
		g_pBlitBuff = NULL;
	}

	// Allocate a buffer to blit our frames to
	g_pBlitBuff = new byte[ desc.Size ];

	if (g_pDeltaBuff != NULL)
	{
		delete [] g_pDeltaBuff;
		g_pDeltaBuff = NULL;
	}

	// Allocate a buffer to blit our frames to
	g_pDeltaBuff = new byte[desc.Size];



	if ( g_pDeltaBuff == NULL )
		return 1 ;

	memset( g_pDeltaBuff, 0x00, desc.Size ) ;

	RECT rectSource;
	rectSource.top = 0;
	rectSource.left = 0;
	rectSource.bottom = theHeight-1 ;
	rectSource.right  = theWidth-1 ;

	D3DLOCKED_RECT d3dlr;
	Texture->LockRect(0, &d3dlr, &rectSource, 0);

	m_pitch = d3dlr.Pitch ;

	// Unlock our texture
	Texture->UnlockRect(0);

	return 0;
}



void  CXBoxSample::ResetResolution() {
	m_pd3dDevice->Reset(&m_d3dpp);
	m_pd3dDevice->Clear(0,NULL,D3DCLEAR_TARGET,0x0,1.0f,0);
	m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
	//Device->SetFlickerFilter(FlickerFilter) ;
	//Device->SetSoftDisplayFilter(Soften) ;
}




void CXBoxSample::saveSettings( char *filename )
{

	FILE *setfile ;

	setfile = fopen( filename, "wb" ) ;

	if ( !setfile )
		return ;

	fwrite( &m_xboxSFilter, sizeof(unsigned int), 1, setfile ) ;
	fwrite( &m_nScreenX, sizeof(unsigned int), 1, setfile ) ;
	fwrite( &m_nScreenY, sizeof(unsigned int), 1, setfile ) ;
	fwrite( &m_nScreenMaxX, sizeof(unsigned int), 1, setfile ) ;
	fwrite( &m_nScreenMaxY, sizeof(unsigned int), 1, setfile ) ;

	fclose( setfile ) ;

}


void CXBoxSample::setPalette( unsigned char *palette)
{

	memset( color_palette, 0, 256*sizeof(D3DCOLOR) ) ;
	DWORD r, g, b;



	for (int i = 0; i < 256; i++)
	{
		r = palette[i*3] ;
		g = palette[(i*3)+1] ;
		b = palette[(i*3)+2] ;

		color_palette[i] =  ( ( r >> 3 ) << 11 ) | ( ( g >> 2 ) << 5 ) | ( b >> 3 )  ;
	}
}

int CXBoxSample::loadSettings(char *filename)
{

	FILE *setfile ;

	setfile = fopen( filename, "rb" ) ;

	if ( !setfile )
	{
		saveSettings( filename ) ;
		return 1;
	}

	fread( &m_xboxSFilter, sizeof(unsigned int), 1, setfile ) ;
	fread( &m_nScreenX, sizeof(unsigned int), 1, setfile ) ;
	fread( &m_nScreenY, sizeof(unsigned int), 1, setfile ) ;
	fread( &m_nScreenMaxX, sizeof(unsigned int), 1, setfile ) ;
	fread( &m_nScreenMaxY, sizeof(unsigned int), 1, setfile ) ;


	fclose( setfile ) ;

	if ( ( m_xboxSFilter < 0 ) || ( m_xboxSFilter >= NUM_SOFTWARE_FILTERS ))
	{
		m_xboxSFilter = 0 ;

	}

	return 0 ;
}

HRESULT CXBoxSample::InitializeWithScreen()
{
	FILE *debugfile ;
	char initext[100] ;
	char szDevice[2000] ;
	char *fpos, *epos ;
	int numread ;
	char tmpfilename[MAX_PATH] ;



	XBInput_GetInput();


	XMountUtilityDrive( TRUE )  ;

	m_io.Unmount("C:") ;
	m_io.Unmount("E:") ;
	m_io.Unmount("F:") ;
	m_io.Unmount("G:") ;
	m_io.Unmount("X:") ;
	m_io.Unmount("Y:") ;
	m_io.Unmount("Z:") ;
	m_io.Unmount("R:") ;
	m_io.Mount("C:", "Harddisk0\\Partition2");
	m_io.Mount("E:", "Harddisk0\\Partition1");
	m_io.Mount("F:", "Harddisk0\\Partition6");
	m_io.Mount("G:", "Harddisk0\\Partition7");
	m_io.Mount("X:", "Harddisk0\\Partition3");
	m_io.Mount("Y:", "Harddisk0\\Partition4");
	m_io.Mount("Z:", "Harddisk0\\Partition5");
	m_io.Mount("R:","Cdrom0");

	XFormatUtilityDrive() ;


	char szNewDir[MAX_PATH] ;
	char *s, *p ;



	strcpy( szNewDir, DEFAULT_SAVE_PATH ) ;

	s = szNewDir+3 ;


	while ( p = strchr( s, '\\' ) )
	{
		*p = 0 ;
		CreateDirectory( szNewDir, NULL ) ;

		*p = '\\' ;

		p++ ;
		s = p ;
	}


	CreateDirectory( DEFAULT_SAVE_PATH, NULL ) ;


	m_xboxSFilter = 0 ;
	m_nScreenMaxX = 640 ;
	m_nScreenMaxY = 480 ;
	m_nScreenX = 0 ;
	m_nScreenY = 0 ;






	DWORD tdiff = GetTickCount() ;

	tdiff = GetTickCount() - tdiff ;


	m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL|D3DCLEAR_TARGET,
						 0x00000000, 0.0f, 0L );


	m_pd3dDevice->Present( NULL, NULL, NULL, NULL );


	QueryPerformanceFrequency((union _LARGE_INTEGER *) m_performanceFreq);

	InitLUTs() ;

	XGetCustomLaunchData() ;

	m_msgDelay = 0 ;
	wcscpy( m_strMessage, L" " ) ;

	g_pBlitBuff = NULL ;
	g_pDeltaBuff = NULL ;

	WhiteTexture = NULL ;
	Texture = NULL ;
	Sprite = NULL ;


	MenuSprite = NULL ;


	g_saveprefix[0] = 0 ;


	initConsole(0,0,0) ;

    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: FrameMove
// Desc: Performs per-frame updates
//-----------------------------------------------------------------------------
HRESULT CXBoxSample::FrameMove()
{

	InitializeWithScreen() ;
    return S_OK;
}


int CXBoxSample::render_to_texture(int src_w, int src_h, s_screen* source)
{
    int vp_vstart ;
    int vp_vend   ;
    int vp_hstart ;
    int vp_hend   ;
	int w,h ;
	RECT src, dst;
	WORD *curr1 ;
	byte *curr2 ;
	DWORD pitchDiff1 ;
	DWORD pitchDiff2 ;
	D3DCOLOR *palette ;
	char palstr[200] ;
	DWORD pixel ;
	byte r,g,b ;

	// Get a description of our level 0 texture so we can figure
	// out the pitch of the texture

	D3DSURFACE_DESC desc;
    Texture->GetLevelDesc(0, &desc);


	// Allocate a buffer to blit our frames to
	unsigned char *bsrc = source->data;
	unsigned short  *bdst = (unsigned short*)g_pBlitBuff;

	for ( int y = 0 ; y < source->height ; y++ )
	{
		for ( int x = 0 ; x < source->width ; x++ )
		{
			*(bdst+x) = color_palette[ *bsrc++ ] ;
		}
		bdst += m_pitch/2 ;
	}


	// Figure out how big of a rect to lock in our texture
	RECT rectSource;
	rectSource.top = 0;
	rectSource.left = 0;
	rectSource.bottom = source->height;
	rectSource.right  = source->width;

	// Lock the rect in our texture
	D3DLOCKED_RECT d3dlr;
	Texture->LockRect(0, &d3dlr, &rectSource, 0);

	float mx, my ;

	if ( m_xboxSFilter )
	{
		SOFTWARE_FILTERS[m_xboxSFilter].blitfunc(g_pBlitBuff + m_pitch,m_pitch, g_pDeltaBuff+m_pitch, ((unsigned char*)d3dlr.pBits)+m_pitch, m_pitch, src_w, src_h, 0 ) ;
		Texture->UnlockRect(0);
		g_pd3dDevice->Clear(0L, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0L);
		g_pd3dDevice->BeginScene();

		mx = (float)m_nScreenMaxX / ((float)src_w*SOFTWARE_FILTERS[m_xboxSFilter].multiplier) ;
		my = (float)m_nScreenMaxY / ((float)src_h*SOFTWARE_FILTERS[m_xboxSFilter].multiplier);

		m_gameRectSource.top = 0 ;
		m_gameRectSource.left = 0 ;
		m_gameRectSource.bottom = (src_h)*SOFTWARE_FILTERS[m_xboxSFilter].multiplier ;
		m_gameRectSource.right  = (src_w)*SOFTWARE_FILTERS[m_xboxSFilter].multiplier ;

	}
	else
	{
		unsigned char *src = g_pBlitBuff;
		unsigned char *dst = (unsigned char*)d3dlr.pBits;

		for ( int y = 0 ; y < src_h ; y++ )
		{
			memcpy( dst, src, src_w*2 ) ;
			dst += m_pitch ;
			src += m_pitch ;
		}


		Texture->UnlockRect(0);
		g_pd3dDevice->Clear(0L, NULL, D3DCLEAR_TARGET, D3DCOLOR_XRGB(0,0,0), 1.0f, 0L);
		g_pd3dDevice->BeginScene();

		mx = (float)m_nScreenMaxX / ((float)src_w) ;
		my = (float)m_nScreenMaxY / ((float)src_h);

		m_gameRectSource.top = 0;
		m_gameRectSource.left = 0;
		m_gameRectSource.bottom = src_h;
		m_gameRectSource.right  = src_w;
	}



	m_gameVecScale.x = mx ; m_gameVecScale.y = my;
	m_gameVecTranslate.x = m_nScreenX ; m_gameVecTranslate.y = m_nScreenY ;
	D3DXCOLOR d3color(1.0, 1.0, 1.0, 1.0);




	m_pnlGameScreen.Render( m_gameRectSource.left, m_gameRectSource.top,m_gameRectSource.right,m_gameRectSource.bottom,m_nScreenX ,m_nScreenY, m_nScreenMaxX, m_nScreenMaxY) ;

	if ( global_error_message[0] )
	{
		WCHAR msg[500] ;
		m_msgDelay-- ;
		swprintf( msg, L"%S", global_error_message ) ;


		if ( m_msgDelay <= 0 )
		{
			global_error_message[0] = 0 ;
		}
	}

	// End the scene.
	g_pd3dDevice->EndScene();

	g_pd3dDevice->Present(NULL, NULL, NULL, NULL);

	return 1;

}

struct vidmodes
{
	char name[100] ;
	unsigned int width ;
	unsigned int height ;
	char progressive ;
	float multx ;
	float multy ;
} VIDEOMODES[] =
{
	{ "Standard 480i", 640, 480, 0, 1.0f, 1.0f },
	{ "480p", 640, 480, 1, 1.0f, 1.0f },
	{ "720p", 1280, 720, 1, 2.0f, 1.5f },
	{ "1080i", 1920, 1080, 0, 3.0f, 2.25f },
	{ "720x480", 720, 480, 0, 1.125f, 1.0f },
	{ "720x576", 720, 576, 0, 1.125f, 1.2f },
} ;

void CXBoxSample::fillPresentationParams()
{
    IDirect3D8 *pD3D;
    DWORD videoFlags = XGetVideoFlags();

    ZeroMemory(&m_d3dpp, sizeof(m_d3dpp));

    m_d3dpp.BackBufferWidth                 = 640;
    m_d3dpp.BackBufferHeight                = 480;
    m_d3dpp.BackBufferFormat                = D3DFMT_LIN_R5G6B5;
    m_d3dpp.Flags                           = D3DPRESENTFLAG_INTERLACED;
    m_d3dpp.BackBufferCount                 = 1;
    m_d3dpp.EnableAutoDepthStencil          = TRUE;
    m_d3dpp.AutoDepthStencilFormat          = D3DFMT_D16;
    m_d3dpp.SwapEffect                      = D3DSWAPEFFECT_DISCARD;
    m_d3dpp.FullScreen_PresentationInterval = D3DPRESENT_INTERVAL_DEFAULT;
    m_d3dpp.MultiSampleType                 = D3DMULTISAMPLE_NONE;

    if(XGetVideoStandard() == XC_VIDEO_STANDARD_PAL_I)
    {
        // Set pal60 if available.
        if(videoFlags & XC_VIDEO_FLAGS_PAL_60Hz) m_d3dpp.FullScreen_RefreshRateInHz = 60;
        else m_d3dpp.FullScreen_RefreshRateInHz = 50;
    }
    else m_d3dpp.FullScreen_RefreshRateInHz = 60;


    if(XGetAVPack() == XC_AV_PACK_HDTV)
	{
		if(videoFlags & XC_VIDEO_FLAGS_HDTV_1080i)
		{
			m_d3dpp.Flags            = D3DPRESENTFLAG_WIDESCREEN | D3DPRESENTFLAG_INTERLACED;
			m_d3dpp.BackBufferWidth  = 1920;
			m_d3dpp.BackBufferHeight = 1080;
			return;
		}
		else if(videoFlags & XC_VIDEO_FLAGS_HDTV_720p)
		{
			m_d3dpp.Flags            = D3DPRESENTFLAG_PROGRESSIVE | D3DPRESENTFLAG_WIDESCREEN;
			m_d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
			m_d3dpp.BackBufferWidth  = 1280;
			m_d3dpp.BackBufferHeight = 720;
			return;
		}
        else if(videoFlags & XC_VIDEO_FLAGS_HDTV_480p)
		{
			m_d3dpp.Flags            = D3DPRESENTFLAG_PROGRESSIVE;
			m_d3dpp.BackBufferFormat = D3DFMT_A8R8G8B8;
			m_d3dpp.BackBufferWidth  = 640;
			m_d3dpp.BackBufferHeight = 480;
			return;
		}
	}
}


CXBoxSample xbApp;
extern "C" {
	void changeResolution() {
		xbApp.fillPresentationParams() ;
		xbApp.ResetResolution();
	}
}

VOID __cdecl main()
{

	g_app = &xbApp ;



	xbApp.fillPresentationParams() ;


	memcpy( &xbApp.m_origPP, &xbApp.m_d3dpp, sizeof(xbApp.m_origPP) ) ;


    if( FAILED( xbApp.Create() ) )
	{
        return;
	}

	xbApp.Run();
}

//-----------------------------------------------------------------------------
// Name: CXBoxSample (constructor)
// Desc: Constructor for CXBoxSample class
//-----------------------------------------------------------------------------
CXBoxSample::CXBoxSample()
            :CXBApplication()
{
	global_error_message[0] = 0 ;

}


void CXBoxSample::ClearScreen()
{
	m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL|D3DCLEAR_TARGET,
						 0x00000000, 1.0f, 0L );
	m_pd3dDevice->Present( NULL, NULL, NULL, NULL );
}

#define XBOX_DPAD_UP          0x00000001
#define XBOX_DPAD_RIGHT       0x00000002
#define XBOX_DPAD_DOWN        0x00000004
#define XBOX_DPAD_LEFT        0x00000008
#define XBOX_A                0x00000010
#define XBOX_B                0x00000020
#define XBOX_X                0x00000040
#define XBOX_Y                0x00000080
#define XBOX_BLACK            0x00000100
#define XBOX_WHITE            0x00000200
#define XBOX_START            0x00000400
#define XBOX_BACK             0x00000800
#define XBOX_LEFT_TRIGGER     0x00001000
#define XBOX_RIGHT_TRIGGER    0x00002000
#define XBOX_LEFT_THUMB       0x00004000
#define XBOX_RIGHT_THUMB      0x00008000
#define XBOX_LTHUMB_UP        0x00010000
#define XBOX_LTHUMB_RIGHT     0x00020000
#define XBOX_LTHUMB_DOWN      0x00040000
#define XBOX_LTHUMB_LEFT      0x00080000
#define XBOX_RTHUMB_UP        0x00100000
#define XBOX_RTHUMB_RIGHT     0x00200000
#define XBOX_RTHUMB_DOWN      0x00400000
#define XBOX_RTHUMB_LEFT      0x00800000



void CXBoxSample::pollXBoxControllers(void)
{
	int i = 0;
	static int didfilter = 0;

	XBInput_GetInput();

	for(i=0; i<4; i++)
	{
		m_xboxControllers[i] = 0;

		if(g_Gamepads[i].hDevice)
		{
			if(g_Gamepads[i].bAnalogButtons[XINPUT_GAMEPAD_A] > 25)				m_xboxControllers[i] |= XBOX_A;
			if(g_Gamepads[i].bAnalogButtons[XINPUT_GAMEPAD_B] > 25)				m_xboxControllers[i] |= XBOX_B;
			if(g_Gamepads[i].bAnalogButtons[XINPUT_GAMEPAD_X] > 25)				m_xboxControllers[i] |= XBOX_X;
			if(g_Gamepads[i].bAnalogButtons[XINPUT_GAMEPAD_Y] > 25)				m_xboxControllers[i] |= XBOX_Y;
			if(g_Gamepads[i].bAnalogButtons[XINPUT_GAMEPAD_BLACK] > 25)			m_xboxControllers[i] |= XBOX_BLACK;
			if(g_Gamepads[i].bAnalogButtons[XINPUT_GAMEPAD_WHITE] > 25)			m_xboxControllers[i] |= XBOX_WHITE;
			if(g_Gamepads[i].wButtons & XINPUT_GAMEPAD_DPAD_UP)					m_xboxControllers[i] |= XBOX_DPAD_UP;
			if(g_Gamepads[i].wButtons & XINPUT_GAMEPAD_DPAD_RIGHT) 				m_xboxControllers[i] |= XBOX_DPAD_RIGHT;
			if(g_Gamepads[i].wButtons & XINPUT_GAMEPAD_DPAD_DOWN) 				m_xboxControllers[i] |= XBOX_DPAD_DOWN;
			if(g_Gamepads[i].wButtons & XINPUT_GAMEPAD_DPAD_LEFT) 				m_xboxControllers[i] |= XBOX_DPAD_LEFT;
			if(g_Gamepads[i].wButtons & XINPUT_GAMEPAD_START)  					m_xboxControllers[i] |= XBOX_START;
			if(g_Gamepads[i].wButtons & XINPUT_GAMEPAD_BACK)  					m_xboxControllers[i] |= XBOX_BACK;
			if(g_Gamepads[i].bAnalogButtons[XINPUT_GAMEPAD_LEFT_TRIGGER] > 25)	m_xboxControllers[i] |= XBOX_LEFT_TRIGGER;
			if(g_Gamepads[i].bAnalogButtons[XINPUT_GAMEPAD_RIGHT_TRIGGER] > 25) m_xboxControllers[i] |= XBOX_RIGHT_TRIGGER;
			if(g_Gamepads[i].fX1 >  0.40f)										m_xboxControllers[i] |= XBOX_DPAD_RIGHT;
			if(g_Gamepads[i].fX1 < -0.40f)										m_xboxControllers[i] |= XBOX_DPAD_LEFT;
			if(g_Gamepads[i].fY1 >  0.40f)										m_xboxControllers[i] |= XBOX_DPAD_UP;
			if(g_Gamepads[i].fY1 < -0.40f)										m_xboxControllers[i] |= XBOX_DPAD_DOWN;
			if(g_Gamepads[i].fX2 >  0.40f)										m_xboxControllers[i] |= XBOX_RTHUMB_RIGHT;
			if(g_Gamepads[i].fX2 < -0.40f)										m_xboxControllers[i] |= XBOX_RTHUMB_LEFT;
			if(g_Gamepads[i].fY2 >  0.40f)										m_xboxControllers[i] |= XBOX_RTHUMB_UP;
			if(g_Gamepads[i].fY2 < -0.40f)										m_xboxControllers[i] |= XBOX_RTHUMB_DOWN;
			if(g_Gamepads[i].wButtons & XINPUT_GAMEPAD_LEFT_THUMB)  			m_xboxControllers[i] |= XBOX_LEFT_THUMB;
			if(g_Gamepads[i].wButtons & XINPUT_GAMEPAD_RIGHT_THUMB)
			{
				m_xboxControllers[i] |= XBOX_RIGHT_THUMB;
				if(!didfilter)
				{
					xboxChangeFilter();
					didfilter = 1;
				}
			}
			else didfilter = 0;

			if((g_Gamepads[i].wButtons & XINPUT_GAMEPAD_START) && (g_Gamepads[i].wButtons & XINPUT_GAMEPAD_BACK))
            {
                PLAUNCH_DATA ldata;
                memset(&ldata, 0, sizeof(PLAUNCH_DATA));
                XLaunchNewImage("D:\\default.xbe", ldata);
            }
		}
	}
}

BOOL CXBoxSample::SetRefreshRate(INT iRefreshRate)
{
	char xmsg[100] ;

	m_d3dpp.FullScreen_RefreshRateInHz = iRefreshRate;
	DWORD res = 	m_pd3dDevice->Reset(&m_d3dpp);

	return TRUE;
}

HRESULT CXBoxSample::Initialize()
{


	//m_logfile = fopen( "D:\\err.log", "wb" ) ;



    // Create DirectSound
    if( FAILED( DirectSoundCreate( NULL, &(m_sound.dsound),  NULL ) ) )
        return E_FAIL;

	if ( ( XCreateSaveGame( "U:\\", PLATFORM_SAV, OPEN_ALWAYS, 0, g_savePath, 500 ) ) != ERROR_SUCCESS )
	{
        //return E_FAIL;
	}

	m_sound.dsound_init() ;

	m_sound.m_fps = m_d3dpp.FullScreen_RefreshRateInHz ;



	return S_OK ;
}


void CXBoxSample::xboxChangeFilter()
{
	g_app->m_xboxSFilter = (g_app->m_xboxSFilter+1)%NUM_SOFTWARE_FILTERS ;

	saveSettings( "d:\\Saves\\settings.cfg" ) ;

	memset( g_app->g_pDeltaBuff, 0x00, 480*g_app->m_pitch ) ;

	sprintf( global_error_message, "%s Filtering", SOFTWARE_FILTERS[m_xboxSFilter].name ) ;
	debug_printf("%s Filtering", SOFTWARE_FILTERS[m_xboxSFilter].name);
	m_msgDelay = 120 ;
}




DWORD WINAPI Sound_ThreadFunc( LPVOID lpParam )
{

	float FPS ;
	unsigned int perfCurr[2] ;
	unsigned int perfPrev[2] ;

	QueryPerformanceCounter((union _LARGE_INTEGER *) perfPrev);

	while ( 1 )
	{

		do
		{
			QueryPerformanceCounter((union _LARGE_INTEGER *) perfCurr);

			if (perfCurr[0] != perfPrev[0])
			{
				FPS = (float) (m_performanceFreq[0])  / (float) (perfCurr[0] - perfPrev[0]);
			}
			else
			{
				FPS = 200.0f ;
			}
			Sleep(1) ;

		} while ( FPS > 120 ) ;

		perfPrev[0] = perfCurr[0];

		g_app->m_sound.process( g_app->m_throttle ) ;

	}

    return 0;
}

void CXBoxSample::doScreenSize()
{
	WCHAR str[200];
	int x, y, maxx, maxy ;
	float fx, fy, fmaxx, fmaxy ;
	float origw, origh ;
	DWORD mtime ;

	mtime = GetTickCount() ;

	x = m_nScreenX ;
	y = m_nScreenY ;
	maxx = m_nScreenMaxX;
	maxy = m_nScreenMaxY ;

	fx = (float)x ;
	fy = (float)y ;
	fmaxx = (float)maxx ;
	fmaxy = (float)maxy ;

	origw = (float)m_nScreenMaxX/m_gameVecScale.x ;
	origh = (float)m_nScreenMaxY/m_gameVecScale.y ;

	while ( 1 )
	{
		m_pd3dDevice->Clear( 0L, NULL, D3DCLEAR_TARGET|D3DCLEAR_ZBUFFER|D3DCLEAR_STENCIL,
							 0x00000000, 1.0f, 0L );

		x = (int)fx ;
		y = (int)fy ;
		maxx = (int)fmaxx ;
		maxy = (int)fmaxy ;

		D3DXCOLOR d3color(1.0, 1.0, 1.0, 1.0);
		m_gameVecScale.x = (float)maxx / ((float)origw);
		m_gameVecScale.y = (float)maxy / ((float)origh);
		m_gameVecTranslate.x = x ;
		m_gameVecTranslate.y = y ;
		m_pnlGameScreen.Render(m_gameRectSource.left,m_gameRectSource.top,m_gameRectSource.right,m_gameRectSource.bottom,x,y,maxx,maxy) ;
		m_pd3dDevice->Present( NULL, NULL, NULL, NULL );

        XBInput_GetInput();
		if(g_Gamepads[0].hDevice && g_Gamepads[0].bPressedAnalogButtons[XINPUT_GAMEPAD_B])
		{
			break ;
		}
		else if(g_Gamepads[0].hDevice && g_Gamepads[0].bPressedAnalogButtons[XINPUT_GAMEPAD_A])
		{
			m_nScreenX = x ;
			m_nScreenY = y ;
			m_nScreenMaxX = maxx ;
			m_nScreenMaxY = maxy ;
			saveSettings( "d:\\Saves\\settings.cfg" ) ;
			break ;
		}
		else
		{
			if ( g_Gamepads[0].hDevice )
			{
				if ( GetTickCount() - mtime > 5 )
				{
					mtime = GetTickCount() ;
					fx += (g_Gamepads[0].fX1) ;
					fy -= (g_Gamepads[0].fY1) ;
					fmaxx += (g_Gamepads[0].fX2) ;
					fmaxy -= (g_Gamepads[0].fY2) ;
				}
			}
		}
	}

}

void CXBoxSample::initConsole( UINT32 idx, int isFavorite, int forceConfig )
{
	char                filename[500];
	char				shortpath[100];
	unsigned char       *gimage;
	int                 ntsccol;
	FILE				*infile;
	UINT32				fsize;
	char				*forcebuf;
	int					isOther;

	setSystemRam();

	// Create necessary directories First
	CreateDirectory("d:\\Paks", NULL);
	CreateDirectory("d:\\Saves", NULL);
	CreateDirectory("d:\\Logs", NULL);
	CreateDirectory("d:\\ScreenShots", NULL);


	m_throttle = 0 ;


	global_error_message[0] = 0 ;

	m_sound.init() ;

	// Create our texture
	init_texture();

	// Create our sprite driver
	if ( Sprite == NULL )
		D3DXCreateSprite(m_pd3dDevice, &Sprite);

	m_fAppTime = 0.0f ;

	m_startTime = GetTickCount() ;

	QueryPerformanceCounter((union _LARGE_INTEGER *) m_performancePrev);


	xbox_set_refreshrate(60) ;

	//Then start it up
	m_sound.pause( FALSE ) ;

	loadSettings("d:\\Saves\\settings.cfg");

    g_hSoundThread = CreateThread(
        NULL,                        // (this parameter is ignored)
        0,                           // use default stack size
        Sound_ThreadFunc,                  // thread function
        NULL,                // argument to thread function
        0,                           // use default creation flags
        &g_dwSoundThreadId);                // returns the thread identifier


	xbox_init();
	packfile_mode(0);
	openborMain(0,NULL);
}





#ifdef __cplusplus
extern "C" {
#endif

void borExit(int reset)
{
	tracemalloc_dump();
	xbox_return() ;
}

void xbox_resize()
{
	g_app->doScreenSize() ;
}

void xbox_clear_screen()
{
	g_app->ClearScreen() ;
}

void xbox_pause_audio( int state )
{
	g_app->m_sound.pause( state ? true : false ) ;
}

void xbox_check_events(void)
{
	g_app->pollXBoxControllers();
}

unsigned long xbox_get_playerinput( int playernum )
{
	return g_app->m_xboxControllers[playernum];
}

void xbox_put_image(int src_w, int src_h, s_screen* source)
{
	if ( g_app->m_throttle )
		g_app->m_throttle-- ;

	if ( g_app->m_throttle == 0 )
		g_app->render_to_texture(src_w, src_h, source) ;

}

void xbox_set_palette( char *palette )
{
	g_app->setPalette( (unsigned char*)palette) ;
}

void xbox_init()
{
	XGetCustomLaunchData();
}

void xbox_return()
{
	XReturnToLaunchingXBE() ;
}


void xbox_Sleep( int d )
{
	Sleep( d ) ;
}

unsigned int xbox_get_throttle()
{
	return g_app->m_throttle ;
}

int xbox_get_filter()
{
	return g_app->m_xboxSFilter ;
}


#ifdef __cplusplus
}
#endif




