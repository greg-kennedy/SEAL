/*
 * @(#)audiodj.c  0.2 1998/12/20 Carlos Hasan (chasan@dcc.uchile.cl)
 *
 * Hack for DJGPP 2.01 to lock code and data into a contiguous memory region.
 *
 * Copyright (C) 1998-1999 Carlos Hasan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <dpmi.h>
#include <sys/nearptr.h>

static int __SEAL_BSS_BEGIN;
static int __SEAL_DATA_BEGIN = 0xCafe;
static int __SEAL_TEXT_BEGIN(void) { return 0; }

static void SEAL_LOCK_MEMORY(void);

#include "audio.c"
#include "wavfile.c"
#include "xmfile.c"
#include "s3mfile.c"
#include "modfile.c"
#include "mtmfile.c"
#include "iofile.c"
#include "modeng.c"

#undef BUFFERSIZE
#undef CLIP
#undef TIMEOUT
#undef DEBUG

#undef GetAudioCaps
#undef PingAudio
#undef OpenAudio
#undef CloseAudio
#undef UpdateAudio
#undef SetAudioCallback

#define GetAudioCaps noneGetAudioCaps
#define PingAudio nonePingAudio
#define OpenAudio noneOpenAudio
#define CloseAudio noneCloseAudio
#define UpdateAudio noneUpdateAudio
#define SetAudioCallback noneSetAudioCallback

#include "nondrv.c"

#undef BUFFERSIZE
#undef CLIP
#undef TIMEOUT
#undef DEBUG

#undef GetAudioCaps
#undef PingAudio
#undef OpenAudio
#undef CloseAudio
#undef UpdateAudio
#undef SetAudioCallback

#define GetAudioCaps sbGetAudioCaps
#define PingAudio sbPingAudio
#define OpenAudio sbOpenAudio
#define CloseAudio sbCloseAudio
#define UpdateAudio sbUpdateAudio
#define SetAudioCallback sbSetAudioCallback

#include "sbdrv.c"

#undef BUFFERSIZE
#undef CLIP
#undef TIMEOUT
#undef DEBUG

#undef GetAudioCaps
#undef PingAudio
#undef OpenAudio
#undef CloseAudio
#undef UpdateAudio
#undef SetAudioCallback

#define GetAudioCaps pasGetAudioCaps
#define PingAudio pasPingAudio
#define OpenAudio pasOpenAudio
#define CloseAudio pasCloseAudio
#define UpdateAudio pasUpdateAudio
#define SetAudioCallback pasSetAudioCallback

#include "pasdrv.c"

#undef BUFFERSIZE
#undef CLIP
#undef TIMEOUT
#undef DEBUG

#undef GetAudioCaps
#undef PingAudio
#undef OpenAudio
#undef CloseAudio
#undef UpdateAudio
#undef SetAudioCallback

#define GetAudioCaps wssGetAudioCaps
#define PingAudio wssPingAudio
#define OpenAudio wssOpenAudio
#define CloseAudio wssCloseAudio
#define UpdateAudio wssUpdateAudio
#define SetAudioCallback wssSetAudioCallback

#include "wssdrv.c"

#undef BUFFERSIZE
#undef CLIP
#undef TIMEOUT
#undef DEBUG

#undef GetAudioCaps
#undef PingAudio
#undef OpenAudio
#undef CloseAudio
#undef UpdateAudio
#undef SetAudioCallback

#define GetAudioCaps ariaGetAudioCaps
#define PingAudio ariaPingAudio
#define OpenAudio ariaOpenAudio
#define CloseAudio ariaCloseAudio
#define UpdateAudio ariaUpdateAudio
#define SetAudioCallback ariaSetAudioCallback

#include "ariadrv.c"


#undef BUFFERSIZE
#undef CLIP
#undef TIMEOUT
#undef DEBUG

#undef VOICE_START
#undef VOICE_STOP
#undef VOICE_LOOP
#undef VOICE_BIDILOOP

#undef aVolumeTable
#undef aFrequencyTable

#define aVolumeTable mixVolumeTable
#define aFrequencyTable mixFrequencyTable

#undef GetAudioCaps
#undef PingAudio
#undef OpenAudio
#undef CloseAudio
#undef UpdateAudio
#undef SetAudioCallback
#undef SetAudioMixerValue
#undef OpenVoices
#undef CloseVoices
#undef GetAudioDataAvail
#undef CreateAudioData
#undef DestroyAudioData
#undef WriteAudioData
#undef PrimeVoice
#undef StartVoice
#undef StopVoice
#undef SetVoicePosition
#undef SetVoiceFrequency
#undef SetVoiceVolume
#undef SetVoicePanning
#undef GetVoicePosition
#undef GetVoiceFrequency
#undef GetVoiceVolume
#undef GetVoicePanning
#undef GetVoiceStatus
#undef SetAudioTimerProc
#undef SetAudioTimerRate

#define GetAudioCaps mixGetAudioCaps
#define PingAudio mixPingAudio
#define OpenAudio mixOpenAudio
#define CloseAudio mixCloseAudio
#define UpdateAudio mixUpdateAudio
#define SetAudioCallback mixSetAudioCallback
#define SetAudioMixerValue mixSetAudioMixerValue
#define OpenVoices mixOpenVoices
#define CloseVoices mixCloseVoices
#define GetAudioDataAvail mixGetAudioDataAvail
#define CreateAudioData mixCreateAudioData
#define DestroyAudioData mixDestroyAudioData
#define WriteAudioData mixWriteAudioData
#define PrimeVoice mixPrimeVoice
#define StartVoice mixStartVoice
#define StopVoice mixStopVoice
#define SetVoicePosition mixSetVoicePosition
#define SetVoiceFrequency mixSetVoiceFrequency
#define SetVoiceVolume mixSetVoiceVolume
#define SetVoicePanning mixSetVoicePanning
#define GetVoicePosition mixGetVoicePosition
#define GetVoiceFrequency mixGetVoiceFrequency
#define GetVoiceVolume mixGetVoiceVolume
#define GetVoicePanning mixGetVoicePanning
#define GetVoiceStatus mixGetVoiceStatus
#define SetAudioTimerProc mixSetAudioTimerProc
#define SetAudioTimerRate mixSetAudioTimerRate

#include "mixdrv.c"

#undef BUFFERSIZE
#undef CLIP
#undef TIMEOUT
#undef DEBUG

#undef VOICE_START
#undef VOICE_STOP
#undef VOICE_LOOP
#undef VOICE_BIDILOOP

#undef aVolumeTable
#undef aFrequencyTable

#define aVolumeTable aweVolumeTable
#define aFrequencyTable aweFrequencyTable

#undef GetAudioCaps
#undef PingAudio
#undef OpenAudio
#undef CloseAudio
#undef UpdateAudio
#undef SetAudioCallback
#undef SetAudioMixerValue
#undef OpenVoices
#undef CloseVoices
#undef GetAudioDataAvail
#undef CreateAudioData
#undef DestroyAudioData
#undef WriteAudioData
#undef PrimeVoice
#undef StartVoice
#undef StopVoice
#undef SetVoicePosition
#undef SetVoiceFrequency
#undef SetVoiceVolume
#undef SetVoicePanning
#undef GetVoicePosition
#undef GetVoiceFrequency
#undef GetVoiceVolume
#undef GetVoicePanning
#undef GetVoiceStatus
#undef SetAudioTimerProc
#undef SetAudioTimerRate

#define GetAudioCaps aweGetAudioCaps
#define PingAudio awePingAudio
#define OpenAudio aweOpenAudio
#define CloseAudio aweCloseAudio
#define UpdateAudio aweUpdateAudio
#define SetAudioCallback aweSetAudioCallback
#define SetAudioMixerValue aweSetAudioMixerValue
#define OpenVoices aweOpenVoices
#define CloseVoices aweCloseVoices
#define GetAudioDataAvail aweGetAudioDataAvail
#define CreateAudioData aweCreateAudioData
#define DestroyAudioData aweDestroyAudioData
#define WriteAudioData aweWriteAudioData
#define PrimeVoice awePrimeVoice
#define StartVoice aweStartVoice
#define StopVoice aweStopVoice
#define SetVoicePosition aweSetVoicePosition
#define SetVoiceFrequency aweSetVoiceFrequency
#define SetVoiceVolume aweSetVoiceVolume
#define SetVoicePanning aweSetVoicePanning
#define GetVoicePosition aweGetVoicePosition
#define GetVoiceFrequency aweGetVoiceFrequency
#define GetVoiceVolume aweGetVoiceVolume
#define GetVoicePanning aweGetVoicePanning
#define GetVoiceStatus aweGetVoiceStatus
#define SetAudioTimerProc aweSetAudioTimerProc
#define SetAudioTimerRate aweSetAudioTimerRate

#include "awedrv.c"

#undef BUFFERSIZE
#undef CLIP
#undef TIMEOUT
#undef DEBUG

#undef VOICE_START
#undef VOICE_STOP
#undef VOICE_LOOP
#undef VOICE_BIDILOOP

#undef aVolumeTable
#undef aFrequencyTable

#define aVolumeTable gusVolumeTable
#define aFrequencyTable gusFrequencyTable

#undef GetAudioCaps
#undef PingAudio
#undef OpenAudio
#undef CloseAudio
#undef UpdateAudio
#undef SetAudioCallback
#undef SetAudioMixerValue
#undef OpenVoices
#undef CloseVoices
#undef GetAudioDataAvail
#undef CreateAudioData
#undef DestroyAudioData
#undef WriteAudioData
#undef PrimeVoice
#undef StartVoice
#undef StopVoice
#undef SetVoicePosition
#undef SetVoiceFrequency
#undef SetVoiceVolume
#undef SetVoicePanning
#undef GetVoicePosition
#undef GetVoiceFrequency
#undef GetVoiceVolume
#undef GetVoicePanning
#undef GetVoiceStatus
#undef SetAudioTimerProc
#undef SetAudioTimerRate

#define GetAudioCaps gusGetAudioCaps
#define PingAudio gusPingAudio
#define OpenAudio gusOpenAudio
#define CloseAudio gusCloseAudio
#define UpdateAudio gusUpdateAudio
#define SetAudioCallback gusSetAudioCallback
#define SetAudioMixerValue gusSetAudioMixerValue
#define OpenVoices gusOpenVoices
#define CloseVoices gusCloseVoices
#define GetAudioDataAvail gusGetAudioDataAvail
#define CreateAudioData gusCreateAudioData
#define DestroyAudioData gusDestroyAudioData
#define WriteAudioData gusWriteAudioData
#define PrimeVoice gusPrimeVoice
#define StartVoice gusStartVoice
#define StopVoice gusStopVoice
#define SetVoicePosition gusSetVoicePosition
#define SetVoiceFrequency gusSetVoiceFrequency
#define SetVoiceVolume gusSetVoiceVolume
#define SetVoicePanning gusSetVoicePanning
#define GetVoicePosition gusGetVoicePosition
#define GetVoiceFrequency gusGetVoiceFrequency
#define GetVoiceVolume gusGetVoiceVolume
#define GetVoicePanning gusGetVoicePanning
#define GetVoiceStatus gusGetVoiceStatus
#define SetAudioTimerProc gusSetAudioTimerProc
#define SetAudioTimerRate gusSetAudioTimerRate

#include "gusdrv.c"


#include "msdos.c"

static int __SEAL_BSS_END;
static int __SEAL_DATA_END = 0xCafe;
static int __SEAL_TEXT_END(void) { return 0; }

static void SEAL_LOCK_MEMORY(void)
{
    /* lock in host memory the text, data and bss segments */
    _go32_dpmi_lock_code(__SEAL_TEXT_BEGIN, 
			 (long) __SEAL_TEXT_END - (long) __SEAL_TEXT_BEGIN);

    _go32_dpmi_lock_data(&__SEAL_DATA_BEGIN,
			 (long) &__SEAL_DATA_END - (long) &__SEAL_DATA_BEGIN);

    _go32_dpmi_lock_data(&__SEAL_BSS_BEGIN,
			 (long) &__SEAL_BSS_END - (long) &__SEAL_BSS_BEGIN);

    /* lock conventional memory used for DMA buffers and IRQ callbacks */
    _crt0_startup_flags |= _CRT0_FLAG_NEARPTR;
    __djgpp_nearptr_enable();
    _go32_dpmi_lock_data((void *) __djgpp_conventional_base, 640 << 10);
}
