/*
 * $Id: mixdrv.c 1.9 1996/06/16 00:51:36 chasan released $
 *
 * Software-based waveform synthesizer emulator driver.
 *
 * Copyright (C) 1995, 1996 Carlos Hasan. All Rights Reserved.
 *
 */

#define __FILTER__

#ifdef __GNUC__
#include <memory.h>
#endif
#include <string.h>
#include <malloc.h>
#include "audio.h"
#include "drivers.h"


/*
 * Voice control bit fields
 */
#define VOICE_STOP              0x01
#define VOICE_16BITS            0x02
#define VOICE_LOOP              0x04
#define VOICE_BIDILOOP          0x08
#define VOICE_REVERSE           0x10

/*
 * Voice pitch accurary and mixing buffer size
 */
#define ACCURACY                12
#define BUFFERSIZE              512


/*
 * Waveform synthesizer voice structure
 */
typedef struct {
    PVOID   pData;
    LONG    dwAccum;
    LONG    dwFrequency;
    LONG    dwLoopStart;
    LONG    dwLoopEnd;
    UCHAR   nVolume;
    UCHAR   nPanning;
    UCHAR   bControl;
    UCHAR   bReserved;
} VOICE, *PVOICE;

/*
 * Low level voice mixing routine prototype
 */
typedef VOID (AIAPI* PFNMIXAUDIO)(PLONG, UINT, PVOICE);

/*
 * Waveform synthesizer state structure
 */
static struct {
    VOICE   aVoices[AUDIO_MAX_VOICES];
    UINT    nVoices;
    UINT    wFormat;
    UINT    nSampleRate;
    LONG    dwTimerRate;
    LONG    dwTimerAccum;
    PUCHAR  pMemoryBlock;
    PFNAUDIOTIMER pfnAudioTimer;
    PFNMIXAUDIO pfnMixAudioProc[2];
} Synth;

#ifndef __MSC__
PLONG AIAPI pVolumeTable;
PUCHAR AIAPI pFilterTable;
#else
PLONG pVolumeTable;
PUCHAR pFilterTable;
#endif

static VOID AIAPI UpdateVoices(PUCHAR pData, UINT nCount);

/* low level resamplation and quantization routines */
#ifdef __ASM__

extern VOID AIAPI QuantAudioData08(PVOID pBuffer, PLONG pData, UINT nCount);
extern VOID AIAPI QuantAudioData16(PVOID pBuffer, PLONG pData, UINT nCount);
extern VOID AIAPI MixAudioData08M(PLONG pBuffer, UINT nCount, PVOICE pVoice);
extern VOID AIAPI MixAudioData08S(PLONG pBuffer, UINT nCount, PVOICE pVoice);
extern VOID AIAPI MixAudioData08MI(PLONG pBuffer, UINT nCount, PVOICE pVoice);
extern VOID AIAPI MixAudioData08SI(PLONG pBuffer, UINT nCount, PVOICE pVoice);
extern VOID AIAPI MixAudioData16M(PLONG pBuffer, UINT nCount, PVOICE pVoice);
extern VOID AIAPI MixAudioData16S(PLONG pBuffer, UINT nCount, PVOICE pVoice);
extern VOID AIAPI MixAudioData16MI(PLONG pBuffer, UINT nCount, PVOICE pVoice);
extern VOID AIAPI MixAudioData16SI(PLONG pBuffer, UINT nCount, PVOICE pVoice);

static VOID QuantAudioData(PVOID pBuffer, PLONG pData, UINT nCount)
{
    if (Synth.wFormat & AUDIO_FORMAT_16BITS)
        QuantAudioData16(pBuffer, pData, nCount);
    else
        QuantAudioData08(pBuffer, pData, nCount);
}

#else

