//-----------------------------------------------------------------------------
// File: XBUtil.cpp
//
// Desc: Shortcut macros and helper functions for the XBox samples
//
// Hist: 11.01.00 - New for November XDK release
//       12.01.00 - Moved input code to XBInput.cpp
//       12.15.00 - Changes for December XDK release
//       02.19.00 - Changes for March XDK release
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include <xtl.h>
#include <xgraphics.h>
#include <tchar.h> 
#include <cstdio>
#include <cassert>
#include "XBUtil.h"



//-----------------------------------------------------------------------------
// Path to the XBox media files on the target machine
//-----------------------------------------------------------------------------
CHAR g_strMediaPath[512] = "D:\\Media\\";




//-----------------------------------------------------------------------------
// Name: XBUtil_SetMediaPath()
// Desc: Sets the path to media files
//-----------------------------------------------------------------------------
VOID XBUtil_SetMediaPath( const CHAR* strPath )
{
    strcpy( g_strMediaPath, strPath );
}

    
    
    
//-----------------------------------------------------------------------------
// Name: XBUtil_FindMediaFile()
// Desc: Returns a valid path to a media file.
//-----------------------------------------------------------------------------
HRESULT XBUtil_FindMediaFile( CHAR* strPath, const CHAR* strFilename )
{
    // Check for valid arguments
    if( NULL==strFilename || NULL==strPath )
    {
        OUTPUT_DEBUG_STRING( "XBUtil_FindMediaFile(): Invalid arguments\n" );
        return E_INVALIDARG;
    }

    // Default path is the filename itself as a fully qualified path
    strcpy( strPath, strFilename );

    // Check for the ':' character to see if the filename is a fully
    // qualified path. If not, pre-pend the media directory
    if( strFilename[1] != ':' )
        sprintf( strPath, "%s%s", g_strMediaPath, strFilename );

    // Try to open the file
    HANDLE hFile = CreateFile( strPath, GENERIC_READ, FILE_SHARE_READ, NULL, 
                               OPEN_EXISTING, 0, NULL );
    if( INVALID_HANDLE_VALUE == hFile )
    {
        // Return error
        CHAR strBuffer[80];
        sprintf( strBuffer, "XBUtil_FindMediaFile(): Could not find file [%s]\n", 
                            strFilename );
        OUTPUT_DEBUG_STRING( strBuffer );
        return 0x82000004;
    }

    // Found the file. Close the file and return
    CloseHandle( hFile );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: XBUtil_Timer()
// Desc: Performs timer operations. Use the following commands:
//          TIMER_RESET           - to reset the timer
//          TIMER_START           - to start the timer
//          TIMER_STOP            - to stop (or pause) the timer
//          TIMER_ADVANCE         - to advance the timer by 0.1 seconds
//          TIMER_GETABSOLUTETIME - to get the absolute system time
//          TIMER_GETAPPTIME      - to get the current time
//-----------------------------------------------------------------------------
FLOAT XBUtil_Timer( TIMER_COMMAND command )
{
    static BOOL  m_bTimerInitialized = FALSE;
    static FLOAT m_fSecsPerTick = 0.0f;
    static FLOAT m_fBaseTime    = 0.0f;
    static FLOAT m_fStopTime    = 0.0f;
    FLOAT        fTime;

    // Initialize the timer
    if( FALSE == m_bTimerInitialized )
    {
        m_bTimerInitialized = TRUE;

        // Use QueryPerformanceFrequency() to get frequency of timer.
        LARGE_INTEGER qwTicksPerSec;
        QueryPerformanceFrequency( &qwTicksPerSec );
        m_fSecsPerTick = 1.0f / (FLOAT)qwTicksPerSec.QuadPart;
    }

    // Get the current time using QueryPerformanceCounter() or timeGetTime()
    LARGE_INTEGER qwTime;
    QueryPerformanceCounter( &qwTime );
    fTime = ((FLOAT)qwTime.QuadPart) * m_fSecsPerTick;

    // Reset the timer
    if( command == TIMER_RESET )
    {
        m_fBaseTime = fTime;
        return 0.0f;
    }

    // Return the current time
    if( command == TIMER_GETAPPTIME )
        return fTime - m_fBaseTime;

    // Start the timer
    if( command == TIMER_START )
        m_fBaseTime += fTime - m_fStopTime;

    // Stop the timer
    if( command == TIMER_STOP )
        m_fStopTime = fTime;

    // Advance the timer by 1/10th second
    if( command == TIMER_ADVANCE )
        m_fBaseTime += fTime - ( m_fStopTime + 0.1f );

    // Retract the timer by 1/10th second
    if( command == TIMER_RETRACT )
        m_fBaseTime += fTime - ( m_fStopTime - 0.1f );

    return fTime;
}




//-----------------------------------------------------------------------------
// Name: XBUtil_InitMaterial()
// Desc: Initializes a D3DMATERIAL8 structure, setting the diffuse and ambient
//       colors. It does not set emissive or specular colors.
//-----------------------------------------------------------------------------
VOID XBUtil_InitMaterial( D3DMATERIAL8& mtrl, FLOAT r, FLOAT g, FLOAT b,
                          FLOAT a )
{
    ZeroMemory( &mtrl, sizeof(D3DMATERIAL8) );
    mtrl.Diffuse.r = mtrl.Ambient.r = r;
    mtrl.Diffuse.g = mtrl.Ambient.g = g;
    mtrl.Diffuse.b = mtrl.Ambient.b = b;
    mtrl.Diffuse.a = mtrl.Ambient.a = a;
}




//-----------------------------------------------------------------------------
// Name: XBUtil_InitLight()
// Desc: Initializes a D3DLIGHT structure, setting the light position. The
//       diffuse color is set to white, specular and ambient left as black.
//-----------------------------------------------------------------------------
VOID XBUtil_InitLight( D3DLIGHT8& light, D3DLIGHTTYPE ltType,
                       FLOAT x, FLOAT y, FLOAT z )
{
    ZeroMemory( &light, sizeof(D3DLIGHT8) );
    light.Type         = ltType;
    light.Diffuse.r    = 1.0f;
    light.Diffuse.g    = 1.0f;
    light.Diffuse.b    = 1.0f;
    light.Position     = D3DXVECTOR3(x,y,z);

    light.Position.x   = x;
    light.Position.y   = y;
    light.Position.z   = z;
    D3DXVECTOR3 vSource(x,y,z);
    D3DXVec3Normalize( (D3DXVECTOR3*)&light.Direction, &vSource );
    light.Range        = 1000.0f;
}




//-----------------------------------------------------------------------------
// Name: XBUtil_CreateTexture()
// Desc: Helper function to create a texture.
//-----------------------------------------------------------------------------
HRESULT XBUtil_CreateTexture( LPDIRECT3DDEVICE8 pd3dDevice, const CHAR* strTexture,
                              LPDIRECT3DTEXTURE8* ppTexture, D3DFORMAT d3dFormat )
{
    HRESULT hr;

    // Find the media file
    CHAR strTexturePath[512];
    if( FAILED( hr = XBUtil_FindMediaFile( strTexturePath, strTexture ) ) )
        return hr;

    // Create the texture using D3DX. Check the current directory
    return D3DXCreateTextureFromFileEx( pd3dDevice, strTexturePath, 
                                        D3DX_DEFAULT, D3DX_DEFAULT, D3DX_DEFAULT, 
                                        0, d3dFormat, D3DPOOL_DEFAULT, 
                                        D3DX_DEFAULT, D3DX_DEFAULT, 0, NULL, NULL, 
                                        ppTexture );
}




//-----------------------------------------------------------------------------
// Name: XBUtil_UnswizzleTexture2D()
// Desc: Unswizzles a 2D texture before it gets unlocked. Note: this operation
//       can be very slow.
//-----------------------------------------------------------------------------
VOID XBUtil_UnswizzleTexture2D( D3DLOCKED_RECT* pLock, const D3DSURFACE_DESC* pDesc )
{
    DWORD dwPixelSize   = XGBytesPerPixelFromFormat( pDesc->Format );
    DWORD dwTextureSize = pDesc->Width * pDesc->Height * dwPixelSize;

    BYTE* pSrcBits = new BYTE[ dwTextureSize ];
    memcpy( pSrcBits, pLock->pBits, dwTextureSize );
    
    XGUnswizzleRect( pSrcBits, pDesc->Width, pDesc->Height, NULL, pLock->pBits, 
                     0, NULL, dwPixelSize );

    SAFE_DELETE_ARRAY( pSrcBits );
}




//-----------------------------------------------------------------------------
// Name: XBUtil_UnswizzleTexture3D()
// Desc: Unswizzles a 3D texture before it gets unlocked. Note: this operation
//       can be very slow.
//-----------------------------------------------------------------------------
VOID XBUtil_UnswizzleTexture3D( D3DLOCKED_BOX* pLock, const D3DVOLUME_DESC* pDesc )
{
    DWORD dwPixelSize   = XGBytesPerPixelFromFormat( pDesc->Format );
    DWORD dwTextureSize = pDesc->Width * pDesc->Height * pDesc->Depth * dwPixelSize;

    BYTE* pSrcBits = new BYTE[ dwTextureSize ];
    memcpy( pSrcBits, pLock->pBits, dwTextureSize );
    
    XGUnswizzleBox( pSrcBits, pDesc->Width, pDesc->Height, pDesc->Depth, NULL, pLock->pBits,
                    0, 0, NULL, dwPixelSize );

    SAFE_DELETE_ARRAY( pSrcBits );
}




//-----------------------------------------------------------------------------
// Name: XBUtil_SwizzleTexture2D()
// Desc: Swizzles a 2D texture before it gets unlocked. Note: this operation
//       can be very slow.
//-----------------------------------------------------------------------------
VOID XBUtil_SwizzleTexture2D( D3DLOCKED_RECT* pLock, const D3DSURFACE_DESC* pDesc )
{
    DWORD dwPixelSize   = XGBytesPerPixelFromFormat( pDesc->Format );
    DWORD dwTextureSize = pDesc->Width * pDesc->Height * dwPixelSize;

    BYTE* pSrcBits = new BYTE[ dwTextureSize ];
    memcpy( pSrcBits, pLock->pBits, dwTextureSize );
    
    XGSwizzleRect( pSrcBits, 0, NULL, pLock->pBits,
                  pDesc->Width, pDesc->Height, 
                  NULL, dwPixelSize );

    SAFE_DELETE_ARRAY( pSrcBits );
}




//-----------------------------------------------------------------------------
// Name: XBUtil_SwizzleTexture3D()
// Desc: Swizzles a 3D texture before it gets unlocked. Note: this operation
//       can be very slow.
//-----------------------------------------------------------------------------
VOID XBUtil_SwizzleTexture3D( D3DLOCKED_BOX* pLock, const D3DVOLUME_DESC* pDesc )
{
    DWORD dwPixelSize   = XGBytesPerPixelFromFormat( pDesc->Format );
    DWORD dwTextureSize = pDesc->Width * pDesc->Height * pDesc->Depth * dwPixelSize;

    BYTE* pSrcBits = new BYTE[ dwTextureSize ];
    memcpy( pSrcBits, pLock->pBits, dwTextureSize );
    
    XGSwizzleBox( pSrcBits, 0, 0, NULL, pLock->pBits,
                  pDesc->Width, pDesc->Height, pDesc->Depth, 
                  NULL, dwPixelSize );

    SAFE_DELETE_ARRAY( pSrcBits );
}




//-----------------------------------------------------------------------------
// Name: XBUtil_CreateVertexShader()
// Desc: Creates a file-based vertex shader
//-----------------------------------------------------------------------------
HRESULT XBUtil_CreateVertexShader( LPDIRECT3DDEVICE8 pd3dDevice, 
                                   const CHAR* strFilename,
                                   const DWORD* pdwVertexDecl,
                                   DWORD* pdwVertexShader )
{
    HRESULT hr;

    // Find the media file
    CHAR strShaderPath[512];
    if( FAILED( hr = XBUtil_FindMediaFile( strShaderPath, strFilename ) ) )
        return hr;

    // Open the vertex shader file
    HANDLE hFile;
    DWORD dwNumBytesRead;
    hFile = CreateFile(strShaderPath, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
        return E_FAIL;

    // Allocate memory to read the vertex shader file
    DWORD dwSize = GetFileSize(hFile, NULL);
    BYTE* pData  = new BYTE[dwSize+4];
    if( NULL == pData )
        return E_FAIL;
    ZeroMemory( pData, dwSize+4 );

    // Read the pre-compiled vertex shader microcode
    ReadFile(hFile, pData, dwSize, &dwNumBytesRead, 0);
    
    // Create the vertex shader
    hr = pd3dDevice->CreateVertexShader( pdwVertexDecl, (const DWORD*)pData,
                                         pdwVertexShader, 0 );

    // Cleanup and return
    CloseHandle(hFile);
    delete [] pData;
    return hr;
}




//-----------------------------------------------------------------------------
// Name: XBUtil_CreatePixelShader()
// Desc: Creates a file-based pixel shader
//-----------------------------------------------------------------------------
HRESULT XBUtil_CreatePixelShader( LPDIRECT3DDEVICE8 pd3dDevice, 
                                  const CHAR* strFilename, DWORD* pdwPixelShader )
{
    HRESULT hr;

    // Find the media file
    CHAR strShaderPath[512];
    if( FAILED( hr = XBUtil_FindMediaFile( strShaderPath, strFilename ) ) )
        return hr;

    // Open the pixel shader file
    HANDLE hFile;
    DWORD dwNumBytesRead;
    hFile = CreateFile(strShaderPath, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
        return E_FAIL;


    // Load the pre-compiled pixel shader microcode
    D3DPIXELSHADERDEF_FILE psdf;
    
    ReadFile( hFile, &psdf, sizeof(D3DPIXELSHADERDEF_FILE), &dwNumBytesRead, NULL );
    
    // Make sure the pixel shader is valid
    if( psdf.FileID != D3DPIXELSHADERDEF_FILE_ID )
    {
        OUTPUT_DEBUG_STRING( "XBUtil_CreatePixelShader(): Invalid pixel shader file\n" );
        return E_FAIL;
    }

    // Create the pixel shader
    if( FAILED( hr = pd3dDevice->CreatePixelShader( &(psdf.Psd), pdwPixelShader ) ) )
    {
        OUTPUT_DEBUG_STRING( "XBUtil_CreatePixelShader(): Could not create pixel shader\n" );
        return hr;
    }

    // cleanup
    CloseHandle( hFile );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: XBUtil_GetCubeMapViewMatrix()
// Desc: Returns a view matrix for rendering to a face of a cubemap.
//-----------------------------------------------------------------------------
D3DXMATRIX XBUtil_GetCubeMapViewMatrix( DWORD dwFace )
{
    D3DXVECTOR3 vEyePt   = D3DXVECTOR3( 0.0f, 0.0f, 0.0f );
    D3DXVECTOR3 vLookDir = D3DXVECTOR3( 1.0f, 0.0f, 0.0f );
    D3DXVECTOR3 vUpDir   = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );

    switch( dwFace )
    {
        case D3DCUBEMAP_FACE_POSITIVE_X:
            vLookDir = D3DXVECTOR3( 1.0f, 0.0f, 0.0f );
            vUpDir   = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );
            break;
        case D3DCUBEMAP_FACE_NEGATIVE_X:
            vLookDir = D3DXVECTOR3(-1.0f, 0.0f, 0.0f );
            vUpDir   = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );
            break;
        case D3DCUBEMAP_FACE_POSITIVE_Y:
            vLookDir = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );
            vUpDir   = D3DXVECTOR3( 0.0f, 0.0f,-1.0f );
            break;
        case D3DCUBEMAP_FACE_NEGATIVE_Y:
            vLookDir = D3DXVECTOR3( 0.0f,-1.0f, 0.0f );
            vUpDir   = D3DXVECTOR3( 0.0f, 0.0f, 1.0f );
            break;
        case D3DCUBEMAP_FACE_POSITIVE_Z:
            vLookDir = D3DXVECTOR3( 0.0f, 0.0f, 1.0f );
            vUpDir   = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );
            break;
        case D3DCUBEMAP_FACE_NEGATIVE_Z:
            vLookDir = D3DXVECTOR3( 0.0f, 0.0f,-1.0f );
            vUpDir   = D3DXVECTOR3( 0.0f, 1.0f, 0.0f );
            break;
    }

    // Set the view transform for this cubemap surface
    D3DXMATRIX matView;
    D3DXMatrixLookAtLH( &matView, &vEyePt, &vLookDir, &vUpDir );
    return matView;
}




