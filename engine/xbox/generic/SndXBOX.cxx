//============================================================================
//
//   SSSS    tt          lll  lll       
//  SS  SS   tt           ll   ll        
//  SS     tttttt  eeee   ll   ll   aaaa 
//   SSSS    tt   ee  ee  ll   ll      aa
//      SS   tt   eeeeee  ll   ll   aaaaa  --  "An Atari 2600 VCS Emulator"
//  SS  SS   tt   ee      ll   ll  aa  aa
//   SSSS     ttt  eeeee llll llll  aaaaa
//
// Copyright (c) 1995-1998 by Bradford W. Mott
//
// See the file "license" for information on usage and redistribution of
// this file, and for a DISCLAIMER OF ALL WARRANTIES.
//
// $Id: SndXBOX.cxx,v 1.1.1.1 2001/12/27 19:54:32 bwmott Exp $
//============================================================================


#include "SndXBOX.hxx"
#include <stdio.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

int get_mixingactive();
unsigned char* updatemixing_xbox( unsigned int todo );
void writexbox( char *msg ) ;

#ifdef __cplusplus
}
#endif

// the local buffer is what the stream buffer feeds from
// note that this needs to be large enough to buffer at frameskip 11
// for 30fps games like Tapper; we will scale the value down based
// on the actual framerate of the game
#define MAX_BUFFER_SIZE			(128 * 1024 * 4)

// this is the maximum number of extra samples we will ask for
// per frame (I know this looks like a lot, but most of the
// time it will generally be nowhere close to this)
#define MAX_SAMPLE_ADJUST		16

extern DWORD g_dwStartTime ;
extern DWORD g_dwTimePaused ;

/**
  Compute the buffer size to use based on the given sample rate

  @param The sample rate to compute the buffer size for
*/
static unsigned long computeBufferSize(int sampleRate)
{
  int t;

  for(t = 7; t <= 12; ++t)
  {
    if((1 << t) > (sampleRate / 60))
    {
      return (1 << (t - 1));
    }
  }

  return 256;
}

// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundXBOX::SoundXBOX()
{
	dsound = NULL ;
	stream_buffer = NULL ;
	attenuation = 0 ;
	samples_to_read = 0 ;
	m_fps = 60 ;


}
 
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
SoundXBOX::~SoundXBOX()
{
}

int SoundXBOX::dsound_init(void)
{
	HRESULT result;


	current_adjustment = 0 ;
	m_totalBytesWritten = 0 ;

	// make a format description for what we want
	stream_format.wBitsPerSample	= 16;
	stream_format.wFormatTag		= WAVE_FORMAT_PCM;
	stream_format.nChannels			= 1;
	stream_format.nSamplesPerSec	= 44100;
	stream_format.nBlockAlign		= stream_format.wBitsPerSample * stream_format.nChannels / 8;
	stream_format.nAvgBytesPerSec	= stream_format.nSamplesPerSec * stream_format.nBlockAlign;

	// compute the buffer sizes
	stream_buffer_size = (UINT64)MAX_BUFFER_SIZE ;
//	stream_buffer_size = ((UINT64)MAX_BUFFER_SIZE * (UINT64)stream_format.nSamplesPerSec) / 44100;
	stream_buffer_size = (stream_buffer_size * stream_format.nBlockAlign) / 4;
	stream_buffer_size = (stream_buffer_size * 30) / 60 ;
	//stream_buffer_size = (stream_buffer_size / 1024) * 1024;
	//stream_buffer_size = (stream_buffer_size / 2940) * 2940;
	stream_buffer_size = 2940*6;

	stream_buffer_in = 0 ;

	DSMIXBINVOLUMEPAIR dsmbvp[8] = {
		{DSMIXBIN_FRONT_LEFT, DSBVOLUME_MAX},   // left channel
		{DSMIXBIN_FRONT_RIGHT, DSBVOLUME_MAX},  // right channel
		{DSMIXBIN_FRONT_CENTER, DSBVOLUME_MAX}, // left channel
		{DSMIXBIN_FRONT_CENTER, DSBVOLUME_MAX}, // right channel
		{DSMIXBIN_BACK_LEFT, DSBVOLUME_MAX},    // left channel
		{DSMIXBIN_BACK_RIGHT, DSBVOLUME_MAX},   // right channel
		{DSMIXBIN_LOW_FREQUENCY, DSBVOLUME_MAX},    // left channel
		{DSMIXBIN_LOW_FREQUENCY, DSBVOLUME_MAX}};   // right channel
		
	DSMIXBINS dsmb;
	dsmb.dwMixBinCount = 8;
	dsmb.lpMixBinVolumePairs = dsmbvp;
	
	// create a buffer desc for the stream buffer
	stream_desc.dwSize				= sizeof(stream_desc);
	stream_desc.dwFlags				= 0 ;
	stream_desc.dwBufferBytes 		= 0; //we'll specify our own data
//	stream_desc.dwBufferBytes 		= stream_buffer_size;
	stream_desc.lpwfxFormat			= &stream_format;
	stream_desc.lpMixBins			= &dsmb;
	
	// create the stream buffer
	if ((result = dsound->CreateSoundBuffer(&stream_desc, &stream_buffer, NULL)) != DS_OK)
	{
		stream_buffer = NULL ;
		return 1 ;
	}
	
	m_pSoundBufferData = (byte*)malloc( stream_buffer_size ) ;

	stream_buffer->SetBufferData( m_pSoundBufferData, stream_buffer_size ) ;
	stream_buffer->SetPlayRegion( 0, stream_buffer_size ) ;
	stream_buffer->SetLoopRegion( 0, stream_buffer_size ) ;

	memset( m_pSoundBufferData, 0, stream_buffer_size);
	
	//m_mixbuf = (signed short*)malloc( ( (44100/60) * 2 * 2 ) + 100 ) ;

	stream_buffer->SetVolume( DSBVOLUME_MAX );
	stream_buffer->SetCurrentPosition( 0 ) ;

	m_dwWritePos = 0 ;
	return 0 ;
}


