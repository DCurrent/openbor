/*
 * OpenBOR - http://www.chronocrash.com
 * -----------------------------------------------------------------------
 * All rights reserved, see LICENSE in OpenBOR root for details.
 *
 * Copyright (c) 2004 - 2014 OpenBOR Team
 */

/*
**	Sound mixer.
**	High quality, with support for ADPCM and Vorbis-compressed music.
**
**	Also plays WAV files (both 8-bit and 16-bit).
**	Note: 8-bit wavs are unsigned, 16-bit wavs are signed!!!
**
**
**	Function naming convention:
**	- Public functions start with "sound_"
**	- Music-related functions end with "_music"
**	- Soundeffect-related functions end with "_sample"
**
**
**	To do:
**	- I think it's stable now, but stay alert!
**	- test 16-bit soundfx
**
**
**	Note:
**  If any of the #defines are increased in size
**  pay close attention to the the size of variables
**  which are used in conjunction with it.  You could
**  be going beyond the variable's current size which
**  will cause errors!!!
*/

#include <stdio.h>
#include <string.h>
/*
Caution: move vorbis headers here otherwise the structs will
 get poisoned by #pragma in other header files, i.e. list.h
*/
#ifdef DC
#include <ivorbisfile.h>
#elif TREMOR
#include <tremor/ivorbisfile.h>
#else
#include <vorbis/vorbisfile.h>
#endif
#include "openbor.h"
#include "adpcm.h"
#include "borendian.h"


#if LINUX || GP2X || OPENDINGUX || SYMBIAN
#define stricmp strcasecmp
#endif

#define		AUDIOCIDE_VERSION	"2.00"
#define		MIXSHIFT		     3	    // 2 should be OK
#define		MAXVOLUME		     64	    // 64 for backw. compat.
//#define		MAX_SAMPLES		     1024	// Should be well enough
#define		MAX_CHANNELS	     64	    // Should be well enough

// Hardware settings for SoundBlaster (change only if latency is too big)
#define		SB_BUFFER_SIZE		 0x8000
#define		SB_BUFFER_SIZE_MASK	 0x7FFF
#define		SB_WBUFFER_SIZE		 0x4000
#define		SB_WBUFFER_SIZE_MASK 0x3FFF
#define		MIXBUF_SIZE		     SB_BUFFER_SIZE*8
#define		PREMIX_SIZE		     1024
#define		MIX_BLOCK_SIZE		 32

// 20:12 fixed-point conversion macros.
// The maximum size of a sound is linked directly
// to the range of the fixed-point variables!
#define		INT_TO_FIX(i)		((unsigned int)i<<12)
#define		FIX_TO_INT(f)		((unsigned int)f>>12)
#define		MAX_SOUND_LEN		0xFFFFF
#define		CHANNEL_PLAYING		1
#define		CHANNEL_LOOPING		2
#define		MUSIC_NUM_BUFFERS	4
#define		MUSIC_BUF_SIZE		(16*1024)	// In samples
#define		SOUND_MONO			1
#define		SOUND_STEREO		2

#ifndef DC
#pragma pack(4)
#endif

typedef struct
{
    int            active;		 // 1 = play, 2 = loop
    int				paused;
    int            samplenum;	 // Index of sound playing
    unsigned int   priority;	 // Used for SFX
    int				playid;
    int            volume[2];	 // Stereo :)
    int            channels;
    unsigned int   fp_samplepos; // Position (fixed-point)
    unsigned int   fp_period;	 // Period (fixed-point)
} channelstruct;

typedef struct
{
    void 		   *sampleptr;
    int			   soundlen;	 // Length in samples
    int            bits;		 // 8/16 bit
    int            frequency;    // 11025 * 1,2,4
    int            channels;
} samplestruct;

typedef struct
{
    samplestruct  sample;
    int index;
    char *filename;
} s_soundcache;

typedef struct
{
    int            active;
    int            paused;
    short 		   *buf[MUSIC_NUM_BUFFERS];
    unsigned int   fp_playto[MUSIC_NUM_BUFFERS];
    unsigned int   fp_samplepos;  // Position (fixed-point)
    unsigned int   fp_period;	  // Period (fixed-point)
    int			   playing_buffer;
    int            volume[2];
    int            channels;
} musicchannelstruct;


static List samplelist;
static s_soundcache *soundcache = NULL;
static int sound_cached = 0;
int sample_play_id = 0;
static channelstruct vchannel[MAX_CHANNELS];
static musicchannelstruct musicchannel;
static s32 *mixbuf = NULL;
static int playbits;
static int playfrequency;
static int max_channels = 0;

#ifdef XBOX
static char *DMAbuf8 = NULL;
static u16 *DMAbuf16 = NULL;
#endif

// Global shifter for hardware frequency adjustment (applied to periods)
static u32 hard_shift;

// Indicates whether the hardware is playing, and if mixing is active
static int mixing_active = 0;

// Indicates whether the sound system is initialized
static int mixing_inited = 0;

// Counts the total number of samples played
static u32 samplesplayed;

// Records type of currently playing music: 0=ADPCM, 1=Vorbis
static int music_type = 0;

//////////////////////////////// WAVE LOADER //////////////////////////////////

#ifndef DC
#ifdef SDL
#pragma pack(push, r1, 16)
#else
#pragma pack(push)
#endif
#pragma pack(1)
#endif


#define		HEX_RIFF	0x46464952
#define		HEX_WAVE	0x45564157
#define		HEX_fmt		0x20746D66
#define		HEX_data	0x61746164
#define		FMT_PCM		0x0001