//-----------------------------------------------------------------------------
// Name: XBUtil_CreateNormalizationCubeMap()
// Desc: Creates a cubemap and fills it with normalized RGBA vectors
//-----------------------------------------------------------------------------
HRESULT XBUtil_CreateNormalizationCubeMap( LPDIRECT3DDEVICE8 pd3dDevice, 
                                           DWORD dwSize, 
                                           LPDIRECT3DCUBETEXTURE8* ppCubeMap )
{
    HRESULT hr;

    // Create the cube map
    if( FAILED( hr = pd3dDevice->CreateCubeTexture( dwSize, 1, 0, D3DFMT_X8R8G8B8, 
                                                    D3DPOOL_DEFAULT, ppCubeMap ) ) )
        return E_FAIL;
    
    // Allocate temp space for swizzling the cubemap surfaces
    DWORD* pSourceBits = new DWORD[ dwSize * dwSize ];

    // Fill all six sides of the cubemap
    for( DWORD i=0; i<6; i++ )
    {
        // Lock the i'th cubemap surface
        LPDIRECT3DSURFACE8 pCubeMapFace;
        (*ppCubeMap)->GetCubeMapSurface( (D3DCUBEMAP_FACES)i, 0, &pCubeMapFace );

        // Write the RGBA-encoded normals to the surface pixels
        DWORD*      pPixel = pSourceBits;
        D3DXVECTOR3 n;
        FLOAT       w, h;

        for( DWORD y = 0; y < dwSize; y++ )
        {
            h  = (FLOAT)y / (FLOAT)(dwSize-1);  // 0 to 1
            h  = ( h * 2.0f ) - 1.0f;           // -1 to 1
            
            for( DWORD x = 0; x < dwSize; x++ )
            {
                w = (FLOAT)x / (FLOAT)(dwSize-1);   // 0 to 1
                w = ( w * 2.0f ) - 1.0f;            // -1 to 1

                // Calc the normal for this texel
                switch( i )
                {
                    case D3DCUBEMAP_FACE_POSITIVE_X:    // +x
                        n.x = +1.0;
                        n.y = -h;
                        n.z = -w;
                        break;
                        
                    case D3DCUBEMAP_FACE_NEGATIVE_X:    // -x
                        n.x = -1.0;
                        n.y = -h;
                        n.z = +w;
                        break;
                        
                    case D3DCUBEMAP_FACE_POSITIVE_Y:    // y
                        n.x = +w;
                        n.y = +1.0;
                        n.z = +h;
                        break;
                        
                    case D3DCUBEMAP_FACE_NEGATIVE_Y:    // -y
                        n.x = +w;
                        n.y = -1.0;
                        n.z = -h;
                        break;
                        
                    case D3DCUBEMAP_FACE_POSITIVE_Z:    // +z
                        n.x = +w;
                        n.y = -h;
                        n.z = +1.0;
                        break;
                        
                    case D3DCUBEMAP_FACE_NEGATIVE_Z:    // -z
                        n.x = -w;
                        n.y = -h;
                        n.z = -1.0;
                        break;
                }

                // Store the normal as an RGBA color
                D3DXVec3Normalize( &n, &n );
                *pPixel++ = XBUtil_VectorToRGBA( &n );
            }
        }
        
        // Swizzle the result into the cubemap face surface
        D3DLOCKED_RECT lock;
        pCubeMapFace->LockRect( &lock, 0, 0L );
        XGSwizzleRect( pSourceBits, 0, NULL, lock.pBits, dwSize, dwSize,
                       NULL, sizeof(DWORD) );
        pCubeMapFace->UnlockRect();

        // Release the cubemap face
        pCubeMapFace->Release();
    }

    // Free temp space
    SAFE_DELETE_ARRAY( pSourceBits );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: XBUtil_DumpSurface()
// Desc: Writes the contents of a surface (32-bit only) to a .tga file. This
//       could be a backbuffer, texture, or any other 32-bit surface.
//-----------------------------------------------------------------------------
HRESULT XBUtil_DumpSurface( LPDIRECT3DSURFACE8 pSurface, const CHAR* strFileName,
                            BOOL bSurfaceIsTiled )
{
    // Get the surface description. Make sure it's a 32-bit format
    D3DSURFACE_DESC desc;
    pSurface->GetDesc( &desc );
    if( desc.Size != ( desc.Width * desc.Height * sizeof(DWORD) ) )
        return E_NOTIMPL;

    // Lock the surface
    D3DLOCKED_RECT lock;
    if( FAILED( pSurface->LockRect( &lock, 0, bSurfaceIsTiled ? D3DLOCK_TILED : 0 ) ) )
        return E_FAIL;

    // Allocate memory for storing the surface bits
    VOID* pBits = (VOID*)new DWORD[desc.Width*desc.Height];

    // Unswizzle the bits, if necessary
    if( XGIsSwizzledFormat( desc.Format ) )
        XGUnswizzleRect( lock.pBits, desc.Width, desc.Height, NULL,
                         pBits, lock.Pitch, NULL, sizeof(DWORD) );
    else
        memcpy( pBits, lock.pBits, desc.Size );
    
    // Unlock the surface
    pSurface->UnlockRect();

    // Setup the TGA file header
    struct TargaHeader
    {
        BYTE IDLength;
        BYTE ColormapType;
        BYTE ImageType;
        BYTE ColormapSpecification[5];
        WORD XOrigin;
        WORD YOrigin;
        WORD ImageWidth;
        WORD ImageHeight;
        BYTE PixelDepth;
        BYTE ImageDescriptor;
    } tgaHeader;

    ZeroMemory( &tgaHeader, sizeof(tgaHeader) );
    tgaHeader.IDLength        = 0;
    tgaHeader.ImageType       = 2;
    tgaHeader.ImageWidth      = (WORD)desc.Width;
    tgaHeader.ImageHeight     = (WORD)desc.Height;
    tgaHeader.PixelDepth      = 32;
    tgaHeader.ImageDescriptor = 0x28;

    // Create a new file
    FILE* file = fopen( strFileName, "wb" );
    if( NULL == file )
    {
        pSurface->UnlockRect();
        return E_FAIL;
    }

    // Write the Targa header and the surface pixels to the file
    fwrite( &tgaHeader, sizeof(TargaHeader), 1, file );
    fwrite( pBits, sizeof(BYTE), desc.Size, file );
    fclose( file );

    // Cleanup and return
    delete[] pBits;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: XBUtil_EvaluateHermite()
// Desc: Evaluate a cubic parametric equation. Returns the point at u on a
//       Hermite curve.
//-----------------------------------------------------------------------------
D3DXVECTOR3 XBUtil_EvaluateHermite( const D3DXVECTOR3& p0, const D3DXVECTOR3& p1, 
                                    const D3DXVECTOR3& v0, const D3DXVECTOR3& v1,
                                    FLOAT u )
{
    // Generate coeffecients from the two end points and two tangents
    D3DXVECTOR3 a =  2*p0 - 2*p1 +   v0 + v1; // a = 2p0 - 2p1 +  v0 + v1
    D3DXVECTOR3 b = -3*p0 + 3*p1 - 2*v0 - v1; // b =-3p0 + 3p1 - 2v0 + v1
    D3DXVECTOR3 c =                  v0;      // c = v0  
    D3DXVECTOR3 d =    p0;                    // d = p0

    // Evaluate the equation at u, where:
    //    f(u) = au^3 + bu^2 + cu + d
    return ( ( a * u + b ) * u + c ) * u + d;
}




//-----------------------------------------------------------------------------
// Name: XBUtil_EvaluateCatmullRom()
// Desc: Evaluate a cubic parametric equation. Returns the point at u on a
//       Catmull-Rom curve.
//-----------------------------------------------------------------------------
D3DXVECTOR3 XBUtil_EvaluateCatmullRom( const D3DXVECTOR3& p1, const D3DXVECTOR3& p2, 
                                       const D3DXVECTOR3& p3, const D3DXVECTOR3& p4,
                                       FLOAT u )
{
    // Generate coefficients from four spline points
    D3DXVECTOR3 a =   -p1 + 3*p2 - 3*p3 + p4;
    D3DXVECTOR3 b =  2*p1 - 5*p2 + 4*p3 - p4;
    D3DXVECTOR3 c =   -p1        +   p3;
    D3DXVECTOR3 d =         2*p2;

    // Evaluate the equation at u, where:
    //    f(u) = 0.5 * ( au^3 + bu^2 + cu + d )
    return 0.5f * ( ( ( a * u + b ) * u + c ) * u + d );
}




//-----------------------------------------------------------------------------
// Name: XBUtil_GetSplinePoint()
// Desc: Returns a point on a spline. The spline is defined by an array of
//       points, and the point and tangent returned are located at position t
//       on the spline, where 0 < t < dwNumSpinePts.
//-----------------------------------------------------------------------------
VOID XBUtil_GetSplinePoint( const D3DXVECTOR3* pSpline, DWORD dwNumSpinePts,
                            FLOAT t, D3DXVECTOR3* pvPoint, D3DXVECTOR3* pvTangent )
{
    DWORD p0 = ( t > 1.0 ) ? (DWORD)floorf(t)-1 : dwNumSpinePts-1;
    DWORD p1 = ( p0 < dwNumSpinePts-1 ) ? p0 + 1 : 0;
    DWORD p2 = ( p1 < dwNumSpinePts-1 ) ? p1 + 1 : 0;
    DWORD p3 = ( p2 < dwNumSpinePts-1 ) ? p2 + 1 : 0;
    FLOAT u  = t - floorf(t);

    if( pvPoint )
        (*pvPoint) = XBUtil_EvaluateCatmullRom( pSpline[p0], pSpline[p1], 
                                                pSpline[p2], pSpline[p3], u );

    if( pvTangent )
        (*pvTangent) = 0.5f * ( (1-u) * ( pSpline[p2] - pSpline[p0] ) + 
                                  (u) * ( pSpline[p3] - pSpline[p1] ) );
}




//-----------------------------------------------------------------------------
// Name: XBUtil_RenderSpline()
// Desc: For debugging purposes, visually renders a spline.
//-----------------------------------------------------------------------------
VOID XBUtil_RenderSpline( LPDIRECT3DDEVICE8 pd3dDevice, const D3DXVECTOR3* pSpline, 
                          DWORD dwNumSplinePts, DWORD dwColor, BOOL bRenderAxes )
{
    pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
    pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_SELECTARG1 );
    pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TFACTOR );
    pd3dDevice->SetVertexShader( D3DFVF_XYZ );

    for( FLOAT u = 0; u < dwNumSplinePts; u += 1.0f )
    {
        D3DXVECTOR3 p[2];
        D3DXVECTOR3 vTangent, vSide, vUp;

        XBUtil_GetSplinePoint( pSpline, dwNumSplinePts, u+0, &p[0], &vTangent );
        XBUtil_GetSplinePoint( pSpline, dwNumSplinePts, u+1, &p[1], NULL );

        D3DXVec3Normalize( &vTangent, &vTangent );
        D3DXVECTOR3 v1( 0, 1, 0 );
        D3DXVec3Cross( &vSide, &v1, &vTangent );
        D3DXVec3Cross( &vUp, &vTangent, &vSide );

        pd3dDevice->SetRenderState( D3DRS_TEXTUREFACTOR, dwColor );
        pd3dDevice->DrawPrimitiveUP( D3DPT_LINELIST, 1, p, sizeof(D3DXVECTOR3) );

        if( bRenderAxes )
        {
            p[1] = p[0] + vTangent/4;
            pd3dDevice->SetRenderState( D3DRS_TEXTUREFACTOR, 0xffff0000 );
            pd3dDevice->DrawPrimitiveUP( D3DPT_LINELIST, 1, p, sizeof(D3DXVECTOR3) );

            p[1] = p[0] + vSide/4;
            pd3dDevice->SetRenderState( D3DRS_TEXTUREFACTOR, 0xff00ff00 );
            pd3dDevice->DrawPrimitiveUP( D3DPT_LINELIST, 1, p, sizeof(D3DXVECTOR3) );

            p[1] = p[0] + vUp/4;
            pd3dDevice->SetRenderState( D3DRS_TEXTUREFACTOR, 0xffffffff );
            pd3dDevice->DrawPrimitiveUP( D3DPT_LINELIST, 1, p, sizeof(D3DXVECTOR3) );
        }
    }
}




