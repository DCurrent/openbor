/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

//-----------------------------------------------------------------------------
// File: Panel.cpp
//
// Desc: Support class for rendering an image.
//-----------------------------------------------------------------------------
#include <xtl.h>
#include "Panel.h"
#include "GraphicsContext.h"

// Here we create a structure that we can use to define a vertex.
struct sVertex
{
    D3DXVECTOR3 position;	// The 3-D position for the vertex.
//	D3DXVECTOR3 normal;		// The Normal of the vertex (Used for lighting)
    DWORD diffuse;			// Value used to hold the vertices diffuse colour value
// <**************************************** NEW ********************************************>
	// We've added another variable to the vertex structure for texturing.
	// When applying textures to polygons, DirectX needs to how it should position the texture
	// The tu is for the X axis and tv for the Y axis. We use u & v, as this is the standard
	// format used for texture coordinates, but we could've called them hamster and potato if
	// we wanted :)
	FLOAT       tu, tv;		// The texture coordinates
// <**************************************** NEW ********************************************>
};

sVertex sCube[24];	// Create array of above structure to hold vertices for a cube
// <**************************************** NEW ********************************************>
// As we've added tu and tv variables into the sVertex structure above, we need to tell
// DirectX what data, this structure contains. This is very similar to the previous tutorial,
// but with the inclusion of the D3DFVF_TEX1 tag, which specdifies that the above sVertex
// structure includes texture coordinates aswell.
#define D3DFVF_sVertex (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_TEX1)	// Custom FVF.
//#define D3DFVF_sVertex (D3DFVF_XYZ|D3DFVF_DIFFUSE|D3DFVF_NORMAL|D3DFVF_TEX1)	// Custom FVF.


//-----------------------------------------------------------------------------
// Name: CPanel()
// Desc: Help class constructor
//-----------------------------------------------------------------------------
CPanel::CPanel()
{
    m_pd3dDevice    = NULL;
    m_pTexture		= NULL;
    m_pVB           = NULL;
	g_pVBCube       = NULL ;
	m_nWidth		= 0;
	m_nHeight		= 0;
	m_bManaged		= FALSE;
	m_colDiffuse	= 0xFFFFFFFF;
	m_pFileBuf      = NULL ;
	m_nFileBufSize  = 0 ;
}


#ifdef __cplusplus
extern "C" {
#endif

#ifdef __cplusplus
}
#endif


//-----------------------------------------------------------------------------
// Name: ~CPanel()
// Desc: Help class destructor
//-----------------------------------------------------------------------------
CPanel::~CPanel()
{
    Destroy();
}


//-----------------------------------------------------------------------------
// Name: Create()
// Desc: Creates the panel class' internal objects
//-----------------------------------------------------------------------------
HRESULT CPanel::CreateMemory( LPDIRECT3DDEVICE8 pd3dDevice, char *filename, float width, float height )
{
	FILE *infile ;

	infile = fopen( filename, "rb" ) ;

	if ( !infile )
	{
		return D3DERR_NOTAVAILABLE ;
	}

	fseek( infile, 0, SEEK_END ) ;
	m_nFileBufSize = ftell( infile ) ;
	fseek( infile, 0, SEEK_SET ) ;

	if ( m_pFileBuf )
	{
		free( m_pFileBuf ) ;
		m_pFileBuf = NULL ;
	}

	m_pFileBuf = (unsigned char*)malloc( m_nFileBufSize ) ;

	if ( !m_pFileBuf )
	{
		fclose( infile ) ;
		return D3DERR_NOTAVAILABLE  ;
	}

	fread( m_pFileBuf, 1, m_nFileBufSize, infile ) ;
	fclose( infile ) ;


	return (Create(pd3dDevice, NULL, FALSE, width, height)) ;


}


