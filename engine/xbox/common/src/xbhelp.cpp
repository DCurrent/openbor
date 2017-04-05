//-----------------------------------------------------------------------------
// File: XBHelp.cpp
//
// Desc: Support class for rendering a help image, which is an image of an Xbox
//       gamepad, with labelled callouts to each of the gamepad's controls.
//
// Hist: 11.01.00 - New for November XDK release
//       12.15.00 - Changes for December XDK release
//       03.06.01 - Changes for April XDK release
//       04.15.01 - Using packed resources for May XDK
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include <xtl.h>
#include <xgraphics.h>
#include "XBHelp.h"
#include "XBResource.h"




//-----------------------------------------------------------------------------
// Constants for rendering callouts on the help screen. The order of these
// callouts is in agreement with the enum structure in XBHelp.h
//-----------------------------------------------------------------------------
static D3DXVECTOR2 g_vHelpCallouts[] = 
{
    // Order:
    // Button position (start of line),
    // End of line for placement one, Start of text for placement one
    // End of line for placement two, Start of text for placement two

    // Left thumbstick
    D3DXVECTOR2( 255.0f, 149.0f ),
    D3DXVECTOR2( 141.0f,  99.0f ), D3DXVECTOR2( 108.0f,  77.0f ),
    D3DXVECTOR2( 145.0f, 124.0f ), D3DXVECTOR2( 108.0f,  77.0f ),
    
    // Right thumbstick 
    D3DXVECTOR2( 370.0f, 191.0f ),
    D3DXVECTOR2( 370.0f, 363.0f ), D3DXVECTOR2( 352.0f, 364.0f ),
    D3DXVECTOR2( 370.0f, 363.0f ), D3DXVECTOR2( 352.0f, 364.0f ),

    // D-pad
    D3DXVECTOR2( 254.0f, 194.0f ),
    D3DXVECTOR2( 117.0f, 223.0f ), D3DXVECTOR2(  78.0f, 222.0f ),
    D3DXVECTOR2( 117.0f, 223.0f ), D3DXVECTOR2(  78.0f, 222.0f ),
    
    // Back button
    D3DXVECTOR2( 288.0f, 221.0f ),
    D3DXVECTOR2( 185.0f, 284.0f ), D3DXVECTOR2( 145.0f, 282.0f ),
    D3DXVECTOR2( 185.0f, 284.0f ), D3DXVECTOR2( 145.0f, 282.0f ),

    // Start button
    D3DXVECTOR2( 322.0f, 223.0f ),
    D3DXVECTOR2( 262.0f, 343.0f ), D3DXVECTOR2( 217.0f, 340.0f ),
    D3DXVECTOR2( 262.0f, 343.0f ), D3DXVECTOR2( 217.0f, 340.0f ),

    // X button
    D3DXVECTOR2( 394.0f, 176.0f ),
    D3DXVECTOR2( 295.0f, 124.0f ), D3DXVECTOR2( 268.0f, 101.0f ),
    D3DXVECTOR2( 300.0f, 149.0f ), D3DXVECTOR2( 268.0f, 101.0f ),

    // Y button
    D3DXVECTOR2( 405.0f, 164.0f ),
    D3DXVECTOR2( 416.0f,  99.0f ), D3DXVECTOR2( 404.0f, 76.0f ),
    D3DXVECTOR2( 416.0f,  99.0f ), D3DXVECTOR2( 404.0f, 51.0f ),

    // A button
    D3DXVECTOR2( 411.0f, 189.0f ),
    D3DXVECTOR2( 431.0f, 298.0f ), D3DXVECTOR2( 424.0f, 298.0f ),
    D3DXVECTOR2( 431.0f, 298.0f ), D3DXVECTOR2( 424.0f, 298.0f ),

    // B button
    D3DXVECTOR2( 422.0f, 175.0f ),
    D3DXVECTOR2( 443.0f, 244.0f ), D3DXVECTOR2( 435.0f, 243.0f ),
    D3DXVECTOR2( 443.0f, 244.0f ), D3DXVECTOR2( 435.0f, 243.0f ),

    // White button
    D3DXVECTOR2( 424.0f, 158.0f ),
    D3DXVECTOR2( 458.0f, 124.0f ), D3DXVECTOR2( 444.0f, 102.0f ),
    D3DXVECTOR2( 448.0f, 148.0f ), D3DXVECTOR2( 444.0f, 102.0f ),
    
    // Black button
    D3DXVECTOR2( 443.0f, 170.0f ),
    D3DXVECTOR2( 456.0f, 183.0f ), D3DXVECTOR2( 442.0f, 182.0f ),
    D3DXVECTOR2( 456.0f, 183.0f ), D3DXVECTOR2( 442.0f, 182.0f ),

    // Left trigger button
    D3DXVECTOR2( 230.0f, 173.0f ),
    D3DXVECTOR2( 164.0f, 160.0f ), D3DXVECTOR2( 66.0f, 150.0f ),
    D3DXVECTOR2( 164.0f, 160.0f ), D3DXVECTOR2( 66.0f, 150.0f ),

    // Right trigger button
    D3DXVECTOR2( 462.0f, 172.0f ),
    D3DXVECTOR2( 480.0f, 170.0f ), D3DXVECTOR2( 482.0f, 158.0f ),
    D3DXVECTOR2( 480.0f, 170.0f ), D3DXVECTOR2( 482.0f, 158.0f ),

    // Misc callout
    D3DXVECTOR2(  64.0f, 380.0f ),
    D3DXVECTOR2(  64.0f, 380.0f ), D3DXVECTOR2( 64.0f, 405.0f ),
    D3DXVECTOR2(  64.0f, 380.0f ), D3DXVECTOR2( 64.0f, 380.0f ),
};