void SoundXBOX::init( )
{
	m_totalBytesWritten = 0 ;
	m_dwWritePos = 0 ;
	//m_dwWritePos = stream_buffer_size - 35025 ; //half second ahead
	memset( m_pSoundBufferData, 0, stream_buffer_size ) ;
	stream_buffer->SetVolume( DSBVOLUME_MAX );
	DWORD dwStatus;

	stream_buffer->StopEx( 0, DSBSTOPEX_IMMEDIATE );
	do
	{
		stream_buffer->GetStatus( &dwStatus );
	} while( dwStatus & DSBSTATUS_PLAYING );
	stream_buffer->SetCurrentPosition( 0 ) ;
	m_bDanger = 0 ;
	m_dwOldTick = GetTickCount() ;
	m_nVolume = DSBVOLUME_MAX ;
}

void SoundXBOX::adjust_volume( int volchange ) 
{
	m_nVolume += volchange ;

	if ( m_nVolume > DSBVOLUME_MAX )
		m_nVolume = DSBVOLUME_MAX ;
	if ( m_nVolume < DSBVOLUME_MIN )
		m_nVolume = DSBVOLUME_MIN ;

	stream_buffer->SetVolume( m_nVolume ) ;

}

void SoundXBOX::cleanup()
{
	DWORD dwStatus;

	stream_buffer->SetVolume( DSBVOLUME_MIN );

	stream_buffer->StopEx( 0, DSBSTOPEX_IMMEDIATE );
	do
	{
		stream_buffer->GetStatus( &dwStatus );
	} while( dwStatus & DSBSTATUS_PLAYING );
}
void SoundXBOX::insertSilence( int samples  )
{
	unsigned int            datalen ;
	DWORD          bytesToEnd ;


	return ;

	datalen = samples ;


	bytesToEnd = stream_buffer_size - m_dwWritePos ;

	if ( datalen > bytesToEnd )
	{
		memset( m_pSoundBufferData + m_dwWritePos, 0, bytesToEnd ) ;
		memset( m_pSoundBufferData, 0, datalen - bytesToEnd ) ;
	}
	else
	{
		memset( m_pSoundBufferData + m_dwWritePos, 0, datalen ) ;
	}

	m_dwWritePos = ( m_dwWritePos + datalen ) % stream_buffer_size ;

	m_totalBytesWritten += datalen ;



}

