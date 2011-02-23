/*
**  Sound mixer.
**  High quality, with support for ADPCM-compressed music.
**
**  Also plays WAV files (both 8-bit and 16-bit).
**  Note: 8-bit wavs are unsigned, 16-bit wavs are signed!!!
**
**
**  Function naming convention:
**  - Public functions start with "sound_"
**  - Music-related functions end with "_music"
**  - Soundeffect-related functions end with "_sample"
**
**
**  To do:
**  - I think it's stable now, but stay alert!
**  - test 16-bit soundfx
*/

#include "ps2port.h"

#include "adpcm.h"
#include "ps2sdr.h"
#include "packfile.h"


/***************************************************************************/
/*
** Static information
*/

#define    AUDIOCIDE_VERSION  "2.00"
#define    MIXSHIFT    2  // 2 should be OK (was 3)
#define    MAXVOLUME    64  // 64 for backw. compat.
#define    MAX_SAMPLES    256  // Should be well enough
#define    MAX_CHANNELS    64  // Should be well enough



// Hardware settings for SoundBlaster (change only if latency is too big)
#define    SB_BUFFER_SIZE    0x8000
#define    SB_BUFFER_SIZE_MASK  0x7FFF
#define    SB_WBUFFER_SIZE    0x4000
#define    SB_WBUFFER_SIZE_MASK  0x3FFF
#define    MIXBUF_SIZE    SB_BUFFER_SIZE*8
#define    PREMIX_SIZE    1024
#define    MIX_BLOCK_SIZE    32




// 20:12 fixed-point conversion macros.
// The maximum size of a sound is linked directly
// to the range of the fixed-point variables!
#define    INT_TO_FIX(i)    (((unsigned)(i))<<12)
#define    FIX_TO_INT(f)    (((unsigned)(f))>>12)
#define    MAX_SOUND_LEN    0xFFFFF



#define    CHANNEL_PLAYING    1
#define    CHANNEL_LOOPING    2



#define    MUSIC_NUM_BUFFERS  4
#define    MUSIC_BUF_SIZE    (16*1024)  // In samples




#pragma pack(4)

typedef struct{
  int      active;    // 1 = play, 2 = loop
  int      samplenum;  // Index of sound playing
  unsigned int    priority;  // Used for SFX
  int      volume[2];  // Stereo :)
  unsigned gamelib_long  fp_samplepos;  // Position (fixed-point)
  unsigned gamelib_long  fp_period;  // Period (fixed-point)
}channelstruct;

typedef struct{
  void *      sampleptr;
  int      soundlen;  // Length in samples
  int      bits;    // 8/16 bit
  int      frequency;  // 11025 * 1,2,4
}samplestruct;

typedef struct{
  int      active;
  int      paused;
  short *      buf[MUSIC_NUM_BUFFERS];
  unsigned gamelib_long  fp_playto[MUSIC_NUM_BUFFERS];
  unsigned gamelib_long  fp_samplepos;  // Position (fixed-point)
  unsigned gamelib_long  fp_period;  // Period (fixed-point)
  int      playing_buffer;
  int      volume[2];
}musicchannelstruct;



static channelstruct vchannel[MAX_CHANNELS];
static musicchannelstruct musicchannel;
static samplestruct sampledata[MAX_SAMPLES];
//static samplestruct musicdata;

static signed int *mixbuf = NULL;

static int playstereo;    // 0 / 1
static int playbits;
static int playfrequency;

static int max_channels = 0;

// Indicates wether the hardware is playing, and if mixing is active
static int mixing_active = 0;

// Indicates wether the sound system is initialized
static int mixing_inited = 0;

// Counts the total number of samples played
static unsigned gamelib_long samplesplayed;






//////////////////////////////// WAVE LOADER //////////////////////////////////


#define    HEX_RIFF  0x46464952
#define    HEX_WAVE  0x45564157
#define    HEX_fmt    0x20746D66
#define    HEX_data  0x61746164

#define    FMT_PCM    0x0001

