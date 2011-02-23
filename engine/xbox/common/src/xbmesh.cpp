//-----------------------------------------------------------------------------
// File: XBMesh.cpp
//
// Desc: Support code for loading geometry stored in .xbg files. See the
//       <XBMesh.h> header file for information on using this class.
//
// Hist: 11.01.00 - New for November XDK release
//       12.15.00 - Changes for December XDK release
//       03.15.01 - Mass changes (removed D3DX and .x support) for April XDK
//       04.15.01 - Using packed resources for May XDK
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include <xtl.h>
#include <xgmath.h>
#include <stdio.h>
#include "XBMesh.h"
#include "XBUtil.h"




//-----------------------------------------------------------------------------
// Name: CXBMesh()
// Desc: 
//-----------------------------------------------------------------------------
CXBMesh::CXBMesh()
{
    m_pAllocatedSysMem = NULL;
    m_pAllocatedVidMem = NULL;
    m_pMeshFrames      = NULL;
    m_dwNumFrames      = 0;
    m_dwRefCount       = 1L;
}




//-----------------------------------------------------------------------------
// Name: ~CXBMesh()
// Desc: 
//-----------------------------------------------------------------------------
CXBMesh::~CXBMesh()
{
    // Free textures
    for( DWORD i=0; i<m_dwNumFrames; i++ )
    {
        for( DWORD j = 0; j < m_pMeshFrames[i].m_MeshData.m_dwNumSubsets; j++ )
        {
            SAFE_RELEASE( m_pMeshFrames[i].m_MeshData.m_pSubsets[j].pTexture );
        }
    }

    // Free allocated memory
    if( m_pAllocatedSysMem )
        delete[] m_pAllocatedSysMem;

    if( m_pAllocatedVidMem )
        D3D_FreeContiguousMemory( m_pAllocatedVidMem );
}




