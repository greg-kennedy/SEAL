/*
 * $Id: audio.c 1.13 1996/12/12 16:32:06 chasan Exp $
 *              1.14 1998/10/18 14:59:21 chasan (BeOS and OS/2 driver)
 *              1.15 1998/11/30 18:20:26 chasan (Mixer and UpdateAudioEx API)
 *
 * Audio device drivers API interface
 *
 * Copyright (C) 1995-1999 Carlos Hasan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifdef __GNUC__
#include <memory.h>
#endif
#include <string.h>
#include <malloc.h>
#include "audio.h"
#include "drivers.h"


/*
 * Copyright information hard-coded into the library
 */
CHAR szAudioCopyright[] =
"SEAL Synthetic Audio Library 1.07 (Build " __DATE__ ")\n"
"Copyright (C) 1995, 1996, 1997, 1998, 1999 Carlos Hasan\n";
#ifdef __OS2__
CHAR szMartyCopyright[] =
"OS/2 Audio Driver Copyright (C) 1998 by Martin Amodeo";
#endif


/*
 * Audio device drivers resource limits
 */
#define MAXDRIVERS              8
#define MAXVOICES               32

/*
 * Macros to work with audio channels handles
 */
#define VOICENUM(handle)        ((UINT)handle-1)

/*
 * Table of registered audio device drivers and channels.
 */
static LPAUDIODRIVER aDriverTable[MAXDRIVERS];
static HAC aVoiceTable[MAXVOICES];
static UINT nNumVoices;

/*
 * Currently active audio device driver virtual method tables
 */
static UINT nDriverId = AUDIO_DEVICE_MAPPER;
static AUDIOWAVEDRIVER WaveDriver;
static AUDIOSYNTHDRIVER SynthDriver;


/*
 * Audio device drivers API interface
 */
UINT AIAPI AAudioFatalError(UINT nErrorCode)
{
    nNumVoices = 0;
    nDriverId = AUDIO_DEVICE_MAPPER;
    memset(aVoiceTable, 0, sizeof(aVoiceTable));
    memcpy(&WaveDriver, &NoneWaveDriver, sizeof(AUDIOWAVEDRIVER));
    memcpy(&SynthDriver, &NoneSynthDriver, sizeof(AUDIOSYNTHDRIVER));
    return nErrorCode;
}

UINT AIAPI AGetErrorText(UINT nErrorCode, LPSTR lpText, UINT nSize)
{
    static LPSTR aErrorTable[] =
    {
        "Unknown audio system error",
        "Bad channel or sample handle",
        "Invalid parameter passed",
        "Invalid audio system call",
        "Bad audio device identifier",
        "No audio device found",
        "Audio device is busy",
        "Bad audio data format",
        "Not enough memory",
        "Not enough device memory",
        "File not found",
        "Bad file format"
    };

    if (lpText != NULL) {
        if (nErrorCode <= AUDIO_LAST_ERROR) {
            strncpy(lpText, aErrorTable[nErrorCode], nSize);
            return AUDIO_ERROR_NONE;
        }
        strncpy(lpText, aErrorTable[AUDIO_ERROR_NONE], nSize);
    }
    return AUDIO_ERROR_INVALPARAM;
}

UINT AIAPI ARegisterAudioDriver(LPAUDIODRIVER lpDriver)
{
    UINT nDeviceId;

    for (nDeviceId = 0; nDeviceId < MAXDRIVERS; nDeviceId++) {
        if (aDriverTable[nDeviceId] == NULL ||
            aDriverTable[nDeviceId] == lpDriver) {
            aDriverTable[nDeviceId] = lpDriver;
            return AUDIO_ERROR_NONE;
        }
    }
    return AUDIO_ERROR_NOMEMORY;
}

UINT AIAPI AGetAudioNumDevs(VOID)
{
    UINT nDeviceId, nNumDevs;

    nNumDevs = 0;
    for (nDeviceId = 0; nDeviceId < MAXDRIVERS; nDeviceId++) {
        if (aDriverTable[nDeviceId] != NULL)
            nNumDevs++;
    }
    return nNumDevs;
}