static int loadwave(char *filename, char *packname, samplestruct *buf, unsigned int maxsize){
  struct{
    unsigned gamelib_long  riff;
    unsigned gamelib_long  size;
    unsigned gamelib_long  type;
  }riffheader;
  struct{
    unsigned gamelib_long  tag;
    unsigned gamelib_long  size;
  }rifftag;
  struct{
    unsigned short  format;    // 1 = PCM
    unsigned short  channels;  // Mono, stereo
    unsigned gamelib_long  samplerate;  // 11025, 22050, 44100
    unsigned gamelib_long  bps;    // Bytes/second
    unsigned short  unknown;
    unsigned short  samplebits;  // 8, 12, 16
  }fmt;

  int handle;
  int mulbytes;


  if(buf==NULL) return 0;

  if((handle=openpackfile(filename, packname))==-1) return 0;
  if(readpackfile(handle, &riffheader, sizeof(riffheader)) != sizeof(riffheader)){
    closepackfile(handle);
    return 0;
  }
  if(riffheader.riff!=HEX_RIFF || riffheader.type!=HEX_WAVE){
    closepackfile(handle);
    return 0;
  }

  rifftag.tag = 0;
  // Search for format tag
  while(rifftag.tag!=HEX_fmt){
    if(readpackfile(handle, &rifftag, sizeof(rifftag))!=sizeof(rifftag)){
      closepackfile(handle);
      return 0;
    }
    if(rifftag.tag!=HEX_fmt) seekpackfile(handle,rifftag.size,SEEK_CUR);
  }
  if(readpackfile(handle, &fmt, sizeof(fmt))!=sizeof(fmt)){
    closepackfile(handle);
    return 0;
  }
  if(rifftag.size>sizeof(fmt)) seekpackfile(handle,rifftag.size-sizeof(fmt),SEEK_CUR);

  if(fmt.format!=FMT_PCM || fmt.channels!=1 || (fmt.samplebits!=8 && fmt.samplebits!=16)){
    closepackfile(handle);
    return 0;
  }
  mulbytes = (fmt.samplebits==16 ? 2 : 1);


  // Search for data tag
  while(rifftag.tag!=HEX_data){
    if(readpackfile(handle, &rifftag, sizeof(rifftag))!=sizeof(rifftag)){
      closepackfile(handle);
      return 0;
    }
    if(rifftag.tag!=HEX_data) seekpackfile(handle,rifftag.size,SEEK_CUR);
  }

  if(rifftag.size<maxsize) maxsize = rifftag.size;
  if((buf->sampleptr=(char*)tracemalloc("loadwave",maxsize*mulbytes+8))==NULL){
    closepackfile(handle);
    return 0;
  }
  if(fmt.samplebits==8) memset(buf->sampleptr, 0x80, maxsize*mulbytes+8);
  else memset(buf->sampleptr, 0, maxsize*mulbytes+8);

  if(readpackfile(handle, buf->sampleptr, maxsize*mulbytes) != maxsize*mulbytes){
    tracefree(buf->sampleptr);
    closepackfile(handle);
    return 0;
  }

  closepackfile(handle);

  buf->soundlen = maxsize;
  buf->bits = fmt.samplebits;
  buf->frequency = fmt.samplerate;

  return maxsize;
}




void sound_unload_sample(int which){

  if(!mixing_inited) return;
  if(which<0 || which>=MAX_SAMPLES) return;

  if(sampledata[which].sampleptr){
    tracefree(sampledata[which].sampleptr);
    memset(&sampledata[which], 0, sizeof(samplestruct));
  }
}


// Returns index of sample, or -1 if not loaded
int sound_load_sample(char *filename, char *packfilename){

  int i;

  if(!mixing_inited) return -1;

  // Search for available slot to store sample data
  for(i=0; i<MAX_SAMPLES; i++){
    if(sampledata[i].sampleptr==NULL) break;
  }
  if(i>=MAX_SAMPLES) return -1;

  memset(&sampledata[i], 0, sizeof(samplestruct));

  if(!loadwave(filename, packfilename, &sampledata[i], MAX_SOUND_LEN)) return -1;

  return i;
}