static int loadwave(char *filename, char *packname, samplestruct *buf, unsigned int maxsize)
{
    struct
    {
        u32				riff;
        u32				size;
        u32				type;
    } riffheader;
    struct
    {
        u32				tag;
        u32				size;
    } rifftag;
    struct
    {
        u16				format;		// 1 = PCM
        u16				channels;	// Mono, stereo
        u32				samplerate;	// 11025, 22050, 44100
        u32				bps;		// Bytes/second
        u16				unknown;
        u16				samplebits;	// 8, 12, 16
    } fmt;

    int handle;
    int mulbytes;

    if(buf == NULL)
    {
        return 0;
    }

    if((handle = openpackfile(filename, packname)) == -1)
    {
        return 0;
    }
    if(readpackfile(handle, &riffheader, sizeof(riffheader)) != sizeof(riffheader))
    {
        closepackfile(handle);
        return 0;
    }

    riffheader.riff = SwapLSB32(riffheader.riff);
    riffheader.size = SwapLSB32(riffheader.size);
    riffheader.type = SwapLSB32(riffheader.type);

    if(riffheader.riff != HEX_RIFF || riffheader.type != HEX_WAVE)
    {
        closepackfile(handle);
        return 0;
    }

    rifftag.tag = 0;
    // Search for format tag
    while(rifftag.tag != HEX_fmt)
    {
        if(readpackfile(handle, &rifftag, sizeof(rifftag)) != sizeof(rifftag))
        {
            closepackfile(handle);
            return 0;
        }
        rifftag.tag = SwapLSB32(rifftag.tag);
        rifftag.size = SwapLSB32(rifftag.size);
        if(rifftag.tag != HEX_fmt)
        {
            seekpackfile(handle, rifftag.size, SEEK_CUR);
        }
    }
    if(readpackfile(handle, &fmt, sizeof(fmt)) != sizeof(fmt))
    {
        closepackfile(handle);
        return 0;
    }

    fmt.format = SwapLSB16(fmt.format);
    fmt.channels = SwapLSB16(fmt.channels);
    fmt.unknown = SwapLSB16(fmt.unknown);
    fmt.samplebits = SwapLSB16(fmt.samplebits);
    fmt.samplerate = SwapLSB32(fmt.samplerate);
    fmt.bps = SwapLSB32(fmt.bps);

    if(rifftag.size > sizeof(fmt))
    {
        seekpackfile(handle, rifftag.size - sizeof(fmt), SEEK_CUR);
    }

    if(fmt.format != FMT_PCM || (fmt.channels != 1 && fmt.channels != 2) || (fmt.samplebits != 8 && fmt.samplebits != 16))
    {
        closepackfile(handle);
        return 0;
    }
    mulbytes = (fmt.samplebits == 16 ? 2 : 1);


    // Search for data tag
    while(rifftag.tag != HEX_data)
    {
        if(readpackfile(handle, &rifftag, sizeof(rifftag)) != sizeof(rifftag))
        {
            closepackfile(handle);
            return 0;
        }
        rifftag.tag = SwapLSB32(rifftag.tag);
        rifftag.size = SwapLSB32(rifftag.size);
        if(rifftag.tag != HEX_data)
        {
            seekpackfile(handle, rifftag.size, SEEK_CUR);
        }
    }

    if(rifftag.size < maxsize)
    {
        maxsize = rifftag.size;
    }
    if((buf->sampleptr = malloc(maxsize + 8)) == NULL)
    {
        closepackfile(handle);
        return 0;
    }
    if(fmt.samplebits == 8)
    {
        memset(buf->sampleptr, 0x80, maxsize + 8);
    }
    else
    {
        memset(buf->sampleptr, 0x80, maxsize + 8);
    }

    if( readpackfile(handle, buf->sampleptr, maxsize) != (int)maxsize)
    {
        if(buf->sampleptr != NULL)
        {
            free(buf->sampleptr);
            buf->sampleptr = NULL;
        }
        closepackfile(handle);
        return 0;
    }

    closepackfile(handle);

    buf->soundlen = maxsize / mulbytes;
    buf->bits = fmt.samplebits;
    buf->frequency = fmt.samplerate;
    buf->channels = fmt.channels;

    return maxsize;
}

int sound_reload_sample(int index)
{
    if(!mixing_inited)
    {
        return 0;
    }
    if(index < 0 || index >= sound_cached)
    {
        return 0;
    }
    if(!soundcache[index].sample.sampleptr)
    {
        //printf("packfile: '%s'\n", packfile);
        return loadwave(soundcache[index].filename, packfile, &(soundcache[index].sample), MAX_SOUND_LEN);
    }
    else
    {
        return 1;
    }
}


// Load a sound or return index
int sound_load_sample(char *filename, char *packfilename, int iLog)
{
    s_soundcache *cache;
    samplestruct sample;
    static char convcache[256];
    if(!mixing_inited)
    {
        return -1;
    }
    /////////////////////////////
    strcpy(convcache, filename);
    lc(convcache, strlen(convcache));
    if(List_FindByName(&samplelist, convcache))
    {
        cache = &soundcache[(size_t)List_Retrieve(&samplelist)];
        if(!cache->sample.sampleptr)
        {
            if(!sound_reload_sample(cache->index) && iLog)
            {
                printf("sound_load_sample can't restore sampleptr from file '%s'!\n", filename);
            }
        }
        return cache->index;
    }

    memset(&sample, 0, sizeof(sample));
    if(!loadwave(filename, packfilename, &sample, MAX_SOUND_LEN))
    {
        if(iLog)
        {
            printf("sound_load_sample can't load sample from file '%s'!\n", filename);
        }
        return -1;
    }

    __realloc(soundcache, sound_cached);
    soundcache[sound_cached].sample = sample;

    List_GotoLast(&samplelist);
    List_InsertAfter(&samplelist, (void *)(size_t)sound_cached, convcache);
    soundcache[sound_cached].index = sound_cached;
    soundcache[sound_cached].filename = List_GetName(&samplelist);

    sound_cached++;
    return sound_cached - 1;

}

// Changed to conserve memory: added this function
void sound_unload_sample(int index)
{
    if(!mixing_inited)
    {
        return;
    }
    if(index < 0 || index >= sound_cached)
    {
        return;
    }
    if(soundcache[index].sample.sampleptr != NULL)
    {
        free(soundcache[index].sample.sampleptr);
        soundcache[index].sample.sampleptr = NULL;
        memset(&soundcache[index].sample, 0, sizeof(samplestruct));
    }
}