//-----------------------------------------------------------------------------
// Name: Create()
// Desc: Creates the panel class' internal objects
//-----------------------------------------------------------------------------
HRESULT CPanel::Create( LPDIRECT3DDEVICE8 pd3dDevice, LPDIRECT3DTEXTURE8 pd3dTexture, BOOL bManaged, FLOAT fSrcWidth, FLOAT fSrcHeight)
{

	InitVertices() ;

    if (m_pVB!=NULL)
	{
		m_pVB->Release();
		m_pVB=NULL;
	}

	if ( m_pTexture )
	{
		m_pTexture->Release() ;
		m_pTexture = NULL ;
	}

    m_pd3dDevice = pd3dDevice;
    m_pTexture = pd3dTexture;
	m_bManaged = bManaged;


	if (fSrcWidth > 0 && fSrcHeight > 0)
	{

		m_nWidth = fSrcWidth;
		m_nHeight = fSrcHeight;
	}
	else
	{
		D3DSURFACE_DESC desc;
		m_pTexture->GetLevelDesc(0,&desc);

		m_nWidth = (float) desc.Width;
		m_nHeight = (float) desc.Height;
	}

    m_pd3dDevice->CreateVertexBuffer( 4*sizeof(VERTEX), D3DUSAGE_WRITEONLY,
                                    0L, D3DPOOL_DEFAULT, &m_pVB );



	//TLVertex * v;

	CPanel::VERTEX* v;
    m_pVB->Lock( 0, 0, (BYTE**)&v, 0L );

	FLOAT fWidth  = 640 ;
	FLOAT fHeight = 480 ;

    v[0].p = D3DXVECTOR4( 0 - 0.5f,			0 - 0.5f,			0, 0 );
	v[0].tu = 0;
	v[0].tv = 0;
	v[0].col= m_colDiffuse;

	v[1].p = D3DXVECTOR4( fWidth - 0.5f,	0 - 0.5f,			0, 0 );
	v[1].tu = m_nWidth;
	v[1].tv = 0;
	v[1].col= m_colDiffuse;

    v[2].p = D3DXVECTOR4( fWidth - 0.5f,	fHeight - 0.5f,	0, 0 );
	v[2].tu = m_nWidth;
	v[2].tv = m_nHeight;
	v[2].col= m_colDiffuse;

    v[3].p = D3DXVECTOR4( 0 - 0.5f,			fHeight - 0.5f,	0, 0 );
	v[3].tu = 0;
	v[3].tv = m_nHeight;
	v[3].col= m_colDiffuse;

    m_pVB->Unlock();

	SetAlpha( 0xFF ) ;

    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: Create()
// Desc: Creates the panel class' internal objects
//-----------------------------------------------------------------------------
HRESULT CPanel::Recreate( LPDIRECT3DDEVICE8 pd3dDevice)
{

    if (m_pVB!=NULL)
	{
		m_pVB->Release();
		m_pVB=NULL;
	}

    m_pd3dDevice = pd3dDevice;


    m_pd3dDevice->CreateVertexBuffer( 4*sizeof(VERTEX), D3DUSAGE_WRITEONLY,
                                    0L, D3DPOOL_DEFAULT, &m_pVB );




	CPanel::VERTEX* v;
    m_pVB->Lock( 0, 0, (BYTE**)&v, 0L );

	FLOAT fWidth  = 640 ;
	FLOAT fHeight = 480 ;



    v[0].p = D3DXVECTOR4( 0 - 0.5f,			0 - 0.5f,			0, 0 );
	v[0].tu = 0;
	v[0].tv = 0;
	v[0].col= m_colDiffuse;

	v[1].p = D3DXVECTOR4( fWidth - 0.5f,	0 - 0.5f,			0, 0 );
	v[1].tu = m_nWidth;
	v[1].tv = 0;
	v[1].col= m_colDiffuse;

    v[2].p = D3DXVECTOR4( fWidth - 0.5f,	fHeight - 0.5f,	0, 0 );
	v[2].tu = m_nWidth;
	v[2].tv = m_nHeight;
	v[2].col= m_colDiffuse;

    v[3].p = D3DXVECTOR4( 0 - 0.5f,			fHeight - 0.5f,	0, 0 );
	v[3].tu = 0;
	v[3].tv = m_nHeight;
	v[3].col= m_colDiffuse;

    m_pVB->Unlock();

	SetAlpha( 0xFF ) ;

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: Create()
// Desc: Creates the panel class' internal objects
//-----------------------------------------------------------------------------
HRESULT CPanel::CreateSized( LPDIRECT3DDEVICE8 pd3dDevice, LPDIRECT3DTEXTURE8 pd3dTexture, FLOAT fX, FLOAT fY, FLOAT fSrcWidth, FLOAT fSrcHeight)
{

	if ( m_pTexture )
	{
		m_pTexture->Release() ;
	}

    m_pd3dDevice = pd3dDevice;
    m_pTexture = pd3dTexture;
	m_bManaged = FALSE;

	if (fSrcWidth > 0 && fSrcHeight > 0)
	{

		m_nWidth = fSrcWidth;
		m_nHeight = fSrcHeight;
	}
	else
	{
		D3DSURFACE_DESC desc;
		m_pTexture->GetLevelDesc(0,&desc);

		m_nWidth = (float) desc.Width;
		m_nHeight = (float) desc.Height;
	}

    // Create a vertex buffer for rendering the image
    m_pd3dDevice->CreateVertexBuffer( 4*sizeof(VERTEX), D3DUSAGE_WRITEONLY,
                                    0L, D3DPOOL_DEFAULT, &m_pVB );



	//m_pd3dDevice->CreateVertexBuffer(
                 //4*sizeof(TLVertex), VertexFVF,
                 //D3DUSAGE_WRITEONLY,
                 //D3DPOOL_DEFAULT, &m_pVB);



	//TLVertex * v;

	CPanel::VERTEX* v;
    m_pVB->Lock( 0, 0, (BYTE**)&v, 0L );

	FLOAT fWidth  = 640 ;
	FLOAT fHeight = 480 ;

/*
    v[0].x = 0 - 0.5f ; v[0].y = 0 - 0.5f ;
	v[0].z = 0 ; v[0].rhw = 0 ;
	v[0].tu = 0;
	v[0].tv = 0;
	v[0].col= 0;
	v[0].specular = 0;

	v[1].x = fWidth - 0.5f ; v[1].y = 0 - 0.5f;
	v[1].z = 0 ; v[0].rhw = 0 ;
	v[1].tu = m_nWidth;
	v[1].tv = 0;
	v[1].col= 0;
	v[1].specular = 0;

    v[2].x = fWidth - 0.5f ; v[2].y = fHeight - 0.5f ;
	v[2].z = 0 ; v[0].rhw = 0 ;
	v[2].tu = m_nWidth;
	v[2].tv = m_nHeight;
	v[2].col= 0;
	v[2].specular = 0;

    v[3].x = 0 - 0.5f ; v[3].y = fHeight - 0.5f ;
	v[3].z = 0 ; v[0].rhw = 0 ;
	v[3].tu = 0;
	v[3].tv = m_nHeight;
	v[3].col= 0;
	v[3].specular = 0;
*/



	v[0].p = D3DXVECTOR4( fX - 0.5f,	fY - 0.5f,		0, 0 );
	v[0].tu = 0;
	v[0].tv = 0;
	v[0].col = m_colDiffuse;

    v[1].p = D3DXVECTOR4( fX+m_nWidth - 0.5f,	fY - 0.5f,		0, 0 );
	v[1].tu = 350;
	v[1].tv = 0;
	v[1].col = m_colDiffuse;

    v[2].p = D3DXVECTOR4( fX+m_nWidth - 0.5f,	fY+m_nHeight - 0.5f,	0, 0 );
	v[2].tu = 350;
	v[2].tv = 250;
	v[2].col = m_colDiffuse;

    v[3].p = D3DXVECTOR4( fX - 0.5f,	fY+m_nHeight - 0.5f,	0, 0 );
	v[3].tu = 0;
	v[3].tv = 250;
	v[3].col = m_colDiffuse;

/*
    v[0].p = D3DXVECTOR4( 0 - 0.5f,			0 - 0.5f,			0, 0 );
	v[0].tu = 0;
	v[0].tv = 0;
	v[0].col= m_colDiffuse;

	v[1].p = D3DXVECTOR4( fWidth - 0.5f,	0 - 0.5f,			0, 0 );
	v[1].tu = m_nWidth;
	v[1].tv = 0;
	v[1].col= m_colDiffuse;

    v[2].p = D3DXVECTOR4( fWidth - 0.5f,	fHeight - 0.5f,	0, 0 );
	v[2].tu = m_nWidth;
	v[2].tv = m_nHeight;
	v[2].col= m_colDiffuse;

    v[3].p = D3DXVECTOR4( 0 - 0.5f,			fHeight - 0.5f,	0, 0 );
	v[3].tu = 0;
	v[3].tv = m_nHeight;
	v[3].col= m_colDiffuse;
*/
    m_pVB->Unlock();

    return S_OK;
}



//-----------------------------------------------------------------------------
// Name: Destroy()
// Desc: Destroys the help class' internal objects/
//-----------------------------------------------------------------------------
HRESULT CPanel::Destroy()
{
    if (m_pVB!=NULL)
	{
		m_pVB->Release();
		m_pVB=NULL;
	}

    if (g_pVBCube!=NULL)
	{
		g_pVBCube->Release();
		g_pVBCube=NULL;
	}

	if (m_pTexture)
	{
		if (m_bManaged)
		{
			m_pTexture->Release();
		}

		m_pTexture = NULL;
	}

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Renders the image at the current position (typically 0,0).
//-----------------------------------------------------------------------------
HRESULT CPanel::Render()
{

	if ( m_pd3dDevice == NULL )
		return S_OK+1;

	if ( m_pTexture == NULL )
	{
		if ( m_pFileBuf == NULL )
			return S_OK+1 ;
		else
		{
			if (D3DXCreateTextureFromFileInMemoryEx(m_pd3dDevice, m_pFileBuf, m_nFileBufSize,
				 m_nWidth, m_nHeight, 1, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED,
				 //D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED,
				 D3DX_FILTER_NONE , D3DX_FILTER_NONE, 0, NULL, NULL, &m_pTexture)!=D3D_OK)
			{
				return S_OK+1 ;
			}
		}
	}

    // Set state to render the image
    m_pd3dDevice->SetTexture( 0, m_pTexture );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
    m_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
    m_pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP );
    m_pd3dDevice->SetRenderState( D3DRS_ZENABLE,      FALSE );
    m_pd3dDevice->SetRenderState( D3DRS_FOGENABLE,    FALSE );
    m_pd3dDevice->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
    m_pd3dDevice->SetRenderState( D3DRS_FILLMODE,     D3DFILL_SOLID );
    m_pd3dDevice->SetRenderState( D3DRS_CULLMODE,     D3DCULL_CCW );
    m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
    m_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
    m_pd3dDevice->SetVertexShader( FVF_VERTEX );

    // Render the image
    m_pd3dDevice->SetStreamSource( 0, m_pVB, sizeof(VERTEX) );
    m_pd3dDevice->DrawPrimitive( D3DPT_QUADLIST, 0, 1 );
    m_pd3dDevice->SetTexture( 0, NULL );

    return S_OK;
}

//-----------------------------------------------------------------------------
// Name: Render(float x, float y)
// Desc: Renders the image at a given position.
//-----------------------------------------------------------------------------

HRESULT CPanel::Render(float x, float y, bool bLogical)
{

	FLOAT fWidth  = m_nWidth;
	FLOAT fHeight = m_nHeight;

	if ( m_pd3dDevice == NULL )
		return S_OK;

	if (bLogical)
	{
		g_graphicsContext.Correct(x,y,fWidth,fHeight);
		g_graphicsContext.Offset(x,y);
	}

    // Set state to render the image
	CPanel::VERTEX* vertex;
    m_pVB->Lock( 0, 0, (BYTE**)&vertex, 0L );

	vertex[0].p = D3DXVECTOR4( x - 0.5f,		y - 0.5f,			0, 0 );
	vertex[0].tu = 0;
	vertex[0].tv = 0;

    vertex[1].p = D3DXVECTOR4( x+fWidth - 0.5f,	y - 0.5f,			0, 0 );
	vertex[1].tu = m_nWidth;
	vertex[1].tv = 0;

    vertex[2].p = D3DXVECTOR4( x+fWidth - 0.5f,	y+fHeight - 0.5f,	0, 0 );
	vertex[2].tu = m_nWidth;
	vertex[2].tv = m_nHeight;

    vertex[3].p = D3DXVECTOR4( x - 0.5f,		y+fHeight - 0.5f,	0, 0 );
	vertex[3].tu = 0;
	vertex[3].tv = m_nHeight;

    m_pVB->Unlock();

	//SetAlpha( 0x80 ) ;

    // Set state to render the image
    m_pd3dDevice->SetTexture( 0, m_pTexture );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
    m_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
    m_pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP );
    m_pd3dDevice->SetRenderState( D3DRS_ZENABLE,      FALSE );
    m_pd3dDevice->SetRenderState( D3DRS_FOGENABLE,    FALSE );
    m_pd3dDevice->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
    m_pd3dDevice->SetRenderState( D3DRS_FILLMODE,     D3DFILL_SOLID );
    m_pd3dDevice->SetRenderState( D3DRS_CULLMODE,     D3DCULL_CCW );
    m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
    m_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
    m_pd3dDevice->SetVertexShader( FVF_VERTEX );

    // Render the image
    m_pd3dDevice->SetStreamSource( 0, m_pVB, sizeof(VERTEX) );
    m_pd3dDevice->DrawPrimitive( D3DPT_QUADLIST, 0, 1 );
    m_pd3dDevice->SetTexture( 0, NULL );

    return S_OK;
}


//-----------------------------------------------------------------------------
// Name: Render(float x, float y, float x2, float y2, width, height)
// Desc: Renders a portion of an image defined by x,y and w and h at a
//       given position x2,y2.
//-----------------------------------------------------------------------------

HRESULT CPanel::Render(float x, float y, float w, float h, float x2, float y2, bool bLogical)
{
	float w2 = w;
	float h2 = h;

	if ( m_pd3dDevice == NULL )
		return S_OK;
	if (bLogical)
	{
		g_graphicsContext.Correct(x2,y2,w2,h2);
		g_graphicsContext.Offset(x2,y2);
	}

    // Set state to render the image
	CPanel::VERTEX* vertex;
    m_pVB->Lock( 0, 0, (BYTE**)&vertex, 0L );
    vertex[0].p = D3DXVECTOR4(   x2 - 0.5f,   y2 - 0.5f, 0, 0 );  vertex[0].tu = x;		vertex[0].tv = y;
    vertex[1].p = D3DXVECTOR4( x2+w2- 0.5f,   y2 - 0.5f, 0, 0 );  vertex[1].tu = x+w;	vertex[1].tv = y;
    vertex[2].p = D3DXVECTOR4( x2+w2- 0.5f, y2+h2- 0.5f, 0, 0 );  vertex[2].tu = x+w;	vertex[2].tv = y+h;
    vertex[3].p = D3DXVECTOR4(   x2 - 0.5f, y2+h2- 0.5f, 0, 0 );  vertex[3].tu = x;		vertex[3].tv = y+h;
    m_pVB->Unlock();

	//SetAlpha( 0x80 ) ;

    // Set state to render the image
    m_pd3dDevice->SetTexture( 0, m_pTexture );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
    m_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
    m_pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP );
    m_pd3dDevice->SetRenderState( D3DRS_ZENABLE,      FALSE );
    m_pd3dDevice->SetRenderState( D3DRS_FOGENABLE,    FALSE );
    m_pd3dDevice->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
    m_pd3dDevice->SetRenderState( D3DRS_FILLMODE,     D3DFILL_SOLID );
    m_pd3dDevice->SetRenderState( D3DRS_CULLMODE,     D3DCULL_CCW );
    m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
    m_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
    m_pd3dDevice->SetVertexShader(  FVF_VERTEX );

    // Render the image
    m_pd3dDevice->SetStreamSource( 0, m_pVB, sizeof(VERTEX) );
    m_pd3dDevice->DrawPrimitive( D3DPT_QUADLIST, 0, 1 );
    m_pd3dDevice->SetTexture( 0, NULL );

    return S_OK;
}