/////////////////////////////// Mix to DMA //////////////////////////////////
// Mixbuffer / DMA buffer data handling
// Writes mixbuffer data (16-bit mixed in 32-bit array)
// to 8-bit or 16-bit DMA buffer.


// Fill the mixbuffer with silence
static void clearmixbuffer(unsigned int * buf, int n){
  while((--n)>=0){
    *buf = 0;
    ++buf;
  }
}



/////////////////////////////////// Mixers ///////////////////////////////////
// Mixers: mix (16-bit) in the mixbuffer, then write to DMA memory (see above).
// The mixing code handles fixed-point conversion and looping.

// Parms: number of samples to output (always even, since it's stereo!)
static void mixstereo(unsigned int todo){

  static int i, chan, lvolume, rvolume;
  static unsigned gamelib_long fp_pos, fp_period, fp_len, fp_playto;
  static int snum;
  static int m, m2;
  static char *sptr8;
  static short *sptr16;

  static int adpcm_prev = 0;

//  todo &= 0xFFFE;

  // First 'mix' the music, if playing
  if(musicchannel.active && !musicchannel.paused){

    sptr16 = musicchannel.buf[musicchannel.playing_buffer];
    fp_playto = musicchannel.fp_playto[musicchannel.playing_buffer];
    fp_pos = musicchannel.fp_samplepos;
    fp_period = musicchannel.fp_period;
    lvolume = musicchannel.volume[0];
    rvolume = musicchannel.volume[1];

    for(i=0; i<todo; ){

      // Reached end of playable area,
      // switch buffers or stop
      if(fp_pos >= fp_playto){
        // Done playing this one
        musicchannel.fp_playto[musicchannel.playing_buffer] = 0;
        // Advance to next buffer
        musicchannel.playing_buffer++;
        musicchannel.playing_buffer %= MUSIC_NUM_BUFFERS;
        // Correct position in next buffer
        fp_pos = fp_pos - fp_playto;
        // Anything to play?
        if(fp_pos < musicchannel.fp_playto[musicchannel.playing_buffer]){
          // Yeah, switch!
          sptr16 = musicchannel.buf[musicchannel.playing_buffer];
          fp_playto = musicchannel.fp_playto[musicchannel.playing_buffer];
        }
        else{
          // Nothing more to do
          musicchannel.fp_playto[musicchannel.playing_buffer] = 0;
          fp_pos = 0;
          musicchannel.active = 0;
          break;
        }
      }

      m = adpcm_prev;
      m2 = sptr16[FIX_TO_INT(fp_pos)];
      if(FIX_TO_INT(fp_pos) != FIX_TO_INT((fp_pos + fp_period))) adpcm_prev = m2;

      m += ((m2 - m) * ((signed int)(fp_pos & 0xFFF))) >> 12;

      mixbuf[i++] += m * lvolume / MAXVOLUME;
      mixbuf[i++] += m * rvolume / MAXVOLUME;
      fp_pos += fp_period;
    }
    musicchannel.fp_samplepos = fp_pos;
  }


  for(chan=0; chan<max_channels; chan++){
    if(vchannel[chan].active){
      unsigned modpos, modlen;
      snum = vchannel[chan].samplenum;
      modlen = sampledata[snum].soundlen;
      fp_len = INT_TO_FIX(sampledata[snum].soundlen);
      fp_pos = vchannel[chan].fp_samplepos;
      fp_period = vchannel[chan].fp_period;
      lvolume = vchannel[chan].volume[0];
      rvolume = vchannel[chan].volume[1];
      if(fp_len < 1) fp_len = 1;
      if(modlen < 1) modlen = 1;
      if(sampledata[snum].bits==8){
        sptr8 = sampledata[snum].sampleptr;
        for(i=0; i<todo; ){

          modpos = FIX_TO_INT(fp_pos);
          m = sptr8[modpos];
          modpos++; if(modpos >= modlen) modpos = 0;
          m2 = sptr8[modpos];
          m &= 0xFF; m -= 0x80;
          m2 &= 0xFF; m2 -= 0x80;
          m  <<= 8;
          m2 <<= 8;

          m += ((m2 - m) * ((signed int)(fp_pos & 0xFFF))) >> 12;

//          m = (signed int)((signed char)(sptr8[FIX_TO_INT(fp_pos)]^0x80));
          mixbuf[i++] += (m * lvolume / MAXVOLUME);
          mixbuf[i++] += (m * rvolume / MAXVOLUME);
          fp_pos += fp_period;

          // Reached end of sample, stop or loop
          if(fp_pos >= fp_len){
            fp_pos %= fp_len; // = INT_TO_FIX(0);
            if(vchannel[chan].active!=CHANNEL_LOOPING){
              vchannel[chan].active = 0;
              break;
            }
          }
        }
      }
      else if(sampledata[snum].bits==16){
        sptr16 = sampledata[snum].sampleptr;
        for(i=0; i<todo; ){
          modpos = FIX_TO_INT(fp_pos);
          m = sptr16[modpos];
          modpos++; if(modpos >= modlen) modpos = 0;
          m2 = sptr16[modpos];

          m += ((m2 - m) * ((int)(fp_pos & 0xFFF))) >> 12;

//          m = sptr16[FIX_TO_INT(fp_pos)];
          mixbuf[i++] += (m * lvolume / MAXVOLUME);
          mixbuf[i++] += (m * rvolume / MAXVOLUME);
          fp_pos += fp_period;

          // Reached end of sample, stop or loop
          if(fp_pos >= fp_len){
            fp_pos %= fp_len; // = INT_TO_FIX(0);
            if(vchannel[chan].active!=CHANNEL_LOOPING){
              vchannel[chan].active = 0;
              break;
            }
          }
        }
      }
      vchannel[chan].fp_samplepos = fp_pos;
    }
  }

}