//-----------------------------------------------------------------------------
// Name: XBUtil_DeclaratorFromFVF()
// Desc: Create a vertex declaration from an FVF. Registers are assigned as
//       follows:
//          v0     = Vertex position
//          v1     = Vertex blend weights
//          v2     = Vertex normal
//          v3     = Vertex diffuse color
//          v4     = Vertex specular color
//       // v5     = Vertex fog (no FVF code)
//       // v6     = Vertex pointsize (no FVF code)
//       // v7     = Vertex back diffuse color (no FVF code)
//       // v8     = Vertex back specular color (no FVF code)
//          v9-v12 = Vertex texture coords
//-----------------------------------------------------------------------------
HRESULT XBUtil_DeclaratorFromFVF( DWORD dwFVF, 
                                  DWORD Declaration[MAX_FVF_DECL_SIZE] )
{
    // Start the declaration
    DWORD decl = 0;
    Declaration[decl++] = D3DVSD_STREAM(0);

    // Handle position
    DWORD dwPositionFVF = ( dwFVF & D3DFVF_POSITION_MASK );
    if( dwPositionFVF == D3DFVF_XYZRHW ) Declaration[decl++] = D3DVSD_REG( 0, D3DVSDT_FLOAT4 ); 
    else                                 Declaration[decl++] = D3DVSD_REG( 0, D3DVSDT_FLOAT3 ); 

    // Handle blend weights
    if( dwPositionFVF == D3DFVF_XYZB1 )  Declaration[decl++] = D3DVSD_REG( 1, D3DVSDT_FLOAT1 ); 
    if( dwPositionFVF == D3DFVF_XYZB2 )  Declaration[decl++] = D3DVSD_REG( 1, D3DVSDT_FLOAT2 ); 
    if( dwPositionFVF == D3DFVF_XYZB3 )  Declaration[decl++] = D3DVSD_REG( 1, D3DVSDT_FLOAT3 ); 
    if( dwPositionFVF == D3DFVF_XYZB4 )  Declaration[decl++] = D3DVSD_REG( 1, D3DVSDT_FLOAT4 ); 

    // Handle normal, diffuse, and specular
    if( dwFVF & D3DFVF_NORMAL )          Declaration[decl++] = D3DVSD_REG( 2, D3DVSDT_FLOAT3 );
    if( dwFVF & D3DFVF_DIFFUSE )         Declaration[decl++] = D3DVSD_REG( 3, D3DVSDT_D3DCOLOR );
    if( dwFVF & D3DFVF_SPECULAR )        Declaration[decl++] = D3DVSD_REG( 4, D3DVSDT_D3DCOLOR );

    // Handle texture coordinates
    DWORD dwNumTextures = (dwFVF & D3DFVF_TEXCOUNT_MASK) >> D3DFVF_TEXCOUNT_SHIFT;

    for( DWORD i=0; i<dwNumTextures; i++ )
    {
        DWORD dwTexCoordSize = ( dwFVF & (0x00030000<<(i*2)) );

        DWORD dwNumTexCoords = 0;
        if( dwTexCoordSize == D3DFVF_TEXCOORDSIZE1(i) ) dwNumTexCoords = D3DVSDT_FLOAT1;
        if( dwTexCoordSize == D3DFVF_TEXCOORDSIZE2(i) ) dwNumTexCoords = D3DVSDT_FLOAT2;
        if( dwTexCoordSize == D3DFVF_TEXCOORDSIZE3(i) ) dwNumTexCoords = D3DVSDT_FLOAT3;
        if( dwTexCoordSize == D3DFVF_TEXCOORDSIZE4(i) ) dwNumTexCoords = D3DVSDT_FLOAT4;

        Declaration[decl++] = D3DVSD_REG( 9 + i, dwNumTexCoords );
    }

    // End the declarator
    Declaration[decl++] = D3DVSD_END();

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: XBUtil_GetWide()
// Desc: Convert CHAR string to WCHAR string. dwMax includes the null byte.
//       Never copies more than dwMax-1 characters into strWide.
//          Ex: GetWide( "abc", strWide, 3 ) gives strWide = "ab"
//       Typical usage:
//          WCHAR strResult[MAX];
//          XBUtil_GetWide( strThin, strResult, MAX );
//-----------------------------------------------------------------------------
VOID XBUtil_GetWide( const CHAR* strThin, WCHAR* strWide, DWORD dwMax )
{
    assert( strThin != NULL );
    assert( strWide != NULL );

    // dwMax includes the null bytes, so must always be at least one.
    // Furthermore, if dwMax is 0, MultiByteToWideChar doesn't do any 
    // conversion, but returns the number of chars it *would* convert.
    assert( dwMax > 0 );

    // Determine how many characters we will convert. Can never be more
    // than dwMax-1
    INT nChars = lstrlenA( strThin );
    if( nChars > INT(dwMax) - 1 )
        nChars = INT(dwMax) - 1;

    // Perform the conversion
    INT iWide = MultiByteToWideChar( CP_ACP, 0, strThin, nChars, 
                                     strWide, dwMax );
    assert( nChars == iWide - 1 );
    (VOID)iWide; // avoid compiler warning in release mode
}