//-----------------------------------------------------------------------------
// Name: Create()
// Desc: 
//-----------------------------------------------------------------------------
HRESULT CXBMesh::Create( LPDIRECT3DDEVICE8 pd3dDevice, CHAR* strFilename,
                         CXBPackedResource* pResource )
{
    // Find the media file
    CHAR strMeshPath[512];
    if( FAILED( XBUtil_FindMediaFile( strMeshPath, strFilename ) ) )
        return E_FAIL;

    // Open the file
    HANDLE hFile;
    DWORD dwNumBytesRead;
    hFile = CreateFile(strMeshPath, GENERIC_READ, FILE_SHARE_READ, NULL,
                       OPEN_EXISTING, FILE_ATTRIBUTE_READONLY, NULL);
    if(hFile == INVALID_HANDLE_VALUE)
    {
        OUTPUT_DEBUG_STRING( "CXBMesh::Create(): ERROR: File not found!\n" );
        return E_FAIL;
    }

    // Read the magic number
    DWORD dwFileID;
    ReadFile(hFile, &dwFileID, sizeof(DWORD), &dwNumBytesRead, NULL);
    if( dwFileID != XBG_FILE_ID )
    {
        OUTPUT_DEBUG_STRING( "CXBMesh::Create(): ERROR: Invalid XBG file type!\n" );
        return E_FAIL;
    }

    // Read in header
    DWORD dwNumFrames;  // Number of mesh frames in the file
    DWORD dwSysMemSize; // Num bytes needed for system memory objects
    DWORD dwVidMemSize; // Num bytes needed for video memory objects

    ReadFile(hFile,  &dwNumFrames, sizeof(DWORD), &dwNumBytesRead, NULL);
    ReadFile(hFile,  &dwSysMemSize, sizeof(DWORD), &dwNumBytesRead, NULL);
    ReadFile(hFile,  &dwVidMemSize, sizeof(DWORD), &dwNumBytesRead, NULL);
    
    // Read in system memory objects
    m_pAllocatedSysMem = (VOID*)new BYTE[dwSysMemSize];
    ReadFile(hFile, m_pAllocatedSysMem, dwSysMemSize, &dwNumBytesRead, NULL);
        

    // Read in video memory objects
    m_pAllocatedVidMem = D3D_AllocContiguousMemory( dwVidMemSize, D3DVERTEXBUFFER_ALIGNMENT );
    ReadFile(hFile, m_pAllocatedVidMem, dwVidMemSize, &dwNumBytesRead, NULL);
        
    // Done with the file
    CloseHandle(hFile);

    // Now we need to patch the mesh data. Any pointers read from the file were
    // stored as file offsets. So, we simply need to add a base address to patch
    // things up.
    m_pMeshFrames = (XBMESH_FRAME*)m_pAllocatedSysMem;
    m_dwNumFrames = dwNumFrames;

    for( DWORD i=0; i<m_dwNumFrames; i++ )
    {
        XBMESH_FRAME* pFrame = &m_pMeshFrames[i];
        XBMESH_DATA*  pMesh  = &m_pMeshFrames[i].m_MeshData;

        if( pFrame->m_pChild )
            pFrame->m_pChild  = (XBMESH_FRAME*)( (DWORD)pFrame->m_pChild - 16 + (DWORD)m_pMeshFrames );
        if( pFrame->m_pNext )
            pFrame->m_pNext   = (XBMESH_FRAME*)( (DWORD)pFrame->m_pNext  - 16 + (DWORD)m_pMeshFrames );
        if( pMesh->m_pSubsets )
            pMesh->m_pSubsets = (XBMESH_SUBSET*)( (DWORD)pMesh->m_pSubsets - 16 + (DWORD)m_pMeshFrames);
        
        if( pMesh->m_dwNumIndices )
            pMesh->m_IB.Data  = pMesh->m_IB.Data - 16 + (DWORD)m_pMeshFrames;
        if( pMesh->m_dwNumVertices )
            pMesh->m_VB.Register( m_pAllocatedVidMem );
    }

    // Finally, create any textures used by the meshes' subsets. In this 
    // implementation, we are pulling textures out of the passed in resource.
    if( pResource )
    {
        for( DWORD i=0; i<m_dwNumFrames; i++ )
        {
            XBMESH_DATA* pMesh = &m_pMeshFrames[i].m_MeshData;

            for( DWORD j = 0; j < pMesh->m_dwNumSubsets; j++ )
            {
                XBMESH_SUBSET* pSubset = &pMesh->m_pSubsets[j];

                pSubset->pTexture = pResource->GetTexture( pSubset->strTexture );
            }
        }
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Renders the hierarchy of frames and meshes.
//-----------------------------------------------------------------------------
HRESULT CXBMesh::Render( LPDIRECT3DDEVICE8 pd3dDevice, DWORD dwFlags )
{
    if( m_pMeshFrames )
        RenderFrame( pd3dDevice, m_pMeshFrames, dwFlags );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RenderFrame()
// Desc: Renders a frame (save state, apply matrix, render children, restore).
//-----------------------------------------------------------------------------
HRESULT CXBMesh::RenderFrame( LPDIRECT3DDEVICE8 pd3dDevice, XBMESH_FRAME* pFrame, 
                              DWORD dwFlags )
{
    // Apply the frame's local transform
    D3DXMATRIX matSavedWorld, matWorld;
    pd3dDevice->GetTransform( D3DTS_WORLD, &matSavedWorld );
    D3DXMatrixMultiply( &matWorld, &pFrame->m_matTransform, &matSavedWorld );
    pd3dDevice->SetTransform( D3DTS_WORLD, &matWorld );

    // Render the mesh data
    if( pFrame->m_MeshData.m_dwNumSubsets ) 
        RenderMesh( pd3dDevice, &pFrame->m_MeshData, dwFlags );

    // Render any child frames
    if( pFrame->m_pChild ) 
        RenderFrame( pd3dDevice, pFrame->m_pChild, dwFlags );

    // Restore the transformation matrix
    pd3dDevice->SetTransform( D3DTS_WORLD, &matSavedWorld );
    
    // Render any sibling frames
    if( pFrame->m_pNext )  
        RenderFrame( pd3dDevice, pFrame->m_pNext, dwFlags );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: RenderMesh()
// Desc: Renders the mesh geometry.
//-----------------------------------------------------------------------------
HRESULT CXBMesh::RenderMesh( LPDIRECT3DDEVICE8 pd3dDevice, XBMESH_DATA* pMesh, 
                             DWORD dwFlags )
{
    D3DVertexBuffer* pVB           = &pMesh->m_VB;
    DWORD            dwNumVertices =  pMesh->m_dwNumVertices;
    D3DIndexBuffer*  pIB           = &pMesh->m_IB;
    DWORD            dwNumIndices  =  pMesh->m_dwNumIndices;
    DWORD            dwFVF         =  pMesh->m_dwFVF;
    DWORD            dwVertexSize  =  pMesh->m_dwVertexSize;
    D3DPRIMITIVETYPE dwPrimType    =  pMesh->m_dwPrimType;
    DWORD            dwNumSubsets  =  pMesh->m_dwNumSubsets;
    XBMESH_SUBSET*   pSubsets      = &pMesh->m_pSubsets[0];

    (VOID)dwNumIndices; // not used

    if( dwNumVertices == 0 )
        return S_OK;

    // Set the vertex stream
    pd3dDevice->SetStreamSource( 0, pVB, dwVertexSize );
    pd3dDevice->SetIndices( pIB, 0 );

    // Set the FVF code, unless the user asked us not to
    if( 0 == ( dwFlags & XBMESH_NOFVF ) )
        pd3dDevice->SetVertexShader( dwFVF );

    // Render the subsets
    for( DWORD i = 0; i < dwNumSubsets; i++ )
    {
        BOOL bRender = FALSE;

        // Render the opaque subsets, unless the user asked us not to
        if( 0 == ( dwFlags & XBMESH_ALPHAONLY ) )
        {
            if( 0 == ( dwFlags & XBMESH_NOMATERIALS ) )
            {
                if( pSubsets[i].mtrl.Diffuse.a >= 1.0f )
                    bRender = TRUE;
            }
            else
                bRender = TRUE;
        }

        // Render the transparent subsets, unless the user asked us not to
        if( 0 == ( dwFlags & XBMESH_OPAQUEONLY ) )
        {
            if( 0 == ( dwFlags & XBMESH_NOMATERIALS ) )
            {
                if( pSubsets[i].mtrl.Diffuse.a < 1.0f )
                    bRender = TRUE;
            }
        }

        if( bRender )
        {
            // Set the material, unless the user asked us not to
            if( 0 == ( dwFlags & XBMESH_NOMATERIALS ) )
                pd3dDevice->SetMaterial( &pSubsets[i].mtrl );

            // Set the texture, unless the user asked us not to
            if( 0 == ( dwFlags & XBMESH_NOTEXTURES ) )
                pd3dDevice->SetTexture( 0, pSubsets[i].pTexture );

            // Call the callback, so the app can tweak state before rendering
            // each subset
            BOOL bRenderSubset = RenderCallback( pd3dDevice, i, &pSubsets[i], dwFlags );

            // Draw the mesh subset
            if( bRenderSubset )
            {
                DWORD dwNumPrimitives = ( D3DPT_TRIANGLESTRIP == dwPrimType ) ? pSubsets[i].dwIndexCount-2 : pSubsets[i].dwIndexCount/3;
                pd3dDevice->DrawIndexedPrimitive( dwPrimType, 0, pSubsets[i].dwIndexCount,
                                                  pSubsets[i].dwIndexStart, dwNumPrimitives );
            }
        }
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: ComputeRadius()
// Desc: Finds the furthest point from zero on the mesh.
//-----------------------------------------------------------------------------
FLOAT CXBMesh::ComputeRadius()
{
    D3DXMATRIX matIdentity;
    D3DXMatrixIdentity( &matIdentity );

    return ComputeFrameRadius( m_pMeshFrames, &matIdentity );
}




//-----------------------------------------------------------------------------
// Name: ComputeFrameRadius()
// Desc: Calls ComputeMeshRadius for each frame with the correct transform.
//-----------------------------------------------------------------------------
FLOAT CXBMesh::ComputeFrameRadius( XBMESH_FRAME* pFrame, D3DXMATRIX* pmatParent )
{
    // Apply the frame's local transform
    D3DXMATRIX matWorld;
    D3DXMatrixMultiply( &matWorld, &pFrame->m_matTransform, pmatParent );

    FLOAT fRadius = 0.0f;

    // Compute bounds for the mesh data
    if( pFrame->m_MeshData.m_dwNumSubsets ) 
        fRadius = ComputeMeshRadius( &pFrame->m_MeshData, &matWorld );

    // Compute bounds for any child frames
    if( pFrame->m_pChild ) 
    {
        FLOAT fChildRadius = ComputeFrameRadius( pFrame->m_pChild, &matWorld  );

        if( fChildRadius > fRadius )
            fRadius = fChildRadius;
    }

    // Compute bounds for any sibling frames
    if( pFrame->m_pNext )  
    {
        FLOAT fSiblingRadius = ComputeFrameRadius( pFrame->m_pNext, pmatParent );

        if( fSiblingRadius > fRadius )
            fRadius = fSiblingRadius;
    }

    return fRadius;
}




//-----------------------------------------------------------------------------
// Name: ComputeMeshRadius()
// Desc: Finds the furthest point from zero on the mesh.
//-----------------------------------------------------------------------------
FLOAT CXBMesh::ComputeMeshRadius( XBMESH_DATA* pMesh, D3DXMATRIX* pmat )
{
    DWORD       dwNumVertices = pMesh->m_dwNumVertices;
    DWORD       dwVertexSize  = pMesh->m_dwVertexSize;
    BYTE*       pVertices;
    D3DXVECTOR3 vPos;
    FLOAT       fMaxDist2 = 0.0f;

    pMesh->m_VB.Lock( 0, 0, &pVertices, 0 );

    while( dwNumVertices-- )
    {
        D3DXVec3TransformCoord( &vPos, (D3DXVECTOR3*)pVertices, pmat );

        FLOAT fDist2 = vPos.x*vPos.x + vPos.y*vPos.y + vPos.z*vPos.z;

        if( fDist2 > fMaxDist2 )
            fMaxDist2 = fDist2;

        pVertices += dwVertexSize;
    }

    pMesh->m_VB.Unlock();

    return sqrtf( fMaxDist2 );
}

//-----------------------------------------------------------------------------
//  Take the union of two boxes
//-----------------------------------------------------------------------------
inline float MAX(float a, float b) { return a > b ? a : b; }
inline float MIN(float a, float b) { return a < b ? a : b; }
static void UnionBox(D3DXVECTOR3 *pvMin, D3DXVECTOR3 *pvMax, const D3DXVECTOR3 &vMin, const D3DXVECTOR3 &vMax)
{
    pvMin->x = MIN(pvMin->x, vMin.x);
    pvMin->y = MIN(pvMin->y, vMin.y);
    pvMin->z = MIN(pvMin->z, vMin.z);
    pvMax->x = MAX(pvMax->x, vMax.x);
    pvMax->y = MAX(pvMax->y, vMax.y);
    pvMax->z = MAX(pvMax->z, vMax.z);
}

//-----------------------------------------------------------------------------
// Name: ComputeBoundingBox()
// Desc: Calculates the bounding box of the entire hierarchy.
//-----------------------------------------------------------------------------
HRESULT CXBMesh::ComputeBoundingBox(D3DXVECTOR3 *pvMin, D3DXVECTOR3 *pvMax)
{
    D3DXMATRIX matIdentity;
    D3DXMatrixIdentity( &matIdentity );
    return ComputeFrameBoundingBox( m_pMeshFrames, &matIdentity, pvMin, pvMax );
}

//-----------------------------------------------------------------------------
// Name: ComputeFrameBoundingBox()
// Desc: Calls ComputeMeshBoundingBox for each frame with the correct transform.
//-----------------------------------------------------------------------------
HRESULT CXBMesh::ComputeFrameBoundingBox( XBMESH_FRAME* pFrame, D3DXMATRIX* pmatParent, D3DXVECTOR3 *pvMin, D3DXVECTOR3 *pvMax)
{
    HRESULT hr;
    
    // initialize bounds to be reset on the first UnionBox
    pvMin->x = pvMin->y = pvMin->z = FLT_MAX;
    pvMax->x = pvMax->y = pvMax->z = -FLT_MAX;
    
    // Apply the frame's local transform
    D3DXMATRIX matWorld;
    D3DXMatrixMultiply( &matWorld, &pFrame->m_matTransform, pmatParent );

    // Compute bounds for the mesh data
    if( pFrame->m_MeshData.m_dwNumSubsets )
    {
        D3DXVECTOR3 vMin, vMax;
        hr = ComputeMeshBoundingBox( &pFrame->m_MeshData, &matWorld, &vMin, &vMax );
        if (FAILED(hr))
            return hr;
        UnionBox(pvMin, pvMax, vMin, vMax);
    }

    // Compute bounds for any child frames
    if( pFrame->m_pChild ) 
    {
        D3DXVECTOR3 vMin, vMax;
        hr = ComputeFrameBoundingBox( pFrame->m_pChild, &matWorld, &vMin, &vMax );
        if (FAILED(hr))
            return hr;
        UnionBox(pvMin, pvMax, vMin, vMax);
    }

    // Compute bounds for any sibling frames
    if( pFrame->m_pNext )  
    {
        D3DXVECTOR3 vMin, vMax;
        hr = ComputeFrameBoundingBox( pFrame->m_pNext, pmatParent, &vMin, &vMax );
        if (FAILED(hr))
            return hr;
        UnionBox(pvMin, pvMax, vMin, vMax);
    }
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: ComputeMeshBoundingBox()
// Desc: Calculate the bounding box of the transformed mesh.
//-----------------------------------------------------------------------------
HRESULT CXBMesh::ComputeMeshBoundingBox( XBMESH_DATA* pMesh, D3DXMATRIX* pmat, D3DXVECTOR3 *pvMin, D3DXVECTOR3 *pvMax)
{
    // initialize bounds to be reset on the first point
    pvMin->x = pvMin->y = pvMin->z = FLT_MAX;
    pvMax->x = pvMax->y = pvMax->z = -FLT_MAX;
    DWORD       dwNumVertices = pMesh->m_dwNumVertices;
    DWORD       dwVertexSize  = pMesh->m_dwVertexSize;
    BYTE*       pVertices;
    D3DXVECTOR3 vPos;
    pMesh->m_VB.Lock( 0, 0, &pVertices, 0 );
    while( dwNumVertices-- )
    {
        D3DXVec3TransformCoord( &vPos, (D3DXVECTOR3*)pVertices, pmat );
        UnionBox(pvMin, pvMax, vPos, vPos);    // expand the bounding box to include the point
        pVertices += dwVertexSize;
    }
    pMesh->m_VB.Unlock();
    return S_OK;
}
