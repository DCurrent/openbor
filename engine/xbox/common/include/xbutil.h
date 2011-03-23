//-----------------------------------------------------------------------------
// File: XBUtil.h
//
// Desc: Shortcut macros and helper functions for the XBox samples
//
// Hist: 11.01.00 - New for November XDK release
//       12.01.00 - Moved input code to XBInput.cpp
//       12.15.00 - Changes for December XDK release
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef XBUTIL_H
#define XBUTIL_H

#include <xtl.h>
#include <tchar.h>
#include <assert.h>


#ifdef __cplusplus
extern "C" {
#endif

void writexbox( char *msg );

void sprintfx( const char *fmt, ... );
#ifdef __cplusplus
}
#endif



//-----------------------------------------------------------------------------
// Miscellaneous helper functions
//-----------------------------------------------------------------------------

// For deleting and releasing objects
#define SAFE_DELETE(p)       { delete (p);     (p)=NULL; }
#define SAFE_DELETE_ARRAY(p) { delete[] (p);   (p)=NULL; }
#define SAFE_RELEASE(p)      { if(p) { (p)->Release(); (p)=NULL; } }

#ifdef _DEBUG
	#define OUTPUT_DEBUG_STRING(s) OutputDebugStringA(s)
	//#define OUTPUT_DEBUG_STRING(s) writexbox(s)
#else
	#define OUTPUT_DEBUG_STRING(s) (VOID)(s)
#endif

// For converting a FLOAT to a DWORD (useful for SetRenderState() calls)
inline DWORD FtoDW( FLOAT f ) { return *((DWORD*)&f); }




//-----------------------------------------------------------------------------
// Name: XBUtil_SetMediaPath() and XBUtil_FindMediaFile()
// Desc: Functions for setting a media path and returning a valid path to a
//       media file.
//-----------------------------------------------------------------------------
VOID    XBUtil_SetMediaPath( const CHAR* strPath );
HRESULT XBUtil_FindMediaFile( CHAR* strPath, const CHAR* strFilename );


//-----------------------------------------------------------------------------
// Name: XBox specifc counters
// Desc: * The CPU runs at 733MHz, therefore
//         Time in micro seconds = ticks / 733MHz = ticks * 3/2200
//       * Using DOUBLE to maintain percision
//       * See "A Note On Timers" whitepaper
//-----------------------------------------------------------------------------
__forceinline __int64 GetMachineTime()          { __asm rdtsc }
__forceinline __int64 GetTimeInMicroSeconds()
						{ return GetMachineTime()*3/2200;}
__forceinline DOUBLE  GetTimeInSeconds()
						{ return GetTimeInMicroSeconds() / 1000000.0;}

//-----------------------------------------------------------------------------
// Name: XBUtil_Timer()
// Desc: Performs timer operations. Use the following commands:
//          TIMER_RESET           - to reset the timer
//          TIMER_START           - to start the timer
//          TIMER_STOP            - to stop (or pause) the timer
//          TIMER_ADVANCE         - to advance the timer by 0.1 seconds
//          TIMER_RETRACT         - to retract the timer by 0.1 seconds
//          TIMER_GETABSOLUTETIME - to get the absolute system time
//          TIMER_GETAPPTIME      - to get the current time
//-----------------------------------------------------------------------------
enum TIMER_COMMAND { TIMER_RESET, TIMER_START, TIMER_STOP,
					 TIMER_ADVANCE, TIMER_RETRACT,
					 TIMER_GETABSOLUTETIME, TIMER_GETAPPTIME };
FLOAT XBUtil_Timer( TIMER_COMMAND command );




//-----------------------------------------------------------------------------
// Name: XBUtil_InitMaterial()
// Desc: Initializes a D3DMATERIAL8 structure, setting the diffuse and ambient
//       colors. It does not set emissive or specular colors.
//-----------------------------------------------------------------------------
VOID XBUtil_InitMaterial( D3DMATERIAL8& mtrl, FLOAT r=0.0f, FLOAT g=0.0f,
											  FLOAT b=0.0f, FLOAT a=1.0f );




