/*
 * XBoxMediaPlayer
 * Copyright (c) 2002 d7o3g4q and RUNTiME
 * Portions Copyright (c) by the authors of ffmpeg and xvid
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

//-----------------------------------------------------------------------------
// File: Panel.h
//
// Desc: Support class for rendering a panel image.
//-----------------------------------------------------------------------------
#ifndef Panel_H
#define Panel_H

#include <xtl.h>

#define WIDE_SCREEN_RATIO_COMPENSATION 0.4444

//-----------------------------------------------------------------------------
// Name: class CPanel
// Desc: Class for rendering an panel image.
//-----------------------------------------------------------------------------
class CPanel
{
protected:
	struct VERTEX { D3DXVECTOR4 p; D3DCOLOR col; float tu, tv; };

	struct TLVertex
	{
	    float    x, y, z, rhw;
	    D3DCOLOR specular, col;
	    float    tu, tv;
	};

	static const DWORD VertexFVF = (D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_SPECULAR | D3DFVF_TEX1 );

	static const DWORD FVF_VERTEX = D3DFVF_XYZRHW|D3DFVF_DIFFUSE|D3DFVF_TEX1;

	LPDIRECT3DVERTEXBUFFER8 m_pVB;
	BOOL  m_bManaged;


public:

	LPDIRECT3DDEVICE8       m_pd3dDevice;
	unsigned char			*m_pFileBuf ;
	unsigned int            m_nFileBufSize ;
	D3DCOLOR m_colDiffuse;
	LPDIRECT3DTEXTURE8      m_pTexture;
	float m_nWidth;
	float m_nHeight;
	D3DXMATRIX matrixProjection;	// Projection matrix (How the scene is rendered to screen)
	D3DXMATRIX matrixView;			// View matrix (Camera's position and rotation)
	D3DXMATRIX matrixWorld;			// World matrix (Object's position and rotation)
	D3DXMATRIX matrixTemp;			// Temp matrix used to combine matrix operations
	LPDIRECT3DVERTEXBUFFER8 g_pVBCube;	// Global pointer to Direct3D vertex buffer

	// Constructor/destructor
	CPanel();
	~CPanel();

	// Functions to create and destroy the internal objects
	ptrdiff_t Recreate( LPDIRECT3DDEVICE8 pd3dDevice);
	ptrdiff_t Create( LPDIRECT3DDEVICE8 pd3dDevice, LPDIRECT3DTEXTURE8 pd3dTexture, BOOL bManaged=FALSE, float fSrcWidth = 0.0f, float fSrcHeight = 0.0f);
	ptrdiff_t CreateMemory( LPDIRECT3DDEVICE8 pd3dDevice, char *filename, float width, float height ) ;
	ptrdiff_t CreateSized( LPDIRECT3DDEVICE8 pd3dDevice, LPDIRECT3DTEXTURE8 pd3dTexture, float fX, float fY, float fSrcWidth = 0.0f, float fSrcHeight = 0.0f);
	// This method's code is covered in a previous tutorial, it basically tells DirectX how
	// to how the scene is displayed on the 2D screen.
	void SetProjection( float FOVdegrees, float closeClippingPlane, float farClippingPlane, int scrWidth, int scrHeight);

	// This method's code is covered in a previous tutorial.
	// It sets the position and rotation of the camera
	void SetView( float Xpos, float Ypos, float Zpos,			// Cams position
				float Xtarget, float Ytarget, float Ztarget,	// Where it's looking
				float Xup, float Yup, float Zup );				// Camera's up vector (usually 0,1,0)

	// The rest of the methods are for the world view matrix.
	// The world view matrix is used to position, rotate and scale whatever polygons that we
	// are about to render.
	
	// One line in this method! Am I lazy or what!
	void ResetWorld(void);	// Resets the world matrix

	// This moves the world matrix to a new position
	void TranslateWorld( float Xpos, float Ypos, float Zpos);	// Move to given position

	// This rotates the matrix
	void RotateWorld( float Xrot, float Yrot, float Zrot);		// Rotate world matrix
	void ScaleWorld( float Xsca, float Ysca, float Zsca);		// Rotate world matrix

	BOOL IsValid();

	operator LPDIRECT3DTEXTURE8()
	{
		if (m_bManaged)
			m_pTexture->AddRef();
		
		return m_pTexture;
	};

	ptrdiff_t Destroy();

	// Renders the panel
	ptrdiff_t Render();
	ptrdiff_t Render(float x, float y, bool bLogical=true);
	ptrdiff_t Render(float x, float y, float w, float h, float x2, float y2, bool bLogical=true);
	ptrdiff_t Render(float x, float y, float nw, float nh, bool bLogical=true);
	ptrdiff_t Render(float x, float y, float w, float h, float x2, float y2, float w2, float h2);
	ptrdiff_t RenderOnCube(float x, float y, float w, float h, float x2, float y2, float w2, float h2, int face, float rx, float ry, float rz);
	ptrdiff_t RenderOnCube2(float x, float y, float w, float h, float x2, float y2, float w2, float h2, int face, float rx, float ry, float rz, float scrw, float scrh, int alpha);
	bool InitVertices();
	bool SetupVertices( float x, float y, float w, float h, float x2, float y2, float w2, float h2, float scrw, float scrh, int alpha);

	ptrdiff_t SetColourDiffuse( D3DCOLOR colour );
	ptrdiff_t SetAlpha(DWORD dwAlpha);

	float GetWidth()	{ return m_nWidth; };
	float GetHeight()	{ return m_nHeight; };
};

#endif