HRESULT CPanel::Render(float x, float y, float w, float h, float x2, float y2, float w2, float h2)
{
	//float w2 = w;
	//float h2 = h;
	static int trtr = 0 ;

	if ( m_pd3dDevice == NULL )
		return S_OK+1;

	if ( m_pTexture == NULL )
	{
		if ( m_pFileBuf == NULL )
			return S_OK+1 ;
		else
		{
			if (D3DXCreateTextureFromFileInMemoryEx(m_pd3dDevice, m_pFileBuf, m_nFileBufSize,
				 m_nWidth, m_nHeight, 1, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED,
				 //D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED,
				 D3DX_FILTER_NONE , D3DX_FILTER_NONE, 0, NULL, NULL, &m_pTexture)!=D3D_OK)
			{
				return S_OK+1 ;
			}
		}
	}



    // Set state to render the image
	CPanel::VERTEX* vertex;
    m_pVB->Lock( 0, 0, (BYTE**)&vertex, 0L );
    vertex[0].p = D3DXVECTOR4(   x2 - 0.5f,   y2 - 0.5f, 0, 0 );  vertex[0].tu = x;		vertex[0].tv = y;
    vertex[1].p = D3DXVECTOR4( x2+w2- 0.5f,   y2 - 0.5f, 0, 0 );  vertex[1].tu = x+w;	vertex[1].tv = y;
    vertex[2].p = D3DXVECTOR4( x2+w2- 0.5f, y2+h2- 0.5f, 0, 0 );  vertex[2].tu = x+w;	vertex[2].tv = y+h;
    vertex[3].p = D3DXVECTOR4(   x2 - 0.5f, y2+h2- 0.5f, 0, 0 );  vertex[3].tu = x;		vertex[3].tv = y+h;
    m_pVB->Unlock();


    // Set state to render the image
    m_pd3dDevice->SetTexture( 0, m_pTexture );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
    m_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
    m_pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP );
    m_pd3dDevice->SetRenderState( D3DRS_ZENABLE,      FALSE );
    m_pd3dDevice->SetRenderState( D3DRS_FOGENABLE,    FALSE );
    m_pd3dDevice->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
    m_pd3dDevice->SetRenderState( D3DRS_FILLMODE,     D3DFILL_SOLID );
    m_pd3dDevice->SetRenderState( D3DRS_CULLMODE,     D3DCULL_CCW );
    m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
    m_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
    m_pd3dDevice->SetVertexShader(  FVF_VERTEX );

    //D3DVIEWPORT8 vpBackBuffer = { 0, 0, 1920, 1080, 0.0f, 1.0f };
    //m_pd3dDevice->SetRenderTarget( pBackBuffer, pZBuffer );
    //m_pd3dDevice->SetViewport( &vpBackBuffer );


    // Render the image
    m_pd3dDevice->SetStreamSource( 0, m_pVB, sizeof(VERTEX) );
    m_pd3dDevice->DrawPrimitive( D3DPT_QUADLIST, 0, 1 );


    //LPDIRECT3DSURFACE8 pBackBuffer, pZBuffer;
    //m_pd3dDevice->GetRenderTarget( &pBackBuffer );
    //m_pd3dDevice->GetDepthStencilSurface( &pZBuffer );
    //pBackBuffer->Release();
    //pZBuffer->Release();

	m_pd3dDevice->SetTexture( 0, NULL );


    return S_OK;
}


