/*
 * $Id: drivers.h 1.4 1996/08/05 18:51:19 chasan released $
 *
 * Audio device drivers interface
 *
 * Copyright (C) 1995-1999 Carlos Hasan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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
	UINT    (AIAPI* GetAudioCaps)(LPAUDIOCAPS);
	UINT    (AIAPI* PingAudio)(VOID);
	UINT    (AIAPI* OpenAudio)(LPAUDIOINFO);
	UINT    (AIAPI* CloseAudio)(VOID);
	UINT    (AIAPI* UpdateAudio)(UINT);
	UINT    (AIAPI* SetAudioCallback)(LPFNAUDIOWAVE);
    } AUDIOWAVEDRIVER, *LPAUDIOWAVEDRIVER;

    typedef struct {
	UINT    (AIAPI* GetAudioCaps)(LPAUDIOCAPS);
	UINT    (AIAPI* PingAudio)(VOID);
	UINT    (AIAPI* OpenAudio)(LPAUDIOINFO);
	UINT    (AIAPI* CloseAudio)(VOID);
	UINT    (AIAPI* UpdateAudio)(VOID);
	UINT    (AIAPI* OpenVoices)(UINT);
	UINT    (AIAPI* CloseVoices)(VOID);
	UINT    (AIAPI* SetAudioTimerProc)(LPFNAUDIOTIMER);
	UINT    (AIAPI* SetAudioTimerRate)(UINT);
	UINT    (AIAPI* SetAudioMixerValue)(UINT, UINT); /*NEW:1998/10/24*/
	LONG    (AIAPI* GetAudioDataAvail)(VOID);
	UINT    (AIAPI* CreateAudioData)(LPAUDIOWAVE);
	UINT    (AIAPI* DestroyAudioData)(LPAUDIOWAVE);
	UINT    (AIAPI* WriteAudioData)(LPAUDIOWAVE, DWORD, UINT);
	UINT    (AIAPI* PrimeVoice)(UINT, LPAUDIOWAVE);
	UINT    (AIAPI* StartVoice)(UINT);
	UINT    (AIAPI* StopVoice)(UINT);
	UINT    (AIAPI* SetVoicePosition)(UINT, LONG);
	UINT    (AIAPI* SetVoiceFrequency)(UINT, LONG);
	UINT    (AIAPI* SetVoiceVolume)(UINT, UINT);
	UINT    (AIAPI* SetVoicePanning)(UINT, UINT);
	UINT    (AIAPI* GetVoicePosition)(UINT, LPLONG);
	UINT    (AIAPI* GetVoiceFrequency)(UINT, LPLONG);
	UINT    (AIAPI* GetVoiceVolume)(UINT, LPUINT);
	UINT    (AIAPI* GetVoicePanning)(UINT, LPUINT);
	UINT    (AIAPI* GetVoiceStatus)(UINT, LPBOOL);
    } AUDIOSYNTHDRIVER, *LPAUDIOSYNTHDRIVER;

    typedef struct {
	LPAUDIOWAVEDRIVER lpWaveDriver;
	LPAUDIOSYNTHDRIVER lpSynthDriver;
    } AUDIODRIVER, *LPAUDIODRIVER;

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