//-----------------------------------------------------------------------------
// Name: XBUtil_InitLight()
// Desc: Initializes a D3DLIGHT structure, setting the light position. The
//       diffuse color is set to white, specular and ambient left as black.
//-----------------------------------------------------------------------------
VOID XBUtil_InitLight( D3DLIGHT8& light, D3DLIGHTTYPE ltType,
					   FLOAT x=0.0f, FLOAT y=0.0f, FLOAT z=0.0f );




//-----------------------------------------------------------------------------
// Name: XBUtil_CreateTexture()
// Desc: Helper function to create a texture.
//-----------------------------------------------------------------------------
HRESULT XBUtil_CreateTexture( LPDIRECT3DDEVICE8 pd3dDevice, const CHAR* strTexture,
							  LPDIRECT3DTEXTURE8* ppTexture,
							  D3DFORMAT d3dFormat = D3DFMT_UNKNOWN );




//-----------------------------------------------------------------------------
// Name: XBUtil_UnswizzleTexture() / XBUtil_SwizzleTexture()
// Desc: Unswizzles / swizzles a texture before it gets unlocked. Note: this
//       operation is typically very slow.
//-----------------------------------------------------------------------------
VOID XBUtil_UnswizzleTexture2D( D3DLOCKED_RECT* pLock, const D3DSURFACE_DESC* pDesc );
VOID XBUtil_UnswizzleTexture3D( D3DLOCKED_BOX* pLock, const D3DVOLUME_DESC* pDesc );
VOID XBUtil_SwizzleTexture2D( D3DLOCKED_RECT* pLock, const D3DSURFACE_DESC* pDesc );
VOID XBUtil_SwizzleTexture3D( D3DLOCKED_BOX* pLock, const D3DVOLUME_DESC* pDesc );




//-----------------------------------------------------------------------------
// Name: XBUtil_CreateVertexShader()
// Desc: Creates a file-based vertex shader
//-----------------------------------------------------------------------------
HRESULT XBUtil_CreateVertexShader( LPDIRECT3DDEVICE8 pd3dDevice,
								   const CHAR* strFilename,
								   const DWORD* pdwVertexDecl,
								   DWORD* pdwVertexShader );




//-----------------------------------------------------------------------------
// Name: XBUtil_CreatePixelShader()
// Desc: Creates a file-based pixel shader
//-----------------------------------------------------------------------------
HRESULT XBUtil_CreatePixelShader( LPDIRECT3DDEVICE8 pd3dDevice,
								  const CHAR* strFilename, DWORD* pdwPixelShader );




//-----------------------------------------------------------------------------
// Name: XBUtil_VectorToRGBA()
// Desc: Converts a normal into an RGBA vector.
//-----------------------------------------------------------------------------
inline D3DCOLOR XBUtil_VectorToRGBA( const D3DXVECTOR3* v, FLOAT fHeight = 1.0f )
{
	D3DCOLOR r = (D3DCOLOR)( ( v->x + 1.0f ) * 127.5f );
	D3DCOLOR g = (D3DCOLOR)( ( v->y + 1.0f ) * 127.5f );
	D3DCOLOR b = (D3DCOLOR)( ( v->z + 1.0f ) * 127.5f );
	D3DCOLOR a = (D3DCOLOR)( 255.0f * fHeight );
	return( (a<<24L) + (r<<16L) + (g<<8L) + (b<<0L) );
}




//-----------------------------------------------------------------------------
// Name: XBUtil_GetCubeMapViewMatrix()
// Desc: Returns a view matrix for rendering to a face of a cube map.
//-----------------------------------------------------------------------------
D3DXMATRIX XBUtil_GetCubeMapViewMatrix( DWORD dwFace );