HRESULT CPanel::RenderOnCube(float x, float y, float w, float h, float x2, float y2, float w2, float h2, int face, float rx, float ry, float rz)
{
	static int trtr = 0 ;

	if ( ( m_pd3dDevice == NULL ) || ( m_pTexture == NULL ) )
		return S_OK;

    // Set state to render the image
	CPanel::VERTEX* vertex;
    m_pVB->Lock( 0, 0, (BYTE**)&vertex, 0L );

	switch ( face )
	{
		case 0 : //front
		{
			vertex[0].p = D3DXVECTOR4(   x2 - 0.5f,   y2 - 0.5f, 0, 0 );  vertex[0].tu = x;		vertex[0].tv = y;
			vertex[1].p = D3DXVECTOR4( x2+w2- 0.5f,   y2 - 0.5f, 0, 0 );  vertex[1].tu = x+w;	vertex[1].tv = y;
			vertex[2].p = D3DXVECTOR4( x2+w2- 0.5f, y2+h2- 0.5f, 0, 0 );  vertex[2].tu = x+w;	vertex[2].tv = y+h;
			vertex[3].p = D3DXVECTOR4(   x2 - 0.5f, y2+h2- 0.5f, 0, 0 );  vertex[3].tu = x;		vertex[3].tv = y+h;
			break ;
		}
		case 1 : //back
		{
			vertex[0].p = D3DXVECTOR4(   x2 - 0.5f,   y2 - 0.5f, w2 - 0.5f, 0 );  vertex[0].tu = x;		vertex[0].tv = y;
			vertex[1].p = D3DXVECTOR4( x2+w2- 0.5f,   y2 - 0.5f, w2 - 0.5f, 0 );  vertex[1].tu = x+w;	vertex[1].tv = y;
			vertex[2].p = D3DXVECTOR4( x2+w2- 0.5f, y2+h2- 0.5f, w2 - 0.5f, 0 );  vertex[2].tu = x+w;	vertex[2].tv = y+h;
			vertex[3].p = D3DXVECTOR4(   x2 - 0.5f, y2+h2- 0.5f, w2 - 0.5f, 0 );  vertex[3].tu = x;		vertex[3].tv = y+h;
			break ;
		}
		case 3 : //misc
		{
			vertex[0].p = D3DXVECTOR4( x2+(w2/2.0f)- 0.5f,   y2 - 0.5f, 0, 0 );  vertex[0].tu = x+(w/2.0f);	vertex[0].tv = y;
			vertex[1].p = D3DXVECTOR4( x2+w2- 0.5f,   y2 - 0.5f, w2/2.0f - 0.5f, 0 );  vertex[1].tu = x+w;	vertex[1].tv = y;
			vertex[2].p = D3DXVECTOR4( x2+w2- 0.5f,   y2+h2 - 0.5f, w2/2.0f - 0.5f, 0 );  vertex[2].tu = x+w;	vertex[2].tv = y+h;
			vertex[3].p = D3DXVECTOR4( x2+(w2/2.0f)- 0.5f,   y2+h2 - 0.5f, 0, 0 );  vertex[3].tu = x+(w/2.0f);	vertex[3].tv = y+h;
			break ;
		}
		case 2 : //right
		default :
		{
			vertex[0].p = D3DXVECTOR4( x2+w2- 0.5f,   y2 - 0.5f, 0, 0 );  vertex[0].tu = x+w;	vertex[0].tv = y;
			vertex[1].p = D3DXVECTOR4( x2+w2- 0.5f,   y2 - 0.5f, w2 - 0.5f, 0 );  vertex[1].tu = x+w;	vertex[1].tv = y;
			vertex[2].p = D3DXVECTOR4( x2+w2- 0.5f,   y2+h2 - 0.5f, w2 - 0.5f, 0 );  vertex[2].tu = x+w;	vertex[2].tv = y+h;
			vertex[3].p = D3DXVECTOR4( x2+w2- 0.5f,   y2+h2 - 0.5f, 0, 0 );  vertex[3].tu = x+w;	vertex[3].tv = y+h;
			break ;
		}
	}

    m_pVB->Unlock();


    // Set state to render the image
    m_pd3dDevice->SetTexture( 0, m_pTexture );




    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
    m_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
    m_pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP );
    m_pd3dDevice->SetRenderState( D3DRS_ZENABLE,      TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_FOGENABLE,    FALSE );
    m_pd3dDevice->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
    m_pd3dDevice->SetRenderState( D3DRS_FILLMODE,     D3DFILL_SOLID );
    m_pd3dDevice->SetRenderState( D3DRS_CULLMODE,     D3DCULL_CCW );
    m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
    m_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );



	SetProjection(45.0f, 1.0f, 1000.0f, w2, h2);

	SetView( 0.0f, 0.0f, -1.0f,	// The position of the camera along x, y and z axis
				 5.0f, 5.0f, 0.0f,	// This sets the position that the camera is looking at
				 0.0f, 1.0f,   0.0f);	// This sets the rotation of the camera.

	ResetWorld();					// Reset/Clear (Whatever ya wanna call it) the matrix
	RotateWorld(rx,ry,rz);	// Rotate cube

	m_pd3dDevice->SetVertexShader(  FVF_VERTEX );

    // Render the image
    m_pd3dDevice->SetStreamSource( 0, m_pVB, sizeof(VERTEX) );
    m_pd3dDevice->DrawPrimitive( D3DPT_QUADLIST, 0, 1 );
	m_pd3dDevice->SetTexture( 0, NULL );

    return S_OK;
}


HRESULT CPanel::RenderOnCube2(float x, float y, float w, float h, float x2, float y2, float w2, float h2, int face, float rx, float ry, float rz, float scrw, float scrh, int alpha)
{
	static int trtr = 0 ;

	if ( m_pd3dDevice == NULL )
		return S_OK+1;

	if ( m_pTexture == NULL )
	{
		if ( m_pFileBuf == NULL )
			return S_OK+1 ;
		else
		{
			if (D3DXCreateTextureFromFileInMemoryEx(m_pd3dDevice, m_pFileBuf, m_nFileBufSize,
				 m_nWidth, m_nHeight, 1, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED,
				 D3DX_FILTER_NONE , D3DX_FILTER_NONE, 0, NULL, NULL, &m_pTexture)!=D3D_OK)
			{
				return S_OK+1 ;
			}
		}
	}

	m_pd3dDevice->SetRenderState( D3DRS_LIGHTING, FALSE );			// Turn on lighting
	m_pd3dDevice->SetRenderState( D3DRS_ZENABLE, TRUE );			// Enable depth testing
	m_pd3dDevice->SetRenderState( D3DRS_CULLMODE, D3DCULL_NONE );	// Turn off culling
    m_pd3dDevice->SetTexture( 0, m_pTexture );

	m_pd3dDevice->SetTextureStageState(0, D3DTSS_COLOROP, D3DTOP_SELECTARG1);
	m_pd3dDevice->SetTextureStageState(0,D3DTSS_COLORARG1, D3DTA_TEXTURE);

	SetProjection(90.0f, 1.0f, 1000.0f, w2,h2);

	SetView( 0.0f, 0.0f, -8.0f - (scrw/scrh),	// The position of the camera along x, y and z axis
				 0.0f, 0.0f,   10.0f,	// This sets the position that the camera is looking at
				 0.0f, 1.0f,   0.0f);	// This sets the rotation of the camera.


	SetupVertices( x,y,w,h, x2, y2, w2, h2, scrw, scrh, alpha ) ;

	ResetWorld();					// Reset/Clear (Whatever ya wanna call it) the matrix

	if ( (int)scrw == 640 )
		ScaleWorld( 4.0, 4.0, 4.0 ) ;
	else
		ScaleWorld( 3.52, 3.52, 3.52 ) ;

	RotateWorld( rx, ry, rz ) ;

	m_pd3dDevice->SetVertexShader( D3DFVF_sVertex );	// Set vertex shader

	m_pd3dDevice->SetStreamSource( 0, g_pVBCube, sizeof(sVertex) );

	if ( face & 0x01 )
		m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLEFAN,  0, 2 );
	if ( face & 0x02 )
		m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLEFAN,  4, 2 );
	if ( face & 0x04 )
		m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLEFAN,  8, 2 );
	if ( face & 0x08 )
		m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLEFAN, 12, 2 );
	if ( face & 0x10 )
		m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLEFAN, 16, 2 );
	if ( face & 0x20 )
		m_pd3dDevice->DrawPrimitive( D3DPT_TRIANGLEFAN, 20, 2 );

	m_pd3dDevice->SetTexture( 0, NULL );

    return S_OK;
}


BOOL CPanel::IsValid()
{
	return (m_pTexture != NULL);
}