static VOID QuantAudioData(PVOID pBuffer, PLONG pData, UINT nCount)
{
    PSHORT pwBuffer;
    PUCHAR pbBuffer;
    LONG dwSample;

    if (Synth.wFormat & AUDIO_FORMAT_16BITS) {
        pwBuffer = (PSHORT) pBuffer;
        while (nCount-- > 0) {
            dwSample = *pData++;
            if (dwSample < -32768)
                dwSample = -32768;
            else if (dwSample > +32767)
                dwSample = +32767;
            *pwBuffer++ = (SHORT) dwSample;
        }
    }
    else {
        pbBuffer = (PUCHAR) pBuffer;
        while (nCount-- > 0) {
            dwSample = *pData++;
            if (dwSample < -32768)
                dwSample = -32768;
            else if (dwSample > +32767)
                dwSample = +32767;
            *pbBuffer++ = (UCHAR) ((dwSample >> 8) + 128);
        }
    }
}

static VOID AIAPI MixAudioData08M(PLONG pBuffer, UINT nCount, PVOICE pVoice)
{
    register UINT count;
    register ULONG accum, delta;
    register PLONG table, buf;
    register PUCHAR data;

    accum = pVoice->dwAccum;
    delta = pVoice->dwFrequency;
    table = pVolumeTable + ((UINT) pVoice->nVolume << 8);
    data = pVoice->pData;
    buf = pBuffer;
    count = nCount;

    do {
        *buf++ += table[data[accum >> ACCURACY]];
        accum += delta;
    } while (--count != 0);

    pVoice->dwAccum = accum;
}

static VOID AIAPI MixAudioData08S(PLONG pBuffer, UINT nCount, PVOICE pVoice)
{
    register UINT a;
    register ULONG accum, delta;
    register PLONG ltable, rtable, buf;
    register PUCHAR data;

    accum = pVoice->dwAccum;
    delta = pVoice->dwFrequency;
    a = ((UINT) pVoice->nVolume * pVoice->nPanning) >> 8;
    rtable = pVolumeTable + (a << 8);
    ltable = pVolumeTable + ((pVoice->nVolume - a) << 8);
    data = pVoice->pData;
    buf = pBuffer;

    do {
        a = data[accum >> ACCURACY];
        buf[0] += ltable[a];
        buf[1] += rtable[a];
        accum += delta;
        buf += 2;
    } while (--nCount != 0);

    pVoice->dwAccum = accum;
}

static VOID AIAPI MixAudioData08MI(PLONG pBuffer, UINT nCount, PVOICE pVoice)
{
    register INT a, frac;
    register ULONG accum, delta;
    register PLONG table, buf;
    register PUCHAR data, ftable;

    /* adaptive oversampling interpolation comparison */
    if (pVoice->dwFrequency < -(1 << ACCURACY) ||
        pVoice->dwFrequency > +(1 << ACCURACY)) {
        MixAudioData08M(pBuffer, nCount, pVoice);
        return;
    }

    accum = pVoice->dwAccum;
    delta = pVoice->dwFrequency;
    table = pVolumeTable + ((UINT) pVoice->nVolume << 8);
    data = pVoice->pData;
    buf = pBuffer;

#ifdef __FILTER__
    a = (UCHAR) pVoice->bReserved;
    frac = ((long) delta < 0 ? -delta : +delta) >> (ACCURACY - 5);
    ftable = pFilterTable + (frac << 8);
    do {
        a = (UCHAR)(a + ftable[data[accum >> ACCURACY]] - ftable[a]);
        *buf++ += table[a];
        accum += delta;
    } while (--nCount != 0);
    pVoice->bReserved = (UCHAR) a;
#else
    do {
        register INT b;
        a = (signed char) data[accum >> ACCURACY];
        b = (signed char) data[(accum >> ACCURACY) + 1];
        frac = (accum & ((1 << ACCURACY) - 1)) >> (ACCURACY - 5);
        a = (UCHAR)(a + ((frac * (b - a)) >> 5));
        *buf++ += table[a];
        accum += delta;
    } while (--nCount != 0);
#endif

    pVoice->dwAccum = accum;
}

