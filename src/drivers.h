/*
 * $Id: drivers.h 1.3 1996/05/24 08:30:44 chasan released $
 *
 * Audio device drivers interface
 *
 * Copyright (C) 1995, 1996 Carlos Hasan. All Rights Reserved.
 *
 */

#ifndef __DRIVERS_H
#define __DRIVERS_H

#ifndef __AUDIO_H
#include "audio.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Audio device driver structures
 */
typedef struct {
    UINT    (AIAPI* GetAudioCaps)(PAUDIOCAPS);
    UINT    (AIAPI* PingAudio)(VOID);
    UINT    (AIAPI* OpenAudio)(PAUDIOINFO);
    UINT    (AIAPI* CloseAudio)(VOID);
    UINT    (AIAPI* UpdateAudio)(VOID);
    UINT    (AIAPI* SetAudioCallback)(PFNAUDIOWAVE);
} AUDIOWAVEDRIVER, *PAUDIOWAVEDRIVER;

typedef struct {
    UINT    (AIAPI* GetAudioCaps)(PAUDIOCAPS);
    UINT    (AIAPI* PingAudio)(VOID);
    UINT    (AIAPI* OpenAudio)(PAUDIOINFO);
    UINT    (AIAPI* CloseAudio)(VOID);
    UINT    (AIAPI* UpdateAudio)(VOID);
    UINT    (AIAPI* OpenVoices)(UINT);
    UINT    (AIAPI* CloseVoices)(VOID);
    UINT    (AIAPI* SetAudioTimerProc)(PFNAUDIOTIMER);
    UINT    (AIAPI* SetAudioTimerRate)(UINT);
    LONG    (AIAPI* GetAudioDataAvail)(VOID);
    UINT    (AIAPI* CreateAudioData)(PAUDIOWAVE);
    UINT    (AIAPI* DestroyAudioData)(PAUDIOWAVE);
    UINT    (AIAPI* WriteAudioData)(PAUDIOWAVE, ULONG, UINT);
    UINT    (AIAPI* PrimeVoice)(UINT, PAUDIOWAVE);
    UINT    (AIAPI* StartVoice)(UINT);
    UINT    (AIAPI* StopVoice)(UINT);
    UINT    (AIAPI* SetVoicePosition)(UINT, LONG);
    UINT    (AIAPI* SetVoiceFrequency)(UINT, LONG);
    UINT    (AIAPI* SetVoiceVolume)(UINT, UINT);
    UINT    (AIAPI* SetVoicePanning)(UINT, UINT);
    UINT    (AIAPI* GetVoicePosition)(UINT, PLONG);
    UINT    (AIAPI* GetVoiceFrequency)(UINT, PLONG);
    UINT    (AIAPI* GetVoiceVolume)(UINT, PUINT);
    UINT    (AIAPI* GetVoicePanning)(UINT, PUINT);
    UINT    (AIAPI* GetVoiceStatus)(UINT, PBOOL);
} AUDIOSYNTHDRIVER, *PAUDIOSYNTHDRIVER;

typedef struct {
    PAUDIOWAVEDRIVER pWaveDriver;
    PAUDIOSYNTHDRIVER pSynthDriver;
} AUDIODRIVER, *PAUDIODRIVER;

/*
 * External device-independant software drivers
 */
extern AUDIODRIVER NoneDriver;
extern AUDIOWAVEDRIVER NoneWaveDriver;
extern AUDIOSYNTHDRIVER NoneSynthDriver;
extern AUDIOSYNTHDRIVER EmuSynthDriver;

#ifdef __cplusplus
};
#endif

#endif
