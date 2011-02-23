//-----------------------------------------------------------------------------
// File: XBApp.h
//
// Desc: Application class for the XBox samples.
//
// Hist: 11.01.00 - New for November XDK release
//       12.15.00 - Changes for December XDK release
//       02.19.01 - Changes for March XDK release
//
// Copyright (c) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef XBAPP_H
#define XBAPP_H

#include <xtl.h>
#include <xgraphics.h>
#include <stdio.h>
#include "XBInput.h"
#include "XBUtil.h"




//-----------------------------------------------------------------------------
// Global access to common members
//-----------------------------------------------------------------------------
extern LPDIRECT3DDEVICE8 g_pd3dDevice;




//-----------------------------------------------------------------------------
// Error codes
//-----------------------------------------------------------------------------
#define XBAPPERR_MEDIANOTFOUND       0x82000003




//-----------------------------------------------------------------------------
// Name: class CXBApplication
// Desc: A base class for creating sample Xbox applications. To create a simple
//       Xbox application, simply derive this class and override the following
//       functions:
//          Initialize()          - To initialize the device-dependant objects
//          FrameMove()           - To animate the scene
//          Render()              - To render the scene
//-----------------------------------------------------------------------------
class CXBApplication
{
public:
    D3DPRESENT_PARAMETERS m_d3dpp;
    LPDIRECT3DDEVICE8     m_pd3dDevice;        // The D3D rendering device
    LPDIRECT3D8           m_pD3D;              // The D3D enumerator object
protected:
    // Main objects used for creating and rendering the 3D scene
    LPDIRECT3DSURFACE8    m_pBackBuffer;       // The back buffer
    LPDIRECT3DSURFACE8    m_pDepthBuffer;      // The depth buffer

    // Variables for timing
    FLOAT      m_fTime;             // Current absolute time in seconds
    FLOAT      m_fElapsedTime;      // Elapsed absolute time since last frame
    FLOAT      m_fAppTime;          // Current app time in seconds
    FLOAT      m_fElapsedAppTime;   // Elapsed app time since last frame
    BOOL       m_bPaused;           // Whether app time is paused by user
    FLOAT      m_fFPS;              // instantaneous frame rate
    WCHAR      m_strFrameRate[20];  // Frame rate written to a string
    HANDLE     m_hFrameCounter;     // Handle to frame rate perf counter

    // Members to init the XINPUT devices.
    XDEVICE_PREALLOC_TYPE* m_InputDeviceTypes;
    DWORD                  m_dwNumInputDeviceTypes;
    XBGAMEPAD*             m_Gamepad;
    XBGAMEPAD              m_DefaultGamepad;

    // Helper functions

    // Overridable functions for the 3D scene created by the app
    virtual HRESULT Initialize()            { return S_OK; }
    virtual HRESULT FrameMove()             { return S_OK; }
    virtual HRESULT Render()                { return S_OK; }
    virtual HRESULT Cleanup()               { return S_OK; }
    HRESULT RenderGradientBackground( DWORD dwTopColor, DWORD dwBottomColor );

public:
    // Functions to create, run, and clean up the application
    HRESULT Create();
    INT     Run();
    VOID    Destroy();

    // Internal constructor
    CXBApplication();
};




#endif