//////////////////////////////// ISR ///////////////////////////////////
// Called by Soundblaster ISR



static void updatemixing(
  signed short *out_l,
  signed short *out_r,
  int todo
) {
  int i;
  if(!mixing_active) return;

  clearmixbuffer(mixbuf, todo * 2);

  mixstereo(todo * 2);
  samplesplayed += todo;

  for(i = 0; i < todo; i++) {
    int u;
    u = mixbuf[2*i+0]>>MIXSHIFT;
    if(u<(-32768)) u = -32768; else if(u>32767) u = 32767;
    *out_l++ = u;
    u = mixbuf[2*i+1]>>MIXSHIFT;
    if(u<(-32768)) u = -32768; else if(u>32767) u = 32767;
    *out_r++ = u;
  }

}


////////////////////////// Sound effects control /////////////////////////////
// Functions to start, stop, loop, etc.


// Speed in percents of normal.
// Returns channel the sample is played on or -1 if not playing.
int sound_play_sample(int samplenum, unsigned int priority, int lvolume, int rvolume, unsigned int speed){

  int i;
  unsigned int prio_low;
  int channel;


  if(speed<1) speed = 100;
  if(!mixing_inited) return -1;
  if(samplenum>=MAX_SAMPLES) return -1;
  if(!sampledata[samplenum].sampleptr) return -1;


  // Try to find unused SFX channel
  channel = -1;
  for(i=0; i<max_channels; i++){
    if(!vchannel[i].active) channel = i;
  }
  if(channel==-1){
    // Find SFX channel with lowest current priority
    for(i=0, prio_low=0xFFFFFFFF; i<max_channels; i++){
      if(vchannel[i].priority < prio_low){
        channel = i;
        prio_low = vchannel[i].priority;
      }
    }
    if(prio_low > priority) return -1;
  }

  if(lvolume>MAXVOLUME) lvolume = MAXVOLUME;
  if(rvolume>MAXVOLUME) rvolume = MAXVOLUME;
  if(lvolume<0) lvolume = 0;
  if(rvolume<0) rvolume = 0;

  vchannel[channel].samplenum = samplenum;
  // Prevent samples from being played at EXACT same point
  vchannel[channel].fp_samplepos = INT_TO_FIX((channel*4)%sampledata[samplenum].soundlen);
  vchannel[channel].fp_period = (INT_TO_FIX(1) * speed / 100) * sampledata[samplenum].frequency / playfrequency;
  vchannel[channel].volume[0] = lvolume;
  vchannel[channel].volume[1] = rvolume;
  vchannel[channel].priority = priority;
  vchannel[channel].active = CHANNEL_PLAYING;

  return channel;
}