void sound_unload_all_samples()
{
    int i;
    if(!soundcache)
    {
        return;
    }
    for(i = 0; i < sound_cached; i++)
    {
        sound_unload_sample(i);
    }
    List_Clear(&samplelist);
    free(soundcache);
    soundcache = NULL;
    sound_cached = 0;
}

#ifndef DC
#ifdef SDL
#pragma pack(pop, r1)
#else
#pragma pack(pop)
#endif
#endif

/////////////////////////////// Mix to DMA //////////////////////////////////
// Mixbuffer / DMA buffer data handling
// Writes mixbuffer data (16-bit mixed in 32-bit array)
// to 8-bit or 16-bit DMA buffer.

// Fill the mixbuffer with silence
static void clearmixbuffer(unsigned int *buf, int n)
{
    while((--n) >= 0)
    {
        *buf = 0x8000 << MIXSHIFT;
        ++buf;
    }
}

#ifdef XBOX
static int mixtoDMAlow(unsigned int *mbuf, char *dbuf, int dmaoffs, int numbytes)
{
    static int u;
    while((--numbytes) >= 0)
    {
        dmaoffs &= SB_BUFFER_SIZE_MASK;
        u = *mbuf >> (MIXSHIFT + 8);
        if(u < 0)
        {
            u = 0;
        }
        else if(u > 0xFF)
        {
            u = 0xFF;
        }
        dbuf[dmaoffs] = u;
        ++mbuf;
        ++dmaoffs;
    }
    dmaoffs &= SB_BUFFER_SIZE_MASK;
    return dmaoffs;
}



static int mixtoDMAhigh(unsigned int *mbuf, unsigned short *dbuf, int dmaoffs, int numwords)
{
    static unsigned int u;
    while((--numwords) >= 0)
    {
        dmaoffs &= SB_WBUFFER_SIZE_MASK;
        //u = *mbuf ;
        u = *mbuf >> MIXSHIFT;
        if(u < 0)
        {
            u = 0;
        }
        else if(u > 0xFFFF)
        {
            u = 0xFFFF;
        }
        dbuf[dmaoffs] = u;
        ++mbuf;
        ++dmaoffs;
    }
    dmaoffs &= SB_WBUFFER_SIZE_MASK;
    return dmaoffs;
}
#endif



/////////////////////////////////// Mixers ///////////////////////////////////
// Mixers: mix (16-bit) in the mixbuffer, then write to DMA memory (see above).
// The mixing code handles fixed-point conversion and looping.

static int dmamixpos;