static VOID AIAPI MixAudioData08SI(PLONG pBuffer, UINT nCount, PVOICE pVoice)
{
    register INT a, frac;
    register ULONG accum, delta;
    register PLONG buf, ltable, rtable;
    register PUCHAR data, ftable;

    /* adaptive oversampling interpolation comparison */
    if (pVoice->dwFrequency < -(1 << ACCURACY) ||
        pVoice->dwFrequency > +(1 << ACCURACY)) {
        MixAudioData08S(pBuffer, nCount, pVoice);
        return;
    }

    accum = pVoice->dwAccum;
    delta = pVoice->dwFrequency;
    a = ((UINT) pVoice->nVolume * pVoice->nPanning) >> 8;
    rtable = pVolumeTable + (a << 8);
    ltable = pVolumeTable + ((pVoice->nVolume - a) << 8);
    data = pVoice->pData;
    buf = pBuffer;

#ifdef __FILTER__
    a = (UCHAR) pVoice->bReserved;
    frac = ((long) delta < 0 ? -delta : +delta) >> (ACCURACY - 5);
    ftable = pFilterTable + (frac << 8);
    do {
        a = (UCHAR)(a + ftable[data[accum >> ACCURACY]] - ftable[a]);
        buf[0] += ltable[a];
        buf[1] += rtable[a];
        accum += delta;
        buf += 2;
    } while (--nCount != 0);
    pVoice->bReserved = (UCHAR) a;
#else
    do {
        register INT b;
        a = (signed char) data[accum >> ACCURACY];
        b = (signed char) data[(accum >> ACCURACY) + 1];
        frac = (accum & ((1 << ACCURACY) - 1)) >> (ACCURACY - 5);
        a = (UCHAR)(a + ((frac * (b - a)) >> 5));
        buf[0] += ltable[a];
        buf[1] += rtable[a];
        accum += delta;
        buf += 2;
    } while (--nCount != 0);
#endif

    pVoice->dwAccum = accum;
}

static VOID AIAPI MixAudioData16M(PLONG pBuffer, UINT nCount, PVOICE pVoice)
{
    register UINT a, count;
    register ULONG accum, delta;
    register PLONG table, buf;
    register PUSHORT data;

    accum = pVoice->dwAccum;
    delta = pVoice->dwFrequency;
    table = pVolumeTable + ((UINT) pVoice->nVolume << 8);
    data = pVoice->pData;
    buf = pBuffer;
    count = nCount;

    do {
        a = data[accum >> ACCURACY];
#ifndef __16BIT__
        *buf++ += table[a >> 8] + table[33 * 256 + (a & 0xff)];
#else
        *buf++ += table[a >> 8];
#endif
        accum += delta;
    } while (--count != 0);

    pVoice->dwAccum = accum;
}

static VOID AIAPI MixAudioData16S(PLONG pBuffer, UINT nCount, PVOICE pVoice)
{
    register UINT a;
    register ULONG accum, delta;
    register PLONG ltable, rtable, buf;
    register PUSHORT data;

    accum = pVoice->dwAccum;
    delta = pVoice->dwFrequency;
    a = ((UINT) pVoice->nVolume * pVoice->nPanning) >> 8;
    rtable = pVolumeTable + (a << 8);
    ltable = pVolumeTable + ((pVoice->nVolume - a) << 8);
    data = pVoice->pData;
    buf = pBuffer;

    do {
        a = data[accum >> ACCURACY];
#ifndef __16BIT__
        buf[0] += ltable[a >> 8] + ltable[33 * 256 + (a & 0xff)];
        buf[1] += rtable[a >> 8] + rtable[33 * 256 + (a & 0xff)];
#else
        buf[0] += ltable[a >> 8];
        buf[1] += rtable[a >> 8];
#endif
        accum += delta;
        buf += 2;
    } while (--nCount != 0);

    pVoice->dwAccum = accum;
}


