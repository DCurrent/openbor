//-----------------------------------------------------------------------------
// File: XBSound.cpp
//
// Desc: Helper class for reading a .wav file and playing it in a DirectSound
//       buffer.
//
// Hist: 12.15.00 - New for December XDK release
//
// Copyright (c) 2000-2001 Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------
#include <xtl.h>
#include <stdio.h>
#include "XBSound.h"
#include "XBUtil.h"




//-----------------------------------------------------------------------------
// FourCC definitions
//-----------------------------------------------------------------------------
const DWORD FOURCC_RIFF   = 'FFIR';
const DWORD FOURCC_WAVE   = 'EVAW';
const DWORD FOURCC_FORMAT = ' tmf';
const DWORD FOURCC_DATA   = 'atad';
const DWORD FOURCC_WSMP   = 'pmsw';



//-----------------------------------------------------------------------------
// Name: struct WAVESAMPE
// Desc: RIFF chunk type that contains loop point information
//-----------------------------------------------------------------------------
struct WAVESAMPLE
{
    ULONG   cbSize;
    USHORT  usUnityNote;
    SHORT   sFineTune;
    LONG    lGain;
    ULONG   ulOptions;
    ULONG   cSampleLoops;
};

//-----------------------------------------------------------------------------
// Name: struct WAVESAMPLE_LOOP
// Desc: Loop point (contained in WSMP chunk)
//-----------------------------------------------------------------------------
struct WAVESAMPLE_LOOP
{
    ULONG cbSize;
    ULONG ulLoopType;
    ULONG ulLoopStart;
    ULONG ulLoopLength;
};




//-----------------------------------------------------------------------------
// Name: CRiffChunk()
// Desc: Object constructor.
//-----------------------------------------------------------------------------
CRiffChunk::CRiffChunk()
{
    // Initialize defaults
    m_fccChunkId   = 0;
    m_pParentChunk = NULL;
    m_hFile        = INVALID_HANDLE_VALUE;
    m_dwDataOffset = 0;
    m_dwDataSize   = 0;
    m_dwFlags      = 0;
}




//-----------------------------------------------------------------------------
// Name: Initialize()
// Desc: Initializes the object
//-----------------------------------------------------------------------------
VOID CRiffChunk::Initialize( FOURCC fccChunkId, const CRiffChunk* pParentChunk, 
                             HANDLE hFile )
{
    m_fccChunkId   = fccChunkId;
    m_pParentChunk = pParentChunk;
    m_hFile        = hFile;
}




//-----------------------------------------------------------------------------
// Name: Open()
// Desc: Opens an existing chunk.
//-----------------------------------------------------------------------------
HRESULT CRiffChunk::Open()
{
    RIFFHEADER rhRiffHeader;
    LONG       lOffset = 0;

    // Seek to the first byte of the parent chunk's data section
    if( m_pParentChunk )
    {
        lOffset = m_pParentChunk->m_dwDataOffset;

        // Special case the RIFF chunk
        if( FOURCC_RIFF == m_pParentChunk->m_fccChunkId )
            lOffset += sizeof(FOURCC);
    }
    
    // Read each child chunk header until we find the one we're looking for
    for( ;; )
    {
        if( INVALID_SET_FILE_POINTER == SetFilePointer( m_hFile, lOffset, NULL, FILE_BEGIN ) )
            return HRESULT_FROM_WIN32( GetLastError() );

        DWORD dwRead;
        if( 0 == ReadFile( m_hFile, &rhRiffHeader, sizeof(rhRiffHeader), &dwRead, NULL ) )
            return HRESULT_FROM_WIN32( GetLastError() );

        // Hit EOF without finding it
        if( 0 == dwRead )
            return E_FAIL;

        // Check if we found the one we're looking for
        if( m_fccChunkId == rhRiffHeader.fccChunkId )
        {
            // Save the chunk size and data offset
            m_dwDataOffset = lOffset + sizeof(rhRiffHeader);
            m_dwDataSize   = rhRiffHeader.dwDataSize;

            // Success
            m_dwFlags |= RIFFCHUNK_FLAGS_VALID;

            return S_OK;
        }

        lOffset += sizeof(rhRiffHeader) + rhRiffHeader.dwDataSize;
    }
}




