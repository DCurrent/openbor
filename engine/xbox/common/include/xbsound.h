//-----------------------------------------------------------------------------
// File: XBSound.h
//
// Desc: Helper class for reading a .wav file and playing it in a DirectSound
//       buffer.
//
// Hist: 12.15.00 - New for December XDK release
//       02.15.01 - Updated for March XDK release
//
// Copyright (c) 2000-2001 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#ifndef XBSOUND_H
#define XBSOUND_H
#include <xtl.h>




//-----------------------------------------------------------------------------
// Name: RIFFHEADER
// Desc: For parsing WAV files
//-----------------------------------------------------------------------------
struct RIFFHEADER
{
	FOURCC  fccChunkId;
	DWORD   dwDataSize;
};

#define RIFFCHUNK_FLAGS_VALID   0x00000001


//-----------------------------------------------------------------------------
// Name: class CRiffChunk
// Desc: RIFF chunk utility class
//-----------------------------------------------------------------------------
class CRiffChunk
{
	FOURCC            m_fccChunkId;       // Chunk identifier
	const CRiffChunk* m_pParentChunk;     // Parent chunk
	HANDLE            m_hFile;
	DWORD             m_dwDataOffset;     // Chunk data offset
	DWORD             m_dwDataSize;       // Chunk data size
	DWORD             m_dwFlags;          // Chunk flags

public:
	CRiffChunk();

	// Initialization
	VOID    Initialize( FOURCC fccChunkId, const CRiffChunk* pParentChunk,
						HANDLE hFile );
	HRESULT Open();
	BOOL    IsValid()     { return !!(m_dwFlags & RIFFCHUNK_FLAGS_VALID); }

	// Data
	HRESULT ReadData( LONG lOffset, VOID* pData, DWORD dwDataSize );

	// Chunk information
	FOURCC  GetChunkId()  { return m_fccChunkId; }
	DWORD   GetDataSize() { return m_dwDataSize; }
};




//-----------------------------------------------------------------------------
// Name: class CWaveFile
// Desc: Wave file utility class
//-----------------------------------------------------------------------------
class CWaveFile
{
	HANDLE      m_hFile;            // File handle
	CRiffChunk  m_RiffChunk;        // RIFF chunk
	CRiffChunk  m_FormatChunk;      // Format chunk
	CRiffChunk  m_DataChunk;        // Data chunk
	CRiffChunk  m_WaveSampleChunk;  // Wave Sample chunk

public:
	CWaveFile();
	~CWaveFile();

	// Initialization
	HRESULT Open( const CHAR* strFileName );
	VOID    Close();

	// File format
	HRESULT GetFormat( WAVEFORMATEX* pwfxFormat, DWORD dwFormatSize, DWORD *pdwRequiredSize = NULL );

	// File data
	HRESULT ReadSample( DWORD dwPosition, VOID* pBuffer, DWORD dwBufferSize,
						DWORD* pdwRead );

	// Loop region
	HRESULT GetLoopRegion( DWORD* pdwStart, DWORD* pdwLength );

	// File properties
	VOID    GetDuration( DWORD* pdwDuration ) { *pdwDuration = m_DataChunk.GetDataSize(); }
};




//-----------------------------------------------------------------------------
// Name: class CSound
// Desc: Encapsulates functionality of a DirectSound buffer.
//-----------------------------------------------------------------------------
class CXBSound
{
protected:
	LPDIRECTSOUNDBUFFER  m_pDSoundBuffer;
	WAVEFORMATEX         m_WaveFormat;
	DSBUFFERDESC         m_dsbd;
	DWORD                m_dwBufferSize;

public:
	HRESULT Create( const CHAR* strFileName, DWORD dwFlags = 0L );
	HRESULT Create( const WAVEFORMATEX* pwfxFormat, DWORD dwFlags,
					const VOID* pBuffer, DWORD dwBytes );
	VOID    Destroy();

	HRESULT Play( DWORD dwFlags = 0L ) const;
	HRESULT Stop() const;
	HRESULT GetStatus(LPDWORD pdwStatus) const;
	HRESULT SetVolume( LONG lValue ) const;
	HRESULT SetPosition( const D3DXVECTOR3& vPosition ) const;
	HRESULT SetVelocity( const D3DXVECTOR3& vVelocity ) const;

	CXBSound();
	~CXBSound();
};



//-----------------------------------------------------------------------------
// Name: class CDSPImage
// Desc: Encapsulates DSP image for loading from file, downloading to DSP, etc.
//-----------------------------------------------------------------------------
class CDSPImage
{
protected:
	DWORD           m_dwImageSize;
	BYTE *          m_pbImageData;

public:
	CDSPImage();
	~CDSPImage();

	HRESULT LoadFromFile( char * szFilename );
	HRESULT DownloadImage( LPDIRECTSOUND8 pDSound );
};
#endif // XBSOUND_H