static VOID AIAPI MixAudioData16MI(PLONG pBuffer, UINT nCount, PVOICE pVoice)
{
    register UINT frac, count;
    register ULONG a, b, accum, delta;
    register PLONG table, buf;
    register PUSHORT data;

    /* adaptive oversampling interpolation comparison */
    if (pVoice->dwFrequency < -(1 << ACCURACY) ||
        pVoice->dwFrequency > +(1 << ACCURACY)) {
        MixAudioData16M(pBuffer, nCount, pVoice);
        return;
    }

    accum = pVoice->dwAccum;
    delta = pVoice->dwFrequency;
    table = pVolumeTable + ((UINT) pVoice->nVolume << 8);
    data = pVoice->pData;
    buf = pBuffer;
    count = nCount;

    do {
        a = data[accum >> ACCURACY];
        b = data[(accum >> ACCURACY) + 1];
        frac = (accum & ((1 << ACCURACY) - 1)) >> (ACCURACY - 5);
        a = (USHORT)((SHORT)a + ((frac * ((SHORT)b - (SHORT)a)) >> 5));
#ifndef __16BIT__
        *buf++ += table[a >> 8] + table[33 * 256 + (a & 0xff)];
#else
        *buf++ += table[a >> 8];
#endif
        accum += delta;
    } while (--count != 0);

    pVoice->dwAccum = accum;
}

static VOID AIAPI MixAudioData16SI(PLONG pBuffer, UINT nCount, PVOICE pVoice)
{
    register UINT frac;
    register ULONG a, b, accum, delta;
    register PLONG ltable, rtable, buf;
    register PUSHORT data;

    /* adaptive oversampling interpolation comparison */
    if (pVoice->dwFrequency < -(1 << ACCURACY) ||
        pVoice->dwFrequency > +(1 << ACCURACY)) {
        MixAudioData16S(pBuffer, nCount, pVoice);
        return;
    }

    accum = pVoice->dwAccum;
    delta = pVoice->dwFrequency;
    a = ((UINT) pVoice->nVolume * pVoice->nPanning) >> 8;
    rtable = pVolumeTable + (a << 8);
    ltable = pVolumeTable + ((pVoice->nVolume - a) << 8);
    data = pVoice->pData;
    buf = pBuffer;

    do {
        a = data[accum >> ACCURACY];
        b = data[(accum >> ACCURACY) + 1];
        frac = (accum & ((1 << ACCURACY) - 1)) >> (ACCURACY - 5);
        a = (USHORT)((SHORT)a + ((frac * ((int)(SHORT)b - (int)(SHORT)a)) >> 5));
#ifndef __16BIT__
        buf[0] += ltable[a >> 8] + ltable[33 * 256 + (a & 0xff)];
        buf[1] += rtable[a >> 8] + rtable[33 * 256 + (a & 0xff)];
#else
        buf[0] += ltable[a >> 8];
        buf[1] += rtable[a >> 8];
#endif
        accum += delta;
        buf += 2;
    } while (--nCount != 0);

    pVoice->dwAccum = accum;
}

#endif