int SoundXBOX::process( int isThrottled )
{
	static int firsttime = 1 ;
	static FILE *outfile ;
	unsigned char *sndbuf ;
	unsigned char mybuf[6*4096] ;
	short *src ;
	short *dst ;
	int            datalen ;
	DWORD          playPos  ;
	DWORD          bytesToEnd ;
	DWORD          distWritePlay ;




	//if ( firsttime )
	//{
		//firsttime = 0 ;
		//outfile = fopen( "d:\\out.wav", "wb" ) ;
	//}

	if ( get_mixingactive() == 0 )
		return 0 ;
	
	stream_buffer->GetCurrentPosition( &playPos, NULL ) ;

	if ( m_dwWritePos > playPos )
	{
		distWritePlay = m_dwWritePos - playPos ;
	}
	else
	{
		distWritePlay = ( stream_buffer_size - playPos ) + m_dwWritePos ;
	}

	if ( distWritePlay > 4096*3 ) 
		return 0 ;


	datalen = 4096 ; 

	sndbuf = updatemixing_xbox( datalen ) ;

	if ( sndbuf == NULL )
		return 0 ;

	src = (short*)sndbuf ;
	dst = (short*)mybuf ;

	for ( int i = 0 ; i < datalen/2 ; i++ )
	{
		*dst++ = (((*src++)*-1)<<1) ;
		//*dst++ = *src++;
	}

	sndbuf = (unsigned char*)mybuf ;

	//fwrite( sndbuf, 2940, 1, outfile ) ;
	//fflush(outfile) ;

	bytesToEnd = stream_buffer_size - m_dwWritePos ;

	if ( datalen > bytesToEnd )
	{
		memcpy( m_pSoundBufferData + m_dwWritePos, sndbuf, bytesToEnd ) ;
		memcpy( m_pSoundBufferData, sndbuf + bytesToEnd, datalen - bytesToEnd ) ;
	}
	else
	{
		memcpy( m_pSoundBufferData + m_dwWritePos, sndbuf, datalen ) ;
	}

	m_dwWritePos = ( m_dwWritePos + datalen ) % stream_buffer_size ;

	m_totalBytesWritten += datalen ;

	
	if ( distWritePlay < datalen ) 
	{
		//m_bDanger = 1 ;
		return 1 ;
	}

	m_bDanger = 0 ;
	
	return 0 ;
}
/*
int SoundXBOX::process( int isThrottled )
{
	unsigned char sndbuf[4*1024] ;
	int            num_written ;
	int            datalen ;
	float          numtowrite_f ;
	int            numtowrite ;
	float          waittime ;
	DWORD          waittime_ms ;
	float          elapsedTime ;
	DWORD          playPos  ;
	DWORD          bytesToEnd ;
	DWORD          distWritePlay ;
	DWORD          newWritePos ;




	datalen = 526 ; 

	
	do
	{
		stream_buffer->GetCurrentPosition( &playPos, NULL ) ;

		if ( m_dwWritePos > playPos )
		{
			distWritePlay = m_dwWritePos - playPos ;
		}
		else
		{
			distWritePlay = ( stream_buffer_size - playPos ) + m_dwWritePos ;
		}
	} while ( ( distWritePlay > 1500 ) && ( !isThrottled ) );


	bytesToEnd = stream_buffer_size - m_dwWritePos ;

	if ( datalen > bytesToEnd )
	{
		memcpy( m_pSoundBufferData + m_dwWritePos, sndbuf, bytesToEnd ) ;
		memcpy( m_pSoundBufferData, sndbuf + bytesToEnd, datalen - bytesToEnd ) ;
	}
	else
	{
		memcpy( m_pSoundBufferData + m_dwWritePos, sndbuf, datalen ) ;
	}

	m_dwWritePos = ( m_dwWritePos + datalen ) % stream_buffer_size ;

	m_totalBytesWritten += datalen ;

	
	if ( distWritePlay < 5000 ) 
	{
		//m_bDanger = 1 ;
		return 1 ;
	}

	m_bDanger = 0 ;
	
	return 0 ;
}
*/
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
void SoundXBOX::pause(bool state)
{
	DWORD dwStatus;

	if (stream_buffer)
	{
		if (state)
		{
			stream_buffer->StopEx( 0, DSBSTOPEX_IMMEDIATE );
			do
			{
				stream_buffer->GetStatus( &dwStatus );
			} while( dwStatus & DSBSTATUS_PLAYING );
		}
		else
			stream_buffer->Play( 0, 0, DSBPLAY_LOOPING ) ;
	}
}