// Input: number of input samples to mix
static void mixaudio(unsigned int todo)
{

    static int i, chan, lvolume, rvolume, lmusic, rmusic;
    static unsigned int fp_pos, fp_period, fp_len, fp_playto;
    static int snum;
    static unsigned char *sptr8;
    static short *sptr16;

    // First mix the music, if playing
    if(musicchannel.active && !musicchannel.paused)
    {

        sptr16 = musicchannel.buf[musicchannel.playing_buffer];
        fp_playto = musicchannel.fp_playto[musicchannel.playing_buffer];
        fp_pos = musicchannel.fp_samplepos;
        fp_period = musicchannel.fp_period;
        lvolume = musicchannel.volume[0];
        rvolume = musicchannel.volume[1];

        // Mix it
        for(i = 0; i < (int)todo;)
        {

            // Reached end of playable area,
            // switch buffers or stop
            if(fp_pos >= fp_playto)
            {
                // Done playing this one
                musicchannel.fp_playto[musicchannel.playing_buffer] = 0;
                // Advance to next buffer
                musicchannel.playing_buffer++;
                musicchannel.playing_buffer %= MUSIC_NUM_BUFFERS;
                // Correct position in next buffer
                fp_pos = fp_pos - fp_playto;
                // Anything to play?
                if(fp_pos < musicchannel.fp_playto[musicchannel.playing_buffer])
                {
                    // Yeah, switch!
                    sptr16 = musicchannel.buf[musicchannel.playing_buffer];
                    fp_playto = musicchannel.fp_playto[musicchannel.playing_buffer];
                }
                else
                {
                    // Nothing more to do
                    // Also disable this buffer, just incase
                    musicchannel.fp_playto[musicchannel.playing_buffer] = 0;
                    fp_pos = 0;
                    musicchannel.active = 0;
                    // End for
                    break;
                }
            }

            // Mix a sample
            lmusic = rmusic = sptr16[FIX_TO_INT(fp_pos)];
            lmusic = (lmusic * lvolume / MAXVOLUME);
            rmusic = (rmusic * rvolume / MAXVOLUME);
            mixbuf[i++] += lmusic;
            if(musicchannel.channels == SOUND_MONO)
            {
                mixbuf[i++] += rmusic;
            }
            fp_pos += fp_period;
        }
        musicchannel.fp_samplepos = fp_pos;
    }


    for(chan = 0; chan < max_channels; chan++)
    {
        if(vchannel[chan].active && !vchannel[chan].paused)
        {
            unsigned modlen;
            snum = vchannel[chan].samplenum;
            if(!soundcache[snum].sample.sampleptr)
            {
                vchannel[chan].active = 0;
                continue;
            }
            modlen = soundcache[snum].sample.soundlen;
            fp_len = INT_TO_FIX(soundcache[snum].sample.soundlen);
            fp_pos = vchannel[chan].fp_samplepos;
            fp_period = vchannel[chan].fp_period;
            lvolume = vchannel[chan].volume[0];
            rvolume = vchannel[chan].volume[1];
            if(fp_len < 1)
            {
                fp_len = 1;
            }
            if(modlen < 1)
            {
                modlen = 1;
            }
            if(soundcache[snum].sample.bits == 8)
            {
                sptr8 = soundcache[snum].sample.sampleptr;
                for(i = 0; i < (int)todo;)
                {
                    lmusic = rmusic = sptr8[FIX_TO_INT(fp_pos)];
                    mixbuf[i++] += ((lmusic << 8) * lvolume / MAXVOLUME) - 0x8000;
                    if(vchannel[chan].channels == SOUND_MONO)
                    {
                        mixbuf[i++] += ((rmusic << 8) * rvolume / MAXVOLUME) - 0x8000;
                    }
                    fp_pos += fp_period;

                    // Reached end of sample, stop or loop
                    if(fp_pos >= fp_len)
                    {
                        fp_pos %= fp_len; // = INT_TO_FIX(0);
                        if(vchannel[chan].active != CHANNEL_LOOPING)
                        {
                            vchannel[chan].active = 0;
                            break;
                        }
                    }
                }
            }
            else if(soundcache[snum].sample.bits == 16)
            {
                sptr16 = soundcache[snum].sample.sampleptr;
                for(i = 0; i < (int)todo;)
                {
                    lmusic = rmusic = sptr16[FIX_TO_INT(fp_pos)];
                    mixbuf[i++] += (lmusic * lvolume / MAXVOLUME);
                    if(vchannel[chan].channels == SOUND_MONO)
                    {
                        mixbuf[i++] += (rmusic * rvolume / MAXVOLUME);
                    }
                    fp_pos += fp_period;

                    // Reached end of sample, stop or loop
                    if(fp_pos >= fp_len)
                    {
                        fp_pos %= fp_len; // = INT_TO_FIX(0);
                        if(vchannel[chan].active != CHANNEL_LOOPING)
                        {
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

#ifdef XBOX

int get_mixingactive()
{
    return mixing_active ;
}

unsigned char *updatemixing_xbox(unsigned int todo)
{
    static int curdmapos;
    if (!mixing_active)
    {
        return NULL;
    }
    clearmixbuffer(mixbuf, todo);
    mixaudio(todo);
    samplesplayed += (todo >> 1);
    dmamixpos = mixtoDMAhigh(mixbuf, DMAbuf16, 0, todo);
    return (unsigned char *)DMAbuf16;
}

#else

void update_sample(unsigned char *buf, int size)
{
    int i, u, todo = size;
    if (playbits == 16)
    {
        todo >>= 1;
    }

    clearmixbuffer((unsigned int *)mixbuf, todo);
    mixaudio(todo);
    samplesplayed += (todo >> 1);

    if (playbits == 8)
    {
        unsigned char *dst = buf;
        for(i = 0; i < todo; i++)
        {
            u = mixbuf[i] >> (MIXSHIFT + 8);
            if (u < 0)
            {
                u = 0;
            }
            else if (u > 0xff)
            {
                u = 0xff;
            }
            dst[i] = u;
        }
    }
    else
    {
        unsigned short *dst = (unsigned short *)buf;
        for(i = 0; i < todo; i++)
        {
            u = mixbuf[i] >> MIXSHIFT;
            if (u < 0)
            {
                u = 0;
            }
            else if (u > 0xffff)
            {
                u = 0xffff;
            }
            u ^= 0x8000;
            dst[i] = u;
        }
    }
}
#endif

////////////////////////// Sound effects control /////////////////////////////
// Functions to start, stop, loop, etc.

// Speed in percents of normal.
// Returns channel the sample is played on or -1 if not playing.
int sound_play_sample(int samplenum, unsigned int priority, int lvolume, int rvolume, unsigned int speed)
{

    int i;
    unsigned int prio_low;
    int channel;

    if(!mixing_inited)
    {
        return -1;
    }
    if(samplenum < 0 || samplenum >= sound_cached)
    {
        return -1;
    }
    if(speed < 1)
    {
        speed = 100;
    }
    if(!soundcache[samplenum].sample.sampleptr &&
            !sound_reload_sample(samplenum))
    {
        return -1;
    }

    // Try to find unused SFX channel
    channel = -1;
    for(i = 0; i < max_channels; i++)
    {
        if(!vchannel[i].active)
        {
            channel = i;
        }
    }

    if(channel == -1)
    {
        // Find SFX channel with lowest current priority
        for(i = 0, prio_low = 0xFFFFFFFF; i < max_channels; i++)
        {
            if(vchannel[i].priority < prio_low)
            {
                channel = i;
                prio_low = vchannel[i].priority;
            }
        }
        if(prio_low > priority)
        {
            return -1;
        }
    }

    if(lvolume < 0)
    {
        lvolume = 0;
    }
    if(rvolume < 0)
    {
        rvolume = 0;
    }
    if(lvolume > MAXVOLUME)
    {
        lvolume = MAXVOLUME;
    }
    if(rvolume > MAXVOLUME)
    {
        rvolume = MAXVOLUME;
    }

    vchannel[channel].samplenum = samplenum;
    // Prevent samples from being played at EXACT same point
    vchannel[channel].fp_samplepos = INT_TO_FIX((channel * 4) % soundcache[samplenum].sample.soundlen);
    vchannel[channel].fp_period = (INT_TO_FIX(1) * speed / 100) * soundcache[samplenum].sample.frequency / playfrequency;
    vchannel[channel].volume[0] = lvolume;
    vchannel[channel].volume[1] = rvolume;
    vchannel[channel].priority = priority;
    vchannel[channel].channels = soundcache[samplenum].sample.channels;
    vchannel[channel].active = CHANNEL_PLAYING;
    vchannel[channel].paused = 0;
    vchannel[channel].playid = ++sample_play_id;

    return channel;
}

int sound_loop_sample(int samplenum, unsigned int priority, int lvolume, int rvolume, unsigned int speed)
{
    int ch = sound_play_sample(samplenum, priority, lvolume, rvolume, speed);
    if(ch >= 0)
    {
        vchannel[ch].active = CHANNEL_LOOPING;
    }
    return ch;
}

int sound_query_channel(int playid)
{
    int i;
    for(i = 0; i < max_channels; i++)
    {
        if(vchannel[i].playid == playid && vchannel[i].active)
        {
            return i;
        }
    }
    return -1;
}

void sound_stop_sample(int channel)
{
    if(channel < 0 || channel >= max_channels)
    {
        return;
    }
    vchannel[channel].active = 0;
}

void sound_stopall_sample()
{
    int channel;
    for(channel = 0; channel < max_channels; channel++)
    {
        vchannel[channel].active = 0;
    }
}

void sound_pause_sample(int toggle)
{
    int channel;
    for(channel = 0; channel < max_channels; channel++)
    {
        vchannel[channel].paused = toggle;
    }
}

void sound_volume_sample(int channel, int lvolume, int rvolume)
{
    if(channel < 0 || channel >= max_channels)
    {
        return;
    }
    if(lvolume < 0)
    {
        lvolume = 0;
    }
    if(rvolume < 0)
    {
        rvolume = 0;
    }
    if(lvolume > MAXVOLUME)
    {
        lvolume = MAXVOLUME;
    }
    if(rvolume > MAXVOLUME)
    {
        rvolume = MAXVOLUME;
    }
    vchannel[channel].volume[0] = lvolume;
    vchannel[channel].volume[1] = rvolume;
}

int sound_getpos_sample(int channel)
{
    if(channel < 0 || channel >= max_channels)
    {
        return 0;
    }
    return FIX_TO_INT(vchannel[channel].fp_samplepos);
}

//////////////////////////////// ADPCM music ////////////////////////////////

static int adpcm_handle = -1;
static unsigned char *adpcm_inbuf;
static int music_looping = 0;
static int music_atend = 0;
#define	BOR_MUSIC_VERSION_MONO   0x00010000
#define	BOR_MUSIC_VERSION_STEREO 0x00010001
#define	BOR_IDENTIFIER "BOR music"

#ifndef DC
#pragma pack (1)
#endif

typedef struct
{
    char	identifier[16];
    char	artist[64];
    char	title[64];
    unsigned int	version;
    int		frequency;
    int		channels;
    int		datastart;
} bor_header;

#ifndef DC
#pragma pack (4)
#endif

static bor_header borhead;
static short loop_valprev[2];
static char loop_index[2];
static int loop_state_set;
static u32 loop_offset;

void sound_close_adpcm()
{

    int i;

    // Prevent any further access by the ISR
    musicchannel.active = 0;
    for(i = 0; i < MUSIC_NUM_BUFFERS; i++)
    {
        musicchannel.fp_playto[i] = 0;
    }

    // Close file...
    if(adpcm_handle >= 0)
    {
        closepackfile(adpcm_handle);
    }
    adpcm_handle = -1;

    if(adpcm_inbuf != NULL)
    {
        free(adpcm_inbuf);
        adpcm_inbuf = NULL;
    }

    for(i = 0; i < MUSIC_NUM_BUFFERS; i++)
    {
        if(musicchannel.buf[i] != NULL)
        {
            free(musicchannel.buf[i]);
            musicchannel.buf[i] = NULL;
        }
    }

    memset(&musicchannel, 0, sizeof(musicchannelstruct));
    memset(&borhead, 0, sizeof(bor_header));

    adpcm_reset();
    loop_valprev[0] = loop_valprev[1] = 0;
    loop_index[0] = loop_index[1] = 0;
    loop_state_set = 0;
}

int sound_open_adpcm(char *filename, char *packname, int volume, int loop, u32 music_offset)
{

    int i;

    if(!mixing_inited)
    {
        return 0;
    }
    if(!mixing_active)
    {
        return 0;
    }

    sound_close_music();

    // Open file, etcetera
    adpcm_handle = openpackfile(filename, packname);
    if(adpcm_handle < 0)
    {
        return 0;
    }

    // Read header
    if(readpackfile(adpcm_handle, &borhead, sizeof(bor_header)) != sizeof(bor_header))
    {
        goto error_exit;
    }

    borhead.version = SwapLSB32(borhead.version);
    borhead.frequency = SwapLSB32(borhead.frequency);
    borhead.channels = SwapLSB32(borhead.channels);
    borhead.datastart = SwapLSB32(borhead.datastart);

    // Is it really a BOR music file?
    if(strncmp(borhead.identifier, BOR_IDENTIFIER, 16) != 0)
    {
        goto error_exit;
    }

    // Can I play it?
    if((borhead.version != BOR_MUSIC_VERSION_MONO && borhead.version != BOR_MUSIC_VERSION_STEREO) ||
            (borhead.channels != 1 && borhead.channels != 2) ||
            borhead.frequency < 11025 || borhead.frequency > 44100)
    {
        goto error_exit;
    }
    // Seek to beginning of data
    if(seekpackfile(adpcm_handle, borhead.datastart, SEEK_SET) != borhead.datastart)
    {
        goto error_exit;
    }

    memset(&musicchannel, 0, sizeof(musicchannelstruct));

    musicchannel.fp_period = INT_TO_FIX(borhead.frequency) / playfrequency;
    musicchannel.volume[0] = volume;
    musicchannel.volume[1] = volume;
    musicchannel.channels = borhead.channels;
    music_looping = loop;
    music_atend = 0;

    adpcm_inbuf = malloc(MUSIC_BUF_SIZE / 2);
    if(adpcm_inbuf == NULL)
    {
        goto error_exit;
    }

    for(i = 0; i < MUSIC_NUM_BUFFERS; i++)
    {
        musicchannel.buf[i] = malloc(MUSIC_BUF_SIZE * sizeof(short));
        if(musicchannel.buf[i] == NULL)
        {
            goto error_exit;
        }
        memset(musicchannel.buf[i], 0, MUSIC_BUF_SIZE * sizeof(short));
    }

    loop_offset = music_offset;
    music_type = 0;

    return 1;
error_exit:
    sound_close_music();
    closepackfile(adpcm_handle);
    return 0;
}

void sound_update_adpcm()
{

    int samples, readsamples, samples_to_read;
    short *outptr;
    int i, j;

    if((adpcm_handle < 0) || (music_type != 0))
    {
        return;
    }
    if(!mixing_inited || !mixing_active)
    {
        sound_close_music();
        return;
    }
    if(musicchannel.paused)
    {
        return;
    }


    // Just to be sure: check if all goes well...
    for(i = 0; i < MUSIC_NUM_BUFFERS; i++)
    {
        if(musicchannel.fp_playto[i] > INT_TO_FIX(MUSIC_BUF_SIZE))
        {
            musicchannel.fp_playto[i] = 0;
            return;
        }
    }


    // Need to update?
    for(j = 0, i = musicchannel.playing_buffer + 1; j < MUSIC_NUM_BUFFERS; j++, i++)
    {
        i %= MUSIC_NUM_BUFFERS;

        if(musicchannel.fp_playto[i] == 0)
        {
            // Buffer needs to be filled

            samples = 0;
            outptr = musicchannel.buf[i];

            if(!music_looping)
            {
                if(music_atend)
                {
                    // Close file when done playing all buffers
                    if(!musicchannel.active)
                    {
                        sound_close_music();
                        return;
                    }
                }
                else
                {
                    readsamples = readpackfile(adpcm_handle, adpcm_inbuf, MUSIC_BUF_SIZE / 2) * 2;
                    if(readsamples <= 0)
                    {
                        // EOF
                        music_atend = 1;
                        return;
                    }
                    // Play this bit
                    adpcm_decode(adpcm_inbuf, outptr, readsamples / 2, musicchannel.channels);
                    samples = readsamples;
                }
            }
            else while(samples < MUSIC_BUF_SIZE)
                {
                    samples_to_read = MUSIC_BUF_SIZE - samples;
                    if(!loop_state_set && seekpackfile(adpcm_handle, 0, SEEK_CUR) <= (borhead.datastart + loop_offset) && seekpackfile(adpcm_handle, 0, SEEK_CUR) > (borhead.datastart + loop_offset - samples_to_read / 2))
                    {
                        readsamples = readpackfile(adpcm_handle, adpcm_inbuf, borhead.datastart + loop_offset - seekpackfile(adpcm_handle, 0, SEEK_CUR)) * 2;
                        adpcm_decode(adpcm_inbuf, outptr, readsamples / 2, musicchannel.channels);
                        loop_valprev[0] = adpcm_valprev(0);
                        loop_index[0] = adpcm_index(0);
                        if(musicchannel.channels == SOUND_STEREO)
                        {
                            loop_valprev[1] = adpcm_valprev(1);
                            loop_index[1] = adpcm_index(1);
                        }
                        loop_state_set = 1;
                        outptr += readsamples;
                        samples += readsamples;
                    }
                    else
                    {
                        readsamples = readpackfile(adpcm_handle, adpcm_inbuf, samples_to_read / 2) * 2;
                        if(readsamples < 0)
                        {
                            // Error
                            sound_close_music();
                            return;
                        }
                        if(readsamples)
                        {
                            adpcm_decode(adpcm_inbuf, outptr, readsamples / 2, musicchannel.channels);
                            outptr += readsamples;
                            samples += readsamples;
                        }
                        if(readsamples < samples_to_read)
                        {
                            // At start of data already?
                            if(seekpackfile(adpcm_handle, 0, SEEK_CUR) == borhead.datastart)
                            {
                                // Must be some error
                                sound_close_music();
                                return;
                            }
                            // Seek to beginning of data
                            if(seekpackfile(adpcm_handle, borhead.datastart + loop_offset, SEEK_SET) != borhead.datastart + loop_offset)
                            {
                                sound_close_music();
                                return;
                            }
                            // Reset decoder
                            adpcm_loop_reset(0, loop_valprev[0], loop_index[0]);
                            if(musicchannel.channels == SOUND_STEREO)
                            {
                                adpcm_loop_reset(1, loop_valprev[1], loop_index[1]);
                            }
                        }
                    }
                }
            // Activate
            musicchannel.fp_playto[i] = INT_TO_FIX(samples);
            if(!musicchannel.active)
            {
                musicchannel.playing_buffer = i;
                musicchannel.active = 1;
            }
        }
    }
}

void sound_adpcm_tempo(int music_tempo)
{
    musicchannel.fp_period = (INT_TO_FIX(1) * music_tempo / 100) * borhead.frequency / playfrequency;
}

int sound_query_adpcm(char *artist, char *title)
{
    if(adpcm_handle < 0)
    {
        return 0;
    }
    if(artist)
    {
        strcpy(artist, borhead.artist);
    }
    if(title)
    {
        strcpy(title, borhead.title);
    }
    return 1;
}

/////////////////////////// Ogg Vorbis decoding ///////////////////////////////
// Plombo's Ogg Vorbis decoder for OpenBOR. Uses libvorbisfile or libvorbisidec.

#if TREMOR || DC
#define ov_decode(vf,buffer,length,bitstream) ov_read(vf,buffer,length,bitstream)
#else
#define ov_decode(vf,buffer,length,bitstream) ov_read(vf,buffer,length,0,2,1,bitstream)
#endif

OggVorbis_File *oggfile;
vorbis_info *stream_info;
int current_section, ogg_handle;

// I/O functions used by libvorbisfile
size_t readpackfile_callback(void *buf, size_t len, size_t nmembers, int *handle)
{
    return readpackfile(*handle, buf, (int)(len * nmembers));
}
int closepackfile_callback(void *ptr)
{
#ifdef VERBOSE
    printf ("closepack cb %d\n", *(int *)ptr);
#endif

    return closepackfile(*(int *)ptr);
}
int seekpackfile_callback(int *handle, ogg_int64_t offset, int whence)
{
    return seekpackfile(*handle, (int)offset, whence);
}
int tellpackfile_callback(int *handle)
{
    return seekpackfile(*handle, 0, SEEK_CUR);
}

void sound_close_ogg()
{
    int i;

    ov_clear(oggfile);
    free(oggfile);
    oggfile = NULL;
    music_type = -1;

    for(i = 0; i < MUSIC_NUM_BUFFERS; i++)
    {
        if(musicchannel.buf[i] != NULL)
        {
            free(musicchannel.buf[i]);
            musicchannel.buf[i] = NULL;
        }
    }

    memset(&musicchannel, 0, sizeof(musicchannelstruct));
}

int sound_open_ogg(char *filename, char *packname, int volume, int loop, u32 music_offset)
{

    int i;

    static ov_callbacks ogg_callbacks =
    {
        (size_t ( *)(void *, size_t, size_t, void *))  readpackfile_callback,
        (int ( *)(void *, ogg_int64_t, int))           seekpackfile_callback,
        (int ( *)(void *))                             closepackfile_callback,
        (long ( *)(void *))                            tellpackfile_callback
    };

    if(!mixing_inited)
    {
        return 0;
    }
    if(!mixing_active)
    {
        return 0;
    }

    sound_close_music();
#ifdef VERBOSE
    printf("trying to open OGG file %s from %s, vol %d, loop %d, ofs %u\n", filename, packname, volume, loop, music_offset);
#endif
    // Open file, etcetera
    ogg_handle = openpackfile(filename, packname);
#ifdef VERBOSE
    printf ("ogg handle %d\n", ogg_handle);
#endif
    if(ogg_handle < 0)
    {
#ifdef VERBOSE
        printf("couldn't get handle\n");
#endif
        return 0;
    }
    oggfile = malloc(sizeof(OggVorbis_File));
    if (ov_open_callbacks(&ogg_handle, oggfile, NULL, 0, ogg_callbacks) != 0)
    {
#ifdef VERBOSE
        printf("ov_open_callbacks failed\n");
#endif
        goto error_exit;
    }
    // Can I play it?
    stream_info = ov_info(oggfile, -1);
    if((stream_info->channels != 1 && stream_info->channels != 2) ||
            stream_info->rate < 11025 || stream_info->rate > 44100)
    {
        sound_close_ogg();
#ifdef VERBOSE
        printf("NOT can i play it\n");
#endif

        goto error_exit;
    }

    memset(&musicchannel, 0, sizeof(musicchannelstruct));

    musicchannel.fp_period = INT_TO_FIX(stream_info->rate) / playfrequency;
    musicchannel.volume[0] = volume;
    musicchannel.volume[1] = volume;
    musicchannel.channels = stream_info->channels;
    music_looping = loop;
    music_atend = 0;

    for(i = 0; i < MUSIC_NUM_BUFFERS; i++)
    {
        musicchannel.buf[i] = malloc(MUSIC_BUF_SIZE * sizeof(short));
        if(musicchannel.buf[i] == NULL)
        {
            sound_close_ogg();
#ifdef VERBOSE
            printf("buf is null\n");
#endif
            goto error_exit;
        }
        memset(musicchannel.buf[i], 0, MUSIC_BUF_SIZE * sizeof(short));
    }

    loop_offset = music_offset;
    music_type = 1;

#ifdef VERBOSE
    printf("ogg is opened\n");
#endif
    return 1;

error_exit:
    closepackfile(ogg_handle);
    return 0;

}

void sound_update_ogg()
{

    int samples, readsamples, samples_to_read;
    short *outptr;
    int i, j;

    if(!mixing_inited || !mixing_active)
    {
        sound_close_music();
        return;
    }
    if(musicchannel.paused)
    {
        return;
    }

    // Just to be sure: check if all goes well...
    for(i = 0; i < MUSIC_NUM_BUFFERS; i++)
    {
        if(musicchannel.fp_playto[i] > INT_TO_FIX(MUSIC_BUF_SIZE))
        {
            musicchannel.fp_playto[i] = 0;
            return;
        }
    }


    // Need to update?
    for(j = 0, i = musicchannel.playing_buffer + 1; j < MUSIC_NUM_BUFFERS; j++, i++)
    {
        i %= MUSIC_NUM_BUFFERS;

        if(musicchannel.fp_playto[i] == 0)
        {
            // Buffer needs to be filled

            samples = 0;
            outptr = musicchannel.buf[i];

            if(!music_looping)
            {
                if(music_atend)
                {
                    // Close file when done playing all buffers
                    if(!musicchannel.active)
                    {
                        sound_close_music();
                        return;
                    }
                }
                else while(samples < MUSIC_BUF_SIZE)
                    {
                        readsamples = ov_decode(oggfile, (char *)outptr, 2 * (MUSIC_BUF_SIZE - samples), &current_section) / 2;
                        if (readsamples == 0)
                        {
                            music_atend = 1;
                            return;
                        }
                        else if (readsamples < 0)
                        {
                            sound_close_music();
                            return;
                        }
                        outptr += readsamples;
                        samples += readsamples;
                    }
            }
            else while(samples < MUSIC_BUF_SIZE)
                {
                    samples_to_read = MUSIC_BUF_SIZE - samples;
                    readsamples = ov_decode(oggfile, (char *)outptr, 2 * samples_to_read, &current_section) / 2;
                    if(readsamples < 0)
                    {
                        // Error
                        sound_close_music();
                        return;
                    }
                    else if(readsamples > 0)
                    {
                        outptr += readsamples;
                        samples += readsamples;
                    }
                    else if(readsamples < samples_to_read)
                    {
                        // At start of data already?
                        if(ov_pcm_tell(oggfile) == 0)
                        {
                            // Must be some error
                            sound_close_music();
                            return;
                        }
                        // Seek to beginning of data
                        if(ov_pcm_seek(oggfile, loop_offset) != 0)
                        {
                            sound_close_music();
                            return;
                        }
                    }
                }
            // Activate
            musicchannel.fp_playto[i] = INT_TO_FIX(samples);
            if(!musicchannel.active)
            {
                musicchannel.playing_buffer = i;
                musicchannel.active = 1;
            }
        }
    }
}

void sound_ogg_tempo(int music_tempo)
{
    musicchannel.fp_period = (INT_TO_FIX(1) * music_tempo / 100) * stream_info->rate / playfrequency;
}

int sound_query_ogg(char *artist, char *title)
{
    int i;
    char *current;
    vorbis_comment *comment = ov_comment(oggfile, -1);

    if (!artist || !title)
    {
        return 1;
    }

    for(i = 0; i < comment->comments; i++)
    {
        current = comment->user_comments[i];
        if (strncmp("ARTIST=", current, 7) == 0)
        {
            strcpy(artist, current + 7);
        }
        else if (strncmp("TITLE=", current, 6) == 0)
        {
            strcpy(title, current + 6);
        }
    }

    return 1;
}

/////////////////////////////// INIT / EXIT //////////////////////////////////

int sound_open_music(char *filename, char *packname, int volume, int loop, u32 music_offset)
{
    static char fnam[128];
#ifdef VERBOSE
    printf("trying to open music file %s from %s, vol %d, loop %d, ofs %u\n", filename, packname, volume, loop, music_offset);
#endif
    // try opening filename exactly as specified
    if(sound_open_adpcm(filename, packname, volume, loop, music_offset))
    {
        return 1;
    }
    if(sound_open_ogg(filename, packname, volume, loop, music_offset))
    {
        return 1;
    }

    // handle adding an extension to the filename
    sprintf(fnam, "%s.bor", filename);
    if(sound_open_adpcm(fnam, packname, volume, loop, music_offset))
    {
        return 1;
    }
    sprintf(fnam, "%s.ogg", filename);
    if(sound_open_ogg(fnam, packname, volume, loop, music_offset))
    {
        return 1;
    }
    sprintf(fnam, "%s.oga", filename);
    if(sound_open_ogg(fnam, packname, volume, loop, music_offset))
    {
        return 1;
    }

    return 0;
}

void sound_close_music()
{
    switch(music_type)
    {
    case 0:
        sound_close_adpcm();
        break;
    case 1:
        sound_close_ogg();
    }
    music_type = -1;
}

void sound_update_music()
{
    switch(music_type)
    {
    case 0:
        sound_update_adpcm();
        break;
    case 1:
        sound_update_ogg();
    }
}

int sound_query_music(char *artist, char *title)
{
    switch(music_type)
    {
    case 0:
        return sound_query_adpcm(artist, title);
    case 1:
        return sound_query_ogg(artist, title);
    default:
        return 0;
    }
}

void sound_music_tempo(int music_tempo)
{

    switch(music_type)
    {
    case 0:
        sound_adpcm_tempo(music_tempo);
        break;
    case 1:
        sound_ogg_tempo(music_tempo);
    }
}

void sound_volume_music(int left, int right)
{
    if(left < 0)
    {
        left = 0;
    }
    if(right < 0)
    {
        right = 0;
    }
    if(left > MAXVOLUME * 8)
    {
        left = MAXVOLUME * 8;
    }
    if(right > MAXVOLUME * 8)
    {
        right = MAXVOLUME * 8;
    }
    musicchannel.volume[0] = left;
    musicchannel.volume[1] = right;
}

void sound_pause_music(int toggle)
{
    musicchannel.paused = toggle;
}

void sound_stop_playback()
{
    int i;
    if(!mixing_inited)
    {
        return;
    }
    if(!mixing_active)
    {
        return;
    }
    sound_close_music();
    for(i = 0; i < max_channels; i++)
    {
        sound_stop_sample(i);
    }
    SB_playstop();
    mixing_active = 0;
}

int sound_start_playback(int bits, int frequency)
{
    int i;

    if(!mixing_inited)
    {
        return 0;
    }

    sound_stop_playback();

    if(bits != 8 && bits != 16)
    {
        return 0;
    }
    if(frequency != 11025 && frequency != 22050 && frequency != 44100)
    {
        return 0;
    }

#if WIN || LINUX || DARWIN || SYMBIAN
    playbits = bits;
    playfrequency = frequency;
#elif WII
    // Wii only supports 16 bit 32000/48000
    bits = 16;
    frequency = 48000;
    playbits = bits;
    playfrequency = frequency;
#else
    // Most consoles support natively 16/44100
    bits = 16;
    frequency = 44100;
    playbits = bits;
    playfrequency = frequency;
#endif

    hard_shift = 0;
    if(frequency == 22050)
    {
        hard_shift = 1;
    }
    if(frequency == 44100)
    {
        hard_shift = 2;
    }

    dmamixpos = PREMIX_SIZE << hard_shift;
    for(i = 0; i < max_channels; i++)
    {
        sound_stop_sample(i);
    }
    SB_playstop();
    if(!SB_playstart(playbits, playfrequency))
    {
        return 0;
    }

    mixing_active = 1;
    samplesplayed = 0;
    return 1;
}

// Stop everything and free used memory
void sound_exit()
{

    sound_stop_playback();
    sound_unload_all_samples();

    if(mixbuf != NULL)
    {
        free(mixbuf);
        mixbuf = NULL;
    }

#ifdef PSP
    SB_exit();
#endif
#ifdef XBOX
    if(DMAbuf8 != NULL)
    {
        free(DMAbuf8);
        DMAbuf8 = NULL;
    }
#endif

    mixing_inited = 0;
}

// Find and initialize SoundBlaster, allocate memory, initialize tables...
int sound_init(int channels)
{
    int i;

    if(channels < 2)
    {
        channels = 2;
    }
    if(channels > MAX_CHANNELS)
    {
        channels = MAX_CHANNELS;
    }
    sound_exit();

#ifdef XBOX
    DMAbuf8 = (char *)malloc(SB_BUFFER_SIZE << 1 ) ;
    DMAbuf16 = (void *)DMAbuf8;
#endif

    // Allocate the maximum amount ever possibly needed for mixing
    if((mixbuf = malloc(MIXBUF_SIZE)) == NULL)
    {

#ifdef PSP
        SB_exit();
#endif
        return 0;
    }

    max_channels = channels;
    for(i = 0; i < max_channels; i++)
    {
        memset(&vchannel[i], 0, sizeof(channelstruct));
    }

    mixing_active = 0;
    mixing_inited = 1;
    List_Init(&samplelist);

    return 1;
}

// Returns time passed in milliseconds (since last call or start of playback),
// or 0xFFFFFFFF if not available. This function is useful when synchronizing
// stuff to sound.
u32 sound_getinterval()
{
    u32 msecs;

    if(!mixing_active)
    {
        return 0xFFFFFFFF;
    }

    msecs = 1000 * samplesplayed / playfrequency;
    samplesplayed -= msecs * playfrequency / 1000;

    return msecs;
}

