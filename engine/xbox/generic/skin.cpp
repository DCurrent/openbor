/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

// Skin.cpp: implementation of the CSkin class.
//
//////////////////////////////////////////////////////////////////////

#include "Skin.h"
#include <stdio.h>

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CSkin::CSkin()
{
	m_pDefaultMode = NULL;
}

CSkin::~CSkin()
{

}

HRESULT CSkin::Create(CHAR* szXmlFile)
{
	if (CXmlDocument::Load(szXmlFile)<0)
	{
		OutputDebugString("\nUnable to load xml file.\n");
		return E_FAIL;
	}

	XmlNode xnNode = GetNextNode(XML_ROOT_NODE);
	while (xnNode>0)
	{
		CHAR* szCurrentTag = GetNodeTag(xnNode);

		if ( !strcmpi(szCurrentTag,"mode") )
		{
			CMode* pNewMode = new CMode;
			if (SUCCEEDED(pNewMode->Create(this,xnNode)))
			{
				OutputDebugString("Enumerated mode: ");
				OutputDebugString(pNewMode->ToString().c_str());
				OutputDebugString("\n");

				m_modes[pNewMode->ToString()] = pNewMode;

				XmlNode xnDefaultMode;
				if (xnDefaultMode = GetChildNode(xnNode,"default"))
				{
					if ( !strcmpi(GetNodeText(xnDefaultMode),"true") )
						m_pDefaultMode = pNewMode;
				}

			}
			else
			{
				delete pNewMode;
			}
		}

		xnNode = GetNextNode(xnNode);
	}

	return S_OK;
}

CMode* CSkin::GetMode(INT iWidth,INT iHeight,BOOL bWidescreen)
{
	CHAR szMode[32];
	wsprintf(szMode,"%dx%d%s",iWidth,iHeight,(bWidescreen ? "@16:9":"@4:3"));
	string strMode = szMode;

	return m_modes[strMode];
}

CMode* CSkin::GetDefaultMode()
{
	return m_pDefaultMode;
}

CMode::CMode()
{
	m_lastid = 0;
}

CMode::~CMode()
{

}

HRESULT CMode::Create(CXmlDocument* pSkin, XmlNode xnMode)
{
	XmlNode xnWidescreen, xnResolution;
	char* ex;

	if (!(xnWidescreen = pSkin->GetChildNode(xnMode,"widescreen")))
		return E_FAIL;

	if (!(xnResolution = pSkin->GetChildNode(xnMode,"resolution")))
		return E_FAIL;

	m_bWidescreen = !strcmpi( pSkin->GetNodeText(xnWidescreen) ,"true");
	m_name = pSkin->GetNodeText(xnResolution);

	m_iWidth = atoi(m_name.c_str());
	if (ex = strstr(m_name.c_str(),"x"))
		m_iHeight = atoi(ex+1);

	m_name += (m_bWidescreen ? "@16:9":"@4:3");

	// process controls, break at next mode or end of document.
	XmlNode xnNode = pSkin->GetNextNode(xnMode);
	while (xnNode>0)
	{
		CHAR* szCurrentTag = pSkin->GetNodeTag(xnNode);

		if ( !strcmpi(szCurrentTag,"control") )
		{
			AddControl(pSkin, xnNode);
		}
		else if ( !strcmpi(szCurrentTag,"mode") )
		{
			break;
		}

		xnNode = pSkin->GetNextNode(xnNode);
	}

	return S_OK;
}

VOID CMode::AddControl(CXmlDocument* pSkin, XmlNode xnControl)
{
	INT id = m_lastid++;

	XmlNode xnPosition, xnDimensions;

	if (xnPosition = pSkin->GetChildNode(xnControl,"position"))
	{
		char* comma;
		char szPosition[32];
		float x,y;

		strcpy( szPosition, pSkin->GetNodeText(xnPosition) );
		x = (float) atoi(szPosition);
		if (comma = strstr(szPosition,","))
		{
			y = (float) atoi(comma+1);
			SetControlPosition(id,x,y);
		}
	}

	if (xnDimensions = pSkin->GetChildNode(xnControl,"dimensions"))
	{
		char* comma;
		char szDimensions[32];
		float width,height;

		strcpy( szDimensions, pSkin->GetNodeText(xnDimensions) );
		width = (float) atoi(szDimensions);
		if (comma = strstr(szDimensions,","))
		{
			height = (float) atoi(comma+1);
			SetControlDimensions(id,width,height);
		}
	}

}

string CMode::ToString()
{
	return m_name;
}

VOID CMode::SetControlPosition(INT iControlId, FLOAT x, FLOAT y)
{
	m_coordinates[iControlId + COORDINATE_X_BASE] = x;
	m_coordinates[iControlId + COORDINATE_Y_BASE] = y;
}

VOID CMode::SetControlDimensions(INT iControlId, FLOAT width, FLOAT height)
{
	m_coordinates[iControlId + DIMENSION_W_BASE] = width;
	m_coordinates[iControlId + DIMENSION_H_BASE] = height;
}
