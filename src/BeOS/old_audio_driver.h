/* audio_driver.h - BeOS Release 3 interface to sound drivers */

#ifndef _AUDIO_DRIVER_H_
#define _AUDIO_DRIVER_H_

#include <Drivers.h>

enum {
  B_AUDIO_GET_PARAMS = B_DEVICE_OP_CODES_END,
  B_AUDIO_SET_PARAMS,
  B_AUDIO_SET_PLAYBACK_COMPLETION_SEM,
  B_AUDIO_SET_CAPTURE_COMPLETION_SEM,
  B_AUDIO_RESERVED_1,   /* unused */
  B_AUDIO_RESERVED_2,   /* unused */
  B_AUDIO_DEBUG_ON,     /* unused */
  B_AUDIO_DEBUG_OFF,    /* unused */
  B_AUDIO_WRITE_BUFFER,
  B_AUDIO_READ_BUFFER,
  B_AUDIO_LOCK_FOR_DMA
};

enum {
  B_AUDIO_ADC_SOURCE_LINE = 0,
  B_AUDIO_ADC_SOURCE_CDROM,
  B_AUDIO_ADC_SOURCE_MIC,
  B_AUDIO_ADC_SOURCE_LOOPBACK
};

enum {
  B_AUDIO_SAMPLE_RATE_8000  = 0,
  B_AUDIO_SAMPLE_RATE_5510  = 1,
  B_AUDIO_SAMPLE_RATE_16000 = 2,
  B_AUDIO_SAMPLE_RATE_11025 = 3,
  B_AUDIO_SAMPLE_RATE_27420 = 4,
  B_AUDIO_SAMPLE_RATE_18900 = 5,
  B_AUDIO_SAMPLE_RATE_32000 = 6,
  B_AUDIO_SAMPLE_RATE_22050 = 7,
  B_AUDIO_SAMPLE_RATE_37800 = 9,
  B_AUDIO_SAMPLE_RATE_44100 = 11,
  B_AUDIO_SAMPLE_RATE_48000 = 12,
  B_AUDIO_SAMPLE_RATE_33075 = 13,
  B_AUDIO_SAMPLE_RATE_9600  = 14,
  B_AUDIO_SAMPLE_RATE_6620  = 15
};

struct audio_channel {
  uint32 adc_source;		/* adc input source */
  char adc_gain;		/* 0-15 adc gain, in 1.5 dB steps */
  char mic_gain_enable;		/* non-zero enables 20 dB MIC input gain */
  char cd_mix_gain;		/* 0-31 cd mix to output gain in -1.5dB steps */
  char cd_mix_mute;		/* non-zero mutes cd mix */
  char aux2_mix_gain;		/* unused */
  char aux2_mix_mute;		/* unused */
  char line_mix_gain;		/* 0-31 line mix to out gain in -1.5dB steps */
  char line_mix_mute;		/* non-zero mutes line mix */
  char dac_attn;		/* 0-61 dac attenuation, in -1.5 dB steps */
  char dac_mute;		/* non-zero mutes dac output */
  char reserved_1;
  char reserved_2;
};

typedef struct audio_params {
  struct audio_channel left;	/* left channel setup */
  struct audio_channel right;	/* right channel setup */
  uint32 sample_rate;		/* sample rate */
  uint32 playback_format;	/* ignore (always 16bit-linear) */
  uint32 capture_format;	/* ignore (always 16bit-linear) */
  char dither_enable;		/* non-zero enables dither on 16 => 8 bit */
  char mic_attn;		/* 0..64 mic input level */
  char mic_enable;		/* non-zero enables mic input */
  char output_boost;		/* ignore (always on) */
  char highpass_enable;		/* ignore (always on) */
  char mono_gain;		/* 0..64 mono speaker gain */
  char mono_mute;		/* non-zero mutes speaker */
} audio_params;

typedef struct audio_buffer_header {
  int32 buffer_number;
  int32 subscriber_count;
  bigtime_t time;
  int32 reserved_1;
  int32 reserved_2;
  bigtime_t sample_clock;
} audio_buffer_header;

#endif