int sound_loop_sample(int samplenum, unsigned int priority, int lvolume, int rvolume, unsigned int speed){
  int ch = sound_play_sample(samplenum, priority, lvolume, rvolume, speed);
  if(ch>=0) vchannel[ch].active = CHANNEL_LOOPING;
  return ch;
}




void sound_stop_sample(int channel){
  if(channel<0 || channel>=max_channels) return;
  vchannel[channel].active = 0;
}



void sound_stopall_sample(){
  int channel;
  for(channel=0; channel<max_channels; channel++){
    vchannel[channel].active = 0;
  }
}



void sound_volume_sample(int channel, int lvolume, int rvolume){
  if(channel<0 || channel>=max_channels) return;
  if(lvolume>MAXVOLUME) lvolume = MAXVOLUME;
  if(rvolume>MAXVOLUME) rvolume = MAXVOLUME;
  if(lvolume<0) lvolume = 0;
  if(rvolume<0) rvolume = 0;
  vchannel[channel].volume[0] = lvolume;
  vchannel[channel].volume[1] = rvolume;
}



int sound_getpos_sample(int channel){
  if(channel<0 || channel>=max_channels) return 0;
  return FIX_TO_INT(vchannel[channel].fp_samplepos);
}







//////////////////////////////// ADPCM music ////////////////////////////////

static int adpcm_handle = -1;
static char *adpcm_inbuf;
static int music_looping = 0;
static int music_atend = 0;


#define    BOR_MUSIC_VERSION    0x00010000
#define    BOR_IDENTIFIER      "BOR music"


#pragma pack (1)
typedef struct{
  char    identifier[16];
  char    artist[64];
  char    title[64];
  unsigned gamelib_long  version;
  int    frequency;
  int    channels;
  int    datastart;
}bor_header;
#pragma pack (4)

static bor_header borhead;

static int openmusicborheadsize = 0;

static int openmusicrequested = 0;
static int openmusicdefaultvolume = 0;
static int openmusicloop = 0;
static int openmusiccompleteflag = 0;

void sound_close_music(){

//  int wait;
  int i;

  // Prevent any further access by the ISR
  musicchannel.active = 0;
  for(i=0; i<MUSIC_NUM_BUFFERS; i++){
    musicchannel.fp_playto[i] = 0;
  }

  // Close file...
  if(adpcm_handle>=0) closepackfile(adpcm_handle);
  adpcm_handle = -1;

  if(adpcm_inbuf) tracefree(adpcm_inbuf);
  adpcm_inbuf = NULL;

  for(i=0; i<MUSIC_NUM_BUFFERS; i++){
    if(musicchannel.buf[i]) tracefree(musicchannel.buf[i]);
    musicchannel.buf[i] = NULL;
  }

  memset(&musicchannel, 0, sizeof(musicchannelstruct));
  memset(&borhead, 0, sizeof(bor_header));

  adpcm_reset();

  openmusicrequested = 0;
  openmusiccompleteflag = 0;
}


int sound_async_open_music(char * filename, char * packname, int defaultvolume, int loop){

  if(!mixing_inited) return 0;
  if(!mixing_active) return 0;

  sound_close_music();

  // Open file, etcetera
  adpcm_handle = openreadaheadpackfile(filename, packname,1536*1024, 0);
  if(adpcm_handle < 0) return 0;

  openmusicdefaultvolume = defaultvolume;
  openmusicloop = loop;

  openmusicrequested = 1;
  openmusicborheadsize = 0;
  return 1;
}

int sound_was_music_opened(void) {
  int f = openmusiccompleteflag;
  openmusiccompleteflag = 0;
  return f;
}

