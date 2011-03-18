/*
 * OpenBOR - http://www.LavaLit.com
 * -----------------------------------------------------------------------
 * Licensed under the BSD license, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2011 OpenBOR Team
 */

// Configuration.h: interface for the CConfiguration class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CONFIGURATION_H__05039157_ACED_4430_B306_D88792C7A755__INCLUDED_)
#define AFX_CONFIGURATION_H__05039157_ACED_4430_B306_D88792C7A755__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include <xtl.h>
#include "XmlDocument.h"

#define OVERLAY_PREVIEW		0
#define	OVERLAY_STANDARD	1

#define CONFIG_VERSION 0x230

#include <string>
#include <vector>
using namespace std;

class CShare
{
	public:
		string strName;
		string strURL;
		int    m_iBufferSize;

};
typedef vector<CShare> VECSHARES;
typedef vector<CShare>::iterator IVECSHARES;


class CConfiguration : public CXmlDocument
{
public:
	int GetPictureCacheSize(const string& strFile);
	int GetMusicCacheSize(const string& strFile);
	int GetVideoCacheSize(const string& strFile);

	struct PlayerSettings
	{
		DWORD dwVersion;

		char szMappings[32];				// drive mappings (user)
		char szDashboard[32];				// dashboard (user)
		char szThumbnailDirectory[128];
		/////////////////////////////////////////////////////////////////////
		// Populated by application

		DWORD dwOverlayMode;

		RECT rSource;			// source dimensions
		RECT rClip;				// clip offsets
		RECT rScreen;			// screen client area
		RECT rOveride;			// overridden client area
		RECT rOutput;			// destination area

		FLOAT fRatioCorrection;

		BOOL bZoom;
		BOOL bStretch;
		BOOL bPostProcessing;

		BOOL bVideoFilenameParsing;
		BOOL bAudioFilenameParsing;
		BOOL bStackMultipartVideo;
		BOOL bSubtitles;

		DWORD dwAudioStream;
		DWORD dwScreenHeight;
		DWORD dwScreenWidth;

		INT   iDecoderAutoSync;			// a/v sync method
		BOOL  bDecoderForceFPS;			// turn 'force output fps' on/off
		FLOAT fDecoderMaxPtsCorrection;	// max pts correction/frame
		FLOAT fDecoderFps;				// forced output fps
		BOOL  bUsePtsFromBps;			// use btp from bps
		BOOL  bUseFrameDropping;		// use frame dropping is decoder is 2 slow
		BOOL  bUsePAL60;				// use PAL 60 if needed&available
		INT   iShutDownTime;			// time in mins before idle shutdown(0=disabled)
		BOOL  bPostProcessingExtra;		// turn xtra postprocessing on/off
		int   iSlideShowTime;			// time in seconds before next picture is shown
		float fIRSensitivity;			// button sensitivity. Can B configured in config.xml
		BOOL  bAudioOnAllSpeakers;		// always put audio on all speakers, or just on the ones needed
		BOOL  bAmplifyAudio;
		int   iAudioAttenuation;
		BOOL  bAudioNormalize;
		BOOL  bAudioExtraStereo;
		BOOL  bViewMusicByList;
		BOOL  bViewVideosByCover;
		BOOL  bViewPicturesByList;

		CHAR szMusicExtensions[128];
		CHAR szVideoExtensions[128];

		DWORD dwLanguage;

		char szLocalApplications[128];
		char szLocalPlaylists[128];
		char szLocalAddress[32];
		char szLocalSubnet[32];
		char szLocalGateway[32];
		char szBookmarks[128]; // will use it to locate the directory when bookmarks are placed

		char szLocalNameServer[32];

		char szLocalHostname[32];

		BOOL bIsDebug;
		BOOL bFlattenApplicationDirs;
		BOOL bCacheISO9660;
		unsigned int uiRelaxBuffer;

		BOOL bAutoClearPlaylist;
		BOOL bSoftenDisplay;

		CHAR szSubtitleEncoding[256];
		BOOL bMPlayerSubtitles; // use mplayer rendering for subtitles
		BOOL bUseTTFUIFont;		// whether use "MEDIA\common-font.ttf" or original TGA bitmap fonts
		BOOL bShowID3TagInfo;
		CHAR szDVDSubtitleLanguage[20]; // language to use for DVD subtitles
		CHAR szDVDAudioLanguage[20];		// language to use for DVD audio streams
		INT   iScreenSaver;			// time in secs before screensaver starts (0=disabled)
		CHAR szSnapStreamServer[20];
		CHAR szSnapStreamUser[20];
	};


	CConfiguration();
	virtual ~CConfiguration();

	HRESULT Create(char* szXmlFile, CConfiguration::PlayerSettings* pPlayerSettings, BOOL bUseDefault);

	void GetPictureShares(VECSHARES& shares);
	void GetMusicShares(VECSHARES& shares);
	void GetVideoShares(VECSHARES& shares);
	void GetApplicationShares(VECSHARES& shares);
private:
	VOID	ProcessMediaSettingsNode(XmlNode thisNode);
	VOID	ProcessMappingNode(XmlNode thisNode);
	VOID	ConvertFormat(CHAR* szDestinationFormat, CHAR* szSourceFormat);
	PlayerSettings*	m_pPlayerSettings;

	VOID	ProcessMusicShares(XmlNode thisNode);
	VOID	ProcessVideoShares(XmlNode thisNode);
	VOID	ProcessPictureShares(XmlNode thisNode);
	VOID	ProcessApplicationShares(XmlNode thisNode);

	VECSHARES m_vecMusicShares;
	VECSHARES m_vecPictureShares;
	VECSHARES m_vecVideoShares;
	VECSHARES m_vecApplicationShares;
};

extern CConfiguration::PlayerSettings g_playerSettings;
extern CHAR g_szTitleIP[32];

#endif // !defined(AFX_CONFIGURATION_H__05039157_ACED_4430_B306_D88792C7A755__INCLUDED_)