UINT AIAPI AGetAudioDevCaps(UINT nDeviceId, LPAUDIOCAPS lpCaps)
{
    LPAUDIODRIVER lpDriver;

    if (lpCaps != NULL) {
        memset(lpCaps, 0, sizeof(AUDIOCAPS));
        if (nDeviceId < MAXDRIVERS) {
            if ((lpDriver = aDriverTable[nDeviceId]) != NULL) {
                if (lpDriver->lpWaveDriver != NULL) {
                    return lpDriver->lpWaveDriver->GetAudioCaps(lpCaps);
                }
                else if (lpDriver->lpSynthDriver != NULL) {
                    return lpDriver->lpSynthDriver->GetAudioCaps(lpCaps);
                }
            }
        }
        return AUDIO_ERROR_BADDEVICEID;
    }
    return AUDIO_ERROR_INVALPARAM;
}

UINT AIAPI APingAudio(LPUINT lpnDeviceId)
{
    LPAUDIODRIVER lpDriver;
    UINT nDeviceId;

    if (lpnDeviceId != NULL) {
        *lpnDeviceId = AUDIO_DEVICE_NONE;
        if (nDriverId == AUDIO_DEVICE_MAPPER) {
            for (nDeviceId = MAXDRIVERS - 1; nDeviceId != 0; nDeviceId--) {
                if ((lpDriver = aDriverTable[nDeviceId]) != NULL) {
                    if (lpDriver->lpWaveDriver != NULL &&
                        !lpDriver->lpWaveDriver->PingAudio()) {
                        *lpnDeviceId = nDeviceId;
                        return AUDIO_ERROR_NONE;
                    }
                    else if (lpDriver->lpSynthDriver != NULL &&
			     !lpDriver->lpSynthDriver->PingAudio()) {
                        *lpnDeviceId = nDeviceId;
                        return AUDIO_ERROR_NONE;
                    }
                }
            }
            return AUDIO_ERROR_NODEVICE;
        }
        return AUDIO_ERROR_NOTSUPPORTED;
    }
    return AUDIO_ERROR_INVALPARAM;
}

UINT AIAPI AOpenAudio(LPAUDIOINFO lpInfo)
{
    LPAUDIODRIVER lpDriver;
    LPAUDIOWAVEDRIVER lpWaveDriver;
    LPAUDIOSYNTHDRIVER lpSynthDriver;
    UINT nErrorCode;

    if (nDriverId == AUDIO_DEVICE_MAPPER) {
        if (lpInfo != NULL && lpInfo->nDeviceId == AUDIO_DEVICE_MAPPER) {
            nErrorCode = APingAudio(&lpInfo->nDeviceId);
            if (nErrorCode != AUDIO_ERROR_NONE)
                return AAudioFatalError(nErrorCode);
        }
        if (lpInfo != NULL && lpInfo->nDeviceId < MAXDRIVERS) {
            if ((lpDriver = aDriverTable[lpInfo->nDeviceId]) != NULL) {
                if ((lpWaveDriver = lpDriver->lpWaveDriver) == NULL)
                    lpWaveDriver = &NoneWaveDriver;
                if ((lpSynthDriver = lpDriver->lpSynthDriver) == NULL)
                    lpSynthDriver = &EmuSynthDriver;
                memcpy(&WaveDriver, lpWaveDriver, sizeof(AUDIOWAVEDRIVER));
                memcpy(&SynthDriver, lpSynthDriver, sizeof(AUDIOSYNTHDRIVER));
                nErrorCode = WaveDriver.OpenAudio(lpInfo);
                if (nErrorCode != AUDIO_ERROR_NONE)
                    return AAudioFatalError(nErrorCode);
                nErrorCode = SynthDriver.OpenAudio(lpInfo);
                if (nErrorCode != AUDIO_ERROR_NONE) {
                    WaveDriver.CloseAudio();
                    return AAudioFatalError(nErrorCode);
                }
                memset(aVoiceTable, 0, sizeof(aVoiceTable));
                nNumVoices = 0;
                nDriverId = lpInfo->nDeviceId;
                return AUDIO_ERROR_NONE;
            }
        }
        return AAudioFatalError(lpInfo != NULL ? AUDIO_ERROR_BADDEVICEID :
				AUDIO_ERROR_INVALPARAM);
    }
    return AUDIO_ERROR_NOTSUPPORTED;
}