int sound_update_open_music(void) {

  int ofs, sizeleft, r, i;

  if(!openmusicrequested) return 0;

  ofs = openmusicborheadsize;
  sizeleft = sizeof(bor_header) - ofs;

  // Read header
  r = readpackfile_noblock(adpcm_handle, ((char*)(&borhead)) + ofs, sizeleft);

  if(r < 0) r = 0;
  if(r > sizeleft) r = sizeleft;
  openmusicborheadsize += r;

  if(openmusicborheadsize < sizeof(bor_header)) return 0;

  // Is it really a BOR music file?
  if(strncmp(borhead.identifier, BOR_IDENTIFIER, 16)!=0){
    sound_close_music();
    return 0;
  }

  // Can I play it?
  if(borhead.version!=BOR_MUSIC_VERSION || borhead.channels!=1 || borhead.frequency<11025 || borhead.frequency>44100){
    sound_close_music();
    return 0;
  }

  // Seek to beginning of data
  if(seekpackfile(adpcm_handle, borhead.datastart, SEEK_SET) != borhead.datastart){
    sound_close_music();
    return 0;
  }

  memset(&musicchannel, 0, sizeof(musicchannelstruct));

  musicchannel.fp_period = INT_TO_FIX(borhead.frequency) / playfrequency;
  musicchannel.volume[0] = openmusicdefaultvolume;
  musicchannel.volume[1] = openmusicdefaultvolume;
  music_looping = openmusicloop;
  music_atend = 0;

  adpcm_inbuf = (char*)tracemalloc("sound_open_music", MUSIC_BUF_SIZE / 2);
  if(adpcm_inbuf==NULL){
    sound_close_music();
    return 0;
  }

  for(i=0; i<MUSIC_NUM_BUFFERS; i++){
    musicchannel.buf[i] = (short *)tracemalloc("sound_open_music",MUSIC_BUF_SIZE*sizeof(short));
    if(musicchannel.buf[i]==NULL){
      sound_close_music();
      return 0;
    }
    memset(musicchannel.buf[i], 0, MUSIC_BUF_SIZE*sizeof(short));
  }

  // signal completion
  openmusicrequested = 0;
  openmusiccompleteflag = 1;

  return 1;
}




void sound_update_music(){

  int samples, readsamples, samples_to_read;
  short * outptr;
  int i, j;

  sound_update_open_music();
  // if it's still in progress, skip this
  if(openmusicrequested) return;

  if(adpcm_handle<0) return;
  if(!mixing_inited || !mixing_active){
    sound_close_music();
    return;
  }
  if(musicchannel.paused) return;


  // Just to be sure: check if all goes well...
  for(i=0; i<MUSIC_NUM_BUFFERS; i++){
    if(musicchannel.fp_playto[i] > INT_TO_FIX(MUSIC_BUF_SIZE)){
      musicchannel.fp_playto[i] = 0;
      return;
    }
  }


  // Need to update?
  for(j=0, i=musicchannel.playing_buffer+1; j<MUSIC_NUM_BUFFERS; j++, i++){
    i %= MUSIC_NUM_BUFFERS;

    if(musicchannel.fp_playto[i]==0){
      // Buffer needs to be filled

      samples = 0;
      outptr = musicchannel.buf[i];

      if(!music_looping){
        if(music_atend){
          // Close file when done playing all buffers
          if(!musicchannel.active){
            sound_close_music();
            return;
          }
        }
        else{
          if(packfileeof(adpcm_handle)) {
            // EOF
            music_atend = 1;
            return;
          }
          readsamples = readpackfile_noblock(adpcm_handle, adpcm_inbuf, MUSIC_BUF_SIZE/2) * 2;
          // Play this bit
          if(readsamples) adpcm_decoder(adpcm_inbuf, outptr, readsamples);
          samples = readsamples;
        }
      }
      else {
        samples_to_read = MUSIC_BUF_SIZE;
        if(packfileeof(adpcm_handle)){
          // Seek to beginning of data
          if(seekpackfile(adpcm_handle, borhead.datastart, SEEK_SET) != borhead.datastart){
            sound_close_music();
            return;
          }
          // Reset decoder
          adpcm_reset();
        }
        readsamples = readpackfile_noblock(adpcm_handle, adpcm_inbuf, samples_to_read/2) * 2;

        if(readsamples){
          adpcm_decoder(adpcm_inbuf, outptr, readsamples);
          outptr += readsamples;
          samples += readsamples;
        }
      }

      // if there's nothing to play, just return
      if(!samples) return;

      // Activate
      musicchannel.fp_playto[i] = INT_TO_FIX(samples);
      if(!musicchannel.active){
        musicchannel.playing_buffer = i;
        musicchannel.active = 1;
      }
    }
  }
}



