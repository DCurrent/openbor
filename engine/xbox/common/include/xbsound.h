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
	ptrdiff_t Open();
	BOOL    IsValid()     { return !!(m_dwFlags & RIFFCHUNK_FLAGS_VALID); }

	// Data
	ptrdiff_t ReadData( LONG lOffset, VOID* pData, DWORD dwDataSize );

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
	ptrdiff_t Open( const char* strFileName );
	VOID    Close();

	// File format
	ptrdiff_t GetFormat( WAVEFORMATEX* pwfxFormat, DWORD dwFormatSize, DWORD *pdwRequiredSize = NULL );

	// File data
	ptrdiff_t ReadSample( DWORD dwPosition, VOID* pBuffer, DWORD dwBufferSize, 
						DWORD* pdwRead );

	// Loop region
	ptrdiff_t GetLoopRegion( DWORD* pdwStart, DWORD* pdwLength );

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
	ptrdiff_t Create( const char* strFileName, DWORD dwFlags = 0L );
	ptrdiff_t Create( const WAVEFORMATEX* pwfxFormat, DWORD dwFlags,
					const VOID* pBuffer, DWORD dwBytes );
	VOID    Destroy();

	ptrdiff_t Play( DWORD dwFlags = 0L ) const;
	ptrdiff_t Stop() const;
	ptrdiff_t GetStatus(LPDWORD pdwStatus) const;
	ptrdiff_t SetVolume( LONG lValue ) const;
	ptrdiff_t SetPosition( const D3DXVECTOR3& vPosition ) const;
	ptrdiff_t SetVelocity( const D3DXVECTOR3& vVelocity ) const;

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

	ptrdiff_t LoadFromFile( char * szFilename );
	ptrdiff_t DownloadImage( LPDIRECTSOUND8 pDSound );
};
#endif // XBSOUND_H