UINT AIAPI ACloseAudio(VOID)
{
    UINT nErrorCode;

    nDriverId = AUDIO_DEVICE_MAPPER;
    if ((nErrorCode = SynthDriver.CloseAudio()) != AUDIO_ERROR_NONE)
        return AAudioFatalError(nErrorCode);
    if ((nErrorCode = WaveDriver.CloseAudio()) != AUDIO_ERROR_NONE)
        return AAudioFatalError(nErrorCode);
    return AAudioFatalError(AUDIO_ERROR_NONE);
}

UINT AIAPI AUpdateAudio(VOID)
{
    return AUpdateAudioEx(0);
}

UINT AIAPI AUpdateAudioEx(UINT nFrames)
{
    static UINT nSemalphore = 0;
    UINT nErrorCode = AUDIO_ERROR_NONE;

    /* TODO: This is not a real semalphore, may fail sometimes. */
    if (nDriverId != AUDIO_DEVICE_MAPPER) {
        if (!nSemalphore++) nErrorCode = WaveDriver.UpdateAudio(nFrames);
        nSemalphore--;
    }
    return nErrorCode;
}

UINT AIAPI AOpenVoices(UINT nVoices)
{
    UINT nErrorCode;

    memset(aVoiceTable, 0, sizeof(aVoiceTable));
    if ((nErrorCode = SynthDriver.OpenVoices(nVoices)) == AUDIO_ERROR_NONE)
        nNumVoices = nVoices;
    return nErrorCode;
}

UINT AIAPI ACloseVoices(VOID)
{
    UINT nVoice;

    for (nVoice = 0; nVoice < nNumVoices; nVoice++) {
        if (aVoiceTable[nVoice] != (HAC) NULL)
            return AUDIO_ERROR_NOTSUPPORTED;
    }
    nNumVoices = 0;
    memset(aVoiceTable, 0, sizeof(aVoiceTable));
    return SynthDriver.CloseVoices();
}

UINT AIAPI ASetAudioCallback(LPFNAUDIOWAVE lpfnAudioWave)
{
    return WaveDriver.SetAudioCallback(lpfnAudioWave);
}

UINT AIAPI ASetAudioTimerProc(LPFNAUDIOTIMER lpfnAudioTimer)
{
    return SynthDriver.SetAudioTimerProc(lpfnAudioTimer);
}

UINT AIAPI ASetAudioTimerRate(UINT nTimerRate)
{
    return SynthDriver.SetAudioTimerRate(nTimerRate);
}

UINT AIAPI ASetAudioMixerValue(UINT nChannel, UINT nValue)
{
    return SynthDriver.SetAudioMixerValue(nChannel, nValue);
}

LONG AIAPI AGetAudioDataAvail(VOID)
{
    return SynthDriver.GetAudioDataAvail();
}

UINT AIAPI ACreateAudioData(LPAUDIOWAVE lpWave)
{
    UINT nErrorCode;

    if (lpWave != NULL) {
        if ((lpWave->lpData = malloc(lpWave->dwLength + 4)) != NULL) {
            nErrorCode = SynthDriver.CreateAudioData(lpWave);
            if (nErrorCode != AUDIO_ERROR_NONE) {
                free(lpWave->lpData);
                lpWave->lpData = NULL;
            }
            return nErrorCode;
        }
        return AUDIO_ERROR_NOMEMORY;
    }
    return AUDIO_ERROR_INVALPARAM;
}