static VOID MixAudioData(PLONG pBuffer, UINT nCount, PVOICE pVoice)
{
    UINT nSize;

    if (Synth.wFormat & AUDIO_FORMAT_STEREO)
        nCount >>= 1;

    while (nCount > 0 && !(pVoice->bControl & VOICE_STOP)) {
        /* check boundary conditions */
        if (pVoice->bControl & VOICE_REVERSE) {
            if (pVoice->dwAccum < pVoice->dwLoopStart) {
                if (pVoice->bControl & VOICE_BIDILOOP) {
                    pVoice->dwAccum = pVoice->dwLoopStart +
                        (pVoice->dwLoopStart - pVoice->dwAccum);
                    pVoice->bControl ^= VOICE_REVERSE;
                    continue;
                }
                else if (pVoice->bControl & VOICE_LOOP) {
                    pVoice->dwAccum = pVoice->dwLoopEnd -
                        (pVoice->dwLoopStart - pVoice->dwAccum);
                    continue;
                }
                else {
                    pVoice->bControl |= VOICE_STOP;
                    break;
                }
            }
        }
        else {
            if (pVoice->dwAccum > pVoice->dwLoopEnd) {
                if (pVoice->bControl & VOICE_BIDILOOP) {
                    pVoice->dwAccum = pVoice->dwLoopEnd -
                        (pVoice->dwAccum - pVoice->dwLoopEnd);
                    pVoice->bControl ^= VOICE_REVERSE;
                    continue;
                }
                else if (pVoice->bControl & VOICE_LOOP) {
                    pVoice->dwAccum = pVoice->dwLoopStart +
                        (pVoice->dwAccum - pVoice->dwLoopEnd);
                    continue;
                }
                else {
                    pVoice->bControl |= VOICE_STOP;
                    break;
                }
            }
        }

        /* check for overflow and clip if necessary */
        nSize = nCount;
        if (pVoice->bControl & VOICE_REVERSE) {
            if (nSize * pVoice->dwFrequency >
                pVoice->dwAccum - pVoice->dwLoopStart)
                nSize = (pVoice->dwAccum - pVoice->dwLoopStart +
                    pVoice->dwFrequency) / pVoice->dwFrequency;
        }
        else {
            if (nSize * pVoice->dwFrequency >
                pVoice->dwLoopEnd - pVoice->dwAccum)
                nSize = (pVoice->dwLoopEnd - pVoice->dwAccum +
                    pVoice->dwFrequency) / pVoice->dwFrequency;
        }

        if (pVoice->bControl & VOICE_REVERSE)
            pVoice->dwFrequency = -pVoice->dwFrequency;

        /* mixes chunk of data in a burst mode */
        if (pVoice->bControl & VOICE_16BITS) {
            Synth.pfnMixAudioProc[1] (pBuffer, nSize, pVoice);
        }
        else {
            Synth.pfnMixAudioProc[0] (pBuffer, nSize, pVoice);
        }

        if (pVoice->bControl & VOICE_REVERSE)
            pVoice->dwFrequency = -pVoice->dwFrequency;

        /* update mixing buffer address and counter */
        pBuffer += nSize;
        if (Synth.wFormat & AUDIO_FORMAT_STEREO)
            pBuffer += nSize;
        nCount -= nSize;
    }
}

static VOID MixAudioVoices(PLONG pBuffer, UINT nCount)
{
    UINT nVoice, nSize;

    while (nCount > 0) {
        nSize = nCount;
        if (Synth.pfnAudioTimer != NULL) {
            nSize = (Synth.dwTimerRate - Synth.dwTimerAccum + 64L) & ~63L;
            if (nSize > nCount)
                nSize = nCount;
            if ((Synth.dwTimerAccum += nSize) >= Synth.dwTimerRate) {
                Synth.dwTimerAccum -= Synth.dwTimerRate;
                Synth.pfnAudioTimer();
            }
        }
        for (nVoice = 0; nVoice < Synth.nVoices; nVoice++) {
            MixAudioData(pBuffer, nSize, &Synth.aVoices[nVoice]);
        }
        pBuffer += nSize;
        nCount -= nSize;
    }
}


/*
 * High level waveform synthesizer interface
 */
static UINT AIAPI GetAudioCaps(PAUDIOCAPS pCaps)
{
    memset(pCaps, 0, sizeof(AUDIOCAPS));
    return AUDIO_ERROR_NOTSUPPORTED;
}

static UINT AIAPI PingAudio(VOID)
{
    return AUDIO_ERROR_NOTSUPPORTED;
}

