//-----------------------------------------------------------------------------
// File: XBResource.h
//
// Desc: Loads resources from an XPR (Xbox Packed Resource) file.  
//
// Hist: 03.12.01 - New for April XDK release
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef XBRESOURCE_H
#define XBRESOURCE_H



//-----------------------------------------------------------------------------
// Name: XBResource_SizeOf()
// Desc: Determines the byte size of a D3DResource
//-----------------------------------------------------------------------------
DWORD XBResource_SizeOf( LPDIRECT3DRESOURCE8 pResource );





//-----------------------------------------------------------------------------
// Name: struct XBRESOURCE
// Desc: Name tag for resources. An app may initialize this structure, and pass
//       it to the resource's Create() function. From then on, the app may call
//       GetResource() to retrieve a resource using an ascii name.
//-----------------------------------------------------------------------------
struct XBRESOURCE
{
    CHAR* strName;
    DWORD dwOffset;
};





//-----------------------------------------------------------------------------
// Name: class CXBPackedResource
// Desc: 
//-----------------------------------------------------------------------------
class CXBPackedResource
{
protected:
    BYTE*       m_pSysMemData;    // Alloc'ed memory for resource headers etc.
    BYTE*       m_pVidMemData;    // Alloc'ed memory for resource data, etc.

    DWORD       m_dwNumResources; // Number of loaded resources
 
    XBRESOURCE* m_pResourceTags;  // Tags to associate names with the resources

public:
    // Loads the resources out of the specified bundle
    HRESULT Create( LPDIRECT3DDEVICE8 pd3dDevice, const CHAR* strFilename, 
                    DWORD dwNumResources, XBRESOURCE* pResourceTags = NULL );

    VOID Destroy();

    // Functions to retrieve resources by their offset
    VOID* GetData( DWORD dwOffset ) const
    { return &m_pSysMemData[dwOffset]; }

    LPDIRECT3DRESOURCE8 GetResource( DWORD dwOffset ) const
    { return (LPDIRECT3DRESOURCE8)GetData(dwOffset); }

    LPDIRECT3DTEXTURE8 GetTexture( DWORD dwOffset ) const
    { return (LPDIRECT3DTEXTURE8)GetData( dwOffset ); }

    LPDIRECT3DCUBETEXTURE8 GetCubemap( DWORD dwOffset ) const
    { return (LPDIRECT3DCUBETEXTURE8)GetData( dwOffset ); }

    LPDIRECT3DVOLUMETEXTURE8 GetVolumeTexture( DWORD dwOffset ) const
    { return (LPDIRECT3DVOLUMETEXTURE8)GetData( dwOffset ); }

    LPDIRECT3DVERTEXBUFFER8 GetVertexBuffer( DWORD dwOffset ) const
    { return (LPDIRECT3DVERTEXBUFFER8)GetData( dwOffset ); }

    // Functions to retrieve resources by their name
    VOID* GetData( const CHAR* strName ) const;

    LPDIRECT3DRESOURCE8 GetResource( const CHAR* strName ) const
    { return (LPDIRECT3DRESOURCE8)GetData( strName ); }

    LPDIRECT3DTEXTURE8 GetTexture( const CHAR* strName ) const
    { return (LPDIRECT3DTEXTURE8)GetResource( strName ); }

    LPDIRECT3DCUBETEXTURE8 GetCubemap( const CHAR* strName ) const
    { return (LPDIRECT3DCUBETEXTURE8)GetResource( strName ); }

    LPDIRECT3DVOLUMETEXTURE8 GetVolumeTexture( const CHAR* strName ) const
    { return (LPDIRECT3DVOLUMETEXTURE8)GetResource( strName ); }

    LPDIRECT3DVERTEXBUFFER8 GetVertexBuffer( const CHAR* strName ) const
    { return (LPDIRECT3DVERTEXBUFFER8)GetResource( strName ); }

    // Constructor/destructor
    CXBPackedResource();
    ~CXBPackedResource();
};




#endif XBRESOURCE_H