//-----------------------------------------------------------------------------
// Name: Read()
// Desc: Reads from the file
//-----------------------------------------------------------------------------
HRESULT CRiffChunk::ReadData( LONG lOffset, VOID* pData, DWORD dwDataSize )
{
    // Seek to the offset
    DWORD dwPosition = SetFilePointer( m_hFile, m_dwDataOffset+lOffset, NULL, FILE_BEGIN );
    if( INVALID_SET_FILE_POINTER == dwPosition )
        return HRESULT_FROM_WIN32( GetLastError() );

    // Read from the file
    DWORD dwRead;
    if( 0 == ReadFile( m_hFile, pData, dwDataSize, &dwRead, NULL ) )
        return HRESULT_FROM_WIN32( GetLastError() );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: CWaveFile()
// Desc: Object constructor.
//-----------------------------------------------------------------------------
CWaveFile::CWaveFile()
{
    m_hFile = INVALID_HANDLE_VALUE;
}




//-----------------------------------------------------------------------------
// Name: ~CWaveFile()
// Desc: Object destructor.
//-----------------------------------------------------------------------------
CWaveFile::~CWaveFile()
{
    Close();
}




//-----------------------------------------------------------------------------
// Name: Open()
// Desc: Initializes the object.
//-----------------------------------------------------------------------------
HRESULT CWaveFile::Open( const CHAR* strFileName )
{
    // If we're already open, close
    Close();
    
    // Open the file
    m_hFile = CreateFile( strFileName, GENERIC_READ, FILE_SHARE_READ, NULL, 
                          OPEN_EXISTING, 0L, NULL );
    if( INVALID_HANDLE_VALUE == m_hFile )
        return HRESULT_FROM_WIN32( GetLastError() );

    // Initialize the chunk objects
    m_RiffChunk.Initialize( FOURCC_RIFF, NULL, m_hFile );
    m_FormatChunk.Initialize( FOURCC_FORMAT, &m_RiffChunk, m_hFile );
    m_DataChunk.Initialize( FOURCC_DATA, &m_RiffChunk, m_hFile );
    m_WaveSampleChunk.Initialize( FOURCC_WSMP, &m_RiffChunk, m_hFile );

    HRESULT hr = m_RiffChunk.Open();
    if( FAILED(hr) )
        return hr;

    hr = m_FormatChunk.Open();
    if( FAILED(hr) )
        return hr;

    hr = m_DataChunk.Open();
    if( FAILED(hr) )
        return hr;

    // Wave Sample chunk is not required
    m_WaveSampleChunk.Open();

    // Validate the file type
    FOURCC fccType;
    hr = m_RiffChunk.ReadData( 0, &fccType, sizeof(fccType) );
    if( FAILED(hr) )
        return hr;

    if( FOURCC_WAVE != fccType )
        return HRESULT_FROM_WIN32( ERROR_FILE_NOT_FOUND );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: GetFormat()
// Desc: Gets the wave file format
//-----------------------------------------------------------------------------
HRESULT CWaveFile::GetFormat( WAVEFORMATEX* pwfxFormat, DWORD dwFormatSize, DWORD * pdwRequiredSize )
{
    HRESULT hr = S_OK;
    DWORD dwValidSize = m_FormatChunk.GetDataSize();

    // We should be reading a wave format and/or
    // telling the caller the size of the format
    if( ( NULL == pwfxFormat ||
          0 == dwFormatSize ) && 
        NULL == pdwRequiredSize )
        return E_INVALIDARG;

    if( pwfxFormat && dwFormatSize )
    {
        // Read the format chunk into the buffer
        hr = m_FormatChunk.ReadData( 0, pwfxFormat, min(dwFormatSize, dwValidSize) );
        if( FAILED(hr) )
            return hr;

        // Zero out remaining bytes, in case enough bytes were not read
        if( dwFormatSize > dwValidSize )
            ZeroMemory( (BYTE*)pwfxFormat + dwValidSize, dwFormatSize - dwValidSize );
    }
    
    // Tell caller how much space they need for the format
    if( pdwRequiredSize )
    {
        *pdwRequiredSize = max( dwValidSize, sizeof( *pwfxFormat ) );
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: ReadSample()
// Desc: Reads data from the audio file.
//-----------------------------------------------------------------------------
HRESULT CWaveFile::ReadSample( DWORD dwPosition, VOID* pBuffer, 
                               DWORD dwBufferSize, DWORD* pdwRead )
{                                   
    // Don't read past the end of the data chunk
    DWORD dwDuration;
    GetDuration( &dwDuration );

    if( dwPosition + dwBufferSize > dwDuration )
        dwBufferSize = dwDuration - dwPosition;

    HRESULT hr = S_OK;
    if( dwBufferSize )
        hr = m_DataChunk.ReadData( (LONG)dwPosition, pBuffer, dwBufferSize );

    if( pdwRead )
        *pdwRead = dwBufferSize;

    return hr;
}




//-----------------------------------------------------------------------------
// Name: GetLoopRegion
// Desc: Gets the loop region, in terms of samples
//-----------------------------------------------------------------------------
HRESULT CWaveFile::GetLoopRegion( DWORD *pdwStart, DWORD *pdwLength )
{
    HRESULT hr = S_OK;
    WAVESAMPLE ws;
    WAVESAMPLE_LOOP wsl;

    // Check arguments
    if( NULL == pdwStart || NULL == pdwLength )
        return E_INVALIDARG;

    // Check to see if there was a wave sample chunk
    if( !m_WaveSampleChunk.IsValid() )
        return E_FAIL;
    
    // Read the WAVESAMPLE struct from the chunk
    hr = m_WaveSampleChunk.ReadData( 0, &ws, sizeof( WAVESAMPLE ) );
    if( FAILED( hr ) )
        return hr;

    // Currently, only 1 loop region is supported
    if( ws.cSampleLoops != 1 )
        return E_FAIL;

    // Read the loop region
    hr = m_WaveSampleChunk.ReadData( ws.cbSize, &wsl, sizeof( WAVESAMPLE_LOOP ) );
    if( FAILED( hr ) )
        return hr;
    
    // Fill output vars with the loop region
    *pdwStart = wsl.ulLoopStart;
    *pdwLength = wsl.ulLoopLength;

    return S_OK;
}
    



//-----------------------------------------------------------------------------
// Name: Close()
// Desc: Closes the object
//-----------------------------------------------------------------------------
VOID CWaveFile::Close()
{
    if( m_hFile != INVALID_HANDLE_VALUE )
    {
        CloseHandle( m_hFile );
        m_hFile = INVALID_HANDLE_VALUE;
    }
}




//-----------------------------------------------------------------------------
// Name: CXBSound()
// Desc: 
//-----------------------------------------------------------------------------
CXBSound::CXBSound()
{
    m_pDSoundBuffer = NULL;
    m_dwBufferSize  = 0L;
}




//-----------------------------------------------------------------------------
// Name: ~CXBSound()
// Desc: 
//-----------------------------------------------------------------------------
CXBSound::~CXBSound()
{
    Destroy();
}




//-----------------------------------------------------------------------------
// Name: Create()
// Desc: Creates the sound. Sound is buffered to memory allocated internally
//       by DirectSound.
//-----------------------------------------------------------------------------
HRESULT CXBSound::Create( const CHAR* strFileName, DWORD dwFlags )
{
    // Find the media file
//    CHAR strWavePath[512];
    HRESULT   hr;
    //if( FAILED( hr = XBUtil_FindMediaFile( strWavePath, strFileName ) ) )
        //return hr;

    // Open the .wav file
    CWaveFile waveFile;
    //hr = waveFile.Open( strWavePath );
    hr = waveFile.Open( strFileName );
    if( FAILED(hr) )
        return hr;

    // Get the WAVEFORMAT structure for the .wav file
    hr = waveFile.GetFormat( &m_WaveFormat, sizeof(WAVEFORMATEX) );
    if( FAILED(hr) )
        return hr;

    // Get the size of the .wav file
    waveFile.GetDuration( &m_dwBufferSize );

    // Create the sound buffer
    hr = Create( &m_WaveFormat, dwFlags, NULL, m_dwBufferSize );
    if( FAILED(hr) )
        return hr;

    // Lock the buffer so it can be filled
    VOID* pLock1 = NULL;
    VOID* pLock2 = NULL;
    DWORD dwLockSize1 = 0L;
    DWORD dwLockSize2 = 0L;
    hr = m_pDSoundBuffer->Lock( 0L, m_dsbd.dwBufferBytes, &pLock1, &dwLockSize1, 
                                &pLock2, &dwLockSize2, 0L );
    if( FAILED(hr) )
        return hr;

    // Read the wave file data into the buffer
    hr = waveFile.ReadSample( 0L, pLock1, dwLockSize1, NULL );
    if( FAILED(hr) )
        return hr;

    // Unlock the buffer
    hr = m_pDSoundBuffer->Unlock( &pLock1, dwLockSize1, &pLock2, dwLockSize2 );
    if( FAILED(hr) )
        return hr;

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: Create()
// Desc: Creates the sound and tells DirectSound where the sound data will be
//       stored. If pBuffer is NULL, DirectSound handles buffer creation.
//-----------------------------------------------------------------------------
HRESULT CXBSound::Create( const WAVEFORMATEX* pwfxFormat, DWORD dwFlags,
                          const VOID* pBuffer, DWORD dwBytes )
{
    // Setup the sound buffer description
    ZeroMemory( &m_dsbd, sizeof(DSBUFFERDESC) );
    m_dsbd.dwSize      = sizeof(DSBUFFERDESC);
    m_dsbd.dwFlags     = dwFlags;
    m_dsbd.lpwfxFormat = (LPWAVEFORMATEX)pwfxFormat;

    // If pBuffer is non-NULL, dwBufferBytes will be zero, which informs
    // DirectSoundCreateBuffer that we will presently be using SetBufferData().
    // Otherwise, we set dwBufferBytes to the size of the WAV data, potentially
    // including alignment bytes.
    if( pBuffer == NULL )
    {
        m_dsbd.dwBufferBytes = ( 0 == m_WaveFormat.nBlockAlign ) ? dwBytes : 
                                 dwBytes - ( dwBytes % m_WaveFormat.nBlockAlign );
    }

    HRESULT hr = DirectSoundCreateBuffer(&m_dsbd, &m_pDSoundBuffer);
    if( FAILED(hr) )
        return hr;

    // If buffer specified, tell DirectSound to use it
    if( pBuffer != NULL )
    {
        hr = m_pDSoundBuffer->SetBufferData( (LPVOID)pBuffer, dwBytes );
        if( FAILED(hr) )
            return hr;
    }

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: Destroy()
// Desc: Destroys the resources used by the sound
//-----------------------------------------------------------------------------
VOID CXBSound::Destroy()
{
    SAFE_RELEASE( m_pDSoundBuffer );
}




//-----------------------------------------------------------------------------
// Name: Play()
// Desc: Plays the sound
//-----------------------------------------------------------------------------
HRESULT CXBSound::Play( DWORD dwFlags ) const
{
    if( NULL == m_pDSoundBuffer )
        return E_INVALIDARG;
        
    return m_pDSoundBuffer->Play( 0, 0, dwFlags );
}



HRESULT CXBSound::GetStatus( LPDWORD pdwStatus ) const
{
	return m_pDSoundBuffer->GetStatus( pdwStatus ) ;
}

HRESULT CXBSound::SetVolume( LONG lValue ) const
{
	return m_pDSoundBuffer->SetVolume( lValue ) ;
}

//-----------------------------------------------------------------------------
// Name: Stop()
// Desc: Stops the sound
//-----------------------------------------------------------------------------
HRESULT CXBSound::Stop() const
{
    if( NULL == m_pDSoundBuffer )
        return E_INVALIDARG;
        
    return m_pDSoundBuffer->Stop();
}




//-----------------------------------------------------------------------------
// Name: SetPosition()
// Desc: Positions the sound
//-----------------------------------------------------------------------------
HRESULT CXBSound::SetPosition( const D3DXVECTOR3& v ) const
{
    if( NULL == m_pDSoundBuffer )
        return E_INVALIDARG;
        
    return m_pDSoundBuffer->SetPosition( v.x, v.y, v.z, DS3D_IMMEDIATE );
}




//-----------------------------------------------------------------------------
// Name: SetVelocity()
// Desc: Sets the sound's velocity
//-----------------------------------------------------------------------------
HRESULT CXBSound::SetVelocity( const D3DXVECTOR3& v ) const
{
    if( NULL == m_pDSoundBuffer )
        return E_INVALIDARG;
        
    return m_pDSoundBuffer->SetVelocity( v.x, v.y, v.z, DS3D_IMMEDIATE );
}



//-----------------------------------------------------------------------------
// Name: CDSPImage (constructor)
// Desc: Initializes member variables
//-----------------------------------------------------------------------------
CDSPImage::CDSPImage()
{
    m_dwImageSize = 0;
    m_pbImageData = NULL;
}




//-----------------------------------------------------------------------------
// Name: ~CDSPImage (destructor)
// Desc: Releases memory held by the object
//-----------------------------------------------------------------------------
CDSPImage::~CDSPImage()
{
    delete[] m_pbImageData;
}


//-----------------------------------------------------------------------------
// Name: LoadFromFile
// Desc: Loads the DSP image from the given file.  Searches in the media 
//       directory if it can't find the file as specified.
//-----------------------------------------------------------------------------
HRESULT 
CDSPImage::LoadFromFile( char * szFilename )
{
    HRESULT hr = S_OK;
    char szImagePath[MAX_PATH];
    HANDLE hFile = INVALID_HANDLE_VALUE;

    // Find the media file
    hr = XBUtil_FindMediaFile( szImagePath, szFilename );
    if( FAILED( hr ) )
        return hr;

    // open scratch image file generated by XGPImage tool
    hFile = CreateFile( szImagePath,
                        GENERIC_READ,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL );
    if( INVALID_HANDLE_VALUE == hFile )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        return hr;
    }

    // Determine the size of the scratch image by seeking to
    // the end of the file
    m_dwImageSize = SetFilePointer( hFile, 0, NULL, FILE_END );
    SetFilePointer(hFile, 0, NULL, FILE_BEGIN);

    // Delete and re-allocate buffer
    delete[] m_pbImageData;
    m_pbImageData = new BYTE[ m_dwImageSize ];

    // Read the image into memory
    DWORD dwBytesRead;
    BOOL bResult = ReadFile( hFile,
                             m_pbImageData,
                             m_dwImageSize,
                             &dwBytesRead,
                             0 );

    if( !bResult || dwBytesRead != m_dwImageSize )
    {
        hr = HRESULT_FROM_WIN32( GetLastError() );
        return hr;
    }

    CloseHandle( hFile );

    return S_OK;
}




//-----------------------------------------------------------------------------
// Name: DownloadImage
// Desc: Downloads the DSP image by calling DownloadEffectsImage on the 
//       given DSound object
//-----------------------------------------------------------------------------
HRESULT 
CDSPImage::DownloadImage( LPDIRECTSOUND8 pDSound )
{
    HRESULT hr = S_OK;
    LPDSEFFECTIMAGEDESC pDesc;

    // Use a copy of the buffer so we can keep the original intact
    BYTE * pBuffer = new BYTE[ m_dwImageSize ];
    memcpy( pBuffer, m_pbImageData, m_dwImageSize );

    // call dsound API to download the image..
    hr = pDSound->DownloadEffectsImage( pBuffer,
                                        m_dwImageSize,
                                        NULL,
                                        &pDesc );
    // delete our modified copy
    delete[] pBuffer;

    return hr;
}
