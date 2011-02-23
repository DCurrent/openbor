/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

// GraphicsContext.cpp: implementation of the CGraphicsContext class.
//
//////////////////////////////////////////////////////////////////////

#include "GraphicsContext.h"
#include "Configuration.h"
#pragma warning (disable:4244)

CGraphicsContext g_graphicsContext;


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CGraphicsContext::CGraphicsContext()
{
	m_pMode			= NULL;
	m_bWidescreen	= FALSE;
}

CGraphicsContext::~CGraphicsContext()
{
}

HRESULT CGraphicsContext::Create(CHAR* szXmlFile)
{
	memset(&m_rClient,0,sizeof(RECT));
	memset(&m_rVisible,0,sizeof(RECT));


	if (SUCCEEDED(m_cSkin.Create(szXmlFile)))
	{
		m_pMode = m_cSkin.GetDefaultMode();

		if (m_pMode!=NULL)
		{
			m_bWidescreen = m_pMode->m_bWidescreen;
			m_rClient.right  = m_pMode->m_iWidth;
			m_rClient.bottom = m_pMode->m_iHeight;
			memcpy(&m_rVisible,&m_rClient,sizeof(RECT));
			return S_OK;
		}

		OutputDebugString("No default mode specified in skin.xml\n");
		return E_NOTIMPL;
	}

	return E_FAIL;
}

BOOL CGraphicsContext::SetMode(INT iWidth, INT iHeight, BOOL bWidescreen )
{
	m_pMode = m_cSkin.GetMode( iWidth, iHeight, bWidescreen );

	if (m_pMode==NULL)
	{
		char szDebug[64];
		wsprintf( szDebug, "Not a supported mode: %dx%d%s\n", iWidth, iHeight, (bWidescreen ? "@16:9":"@4:3") );
		OutputDebugString(szDebug);
		return FALSE;
	}

	m_bWidescreen = bWidescreen;
	m_rClient.right  = iWidth;
	m_rClient.bottom = iHeight;
	memcpy(&m_rVisible,&m_rClient,sizeof(RECT));

	return TRUE;
}

VOID CGraphicsContext::GetClientArea(LPRECT lpClientRect)
{
	memcpy(lpClientRect,&m_rClient,sizeof(RECT));
}

VOID CGraphicsContext::SetVisibleArea(LPRECT lpVisibleRect)
{
	char szDebug[64];
	wsprintf(	szDebug,"CGraphicsContext::SetVisibleArea(%d, %d, %d, %d)\n",
				lpVisibleRect->left, lpVisibleRect->top, lpVisibleRect->right, lpVisibleRect->bottom);
	OutputDebugString(szDebug);

	memcpy(&m_rVisible,lpVisibleRect,sizeof(RECT));
}

VOID CGraphicsContext::GetVisibleArea(LPRECT lpVisibleRect)
{
	memcpy(lpVisibleRect,&m_rVisible,sizeof(RECT));
}

FLOAT CGraphicsContext::GetWidth()
{
	return (FLOAT)(m_rClient.right - m_rClient.left);
}

FLOAT CGraphicsContext::GetHeight()
{
	return (FLOAT)(m_rClient.bottom - m_rClient.top);
}

VOID CGraphicsContext::Correct(FLOAT& fCoordinateX, FLOAT& fCoordinateY, FLOAT& fCoordinateX2, FLOAT& fCoordinateY2)
{
	if (!IsCorrectionEnabled())
		return;

	fCoordinateX	*= WIDE_SCREEN_COMPENSATIONX;
	fCoordinateX2	*= WIDE_SCREEN_COMPENSATIONX;
	fCoordinateY	*= WIDE_SCREEN_COMPENSATIONY;
	fCoordinateY2	*= WIDE_SCREEN_COMPENSATIONY;
}

VOID CGraphicsContext::Offset(FLOAT& fCoordinateX, FLOAT& fCoordinateY)
{
	fCoordinateX	+= (FLOAT) m_rVisible.left;
	fCoordinateY	+= (FLOAT) m_rVisible.top;
}

BOOL CGraphicsContext::IsWidescreen()
{
	return m_bWidescreen;
}

BOOL CGraphicsContext::IsCorrectionEnabled()
{
	return ( (m_bWidescreen) && (GetWidth()>640) && (GetHeight()>480) );
}

FLOAT CGraphicsContext::GetScreenRatioAdjustment()
{
/*
		What size is a television picture?
		There are 576 active lines in a television picture, making it 576 pixels high.
		A 4:3 image would therefore be:		(576 x 4) ÷ 3 = 768 pixels wide.
		However this assumes the pixels are square - but television pixels are not square.
		They have an aspect ratio of approximately 1:1.094.
		A 4:3 television picture would therefore be:  768 ÷ 1.094 = 702 non-square pixels wide
		So a 4:3 television image is 702 pixels wide by 576 high.
		But anyone who has imported graphics from a computer system to the television systems
		knows the correct width to make the image is 720 pixels! Why - and where did the extra 18
		pixels come from?

	  Digital pictures are effectively wider than analogue pictures by 18 pixels but
	  the 4:3 image sits inside the 720 by 576 area. The additional 18 pixels are required
		for digital processing and it would be perfectly acceptable to leave them black -
		but if the image is shrunk via a digital DVE, two 9 pixel wide black stripes
		will be seen at the sides
		These 18 pixels are of course not square, converting them
		to square pixels gives:   18 x 1.094 = 20 square pixels
		When making a 4:3 graphic on a square pixel device for conversion to a 720 by 576
		(non square pixel) video image, the width of the 768 by 576 (square pixel) image must
		be increased to:  768 + 20 = 788 square pixels
		making the 4:3 the square pixel graphic 788 by 576
		so for 4:3 square pixel:788x576 = nonsquare pixel 720x576


		These additional pixels are not taken into account when calculating the aspect ratio,
		but without them images transferred between systems will not be the correct shape.

		Widescreen:
		A widescreen television picture has the same number of vertical lines as a 4:3 picture,
		that is 576.
		The aspect ratio is 16:9 making the picture: (576 x 16) ÷ 9 = 1024 square pixels wide
		A widescreen picture "fits" into the same electronic space as a 4:3 image.
		The 16:9 widescreen (1024 by 576) image is horizontally squeezed to 702 by 576.
		The picture is distorted to be the correct shape on 16:9 screens only,
		hence tall thin people when the image is seen on a 4:3 screen.
		The additional 18 pixels in the 4:3 picture,
		when stretched to 16:9 become: (18 x 4) ÷ 3 = 26
		When making a 16:9 graphic on a square pixel device for conversion
		to a 720 by 576 widescreen video picture, the width of the 1024 by 576
		image must be increased: 26 +1024 = 1050 by 576 square pixels
*/

		FLOAT fScreenWidth  = 640 ;
		FLOAT fScreenHeight = 480 ;
		FLOAT fScreenRatio  = fScreenWidth / fScreenHeight;
		FLOAT fPerfectRatio = m_bWidescreen ? (16.0f/9.0f) : (4.0f / 3.0f);

         // lets face it... its pretty damn unlikely ;-)
         if ( fScreenRatio != fPerfectRatio )
         {
                 return fPerfectRatio / fScreenRatio;
         }

         return 0.0f; //1.3333->1.21
}