// This is called from WinMain() just after the window and DirectX object have been created.
bool CPanel::SetupVertices( float x, float y, float w, float h, float x2, float y2, float w2, float h2, float scrw, float scrh, int alpha)
{


// <**************************************** NEW ********************************************>
	// Setup the values that will be contained within the vertex buffer for a cube

	// This is almost identical to the previous tutorial for setting up the vertices of a cube
	// With just one addition per vertex.
	// This addition is the setting of the texture coordinates.
	// Above I said tu is for the X axis and tv is for the Y axis, now please let me explain
	// what I mean by this in more detail...
/*

	Let's say we are drawing a quad which is made up of four vertices....

	2----------4	Now say we want DirectX position the texture onto this quad, filling it
	| .        |	completely. For the vertices on the left hand side, tu ( x axis) would be
	|   .      |	 set to 0 and the vertices on the right, tu ( x axis ) set at 1.
	|     .    |	For the top vertices, tv ( y axis ) is set to 0, the bottom, tv set to 1.
	|       .  |	You'd think that for the Y axis texture coords would be set the other way
	|         .|	around, but they're not, this is because DirectX flips these. If you did put
	1----------3	the tv values the other way around, the texture image would be flipped upside
					down.

					What if we wanted to tile the texture though, so that DirectX halves the size
					of it and repeats it like a chess board?
					Simple! Just double the texture values from 1 to 2 :)
					Also, if you only want a portion of the texture to be displayed, you'd
					use texture coordinates more than 0 and less than 1.
					For example, if tu (X axis) for the left vertices was   0.25 and if...
									tu (X axis) for the right vertices was  0.75 and if...
									tv (Y axis) for the top vertices was    0.25 and if...
									tv (Y axis) for the bottom vertices was 0.75 then...
					only the centre of the texture would be stretched onto the entire quad.
					Hard to explain, but kind of like trimming the edges off the texture, then
					taking that texture and stretching it over the whole quad.
		I hope that makes sence! If it doesn't and anyone posts a question about this in the
		forum, then I'll update this section again :)
*/
	float minx, miny, maxx, maxy, minz, maxz ;
	DWORD color = ( alpha << 24 ) | 0xFFFFFF ;


	minx = ((2.0f*x2/scrw)-1.0f)*scrw/scrh ;
	maxx = ((2.0f*(x2+w2)/scrw)-1.0f)*scrw/scrh ;

	maxy = -((2.0f*y2/scrh)-1.0f) ;
	miny = -((2.0f*(y2+h2)/scrh)-1.0f) ;

	minz = -1.0f*(scrw/scrh) ;
	maxz = minz + w2*-2.0*minz/scrw ;

	// Front  (White) (Half sized textures, tiled twice)
	sCube[0].position = D3DXVECTOR3( minx, miny , minz );	// Bottom left vertex position
	sCube[0].diffuse = color;			// Bottom left vertex colour
	//sCube[0].normal = D3DXVECTOR3( 0,0,-1);					// Forward pointing normal
	sCube[0].tu=x;	sCube[0].tv=h;							// * NEW * Texture coordinates
	sCube[1].position = D3DXVECTOR3( minx, maxy, minz );	// Top left vertex position
	sCube[1].diffuse = color;			// Top left vertex colour
	//sCube[1].normal = D3DXVECTOR3( 0,0,-1);					// Forward pointing normal
	sCube[1].tu=x;	sCube[1].tv=y;							// * NEW * Texture coordinates
	sCube[2].position = D3DXVECTOR3( maxx, maxy, minz );	// Top right vertex position
	sCube[2].diffuse = color;			// Top right vertex colour
	//sCube[2].normal = D3DXVECTOR3( 0,0,-1);					// Forward pointing normal
	sCube[2].tu=w;	sCube[2].tv=y;							// * NEW * Texture coordinates
	sCube[3].position = D3DXVECTOR3( maxx, miny, minz );	// Bottom right vertex position
	sCube[3].diffuse = color;			// Bottom right vertex colour
	//sCube[3].normal = D3DXVECTOR3( 0,0,-1);					// Forward pointing normal
	sCube[3].tu=w;	sCube[3].tv=h;							// * NEW * Texture coordinates



	// Back (White)
	sCube[4].position = D3DXVECTOR3( minx, miny, maxz );	// Bottom left vertex position
	sCube[4].diffuse = color;			// Bottom left vertex colour
	//sCube[4].normal = D3DXVECTOR3( 0,0,1);					// Forward pointing normal
	sCube[4].tu=w;	sCube[4].tv=h;							// * NEW * Texture coordinates
	sCube[5].position = D3DXVECTOR3( minx, maxy, maxz );	// Top left vertex position
	sCube[5].diffuse = color;			// Top left vertex colour
	//sCube[5].normal = D3DXVECTOR3( 0,0,1);					// Forward pointing normal
	sCube[5].tu=w;	sCube[5].tv=y;							// * NEW * Texture coordinates
	sCube[6].position = D3DXVECTOR3(  maxx, maxy, maxz );	// Top right vertex position
	sCube[6].diffuse = color;			// Top right vertex colour
	//sCube[6].normal = D3DXVECTOR3( 0,0,1);					// Forward pointing normal
	sCube[6].tu=x;	sCube[6].tv=y;							// * NEW * Texture coordinates
	sCube[7].position = D3DXVECTOR3(  maxx, miny, maxz );	// Bottom right vertex position
	sCube[7].diffuse = color;			// Bottom right vertex colour
	//sCube[7].normal = D3DXVECTOR3( 0,0,1);					// Forward pointing normal
	sCube[7].tu=x;	sCube[7].tv=h;							// * NEW * Texture coordinates

	// Left (White)
	sCube[8].position = D3DXVECTOR3( minx, miny, minz );	// Bottom left vertex position
	sCube[8].diffuse = color;			// Bottom left vertex colour)
	//sCube[8].normal = D3DXVECTOR3( -1,0,0);					// Forward pointing normal
	sCube[8].tu=w;	sCube[8].tv=h;							// * NEW * Texture coordinates
	sCube[9].position = D3DXVECTOR3( minx, maxy, minz );	// Top left vertex position
	sCube[9].diffuse = color;			// Top left vertex colour
	//sCube[9].normal = D3DXVECTOR3( -1,0,0);					// Forward pointing normal
	sCube[9].tu=w;	sCube[9].tv=y;							// * NEW * Texture coordinates
	sCube[10].position = D3DXVECTOR3( minx, maxy, maxz );	// Top right vertex position
	sCube[10].diffuse = color;			// Top right vertex colour
	//sCube[10].normal = D3DXVECTOR3( -1,0,0);				// Forward pointing normal
	sCube[10].tu=x;	sCube[10].tv=y;							// * NEW * Texture coordinates
	sCube[11].position = D3DXVECTOR3( minx, miny, maxz );	// Bottom right vertex position
	sCube[11].diffuse = color;			// Bottom right vertex colour
	//sCube[11].normal = D3DXVECTOR3( -1,0,0);				// Forward pointing normal
	sCube[11].tu=x;	sCube[11].tv=h;							// * NEW * Texture coordinates

	// Right (White)
	sCube[12].position = D3DXVECTOR3( maxx, miny, minz );	// Bottom left vertex position
	sCube[12].diffuse = color;			// Bottom left vertex colour
	//sCube[12].normal = D3DXVECTOR3( 1,0,0);					// Forward pointing normal
	sCube[12].tu=x;	sCube[12].tv=h;							// * NEW * Texture coordinates
	sCube[13].position = D3DXVECTOR3( maxx, maxy, minz );	// Top left vertex position
	sCube[13].diffuse = color;			// Top left vertex colour
	//sCube[13].normal = D3DXVECTOR3( 1,0,0);					// Forward pointing normal
	sCube[13].tu=x;	sCube[13].tv=y;							// * NEW * Texture coordinates
	sCube[14].position = D3DXVECTOR3( maxx, maxy, maxz );	// Top right vertex position
	sCube[14].diffuse = color;			// Top right vertex colour
	//sCube[14].normal = D3DXVECTOR3( 1,0,0);					// Forward pointing normal
	sCube[14].tu=w;	sCube[14].tv=y;							// * NEW * Texture coordinates
	sCube[15].position = D3DXVECTOR3( maxx, miny, maxz );	// Bottom right vertex position
	sCube[15].diffuse = color;			// Bottom right vertex colour
	//sCube[15].normal = D3DXVECTOR3( 1,0,0);					// Forward pointing normal
	sCube[15].tu=w;	sCube[15].tv=h;							// * NEW * Texture coordinates

	// Top (White)
	sCube[16].position = D3DXVECTOR3( minx, maxy, minz );	// Bottom left vertex position
	sCube[16].diffuse = color;			// Bottom left vertex colour
	//sCube[16].normal = D3DXVECTOR3( 0,1,0);					// Forward pointing normal
	sCube[16].tu=x;	sCube[16].tv=h;							// * NEW * Texture coordinates
	sCube[17].position = D3DXVECTOR3( minx, maxy, maxz );	// Top left vertex position
	sCube[17].diffuse = color;			// Top left vertex colour
	//sCube[17].normal = D3DXVECTOR3( 0,1,0);					// Forward pointing normal
	sCube[17].tu=w;	sCube[17].tv=h;							// * NEW * Texture coordinates
	sCube[18].position = D3DXVECTOR3( maxx, maxy, maxz );	// Top right vertex position
	sCube[18].diffuse = color;			// Top right vertex colour
	//sCube[18].normal = D3DXVECTOR3( 0,1,0);					// Forward pointing normal
	sCube[18].tu=w;	sCube[18].tv=y;							// * NEW * Texture coordinates
	sCube[19].position = D3DXVECTOR3( maxx, maxy, minz );	// Bottom right vertex position
	sCube[19].diffuse = color;			// Bottom right vertex colour
	//sCube[19].normal = D3DXVECTOR3( 0,1,0);					// Forward pointing normal
	sCube[19].tu=x;	sCube[19].tv=y;							// * NEW * Texture coordinates

/*
	// Top (White)
	sCube[16].position = D3DXVECTOR3( minx, maxy, minz );	// Bottom left vertex position
	sCube[16].diffuse = color;			// Bottom left vertex colour
	//sCube[16].normal = D3DXVECTOR3( 0,1,0);					// Forward pointing normal
	sCube[16].tu=x;	sCube[16].tv=h;							// * NEW * Texture coordinates
	sCube[17].position = D3DXVECTOR3( maxx, maxy, minz );	// Top left vertex position
	sCube[17].diffuse = color;			// Top left vertex colour
	//sCube[17].normal = D3DXVECTOR3( 0,1,0);					// Forward pointing normal
	sCube[17].tu=w;	sCube[17].tv=h;							// * NEW * Texture coordinates
	sCube[18].position = D3DXVECTOR3( maxx, maxy, 1.0f );	// Top right vertex position
	sCube[18].diffuse = color;			// Top right vertex colour
	//sCube[18].normal = D3DXVECTOR3( 0,1,0);					// Forward pointing normal
	sCube[18].tu=w;	sCube[18].tv=y;							// * NEW * Texture coordinates
	sCube[19].position = D3DXVECTOR3( minx, maxy, 1.0f );	// Bottom right vertex position
	sCube[19].diffuse = color;			// Bottom right vertex colour
	//sCube[19].normal = D3DXVECTOR3( 0,1,0);					// Forward pointing normal
	sCube[19].tu=x;	sCube[19].tv=y;							// * NEW * Texture coordinates
*/

	// Bottom (Multi coloured)
	sCube[20].position = D3DXVECTOR3( minx, miny, minz );	// Bottom left vertex position
	sCube[20].diffuse = color;				// Bottom left vertex colour (Red)
	//sCube[20].normal = D3DXVECTOR3( 0,-1,0);				// Forward pointing normal
	sCube[20].tu=x;	sCube[20].tv=y;							// * NEW * Texture coordinates
	sCube[21].position = D3DXVECTOR3( maxx, miny, minz );	// Top left vertex position
	sCube[21].diffuse = color;				// Top left vertex colour (Green)
	//sCube[21].normal = D3DXVECTOR3( 0,-1,0);				// Forward pointing normal
	sCube[21].tu=w;	sCube[21].tv=y;							// * NEW * Texture coordinates
	sCube[22].position = D3DXVECTOR3( maxx, miny, maxz );	// Top right vertex position
	sCube[22].diffuse = color;				// Top right vertex colour (Blue)
	//sCube[22].normal = D3DXVECTOR3( 0,-1,0);				// Forward pointing normal
	sCube[22].tu=w;	sCube[22].tv=h;							// * NEW * Texture coordinates
	sCube[23].position = D3DXVECTOR3( minx, miny, maxz );	// Bottom right vertex position
	sCube[23].diffuse = color;				// Bottom right vertex colour (Blue)
	//sCube[23].normal = D3DXVECTOR3( 0,-1,0);				// Forward pointing normal
	sCube[23].tu=x;	sCube[23].tv=h;							// * NEW * Texture coordinates
// <**************************************** NEW ********************************************>


	// Copy data to vertex buffer
	VOID* pVertices;				// Temp memory to work with

	if ( g_pVBCube == NULL )
	{
		return false ;
	}

	// Lock vertex buffer
	if( FAILED( g_pVBCube->Lock( 0,	sizeof(sCube), (BYTE**)&pVertices, 0 ) ) )
	{
		return false;	// If something went wrong, say so!
	}

	// Copy the data from the array into the buffer
	memcpy( pVertices, sCube, sizeof(sCube) );

	// Unlock vertex buffer
	g_pVBCube->Unlock();


	return true;
}