//-----------------------------------------------------------------------------
// Name: XBUtil_CreateNormalizationCubeMap()
// Desc: Creates a cube map and fills it with normalized RGBA vectors.
//-----------------------------------------------------------------------------
HRESULT XBUtil_CreateNormalizationCubeMap( LPDIRECT3DDEVICE8 pd3dDevice,
										   DWORD dwSize,
										   LPDIRECT3DCUBETEXTURE8* ppCubeMap );




//-----------------------------------------------------------------------------
// Name: XBUtil_DumpSurface()
// Desc: Writes the contents of a surface (32-bit only) to a .tga file. This
//       could be a back buffer, texture, or any other 32-bit surface.
//-----------------------------------------------------------------------------
HRESULT XBUtil_DumpSurface( LPDIRECT3DSURFACE8 pSurface, const CHAR* strFileName,
							BOOL bSurfaceIsTiled = FALSE );




//-----------------------------------------------------------------------------
// Name: XBUtil_EvaluateHermite()
// Desc: Evaluate a cubic parametric equation. Returns the point at u on a
//       Hermite curve.
//-----------------------------------------------------------------------------
D3DXVECTOR3 XBUtil_EvaluateHermite( const D3DXVECTOR3& p0, const D3DXVECTOR3& p1,
									const D3DXVECTOR3& v0, const D3DXVECTOR3& v1,
									FLOAT u );




//-----------------------------------------------------------------------------
// Name: XBUtil_EvaluateCatmullRom()
// Desc: Evaluate a cubic parametric equation. Returns the point at u on a
//       Catmull-Rom curve.
//-----------------------------------------------------------------------------
D3DXVECTOR3 XBUtil_EvaluateCatmullRom( const D3DXVECTOR3& p1, const D3DXVECTOR3& p2,
									   const D3DXVECTOR3& p3, const D3DXVECTOR3& p4,
									   FLOAT u );




//-----------------------------------------------------------------------------
// Name: XBUtil_GetSplinePoint()
// Desc: Returns a point on a spline. The spline is defined by an array of
//       points, and the point and tangent returned are located at position t
//       on the spline, where 0 < t < dwNumSpinePts.
//-----------------------------------------------------------------------------
VOID XBUtil_GetSplinePoint( const D3DXVECTOR3* pSpline, DWORD dwNumSpinePts, FLOAT t,
							D3DXVECTOR3* pvPoint, D3DXVECTOR3* pvTangent );




//-----------------------------------------------------------------------------
// Name: XBUtil_RenderSpline()
// Desc: For debugging purposes, visually renders a spline.
//-----------------------------------------------------------------------------
VOID XBUtil_RenderSpline( LPDIRECT3DDEVICE8 pd3dDevice, const D3DXVECTOR3* pSpline,
						  DWORD dwNumSplinePts, DWORD dwColor, BOOL bRenderAxes );




//-----------------------------------------------------------------------------
// Name: XBUtil_DeclaratorFromFVF()
// Desc: Create a vertex declaration from an FVF. Registers are assigned as
//       follows:
//          v0    = Vertex position
//          v1    = Vertex blend weights
//          v2    = Vertex normal
//          v3    = Vertex pointsize
//          v4    = Vertex diffuse color
//          v5    = Vertex specular color
//          v6-v9 = Vertex texture coords
//-----------------------------------------------------------------------------
HRESULT XBUtil_DeclaratorFromFVF( DWORD dwFVF,
								  DWORD Declaration[MAX_FVF_DECL_SIZE] );




//-----------------------------------------------------------------------------
// Name: XBUtil_GetWide()
// Desc: Convert CHAR string to WCHAR string. dwMax includes the null byte.
//       Never copies more than dwMax-1 characters into strWide.
//          Ex: GetWide( "abc", strWide, 3 ) gives strWide = "ab"
//       Typical usage:
//          WCHAR strResult[MAX];
//          XBUtil_GetWide( strThin, strResult, MAX );
//-----------------------------------------------------------------------------
VOID XBUtil_GetWide( const CHAR* strThin, WCHAR* strWide, DWORD dwMax );


#endif // XBUTIL_H