UINT AIAPI ADestroyAudioData(LPAUDIOWAVE lpWave)
{
    UINT nErrorCode;

    if (lpWave != NULL) {
        nErrorCode = SynthDriver.DestroyAudioData(lpWave);
        if (lpWave->lpData != NULL)
            free(lpWave->lpData);
        return nErrorCode;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

UINT AIAPI AWriteAudioData(LPAUDIOWAVE lpWave, DWORD dwOffset, UINT nCount)
{
    if (lpWave != NULL && lpWave->lpData != NULL) {
        if (nCount != 0) {
            return SynthDriver.WriteAudioData(lpWave, dwOffset, nCount);
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

UINT AIAPI ACreateAudioVoice(LPHAC lphVoice)
{
    UINT nVoice;

    if (lphVoice != NULL) {
        for (nVoice = 0; nVoice < nNumVoices; nVoice++) {
            if (!aVoiceTable[nVoice]) {
                *lphVoice = aVoiceTable[nVoice] = (HAC) (nVoice + 1);
                return AUDIO_ERROR_NONE;
            }
        }
        return AUDIO_ERROR_NOMEMORY;
    }
    return AUDIO_ERROR_INVALPARAM;
}

UINT AIAPI ADestroyAudioVoice(HAC hVoice)
{
    UINT nVoice;

    if ((nVoice = VOICENUM(hVoice)) < nNumVoices) {
        aVoiceTable[nVoice] = (HAC) NULL;
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

UINT AIAPI APlayVoice(HAC hVoice, LPAUDIOWAVE lpWave)
{
    UINT nErrorCode;

    if (lpWave != NULL) {
        if (!(nErrorCode = APrimeVoice(hVoice, lpWave)) &&
            !(nErrorCode = ASetVoiceFrequency(hVoice, lpWave->nSampleRate)))
            return AStartVoice(hVoice);
        return nErrorCode;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

UINT AIAPI APrimeVoice(HAC hVoice, LPAUDIOWAVE lpWave)
{
    return SynthDriver.PrimeVoice(VOICENUM(hVoice), lpWave);
}

UINT AIAPI AStartVoice(HAC hVoice)
{
    return SynthDriver.StartVoice(VOICENUM(hVoice));
}

UINT AIAPI AStopVoice(HAC hVoice)
{
    return SynthDriver.StopVoice(VOICENUM(hVoice));
}

UINT AIAPI ASetVoicePosition(HAC hVoice, LONG dwPosition)
{
    return SynthDriver.SetVoicePosition(VOICENUM(hVoice), dwPosition);
}

UINT AIAPI ASetVoiceFrequency(HAC hVoice, LONG dwFrequency)
{
    return SynthDriver.SetVoiceFrequency(VOICENUM(hVoice), dwFrequency);
}

UINT AIAPI ASetVoiceVolume(HAC hVoice, UINT nVolume)
{
    return SynthDriver.SetVoiceVolume(VOICENUM(hVoice), nVolume);
}

UINT AIAPI ASetVoicePanning(HAC hVoice, UINT nPanning)
{
    return SynthDriver.SetVoicePanning(VOICENUM(hVoice), nPanning);
}

UINT AIAPI AGetVoicePosition(HAC hVoice, LPLONG lpdwPosition)
{
    return SynthDriver.GetVoicePosition(VOICENUM(hVoice), lpdwPosition);
}

UINT AIAPI AGetVoiceFrequency(HAC hVoice, LPLONG lpdwFrequency)
{
    return SynthDriver.GetVoiceFrequency(VOICENUM(hVoice), lpdwFrequency);
}

UINT AIAPI AGetVoiceVolume(HAC hVoice, LPUINT lpnVolume)
{
    return SynthDriver.GetVoiceVolume(VOICENUM(hVoice), lpnVolume);
}

UINT AIAPI AGetVoicePanning(HAC hVoice, LPUINT lpnPanning)
{
    return SynthDriver.GetVoicePanning(VOICENUM(hVoice), lpnPanning);
}

UINT AIAPI AGetVoiceStatus(HAC hVoice, LPBOOL lpnStatus)
{
    return SynthDriver.GetVoiceStatus(VOICENUM(hVoice), lpnStatus);
}



/*
 * External system-dependant audio device drivers
 */
#ifdef __BEOS__
extern AUDIODRIVER BeOSR3Driver;
extern AUDIODRIVER BeOSDriver;
#endif
#ifdef __OS2__
extern AUDIODRIVER OS2MMPMDriver;
#endif
#ifdef __LINUX__
extern AUDIODRIVER LinuxDriver;
#endif
#ifdef __FREEBSD__
extern AUDIODRIVER LinuxDriver;
#endif
#ifdef __SPARC__
extern AUDIODRIVER SparcDriver;
#endif
#ifdef __SOLARIS__
extern AUDIODRIVER SparcDriver;
#endif
#ifdef __SILICON__
extern AUDIODRIVER SiliconDriver;
#endif
#ifdef __WINDOWS__
extern AUDIODRIVER WindowsDriver;
extern AUDIODRIVER DirectSoundDriver;
extern AUDIODRIVER DirectSoundAccelDriver;
#endif
#if defined(__DOS16__) || defined(__DPMI__)
extern AUDIODRIVER SoundBlasterDriver;
extern AUDIODRIVER SoundBlaster32Driver;
extern AUDIODRIVER ProAudioSpectrumDriver;
extern AUDIODRIVER UltraSoundDriver;
extern AUDIODRIVER UltraSoundMaxDriver;
extern AUDIODRIVER WinSndSystemDriver;
extern AUDIODRIVER SoundscapeDriver;
extern AUDIODRIVER AriaDriver;
#endif


UINT AIAPI AGetVersion(VOID)
{
    /* return the current hard-coded audio system version */
    return AUDIO_SYSTEM_VERSION;
}

UINT AIAPI AInitialize(VOID)
{
#ifdef __DJGPP__
    /* 1998/12/20 lock code and data memory */
    SEAL_LOCK_MEMORY();
#endif

    /*
     * Initialize all the audio system state variables,
     * and registers all the built-in audio drivers.
     */
    if (nDriverId != AUDIO_DEVICE_MAPPER)
        return AUDIO_ERROR_NOTSUPPORTED;

    ARegisterAudioDriver(&NoneDriver);
#ifdef __BEOS__
    ARegisterAudioDriver(&BeOSR3Driver);
    ARegisterAudioDriver(&BeOSDriver);
#endif
#ifdef __OS2__
    ARegisterAudioDriver(&OS2MMPMDriver);
#endif
#ifdef __LINUX__
    ARegisterAudioDriver(&LinuxDriver);
#endif
#ifdef __FREEBSD__
    ARegisterAudioDriver(&LinuxDriver);
#endif
#ifdef __SPARC__
    ARegisterAudioDriver(&SparcDriver);
#endif
#ifdef __SOLARIS__
    ARegisterAudioDriver(&SparcDriver);
#endif
#ifdef __SILICON__
    ARegisterAudioDriver(&SiliconDriver);
#endif
#ifdef __WINDOWS__
    ARegisterAudioDriver(&WindowsDriver);
    ARegisterAudioDriver(&DirectSoundAccelDriver);
    ARegisterAudioDriver(&DirectSoundDriver);
#endif
#if defined(__DOS16__) || defined(__DPMI__)
    ARegisterAudioDriver(&SoundBlasterDriver);
    ARegisterAudioDriver(&SoundBlaster32Driver);
    ARegisterAudioDriver(&ProAudioSpectrumDriver);
    ARegisterAudioDriver(&UltraSoundMaxDriver);
    ARegisterAudioDriver(&UltraSoundDriver);
    ARegisterAudioDriver(&WinSndSystemDriver);
    ARegisterAudioDriver(&SoundscapeDriver);
    ARegisterAudioDriver(&AriaDriver);
#endif

    return AAudioFatalError(AUDIO_ERROR_NONE);
}