// This is called from WinMain() just after the window and DirectX object have been created.
bool CPanel::InitVertices()
{

	// Create an empty vertex buffer to hold the triangles information.
	if ( g_pVBCube == NULL )
		if( FAILED( m_pd3dDevice->CreateVertexBuffer( 24*sizeof(sVertex), 0, D3DFVF_sVertex, D3DPOOL_MANAGED, &g_pVBCube ) ) )
			return false;	// If anything went wrong, quit.


// <**************************************** NEW ********************************************>
	// Setup the values that will be contained within the vertex buffer for a cube

	// This is almost identical to the previous tutorial for setting up the vertices of a cube
	// With just one addition per vertex.
	// This addition is the setting of the texture coordinates.
	// Above I said tu is for the X axis and tv is for the Y axis, now please let me explain
	// what I mean by this in more detail...
/*

	Let's say we are drawing a quad which is made up of four vertices....

	2----------4	Now say we want DirectX position the texture onto this quad, filling it
	| .        |	completely. For the vertices on the left hand side, tu ( x axis) would be
	|   .      |	 set to 0 and the vertices on the right, tu ( x axis ) set at 1.
	|     .    |	For the top vertices, tv ( y axis ) is set to 0, the bottom, tv set to 1.
	|       .  |	You'd think that for the Y axis texture coords would be set the other way
	|         .|	around, but they're not, this is because DirectX flips these. If you did put
	1----------3	the tv values the other way around, the texture image would be flipped upside
					down.

					What if we wanted to tile the texture though, so that DirectX halves the size
					of it and repeats it like a chess board?
					Simple! Just double the texture values from 1 to 2 :)
					Also, if you only want a portion of the texture to be displayed, you'd
					use texture coordinates more than 0 and less than 1.
					For example, if tu (X axis) for the left vertices was   0.25 and if...
									tu (X axis) for the right vertices was  0.75 and if...
									tv (Y axis) for the top vertices was    0.25 and if...
									tv (Y axis) for the bottom vertices was 0.75 then...
					only the centre of the texture would be stretched onto the entire quad.
					Hard to explain, but kind of like trimming the edges off the texture, then
					taking that texture and stretching it over the whole quad.
		I hope that makes sence! If it doesn't and anyone posts a question about this in the
		forum, then I'll update this section again :)
*/
	DWORD color = 0xFFFFFFFF ;

	// Front  (White) (Half sized textures, tiled twice)
	sCube[0].position = D3DXVECTOR3( -1.0f, -1.0f,-1.0f );	// Bottom left vertex position
	sCube[0].diffuse = color;			// Bottom left vertex colour
	//sCube[0].normal = D3DXVECTOR3( 0,0,-1);					// Forward pointing normal
	sCube[0].tu=0;	sCube[0].tv=480;							// * NEW * Texture coordinates
	sCube[1].position = D3DXVECTOR3( -1.0f,  1.0f,-1.0f );	// Top left vertex position
	sCube[1].diffuse = color;			// Top left vertex colour
	//sCube[1].normal = D3DXVECTOR3( 0,0,-1);					// Forward pointing normal
	sCube[1].tu=0;	sCube[1].tv=0;							// * NEW * Texture coordinates
	sCube[2].position = D3DXVECTOR3(  1.0f,  1.0f,-1.0f );	// Top right vertex position
	sCube[2].diffuse = color;			// Top right vertex colour
	//sCube[2].normal = D3DXVECTOR3( 0,0,-1);					// Forward pointing normal
	sCube[2].tu=640;	sCube[2].tv=0;							// * NEW * Texture coordinates
	sCube[3].position = D3DXVECTOR3(  1.0f, -1.0f,-1.0f );	// Bottom right vertex position
	sCube[3].diffuse = color;			// Bottom right vertex colour
	//sCube[3].normal = D3DXVECTOR3( 0,0,-1);					// Forward pointing normal
	sCube[3].tu=640;	sCube[3].tv=480;							// * NEW * Texture coordinates

	// Back (White)
	sCube[4].position = D3DXVECTOR3( -1.0f, -1.0f, 1.0f );	// Bottom left vertex position
	sCube[4].diffuse = color;			// Bottom left vertex colour
	//sCube[4].normal = D3DXVECTOR3( 0,0,1);					// Forward pointing normal
	sCube[4].tu=0;	sCube[4].tv=1;							// * NEW * Texture coordinates
	sCube[5].position = D3DXVECTOR3( -1.0f,  1.0f, 1.0f );	// Top left vertex position
	sCube[5].diffuse = color;			// Top left vertex colour
	//sCube[5].normal = D3DXVECTOR3( 0,0,1);					// Forward pointing normal
	sCube[5].tu=0;	sCube[5].tv=0;							// * NEW * Texture coordinates
	sCube[6].position = D3DXVECTOR3(  1.0f,  1.0f, 1.0f );	// Top right vertex position
	sCube[6].diffuse = color;			// Top right vertex colour
	//sCube[6].normal = D3DXVECTOR3( 0,0,1);					// Forward pointing normal
	sCube[6].tu=1;	sCube[6].tv=0;							// * NEW * Texture coordinates
	sCube[7].position = D3DXVECTOR3(  1.0f, -1.0f, 1.0f );	// Bottom right vertex position
	sCube[7].diffuse = color;			// Bottom right vertex colour
	//sCube[7].normal = D3DXVECTOR3( 0,0,1);					// Forward pointing normal
	sCube[7].tu=1;	sCube[7].tv=1;							// * NEW * Texture coordinates

	// Left (White)
	sCube[8].position = D3DXVECTOR3( -1.0f, -1.0f, -1.0f );	// Bottom left vertex position
	sCube[8].diffuse = color;			// Bottom left vertex colour)
	//sCube[8].normal = D3DXVECTOR3( -1,0,0);					// Forward pointing normal
	sCube[8].tu=0;	sCube[8].tv=1;							// * NEW * Texture coordinates
	sCube[9].position = D3DXVECTOR3( -1.0f,  1.0f, -1.0f );	// Top left vertex position
	sCube[9].diffuse = color;			// Top left vertex colour
	//sCube[9].normal = D3DXVECTOR3( -1,0,0);					// Forward pointing normal
	sCube[9].tu=0;	sCube[9].tv=0;							// * NEW * Texture coordinates
	sCube[10].position = D3DXVECTOR3( -1.0f,  1.0f, 1.0f );	// Top right vertex position
	sCube[10].diffuse = color;			// Top right vertex colour
	//sCube[10].normal = D3DXVECTOR3( -1,0,0);				// Forward pointing normal
	sCube[10].tu=0;	sCube[10].tv=0;							// * NEW * Texture coordinates
	sCube[11].position = D3DXVECTOR3( -1.0f, -1.0f, 1.0f );	// Bottom right vertex position
	sCube[11].diffuse = color;			// Bottom right vertex colour
	//sCube[11].normal = D3DXVECTOR3( -1,0,0);				// Forward pointing normal
	sCube[11].tu=0;	sCube[11].tv=1;							// * NEW * Texture coordinates

	// Right (White)
	sCube[12].position = D3DXVECTOR3( 1.0f, -1.0f, -1.0f );	// Bottom left vertex position
	sCube[12].diffuse = color;			// Bottom left vertex colour
	//sCube[12].normal = D3DXVECTOR3( 1,0,0);					// Forward pointing normal
	sCube[12].tu=1;	sCube[12].tv=1;							// * NEW * Texture coordinates
	sCube[13].position = D3DXVECTOR3( 1.0f,  1.0f, -1.0f );	// Top left vertex position
	sCube[13].diffuse = color;			// Top left vertex colour
	//sCube[13].normal = D3DXVECTOR3( 1,0,0);					// Forward pointing normal
	sCube[13].tu=1;	sCube[13].tv=0;							// * NEW * Texture coordinates
	sCube[14].position = D3DXVECTOR3( 1.0f,  1.0f, 1.0f );	// Top right vertex position
	sCube[14].diffuse = color;			// Top right vertex colour
	//sCube[14].normal = D3DXVECTOR3( 1,0,0);					// Forward pointing normal
	sCube[14].tu=0;	sCube[14].tv=0;							// * NEW * Texture coordinates
	sCube[15].position = D3DXVECTOR3( 1.0f, -1.0f, 1.0f );	// Bottom right vertex position
	sCube[15].diffuse = color;			// Bottom right vertex colour
	//sCube[15].normal = D3DXVECTOR3( 1,0,0);					// Forward pointing normal
	sCube[15].tu=1;	sCube[15].tv=0;							// * NEW * Texture coordinates

	// Top (White)
	sCube[16].position = D3DXVECTOR3(-1.0f, 1.0f, -1.0f );	// Bottom left vertex position
	sCube[16].diffuse = color;			// Bottom left vertex colour
	//sCube[16].normal = D3DXVECTOR3( 0,1,0);					// Forward pointing normal
	sCube[16].tu=1;	sCube[16].tv=1;							// * NEW * Texture coordinates
	sCube[17].position = D3DXVECTOR3( 1.0f,  1.0f, -1.0f );	// Top left vertex position
	sCube[17].diffuse = color;			// Top left vertex colour
	//sCube[17].normal = D3DXVECTOR3( 0,1,0);					// Forward pointing normal
	sCube[17].tu=0;	sCube[17].tv=1;							// * NEW * Texture coordinates
	sCube[18].position = D3DXVECTOR3( 1.0f,  1.0f, 1.0f );	// Top right vertex position
	sCube[18].diffuse = color;			// Top right vertex colour
	//sCube[18].normal = D3DXVECTOR3( 0,1,0);					// Forward pointing normal
	sCube[18].tu=0;	sCube[18].tv=0;							// * NEW * Texture coordinates
	sCube[19].position = D3DXVECTOR3(-1.0f, 1.0f, 1.0f );	// Bottom right vertex position
	sCube[19].diffuse = color;			// Bottom right vertex colour
	//sCube[19].normal = D3DXVECTOR3( 0,1,0);					// Forward pointing normal
	sCube[19].tu=1;	sCube[19].tv=0;							// * NEW * Texture coordinates

	// Bottom (Multi coloured)
	sCube[20].position = D3DXVECTOR3(-1.0f,-1.0f, -1.0f );	// Bottom left vertex position
	sCube[20].diffuse = D3DCOLOR_XRGB(255,0,1);				// Bottom left vertex colour (Red)
	//sCube[20].normal = D3DXVECTOR3( 0,-1,0);				// Forward pointing normal
	sCube[20].tu=1;	sCube[20].tv=1;							// * NEW * Texture coordinates
	sCube[21].position = D3DXVECTOR3( 1.0f, -1.0f, -1.0f );	// Top left vertex position
	sCube[21].diffuse = D3DCOLOR_XRGB(0,255,1);				// Top left vertex colour (Green)
	//sCube[21].normal = D3DXVECTOR3( 0,-1,0);				// Forward pointing normal
	sCube[21].tu=0;	sCube[21].tv=1;							// * NEW * Texture coordinates
	sCube[22].position = D3DXVECTOR3( 1.0f, -1.0f, 1.0f );	// Top right vertex position
	sCube[22].diffuse = D3DCOLOR_XRGB(0,0,255);				// Top right vertex colour (Blue)
	//sCube[22].normal = D3DXVECTOR3( 0,-1,0);				// Forward pointing normal
	sCube[22].tu=0;	sCube[22].tv=0;							// * NEW * Texture coordinates
	sCube[23].position = D3DXVECTOR3(-1.0f, -1.0f, 1.0f );	// Bottom right vertex position
	sCube[23].diffuse = D3DCOLOR_XRGB(0,0,255);				// Bottom right vertex colour (Blue)
	//sCube[23].normal = D3DXVECTOR3( 0,-1,0);				// Forward pointing normal
	sCube[23].tu=1;	sCube[23].tv=0;							// * NEW * Texture coordinates
// <**************************************** NEW ********************************************>

	// Copy data to vertex buffer
	VOID* pVertices;				// Temp memory to work with

	// Lock vertex buffer
	if( FAILED( g_pVBCube->Lock( 0,	sizeof(sCube), (BYTE**)&pVertices, 0 ) ) )
		return false;	// If something went wrong, say so!

	// Copy the data from the array into the buffer
	memcpy( pVertices, sCube, sizeof(sCube) );

	// Unlock vertex buffer
	g_pVBCube->Unlock();


	return true;
}

