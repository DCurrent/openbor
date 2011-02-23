//-----------------------------------------------------------------------------
// File: XBMesh.h
//
// Desc: Support code for loading geometry stored in .xbg files. These files
//       typically converted from .x geometry files using the MakeXBG tool. See
//       that tool for more information.
//
//       XBG files were designed to minimize overhead in the loading and
//       rendering process on the Xbox. The data in a .xbg file is basically
//       stored in one system memory chunk, and one video memory chunk.
//       Therefore, loading a .xbg file is simply two fread() calls followed
//       by some patch up (which turns file offsets into real pointers).
//
//       Geometry files are loaded into arrays of the following structures.
//       XBMESH_FRAME structures contain data to make a frame hierarchy (such
//       as "next" and "child" pointers, plus a transformation matrix). The
//       XMMESH_DATA structure contains data for rendering a mesh (such as
//       the vertex buffer, num of indices, etc.). Finally, the XBMESH_SUBSET
//       structure contains subset properties (materials and textures) and
//       primitive ranges (start index, index count, etc.) for each subset of
//       the data in the XBMESH_DATA structure.
//
//       To use this class, simply instantiate the class, and call Create().
//       Thereafter, the mesh can be rendered with the Render() call. Some
//       render flags are available (see below) to limit what gets rendered.
//       For instance, an app might want to render opaque subsets only, or
//       use a custom vertex shader. For truly custom control, override the
//       CXBMesh class with a new RenderCallback() function, and put any
//       custom pre-rendering code in the callback. The typical use for this
//       is to pass data to a custom vertex shader.
//
// Hist: 11.01.00 - New for November XDK release
//       12.15.00 - Changes for December XDK release
//       03.15.01 - Mass changes (removed D3DX and .x support) for April XDK
//       04.15.01 - Using packed resources for May XDK
//       04.17.01 - 16-byte aligning matrices in the file format
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef XBMESH_H
#define XBMESH_H
#include "XBResource.h"
#include "XBUtil.h"




// Rendering flags. Default is no flags (0x00000000)
#define XBMESH_OPAQUEONLY      0x00000001 // Only render opaque subsets
#define XBMESH_ALPHAONLY       0x00000002 // Only render alpha subsets

#define XBMESH_NOMATERIALS     0x00000010 // Do not use mesh materials
#define XBMESH_NOTEXTURES      0x00000020 // Do not use mesh textures
#define XBMESH_NOFVF           0x00000040 // Do not use mesh FVF code


// The magic number to identify .xbg files
#define XBG_FILE_ID (((DWORD)'X'<<0)|(((DWORD)'B'<<8))|(((DWORD)'G'<<16))|(2<<24))




//-----------------------------------------------------------------------------
// Name: struct XBMESH_SUBSET
// Desc: Struct to hold data for rendering a mesh
//-----------------------------------------------------------------------------
struct XBMESH_SUBSET
{
    D3DMATERIAL8       mtrl;            // Material for this subset
    LPDIRECT3DTEXTURE8 pTexture;        // Texture
    CHAR               strTexture[64];
    DWORD              dwVertexStart;   // Range of vertices to render
    DWORD              dwVertexCount;
    DWORD              dwIndexStart;    // Range of vertex indices to render
    DWORD              dwIndexCount;
};




//-----------------------------------------------------------------------------
// Name: struct XBMESH_DATA
// Desc: Struct for mesh data
//-----------------------------------------------------------------------------
struct XBMESH_DATA
{
    D3DVertexBuffer   m_VB;            // Mesh geometry
    DWORD             m_dwNumVertices;
    D3DIndexBuffer    m_IB;
    DWORD             m_dwNumIndices;
    
    DWORD             m_dwFVF;         // Mesh vertex info
    DWORD             m_dwVertexSize;
    D3DPRIMITIVETYPE  m_dwPrimType;

    DWORD             m_dwNumSubsets;  // Subset info for rendering calls
    XBMESH_SUBSET*    m_pSubsets;
};




