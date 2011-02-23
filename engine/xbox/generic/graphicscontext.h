/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

// GraphicsContext.h: interface for the CGraphicsContext class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_GRAPHICSCONTEXT_H__FDD4378D_C6C5_4881_9179_29B08B281F45__INCLUDED_)
#define AFX_GRAPHICSCONTEXT_H__FDD4378D_C6C5_4881_9179_29B08B281F45__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <xtl.h>
#include "Skin.h"

#define WIDE_SCREEN_COMPENSATIONY (FLOAT)1.2
#define WIDE_SCREEN_COMPENSATIONX (FLOAT)0.85

class CGraphicsContext
{
public:
	CGraphicsContext();
	virtual ~CGraphicsContext();

	HRESULT	Create(CHAR* szXmlFile);

	BOOL	SetMode(INT iWidth, INT iHeight, BOOL bWidescreen );

	VOID	GetClientArea(LPRECT lpClientRect);
	FLOAT	GetWidth();
	FLOAT	GetHeight();

	VOID	SetVisibleArea(LPRECT lpVisibleRect);
	VOID	GetVisibleArea(LPRECT lpVisibleRect);

	BOOL	IsWidescreen();
	BOOL	IsCorrectionEnabled();
	FLOAT	GetScreenRatioAdjustment();

	VOID	Correct(FLOAT& fCoordinateX, FLOAT& fCoordinateY, FLOAT& fCoordinateX2, FLOAT& fCoordinateY2);
	VOID	Offset(FLOAT& fCoordinateX, FLOAT& fCoordinateY);

public:
	CMode*	m_pMode;

private:
	RECT	m_rClient;
	RECT	m_rVisible;
	BOOL	m_bWidescreen;
	CSkin	m_cSkin;
};

extern CGraphicsContext g_graphicsContext;

#define CORDX(i) g_graphicsContext.m_pMode->m_coordinates[i + COORDINATE_X_BASE]
#define CORDY(i) g_graphicsContext.m_pMode->m_coordinates[i + COORDINATE_Y_BASE]
#define DIMW(i)  g_graphicsContext.m_pMode->m_coordinates[i + DIMENSION_W_BASE]
#define DIMH(i)  g_graphicsContext.m_pMode->m_coordinates[i + DIMENSION_H_BASE]

#define CAPTION			0
#define BUTTON_PANEL	1
#define COVERCTL		2
#define LISTCTL			3
#define LISTLABEL		4
#define PREVIEW_PANEL	5
#define ALBUM_PREVIEW	6
#define LARGE_PREVIEW	7
#define START_CAPTION	8


#endif // !defined(AFX_GRAPHICSCONTEXT_H__FDD4378D_C6C5_4881_9179_29B08B281F45__INCLUDED_)