//-----------------------------------------------------------------------------
// Name: CXBHelp()
// Desc: Help class constructor
//-----------------------------------------------------------------------------
CXBHelp::CXBHelp()
{
    m_pd3dDevice      = NULL;
    m_pGamepadTexture = NULL;
    m_pVB             = NULL;
}




//-----------------------------------------------------------------------------
// Name: ~CXBHelp()
// Desc: Help class destructor
//-----------------------------------------------------------------------------
CXBHelp::~CXBHelp()
{
    Destroy();
}




//-----------------------------------------------------------------------------
// Name: Create()
// Desc: Creates the help class' internal objects
//-----------------------------------------------------------------------------
HRESULT CXBHelp::Create( LPDIRECT3DDEVICE8 pd3dDevice, CHAR* strResource )
{
    // Keep track of the device
    m_pd3dDevice = pd3dDevice;

    // Create the gamepad resource
    if( FAILED( m_xprResource.Create( pd3dDevice, strResource, 1 ) ) )
        return E_FAIL;

    // Store access to the 640x480, linear gamepad texture
    m_pGamepadTexture = m_xprResource.GetTexture( 0UL );

    // Create a vertex buffer for rendering the help screen
    m_pd3dDevice->CreateVertexBuffer( 4*6*sizeof(FLOAT), D3DUSAGE_WRITEONLY, 
                                      0L, D3DPOOL_DEFAULT, &m_pVB );
    struct VERTEX { D3DXVECTOR4 p; FLOAT tu, tv; };
    VERTEX* v;
    m_pVB->Lock( 0, 0, (BYTE**)&v, 0L );
    v[0].p = D3DXVECTOR4(   0 - 0.5f,   0 - 0.5f, 0, 0 );  v[0].tu =   0; v[0].tv =   0;
    v[1].p = D3DXVECTOR4( 640 - 0.5f,   0 - 0.5f, 0, 0 );  v[1].tu = 640; v[1].tv =   0;
    v[2].p = D3DXVECTOR4( 640 - 0.5f, 480 - 0.5f, 0, 0 );  v[2].tu = 640; v[2].tv = 480;
    v[3].p = D3DXVECTOR4(   0 - 0.5f, 480 - 0.5f, 0, 0 );  v[3].tu =   0; v[3].tv = 480;
    m_pVB->Unlock();

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: Destroy()
// Desc: Destroys the help class' internal objects/
//-----------------------------------------------------------------------------
HRESULT CXBHelp::Destroy()
{
    SAFE_RELEASE( m_pVB );
    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: Render()
// Desc: Renders the gamepad help image, and it's labelled callouts.
//-----------------------------------------------------------------------------
HRESULT CXBHelp::Render( CXBFont* pFont, XBHELP_CALLOUT* tags, 
                         DWORD dwNumCallouts )
{
    // Set state to render the gamepad image
    m_pd3dDevice->SetTexture( 0, m_pGamepadTexture );
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
    m_pd3dDevice->SetVertexShader( D3DFVF_XYZRHW|D3DFVF_TEX1 );

    // Render the gamepad image
    m_pd3dDevice->SetStreamSource( 0, m_pVB, 6*sizeof(FLOAT) );
    m_pd3dDevice->DrawPrimitive( D3DPT_QUADLIST, 0, 1 );

    // Set state to draw the lines
    m_pd3dDevice->SetTexture( 0, NULL );
    m_pd3dDevice->SetRenderState( D3DRS_ALPHABLENDENABLE, FALSE );
    m_pd3dDevice->SetVertexShader( D3DFVF_XYZRHW );

    for( DWORD i=0; i<dwNumCallouts; i++ )
    {
        // Determine the line start and end positions
        WORD wLineStartIndex = tags[i].wControl;
        WORD wLineEndIndex   = tags[i].wControl + 2*(tags[i].wPlacement-1)+1;
        FLOAT line1x = g_vHelpCallouts[wLineStartIndex].x;
        FLOAT line1y = g_vHelpCallouts[wLineStartIndex].y;
        FLOAT line2x = g_vHelpCallouts[wLineEndIndex].x;
        FLOAT line2y = g_vHelpCallouts[wLineEndIndex].y;

        // Draw the callout line
        D3DXVECTOR4 v[2];
        v[0] = D3DXVECTOR4( line1x, line1y, 0.0f, 0.0f );
        v[1] = D3DXVECTOR4( line2x, line2y, 0.0f, 0.0f );
        
        m_pd3dDevice->DrawPrimitiveUP( D3DPT_LINELIST, 1, v, sizeof(D3DXVECTOR4) );
    }

    // Prepare font for rendering
    pFont->Begin();

    // Render the callouts
    for( i=0; i<dwNumCallouts; i++ )
    {
        // Determine the text position
        WORD wTextPosIndex = tags[i].wControl + 2*(tags[i].wPlacement-1)+2;
        FLOAT textx = g_vHelpCallouts[wTextPosIndex].x;
        FLOAT texty = g_vHelpCallouts[wTextPosIndex].y;

        // Draw the callout text
        pFont->DrawText( textx, texty, 0xffffffff, tags[i].strText );
    }

    // Flush the text drawing
    pFont->End();

    return S_OK;
}



