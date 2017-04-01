//-----------------------------------------------------------------------------
// File: XBHelp.h
//
// Desc: Support class for rendering a help image, which is an image of an Xbox
//       game pad, with labelled call outs to each of the game pad's controls.
//
// Hist: 11.01.00 - New for November XDK release
//       12.15.00 - Changes for December XDK release
//       03.06.01 - Changes for April XDK release
//       04.15.01 - Using packed resources for May XDK
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef XBHELP_H
#define XBHELP_H
#include "XBFont.h"
#include "XBResource.h"
#include "XBUtil.h"




//-----------------------------------------------------------------------------
// Name: struct XBHELP_CALLOUT
// Desc: Structure for call out information, used to label controls when
//       rendering an image of an Xbox game pad. An app will define an array of
//       of these, one for each game pad control used.
//-----------------------------------------------------------------------------
struct XBHELP_CALLOUT
{
	WORD     wControl;    // An index to identify a control, as enum'ed below
	WORD     wPlacement;  // An offset to pick from one of the possible placements
	WCHAR*   strText;     // Text to draw when rendering this call out
};




//-----------------------------------------------------------------------------
// Name: class CXBHelp
// Desc: Class for rendering a help image of a game pad with labelled call outs.
//-----------------------------------------------------------------------------
class CXBHelp
{
	CXBPackedResource       m_xprResource;
	LPDIRECT3DDEVICE8       m_pd3dDevice;
	LPDIRECT3DTEXTURE8      m_pGamepadTexture;
	LPDIRECT3DVERTEXBUFFER8 m_pVB;

public:
	// Constructor/destructor
	CXBHelp();
	~CXBHelp();

	// Functions to create and destroy the internal objects
	HRESULT Create( LPDIRECT3DDEVICE8 pd3dDevice, CHAR* pResource );
	HRESULT Destroy();

	// Renders the help screen
	HRESULT Render( CXBFont* pFont, XBHELP_CALLOUT* tags, DWORD dwNumCallouts );
};




//-----------------------------------------------------------------------------
// A bunch of constants used to identify call out positions
//-----------------------------------------------------------------------------
enum
{
	XBHELP_LEFTSTICK,
	XBHELP_LEFTSTICK_LINEEND_1, XBHELP_LEFTSTICK_TEXTPOS_1,
	XBHELP_LEFTSTICK_LINEEND_2, XBHELP_LEFTSTICK_TEXTPOS_2,

	XBHELP_RIGHTSTICK,
	XBHELP_RIGHTSTICK_LINEEND_1, XBHELP_RIGHTSTICK_TEXTPOS_1,
	XBHELP_RIGHTSTICK_LINEEND_2, XBHELP_RIGHTSTICK_TEXTPOS_2,

	XBHELP_DPAD,
	XBHELP_DPAD_LINEEND_1, XBHELP_DPAD_TEXTPOS_1,
	XBHELP_DPAD_LINEEND_2, XBHELP_DPAD_TEXTPOS_2,

	XBHELP_BACK_BUTTON,
	XBHELP_BACK_BUTTON_LINEEND_1, XBHELP_BACK_BUTTON_TEXTPOS_1,
	XBHELP_BACK_BUTTON_LINEEND_2, XBHELP_BACK_BUTTON_TEXTPOS_2,

	XBHELP_START_BUTTON,
	XBHELP_START_BUTTON_LINEEND_1, XBHELP_START_BUTTON_TEXTPOS_1,
	XBHELP_START_BUTTON_LINEEND_2, XBHELP_START_BUTTON_TEXTPOS_2,

	XBHELP_X_BUTTON,
	XBHELP_X_BUTTON_LINEEND_1, XBHELP_X_BUTTON_TEXTPOS_1,
	XBHELP_X_BUTTON_LINEEND_2, XBHELP_X_BUTTON_TEXTPOS_2,

	XBHELP_Y_BUTTON,
	XBHELP_Y_BUTTON_LINEEND_1, XBHELP_Y_BUTTON_TEXTPOS_1,
	XBHELP_Y_BUTTON_LINEEND_2, XBHELP_Y_BUTTON_TEXTPOS_2,

	XBHELP_A_BUTTON,
	XBHELP_A_BUTTON_LINEEND_1, XBHELP_A_BUTTON_TEXTPOS_1,
	XBHELP_A_BUTTON_LINEEND_2, XBHELP_A_BUTTON_TEXTPOS_2,

	XBHELP_B_BUTTON,
	XBHELP_B_BUTTON_LINEEND_1, XBHELP_B_BUTTON_TEXTPOS_1,
	XBHELP_B_BUTTON_LINEEND_2, XBHELP_B_BUTTON_TEXTPOS_2,

	XBHELP_WHITE_BUTTON,
	XBHELP_WHITE_BUTTON_LINEEND_1, XBHELP_WHITE_BUTTON_TEXTPOS_1,
	XBHELP_WHITE_BUTTON_LINEEND_2, XBHELP_WHITE_BUTTON_TEXTPOS_2,

	XBHELP_BLACK_BUTTON,
	XBHELP_BLACK_BUTTON_LINEEND_1, XBHELP_BLACK_BUTTON_TEXTPOS_1,
	XBHELP_BLACK_BUTTON_LINEEND_2, XBHELP_BLACK_BUTTON_TEXTPOS_2,

	XBHELP_LEFT_BUTTON,
	XBHELP_LEFT_BUTTON_LINEEND_1, XBHELP_LEFT_BUTTON_TEXTPOS_1,
	XBHELP_LEFT_BUTTON_LINEEND_2, XBHELP_LEFT_BUTTON_TEXTPOS_2,

	XBHELP_RIGHT_BUTTON,
	XBHELP_RIGHT_BUTTON_LINEEND_1, XBHELP_RIGHT_BUTTON_TEXTPOS_1,
	XBHELP_RIGHT_BUTTON_LINEEND_2, XBHELP_RIGHT_BUTTON_TEXTPOS_2,

	XBHELP_MISC_CALLOUT,
	XBHELP_MISC_CALLOUT_LINEEND_1, XBHELP_MISC_CALLOUT_TEXTPOS_1,
	XBHELP_MISC_CALLOUT_LINEEND_2, XBHELP_MISC_CALLOUT_TEXTPOS_2,
};




//-----------------------------------------------------------------------------
// Placement options for each call out, used as an offset into the enumerated
// list above.
//-----------------------------------------------------------------------------
#define XBHELP_PLACEMENT_CUSTOM 0   // For future implementation
#define XBHELP_PLACEMENT_1      1   // Call out has one line of text
#define XBHELP_PLACEMENT_2      2   // Call out has two lines of text




#endif
