/* audio_driver.h */
/* Jon Watte 19971223 */
/* Interface to drivers found in /dev/audio */
/* Devices found in /dev/old follow a different API! */

#if !defined(_AUDIO_DRIVER_H)
#define _AUDIO_DRIVER_H

#if !defined(_SUPPORT_DEFS_H)
#include <SupportDefs.h>
#endif /* _SUPPORT_DEFS_H */
#if !defined(_DRIVERS_H)
#include <Drivers.h>
#endif /* _DRIVERS_H */

enum {
	/* arg = ptr to struct audio_format */
	B_AUDIO_GET_AUDIO_FORMAT = B_AUDIO_DRIVER_BASE,
	B_AUDIO_SET_AUDIO_FORMAT,
	/* arg = ptr to float[4] */
	B_AUDIO_GET_PREFERRED_SAMPLE_RATES
};

/* this is the definition of what the audio driver can do for you */
typedef struct audio_format {
    float       sample_rate;    /* ~4000 - ~48000, maybe more */
    int32       channels;       /* 1 or 2 */
    int32       format;         /* 0x11 (uchar), 0x2 (short) or 0x24 (float) */
    int32       big_endian;    /* 0 for little endian, 1 for big endian */
    size_t      buf_header;     /* typically 0 or 16 */
    size_t      play_buf_size;	/* size of playback buffer (latency) */
    size_t	rec_buf_size;	/* size of record buffer (latency) */
} audio_format;

/* when buffer header is in effect, this is what gets read before data */
typedef struct audio_buf_header {
    bigtime_t   capture_time;
    uint32      capture_size;
    float       sample_rate;
} audio_buf_header;



#endif /* _AUDIO_DRIVER_H */