HRESULT CPanel::Render(float x, float y, float nw, float nh, bool bLogical)
{
	if ( m_pd3dDevice == NULL )
		return S_OK+1;

	if ( m_pTexture == NULL )
	{
		if ( m_pFileBuf == NULL )
			return S_OK+1 ;
		else
		{
			if (D3DXCreateTextureFromFileInMemoryEx(m_pd3dDevice, m_pFileBuf, m_nFileBufSize,
				 m_nWidth, m_nHeight, 1, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED,
				 //D3DX_DEFAULT, D3DX_DEFAULT, 1, 0, D3DFMT_LIN_A8R8G8B8, D3DPOOL_MANAGED,
				 D3DX_FILTER_NONE , D3DX_FILTER_NONE, 0, NULL, NULL, &m_pTexture)!=D3D_OK)
			{
				return S_OK+1 ;
			}
		}
	}

	if (bLogical)
	{
		g_graphicsContext.Correct(x,y,nw,nh);
		g_graphicsContext.Offset(x,y);
	}

    // Set state to render the image
	CPanel::VERTEX* vertex;
    m_pVB->Lock( 0, 0, (BYTE**)&vertex, 0L );


	vertex[0].p = D3DXVECTOR4( x - 0.5f,	y - 0.5f,		0, 0 );
	vertex[0].tu = 0;
	vertex[0].tv = 0;
	vertex[0].col = m_colDiffuse;

    vertex[1].p = D3DXVECTOR4( x+nw - 0.5f,	y - 0.5f,		0, 0 );
	vertex[1].tu = m_nWidth;
	vertex[1].tv = 0;
	vertex[1].col = m_colDiffuse;

    vertex[2].p = D3DXVECTOR4( x+nw - 0.5f,	y+nh - 0.5f,	0, 0 );
	vertex[2].tu = m_nWidth;
	vertex[2].tv = m_nHeight;
	vertex[2].col = m_colDiffuse;

    vertex[3].p = D3DXVECTOR4( x - 0.5f,	y+nh - 0.5f,	0, 0 );
	vertex[3].tu = 0;
	vertex[3].tv = m_nHeight;
	vertex[3].col = m_colDiffuse;

    m_pVB->Unlock();

    // Set state to render the image
    m_pd3dDevice->SetTexture( 0, m_pTexture );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLOROP,   D3DTOP_MODULATE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG1, D3DTA_TEXTURE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_COLORARG2, D3DTA_DIFFUSE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAOP,   D3DTOP_MODULATE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG1, D3DTA_TEXTURE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ALPHAARG2, D3DTA_DIFFUSE );
    m_pd3dDevice->SetTextureStageState( 1, D3DTSS_COLOROP,   D3DTOP_DISABLE );
    m_pd3dDevice->SetTextureStageState( 1, D3DTSS_ALPHAOP,   D3DTOP_DISABLE );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSU,  D3DTADDRESS_CLAMP );
    m_pd3dDevice->SetTextureStageState( 0, D3DTSS_ADDRESSV,  D3DTADDRESS_CLAMP );
    m_pd3dDevice->SetRenderState( D3DRS_ZENABLE,      FALSE );
    m_pd3dDevice->SetRenderState( D3DRS_FOGENABLE,    FALSE );
    m_pd3dDevice->SetRenderState( D3DRS_FOGTABLEMODE, D3DFOG_NONE );
    m_pd3dDevice->SetRenderState( D3DRS_FILLMODE,     D3DFILL_SOLID );
    m_pd3dDevice->SetRenderState( D3DRS_CULLMODE,     D3DCULL_CCW );
    m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, TRUE );
    m_pd3dDevice->SetRenderState( D3DRS_SRCBLEND,  D3DBLEND_SRCALPHA );
    m_pd3dDevice->SetRenderState( D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA );
    m_pd3dDevice->SetVertexShader( FVF_VERTEX );

    // Render the image
    m_pd3dDevice->SetStreamSource( 0, m_pVB, sizeof(VERTEX) );
    m_pd3dDevice->DrawPrimitive( D3DPT_QUADLIST, 0, 1 );
    m_pd3dDevice->SetTexture( 0, NULL );

    return S_OK;
}

