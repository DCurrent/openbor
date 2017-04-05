/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

// Skin.h: interface for the CSkin class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_Skin_H__05039157_ACED_4430_B306_D88792C7A755__INCLUDED_)
#define AFX_Skin_H__05039157_ACED_4430_B306_D88792C7A755__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <xtl.h>
#include "XmlDocument.h"

#include <map>
#include <string>
using namespace std;

#define COORDINATE_X_BASE	0
#define COORDINATE_Y_BASE	100
#define DIMENSION_W_BASE	200
#define DIMENSION_H_BASE	300

typedef map<int,float>COORDINATE_MAP;

class CMode
{
public:

	CMode();
	virtual ~CMode();

	HRESULT Create(CXmlDocument* pSkin, XmlNode xnMode);
	string	ToString();

	COORDINATE_MAP	m_coordinates;
	INT				m_iWidth;
	INT				m_iHeight;
	BOOL			m_bWidescreen;

private:

	VOID	AddControl(CXmlDocument* pSkin, XmlNode modeNode);
	VOID	SetControlPosition(INT iControlId, FLOAT x, FLOAT y);
	VOID	SetControlDimensions(INT iControlId, FLOAT width, FLOAT height);

private:

	INT				m_lastid;
	string			m_name;
};

typedef map<string,CMode*>MODE_MAP;

class CSkin : public CXmlDocument
{
public:

	CSkin();
	virtual ~CSkin();

	HRESULT Create(CHAR* szXmlFile);
	CMode*  GetMode(INT iWidth, INT iHeight, BOOL bWidescreen);
	CMode*  GetDefaultMode();

private:

	MODE_MAP	m_modes;
	CMode*		m_pDefaultMode;
};

#endif // !defined(AFX_Skin_H__05039157_ACED_4430_B306_D88792C7A755__INCLUDED_)