void sound_volume_music(int left, int right){
  if(left < 0) left = 0;
  if(right < 0) right = 0;
  if(left > MAXVOLUME*8) left = MAXVOLUME*8;
  if(right > MAXVOLUME*8) right = MAXVOLUME*8;
  musicchannel.volume[0] = left;
  musicchannel.volume[1] = right;
}



int sound_query_music(char *artist, char *title){
  if(adpcm_handle<0) return 0;

  if(artist) strcpy(artist, borhead.artist);
  if(title) strcpy(title, borhead.title);

  return 1;
}


void sound_pause_music(int toggle){
  musicchannel.paused = toggle;
}





/////////////////////////////// INIT / EXIT //////////////////////////////////



void sound_stop_playback(){
  int i;
  if(!mixing_inited) return;
  if(!mixing_active) return;
  sound_close_music();
  for(i=0; i<max_channels; i++) sound_stop_sample(i);
  mixing_active = 0;
}



int sound_start_playback(int stereo, int bits, int frequency){
  int i;

  if(!mixing_inited) return 0;

  sound_stop_playback();

  if(stereo) stereo = 1;
  if(bits!=8 && bits!=16) return 0;
  if(frequency!=11025 && frequency!=22050 && frequency!=44100) return 0;

  // you can have any frequency as long as it's...
  stereo = 1;
  bits = 16;
  frequency = 48000;

  playstereo = stereo;
  playbits = bits;
  playfrequency = frequency;

  for(i=0; i<max_channels; i++) sound_stop_sample(i);

  mixing_active = 1;
  samplesplayed = 0;
  return 1;
}







// Stop everything and free used memory
void sound_exit(){
  int i;
  
  ps2sdr_setfillfunction(NULL);

  sound_stop_playback();

  if(mixbuf) tracefree(mixbuf);
  mixbuf = NULL;
  for(i=0;i<MAX_SAMPLES;i++){
    sound_unload_sample(i);
  }

  mixing_inited = 0;
}





// Find and initialize SoundBlaster, allocate memory, initialize tables...
int sound_init(int channels){
  int i;
//  int type;

  if(channels < 2) channels = 2;
  if(channels > MAX_CHANNELS) channels = MAX_CHANNELS;

  sound_exit();

  // Allocate the maximum amount ever possibly needed for mixing
  if((mixbuf = (unsigned int *)tracemalloc("sound_init", MIXBUF_SIZE)) == NULL){
//    SB_exit();
    return 0;
  }

  max_channels = channels;
  for(i=0; i<max_channels; i++){
    memset(&vchannel[i], 0, sizeof(channelstruct));
  }


  for(i=0; i<MAX_SAMPLES; i++) sampledata[i].sampleptr = NULL;
  mixing_active = 0;

  ps2sdr_setfillfunction(updatemixing);

  mixing_inited = 1;

  return 1;
}



// Returns time passed in milliseconds (since last call or start of playback),
// or 0xFFFFFFFF if not available. This function is useful when synchronizing
// stuff to sound.
unsigned gamelib_long sound_getinterval(){
  unsigned gamelib_long msecs;
  
  if(!mixing_active) return 0xFFFFFFFF;

  msecs = 1000 * samplesplayed / playfrequency;
  samplesplayed -= msecs * playfrequency / 1000;

  return msecs;
}