HRESULT CPanel::SetAlpha(DWORD dwAlpha)
{
	D3DCOLOR colour = (dwAlpha << 24) | 0xFFFFFF;
	return SetColourDiffuse(colour);
}

HRESULT CPanel::SetColourDiffuse(D3DCOLOR colour)
{
	if (colour!=m_colDiffuse)
	{
		if ( m_pVB != NULL )
		{
			//TLVertex * vertex;
			CPanel::VERTEX* vertex;
			m_pVB->Lock( 0, 0, (BYTE**)&vertex, 0L );
			vertex[0].col = colour;
			vertex[1].col = colour;
			vertex[2].col = colour;
			vertex[3].col = colour;
			m_pVB->Unlock();

			m_colDiffuse = colour;
		}
	}

	return S_OK;
}


// This method's code is covered in a previous tutorial, it basically tells DirectX how
// to how the scene is displayed on the 2D screen.
void CPanel::SetProjection( float FOVdegrees, float closeClippingPlane, float farClippingPlane, int scrWidth, int scrHeight)
{
	// Set values for matrix
	D3DXMatrixPerspectiveFovLH( &matrixProjection,	// Pointer to matrix
						 D3DXToRadian(FOVdegrees),	// Field of view in degrees
						(float)scrWidth/(float)scrHeight,	// Screen aspect ratio
							   closeClippingPlane,	// Clipping plane
								farClippingPlane);	// Clipping plane
	// Set projection matrix
	m_pd3dDevice->SetTransform( D3DTS_PROJECTION, &matrixProjection );
}

// This method's code is covered in a previous tutorial.
// It sets the position and rotation of the camera
void CPanel::SetView( float Xpos, float Ypos, float Zpos,			// Cams position
			float Xtarget, float Ytarget, float Ztarget,	// Where it's looking
			float Xup, float Yup, float Zup )				// Camera's up vector (usually 0,1,0)
{
	D3DXMatrixLookAtLH( &matrixView,			// The matrix that we are setting
	&D3DXVECTOR3( Xpos, Ypos, Zpos ),			// Cams position
    &D3DXVECTOR3( Xtarget, Ytarget, Ztarget),	// Where it's looking
	&D3DXVECTOR3( Xup, Yup, Zup ));				// This sets the rotation of the camera.
	m_pd3dDevice->SetTransform( D3DTS_VIEW, &matrixView );	// Set matrix
}

// The rest of the methods are for the world view matrix.
// The world view matrix is used to position, rotate and scale whatever polygons that we
// are about to render.

// One line in this method! Am I lazy or what!
// This simpley sets the position and rotation values to zero for all 3 axis
void CPanel::ResetWorld(void)
{
	D3DXMatrixIdentity(&matrixWorld);	// Resets the world matrix
}

// This moves the matrix to a new position
void CPanel::TranslateWorld( float Xpos, float Ypos, float Zpos)
{
	// First we reset the temporary matrix
	D3DXMatrixIdentity(&matrixTemp);	// Reset temporary matrix

	// Now for a new line! D3DXMatrixTranslation()
	// It simply accepts 4 values.
	// The first is the matrix that we want to apply the translation to
	// and the next three are the values to move along X,Y & Z axis

	// Now translate the temp matrix with the X,Y & Z position
	D3DXMatrixTranslation(&matrixTemp, Xpos, Ypos, Zpos);	// Set the position

	// Finally combine the new matrix position with the previous state of the matrix
	D3DXMatrixMultiply( &matrixWorld, &matrixTemp,  &matrixWorld);

	// Now that the matrixWorld matrix stores the old world matrix values with the new
	// positional values, we simply apply the matrix and we are done.
	m_pd3dDevice->SetTransform( D3DTS_WORLD, &matrixWorld );	// Finally set the matrix
}


// This rotates the matrix
// This code is very similar to the previous tutorials rotation code
void CPanel::RotateWorld( float Xrot, float Yrot, float Zrot)		// Rotate world matrix
{
	D3DXMatrixIdentity(&matrixTemp);	// Reset temporary matrix

	// Now we use the temporary matrix to store the combined rotations of the matrix

	// Rotate the temp matrix along X
	D3DXMatrixRotationX( &matrixTemp, D3DXToRadian(Xrot) );
	// Multiply the temp matrix with the actual matrix.
	// and store the combination into the actual matrix
	D3DXMatrixMultiply( &matrixWorld, &matrixTemp, &matrixWorld);

	// Rotate the temp matrix along Y
	D3DXMatrixRotationY( &matrixTemp, D3DXToRadian(Yrot) );
	// Multiply the temp matrix with the actual matrix
	// (Which currently stores the X rotation so far)
	D3DXMatrixMultiply( &matrixWorld, &matrixTemp, &matrixWorld);

	// Rotate around the temp matrix along Z
	D3DXMatrixRotationZ( &matrixTemp, D3DXToRadian(Zrot) );
	D3DXMatrixMultiply( &matrixWorld, &matrixTemp, &matrixWorld);
	// Multiply the temp matrix with the actual matrix
	// Which currently stores the X and Y rotation

	// Now matrixTemp matrix stores the old world matrix values with the new rotation values
	// So we simply apply the matrix and we are done.
	m_pd3dDevice->SetTransform( D3DTS_WORLD, &matrixWorld );	// Finally set the matrix
}

// This rotates the matrix
// This code is very similar to the previous tutorials rotation code
void CPanel::ScaleWorld( float Xsca, float Ysca, float Zsca)		// Rotate world matrix
{
	D3DXMatrixIdentity(&matrixTemp);	// Reset temporary matrix

	// Now we use the temporary matrix to store the combined rotations of the matrix

	// Rotate the temp matrix along X
	D3DXMatrixScaling( &matrixTemp, Xsca, Ysca, Zsca );
	// Multiply the temp matrix with the actual matrix.
	// and store the combination into the actual matrix
	D3DXMatrixMultiply( &matrixWorld, &matrixTemp, &matrixWorld);

	// Now matrixTemp matrix stores the old world matrix values with the new rotation values
	// So we simply apply the matrix and we are done.
	m_pd3dDevice->SetTransform( D3DTS_WORLD, &matrixWorld );	// Finally set the matrix
}