//-----------------------------------------------------------------------------
// Name: struct XBMESHFRAME
// Desc: Struct for building a hierarchy of meshes.
//-----------------------------------------------------------------------------
__declspec(align(16)) struct XBMESH_FRAME
{
    D3DXMATRIX        m_matTransform; // The transformation matrix for this frame
    
    XBMESH_DATA       m_MeshData;     // The mesh data belonging to this frame

    CHAR              m_strName[64];
    
    XBMESH_FRAME*     m_pChild;       // Child and sibling ptrs for the hierarchy
    XBMESH_FRAME*     m_pNext;
};




//-----------------------------------------------------------------------------
// Name: class CXBMesh
// Desc: Wrapper class for loading geometry files, and rendering the resulting
//       hierarchy of meshes and frames.
//-----------------------------------------------------------------------------
class CXBMesh
{
    // Memory allocated during file loading. Ptrs are retained for cleanup.
    VOID* m_pAllocatedSysMem;
    VOID* m_pAllocatedVidMem;

public:
    // Hierarchy (frames and meshes) of loaded geometry
    XBMESH_FRAME* m_pMeshFrames;
    DWORD         m_dwNumFrames;

    // Internal rendering functions
    virtual HRESULT RenderFrame( LPDIRECT3DDEVICE8 pd3dDevice, XBMESH_FRAME* pMesh, 
                                 DWORD dwFlags );
    virtual HRESULT RenderMesh( LPDIRECT3DDEVICE8 pd3dDevice, XBMESH_DATA* pMesh, 
                                DWORD dwFlags );

    // Internal functions to find the radius of sphere centered at zero enclosing mesh.
    float ComputeFrameRadius(XBMESH_FRAME* pFrame, D3DXMATRIX* pParentMat);
    float ComputeMeshRadius(XBMESH_DATA* pMesh, D3DXMATRIX* pMat);
    
    // Internal functions to find the bounding box of the mesh.
    HRESULT ComputeFrameBoundingBox(XBMESH_FRAME* pFrame, D3DXMATRIX* pParentMat, D3DXVECTOR3 *pvMin, D3DXVECTOR3 *pvMax);
    HRESULT ComputeMeshBoundingBox(XBMESH_DATA* pMesh, D3DXMATRIX* pMat, D3DXVECTOR3 *pvMin, D3DXVECTOR3 *pvMax);

public:
    // Reference counting
    DWORD   m_dwRefCount;
    DWORD   AddRef()  { return ++m_dwRefCount; }
    DWORD   Release() { if( --m_dwRefCount ) return m_dwRefCount;
                        delete this; return 0L; }

public:
    // Constructor/destructor
    CXBMesh();
    virtual ~CXBMesh();

    // Creation function. Call this function to create the hierarchy of frames
    // and meshes from a geometry file.
    HRESULT Create( LPDIRECT3DDEVICE8 pd3dDevice, CHAR* strFilename,
                    CXBPackedResource* pResource = NULL );

    // Access functions
    XBMESH_FRAME* GetFrame( DWORD i ) { return &m_pMeshFrames[i]; }
    XBMESH_DATA*  GetMesh( DWORD i )  { return &m_pMeshFrames[i].m_MeshData; }

    // Overridable callback function (called before anything is rendered). 
    // This is useful for setting vertex shader constants, etc., before
    // rendering.
    virtual BOOL RenderCallback( LPDIRECT3DDEVICE8 pd3dDevice, DWORD dwSubset,
                                 XBMESH_SUBSET* pSubset, DWORD dwFlags ) { return TRUE; }
    
    // Render function. Call this function to render the hierarchy of frames
    // and meshes.
    HRESULT Render( LPDIRECT3DDEVICE8 pd3dDevice, DWORD dwFlags = 0x00000000 );

    // Function to find the radius of sphere centered at zero enclosing mesh.
    float ComputeRadius();
    // find the bounding box of all the subsets
    HRESULT ComputeBoundingBox(D3DXVECTOR3 *pvMin, D3DXVECTOR3 *pvMax);
};




#endif
