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
// $Id: SndXBOX.hxx,v 1.1.1.1 2001/12/27 19:54:32 bwmott Exp $
//============================================================================

#ifndef SOUNDXBOX_HXX
#define SOUNDXBOX_HXX

#ifdef __cplusplus
extern "C" {
#endif


#ifdef __cplusplus
}
#endif


#include <XBApp.h>
#include <Dsound.h>

/**
  This class implements the sound API for the XBOX operating system
  using a sound-blaster card.

  @author  Bradford W. Mott
  @version $Id: SndXBOX.hxx,v 1.1.1.1 2001/12/27 19:54:32 bwmott Exp $
*/
class SoundXBOX 
{
  public:
    /**
      Create a new sound object
    */
    SoundXBOX();
	void init( ) ;
	void cleanup() ;
	int process( int isThrottled ) ;
	int dsound_init() ;
	void insertSilence( int samples ) ;
	void dsound_destroy_buffers() ;
	int dsound_create_buffers() ;
	int osd_start_audio_stream(int stereo) ;
	void osd_stop_audio_stream(void) ;
	void osd_set_mastervolume(int _attenuation) ;
	void copy_sample_data( unsigned char *data, int bytes_to_copy) ;
	int osd_update_audio_stream(unsigned char *buffer) ;
	int bytes_in_stream_buffer(void) ;
	void update_sample_adjustment(int buffered) ;
    virtual void pause(bool state);
	void adjust_volume( int volchange ) ;

    /**
      Destructor
    */
    virtual ~SoundXBOX();

  public: 
	int							attenuation ;
	int					current_adjustment ;
	int					m_fps ;
	FILE				*sndfile ;
// DirectSound objects
	LPDIRECTSOUND8		dsound ;

// global sample tracking
	double				samples_per_frame;
	double				samples_left_over;
	UINT32				samples_this_frame;
	UINT32              samples_to_read ;
	signed short		*m_mixbuf ;
	int					m_bDanger ;
	int					m_nVolume ;

// sound buffers
	LPDIRECTSOUNDBUFFER8	stream_buffer ;
	UINT32				stream_buffer_size;
	UINT32				stream_buffer_in;
	UINT32				m_totalBytesWritten ;
	CXBApplication      *m_ptrapp ;
	byte				*m_pSoundBufferData ;
	DWORD				m_dwWritePos ;
	DWORD               m_dwOldTick ;
	DWORD               m_dwNewTick ;

// descriptors and formats
	DSBUFFERDESC			stream_desc;
	WAVEFORMATEX			stream_format;

// sample rate adjustments
	int					lower_thresh;
	int					upper_thresh;


  private:
    // Indicates if the sound system was initialized
    bool myEnabled;
};
#endif

