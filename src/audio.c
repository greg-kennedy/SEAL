/*
 * $Id: audio.c 1.9 1996/06/02 17:02:07 chasan released $
 *
 * Audio device drivers API interface
 *
 * Copyright (C) 1995, 1996 Carlos Hasan. All Rights Reserved.
 *
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
char szAudioCopyright[] =
    "SEAL Synthetic Audio Library 1.0 Copyright (C) 1995, 1996 Carlos Hasan";


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
static PAUDIODRIVER aDriverTable[MAXDRIVERS];
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

UINT AIAPI AGetErrorText(UINT nErrorCode, PSZ pText, UINT nSize)
{
    static PSZ aErrorTable[] =
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

    if (pText != NULL) {
        if (nErrorCode <= AUDIO_LAST_ERROR) {
            strncpy(pText, aErrorTable[nErrorCode], nSize);
            return AUDIO_ERROR_NONE;
        }
        strncpy(pText, aErrorTable[AUDIO_ERROR_NONE], nSize);
    }
    return AUDIO_ERROR_INVALPARAM;
}

UINT AIAPI ARegisterAudioDriver(PAUDIODRIVER pDriver)
{
    UINT nDeviceId;

    for (nDeviceId = 0; nDeviceId < MAXDRIVERS; nDeviceId++) {
        if (aDriverTable[nDeviceId] == NULL ||
            aDriverTable[nDeviceId] == pDriver) {
            aDriverTable[nDeviceId] = pDriver;
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

UINT AIAPI AGetAudioDevCaps(UINT nDeviceId, PAUDIOCAPS pCaps)
{
    PAUDIODRIVER pDriver;

    if (pCaps != NULL) {
        memset(pCaps, 0, sizeof(AUDIOCAPS));
        if (nDeviceId < MAXDRIVERS) {
            if ((pDriver = aDriverTable[nDeviceId]) != NULL) {
                if (pDriver->pWaveDriver != NULL) {
                    return pDriver->pWaveDriver->GetAudioCaps(pCaps);
                }
                else if (pDriver->pSynthDriver != NULL) {
                    return pDriver->pSynthDriver->GetAudioCaps(pCaps);
                }
            }
        }
        return AUDIO_ERROR_BADDEVICEID;
    }
    return AUDIO_ERROR_INVALPARAM;
}

UINT AIAPI APingAudio(PUINT pnDeviceId)
{
    PAUDIODRIVER pDriver;
    UINT nDeviceId;

    if (pnDeviceId != NULL) {
        *pnDeviceId = AUDIO_DEVICE_NONE;
        if (nDriverId == AUDIO_DEVICE_MAPPER) {
            for (nDeviceId = MAXDRIVERS - 1; nDeviceId != 0; nDeviceId--) {
                if ((pDriver = aDriverTable[nDeviceId]) != NULL) {
                    if (pDriver->pWaveDriver != NULL &&
                        !pDriver->pWaveDriver->PingAudio()) {
                        *pnDeviceId = nDeviceId;
                        return AUDIO_ERROR_NONE;
                    }
                    else if (pDriver->pSynthDriver != NULL &&
                        !pDriver->pSynthDriver->PingAudio()) {
                        *pnDeviceId = nDeviceId;
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

UINT AIAPI AOpenAudio(PAUDIOINFO pInfo)
{
    PAUDIODRIVER pDriver;
    PAUDIOWAVEDRIVER pWaveDriver;
    PAUDIOSYNTHDRIVER pSynthDriver;
    UINT nErrorCode;

    if (nDriverId == AUDIO_DEVICE_MAPPER) {
        if (pInfo != NULL && pInfo->nDeviceId == AUDIO_DEVICE_MAPPER) {
            nErrorCode = APingAudio(&pInfo->nDeviceId);
            if (nErrorCode != AUDIO_ERROR_NONE)
                return AAudioFatalError(nErrorCode);
        }
        if (pInfo != NULL && pInfo->nDeviceId < MAXDRIVERS) {
            if ((pDriver = aDriverTable[pInfo->nDeviceId]) != NULL) {
                if ((pWaveDriver = pDriver->pWaveDriver) == NULL)
                    pWaveDriver = &NoneWaveDriver;
                if ((pSynthDriver = pDriver->pSynthDriver) == NULL)
                    pSynthDriver = &EmuSynthDriver;
                memcpy(&WaveDriver, pWaveDriver, sizeof(AUDIOWAVEDRIVER));
                memcpy(&SynthDriver, pSynthDriver, sizeof(AUDIOSYNTHDRIVER));
                nErrorCode = WaveDriver.OpenAudio(pInfo);
                if (nErrorCode != AUDIO_ERROR_NONE)
                    return AAudioFatalError(nErrorCode);
                nErrorCode = SynthDriver.OpenAudio(pInfo);
                if (nErrorCode != AUDIO_ERROR_NONE) {
                    WaveDriver.CloseAudio();
                    return AAudioFatalError(nErrorCode);
                }
                memset(aVoiceTable, 0, sizeof(aVoiceTable));
                nNumVoices = 0;
                nDriverId = pInfo->nDeviceId;
                return AUDIO_ERROR_NONE;
            }
        }
        return AAudioFatalError(pInfo != NULL ? AUDIO_ERROR_BADDEVICEID :
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
    static UINT nSemaphore = 0;
    UINT nErrorCode = AUDIO_ERROR_NONE;

    /* TODO: This is not a real semaphore, may fail sometimes. */
    if (nDriverId != AUDIO_DEVICE_MAPPER) {
        if (!nSemaphore++) nErrorCode = WaveDriver.UpdateAudio();
        nSemaphore--;
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

UINT AIAPI ASetAudioCallback(PFNAUDIOWAVE pfnAudioWave)
{
    return WaveDriver.SetAudioCallback(pfnAudioWave);
}

UINT AIAPI ASetAudioTimerProc(PFNAUDIOTIMER pfnAudioTimer)
{
    return SynthDriver.SetAudioTimerProc(pfnAudioTimer);
}

UINT AIAPI ASetAudioTimerRate(UINT nTimerRate)
{
    return SynthDriver.SetAudioTimerRate(nTimerRate);
}

LONG AIAPI AGetAudioDataAvail(VOID)
{
    return SynthDriver.GetAudioDataAvail();
}

UINT AIAPI ACreateAudioData(PAUDIOWAVE pWave)
{
    UINT nErrorCode;

    if (pWave != NULL) {
        if ((pWave->pData = malloc(pWave->dwLength + 4)) != NULL) {
            nErrorCode = SynthDriver.CreateAudioData(pWave);
            if (nErrorCode != AUDIO_ERROR_NONE) {
                free(pWave->pData);
                pWave->pData = NULL;
            }
            return nErrorCode;
        }
        return AUDIO_ERROR_NOMEMORY;
    }
    return AUDIO_ERROR_INVALPARAM;
}

UINT AIAPI ADestroyAudioData(PAUDIOWAVE pWave)
{
    UINT nErrorCode;

    if (pWave != NULL) {
        nErrorCode = SynthDriver.DestroyAudioData(pWave);
        if (pWave->pData != NULL)
            free(pWave->pData);
        return nErrorCode;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

UINT AIAPI AWriteAudioData(PAUDIOWAVE pWave, ULONG dwOffset, UINT nCount)
{
    if (pWave != NULL && pWave->pData != NULL) {
        if (nCount != 0) {
            return SynthDriver.WriteAudioData(pWave, dwOffset, nCount);
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

UINT AIAPI ACreateAudioVoice(PHAC phVoice)
{
    UINT nVoice;

    if (phVoice != NULL) {
        for (nVoice = 0; nVoice < nNumVoices; nVoice++) {
            if (!aVoiceTable[nVoice]) {
                *phVoice = aVoiceTable[nVoice] = (HAC) (nVoice + 1);
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

UINT AIAPI APlayVoice(HAC hVoice, PAUDIOWAVE pWave)
{
    UINT nErrorCode;

    if (pWave != NULL) {
        if (!(nErrorCode = APrimeVoice(hVoice, pWave)) &&
            !(nErrorCode = ASetVoiceFrequency(hVoice, pWave->nSampleRate)))
            return AStartVoice(hVoice);
        return nErrorCode;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

UINT AIAPI APrimeVoice(HAC hVoice, PAUDIOWAVE pWave)
{
    return SynthDriver.PrimeVoice(VOICENUM(hVoice), pWave);
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

UINT AIAPI AGetVoicePosition(HAC hVoice, PLONG pdwPosition)
{
    return SynthDriver.GetVoicePosition(VOICENUM(hVoice), pdwPosition);
}

UINT AIAPI AGetVoiceFrequency(HAC hVoice, PLONG pdwFrequency)
{
    return SynthDriver.GetVoiceFrequency(VOICENUM(hVoice), pdwFrequency);
}

UINT AIAPI AGetVoiceVolume(HAC hVoice, PUINT pnVolume)
{
    return SynthDriver.GetVoiceVolume(VOICENUM(hVoice), pnVolume);
}

UINT AIAPI AGetVoicePanning(HAC hVoice, PUINT pnPanning)
{
    return SynthDriver.GetVoicePanning(VOICENUM(hVoice), pnPanning);
}

UINT AIAPI AGetVoiceStatus(HAC hVoice, PBOOL pnStatus)
{
    return SynthDriver.GetVoiceStatus(VOICENUM(hVoice), pnStatus);
}



/*
 * External system-dependant audio device drivers
 */
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
#endif
#if defined(__DOS16__) || defined(__DPMI__)
extern AUDIODRIVER SoundBlasterDriver;
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
    /*
     * Initialize all the audio system state variables,
     * and registers all the built-in audio drivers.
     */
    if (nDriverId != AUDIO_DEVICE_MAPPER)
        return AUDIO_ERROR_NOTSUPPORTED;

    ARegisterAudioDriver(&NoneDriver);
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
#endif
#if defined(__DOS16__) || defined(__DPMI__)
    ARegisterAudioDriver(&SoundBlasterDriver);
    ARegisterAudioDriver(&ProAudioSpectrumDriver);
    ARegisterAudioDriver(&UltraSoundMaxDriver);
    ARegisterAudioDriver(&UltraSoundDriver);
    ARegisterAudioDriver(&WinSndSystemDriver);
    ARegisterAudioDriver(&SoundscapeDriver);
    ARegisterAudioDriver(&AriaDriver);
#endif

    return AAudioFatalError(AUDIO_ERROR_NONE);
}