static UINT AIAPI OpenAudio(PAUDIOINFO pInfo)
{
    PUCHAR pFilter;
    PLONG pVolume;
    UINT nVolume, nSample;

    memset(&Synth, 0, sizeof(Synth));
    Synth.wFormat = pInfo->wFormat;
    Synth.nSampleRate = pInfo->nSampleRate;
    if (Synth.wFormat & AUDIO_FORMAT_FILTER) {
        Synth.pfnMixAudioProc[0] = (Synth.wFormat & AUDIO_FORMAT_STEREO) ?
            MixAudioData08SI : MixAudioData08MI;
        Synth.pfnMixAudioProc[1] = (Synth.wFormat & AUDIO_FORMAT_STEREO) ?
            MixAudioData16SI : MixAudioData16MI;
    }
    else {
        Synth.pfnMixAudioProc[0] = (Synth.wFormat & AUDIO_FORMAT_STEREO) ?
            MixAudioData08S : MixAudioData08M;
        Synth.pfnMixAudioProc[1] = (Synth.wFormat & AUDIO_FORMAT_STEREO) ?
            MixAudioData16S : MixAudioData16M;
    }

#if !defined(__16BIT__) && !defined(__ASM__)
    if ((Synth.pMemoryBlock = malloc(2 * 33 * 4 * 256 + 32 * 256 + 1023)) != NULL) {
#else
    if ((Synth.pMemoryBlock = malloc(33 * 4 * 256 + 32 * 256 + 1023)) != NULL) {
#endif
#ifdef __ASM__
        pVolumeTable = (PLONG) (((ULONG) Synth.pMemoryBlock + 1023) & ~1023);
#else
        pVolumeTable = (PLONG) Synth.pMemoryBlock;
#endif
        pVolume = pVolumeTable;
        for (nVolume = 0; nVolume <= 32; nVolume++) {
            for (nSample = 0; nSample <= 255; nSample++) {
                *pVolume++ = (nVolume * (LONG)((signed char) nSample)) << 1;
            }
        }
#if !defined(__16BIT__) && !defined(__ASM__)
        for (nVolume = 0; nVolume <= 32; nVolume++) {
            for (nSample = 0; nSample <= 255; nSample++) {
                *pVolume++ = (nVolume * (LONG)((unsigned char) nSample)) >> 7;
            }
        }
#endif

#ifdef __FILTER__
        pFilterTable = pFilter = (PUCHAR) pVolume;
        for (nVolume = 0; nVolume < 32; nVolume++) {
            for (nSample = 0; nSample <= 255; nSample++) {
                *pFilter++ = (nVolume * (int)((signed char) nSample)) >> 5;
            }
        }
#endif

        ASetAudioCallback(UpdateVoices);
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_NOMEMORY;
}

static UINT AIAPI CloseAudio(VOID)
{
    if (Synth.pMemoryBlock != NULL)
        free(Synth.pMemoryBlock);
    memset(&Synth, 0, sizeof(Synth));
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI OpenVoices(UINT nVoices)
{
    UINT nVoice;

    /*
     * Initialize waveform synthesizer structure for playback
     */
    if (nVoices < AUDIO_MAX_VOICES) {
        Synth.nVoices = nVoices;
        for (nVoice = 0; nVoice < Synth.nVoices; nVoice++)
            Synth.aVoices[nVoice].bControl = VOICE_STOP;
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALPARAM;
}

static UINT AIAPI CloseVoices(VOID)
{
    UINT nVoice;

    memset(Synth.aVoices, 0, sizeof(Synth.aVoices));
    for (nVoice = 0; nVoice < AUDIO_MAX_VOICES; nVoice++)
        Synth.aVoices[nVoice].bControl = VOICE_STOP;
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI UpdateAudio(VOID)
{
    return AUDIO_ERROR_NONE;
}

static LONG AIAPI GetAudioMemAvail(VOID)
{
    return 0;
}

static UINT AIAPI CreateAudioData(PAUDIOWAVE pWave)
{
    if (pWave != NULL) {
        pWave->dwHandle = (ULONG) pWave->pData;
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI DestroyAudioData(PAUDIOWAVE pWave)
{
    if (pWave != NULL && pWave->dwHandle != 0) {
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI WriteAudioData(PAUDIOWAVE pWave, ULONG dwOffset, UINT nCount)
{
    if (pWave != NULL && pWave->dwHandle != 0) {
        /* anticlick removal work around */
        if (pWave->wFormat & AUDIO_FORMAT_LOOP) {
            if (dwOffset <= pWave->dwLoopEnd &&
                dwOffset + nCount >= pWave->dwLoopEnd) {
                *(PULONG) (pWave->dwHandle + pWave->dwLoopEnd) =
                    *(PULONG) (pWave->dwHandle + pWave->dwLoopStart);
            }
        }
        else if (dwOffset + nCount >= pWave->dwLength) {
            *(PULONG) (pWave->dwHandle + pWave->dwLength) = 0;
        }
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI PrimeVoice(UINT nVoice, PAUDIOWAVE pWave)
{
    PVOICE pVoice;

    if (nVoice < Synth.nVoices && pWave != NULL && pWave->dwHandle != 0) {
        pVoice = &Synth.aVoices[nVoice];
        pVoice->pData = (PVOID) pWave->dwHandle;
        pVoice->bControl = VOICE_STOP;
        pVoice->dwAccum = 0;
        if (pWave->wFormat & (AUDIO_FORMAT_LOOP | AUDIO_FORMAT_BIDILOOP)) {
            pVoice->dwLoopStart = pWave->dwLoopStart;
            pVoice->dwLoopEnd = pWave->dwLoopEnd;
            pVoice->bControl |= VOICE_LOOP;
            if (pWave->wFormat & AUDIO_FORMAT_BIDILOOP)
                pVoice->bControl |= VOICE_BIDILOOP;
        }
        else {
            pVoice->dwLoopStart = pWave->dwLength;
            pVoice->dwLoopEnd = pWave->dwLength;
        }
        if (pWave->wFormat & AUDIO_FORMAT_16BITS) {
            pVoice->dwLoopStart >>= 1;
            pVoice->dwLoopEnd >>= 1;
            pVoice->bControl |= VOICE_16BITS;
        }
        pVoice->dwAccum <<= ACCURACY;
        pVoice->dwLoopStart <<= ACCURACY;
        pVoice->dwLoopEnd <<= ACCURACY;
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI StartVoice(UINT nVoice)
{
    if (nVoice < Synth.nVoices) {
        Synth.aVoices[nVoice].bControl &= ~VOICE_STOP;
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI StopVoice(UINT nVoice)
{
    if (nVoice < Synth.nVoices) {
        Synth.aVoices[nVoice].bControl |= VOICE_STOP;
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALHANDLE;

}

static UINT AIAPI SetVoicePosition(UINT nVoice, LONG dwPosition)
{
    if (nVoice < Synth.nVoices) {
        dwPosition <<= ACCURACY;
        if (dwPosition >= 0 && dwPosition < Synth.aVoices[nVoice].dwLoopEnd) {
            Synth.aVoices[nVoice].dwAccum = dwPosition;
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI SetVoiceFrequency(UINT nVoice, LONG dwFrequency)
{
    if (nVoice < Synth.nVoices) {
        if (dwFrequency >= AUDIO_MIN_FREQUENCY &&
            dwFrequency <= AUDIO_MAX_FREQUENCY) {
            Synth.aVoices[nVoice].dwFrequency = ((dwFrequency << ACCURACY) +
                (Synth.nSampleRate >> 1)) / Synth.nSampleRate;
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI SetVoiceVolume(UINT nVoice, UINT nVolume)
{
    if (nVoice < Synth.nVoices) {
        if (nVolume <= AUDIO_MAX_VOLUME) {
            Synth.aVoices[nVoice].nVolume = nVolume >> 1;
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI SetVoicePanning(UINT nVoice, UINT nPanning)
{
    if (nVoice < Synth.nVoices) {
        if (nPanning <= AUDIO_MAX_PANNING) {
            Synth.aVoices[nVoice].nPanning = nPanning;
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoicePosition(UINT nVoice, PLONG pdwPosition)
{
    if (nVoice < Synth.nVoices) {
        if (pdwPosition != NULL) {
            *pdwPosition = Synth.aVoices[nVoice].dwAccum >> ACCURACY;
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoiceFrequency(UINT nVoice, PLONG pdwFrequency)
{
    if (nVoice < Synth.nVoices) {
        if (pdwFrequency != NULL) {
            *pdwFrequency = (Synth.aVoices[nVoice].dwFrequency *
                Synth.nSampleRate) >> ACCURACY;
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoiceVolume(UINT nVoice, PUINT pnVolume)
{
    if (nVoice < Synth.nVoices) {
        if (pnVolume != NULL) {
            *pnVolume = Synth.aVoices[nVoice].nVolume << 1;
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoicePanning(UINT nVoice, PUINT pnPanning)
{
    if (nVoice < Synth.nVoices) {
        if (pnPanning != NULL) {
            *pnPanning = Synth.aVoices[nVoice].nPanning;
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI GetVoiceStatus(UINT nVoice, PBOOL pnStatus)
{
    if (nVoice < Synth.nVoices) {
        if (pnStatus != NULL) {
            *pnStatus = (Synth.aVoices[nVoice].bControl & VOICE_STOP) != 0;
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_INVALHANDLE;
}

static UINT AIAPI SetAudioTimerProc(PFNAUDIOTIMER pfnAudioTimer)
{
    Synth.pfnAudioTimer = pfnAudioTimer;
    return AUDIO_ERROR_NONE;
}

static UINT AIAPI SetAudioTimerRate(UINT nBPM)
{
    if (nBPM >= 0x20 && nBPM <= 0xFF) {
        Synth.dwTimerRate = Synth.nSampleRate;
        if (Synth.wFormat & AUDIO_FORMAT_STEREO)
            Synth.dwTimerRate <<= 1;
        Synth.dwTimerRate = (5 * Synth.dwTimerRate) / (2 * nBPM);
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALPARAM;
}

static VOID AIAPI UpdateVoices(PUCHAR pData, UINT nCount)
{
    static LONG aBuffer[BUFFERSIZE];
    UINT nSamples;

    if (Synth.wFormat & AUDIO_FORMAT_16BITS)
        nCount >>= 1;
    while (nCount > 0) {
        if ((nSamples = nCount) > BUFFERSIZE)
            nSamples = BUFFERSIZE;
        memset(aBuffer, 0, nSamples << 2);
        MixAudioVoices(aBuffer, nSamples);
        QuantAudioData(pData, aBuffer, nSamples);
        pData += nSamples << ((Synth.wFormat & AUDIO_FORMAT_16BITS) != 0);
        nCount -= nSamples;
    }
}


/*
 * Waveform synthesizer public interface
 */
AUDIOSYNTHDRIVER EmuSynthDriver =
{
    GetAudioCaps, PingAudio, OpenAudio, CloseAudio,
    UpdateAudio, OpenVoices, CloseVoices,
    SetAudioTimerProc, SetAudioTimerRate,
    GetAudioMemAvail, CreateAudioData, DestroyAudioData,
    WriteAudioData, PrimeVoice, StartVoice, StopVoice,
    SetVoicePosition, SetVoiceFrequency, SetVoiceVolume,
    SetVoicePanning, GetVoicePosition, GetVoiceFrequency,
    GetVoiceVolume, GetVoicePanning, GetVoiceStatus
};
