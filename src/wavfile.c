/*
 * $Id: wavfile.c 1.5 1996/05/24 08:30:44 chasan released $
 *
 * Windows RIFF/WAVE PCM file loader routines.
 *
 * Copyright (C) 1995, 1996 Carlos Hasan. All Rights Reserved.
 *
 */

#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include "audio.h"
#include "iofile.h"


/*
 * Windows RIFF/WAVE PCM file structures
 */

#define FOURCC_RIFF         0x46464952L
#define FOURCC_WAVE         0x45564157L
#define FOURCC_FMT          0x20746D66L
#define FOURCC_DATA         0x61746164L

#define WAVE_FORMAT_PCM     1

typedef struct {
    USHORT  wFormatTag;
    USHORT  nChannels;
    ULONG   nSamplesPerSec;
    ULONG   nAvgBytesPerSec;
    USHORT  nBlockAlign;
    USHORT  wBitsPerSample;
} PCMWAVEFORMAT;

typedef struct {
    ULONG   fccId;
    ULONG   dwSize;
} CHUNKHEADER;

typedef struct {
    ULONG   fccId;
    ULONG   dwSize;
    ULONG   fccType;
} RIFFHEADER;


UINT AIAPI ALoadWaveFile(PSZ pszFileName, PAUDIOWAVE* ppWave)
{
    static RIFFHEADER Header;
    static CHUNKHEADER Chunk;
    static PCMWAVEFORMAT Fmt;
    PAUDIOWAVE pWave;
    UINT n, nErrorCode;

    if (ppWave == NULL) {
        return AUDIO_ERROR_INVALPARAM;
    }
    *ppWave = NULL;

    if (AIOOpenFile(pszFileName)) {
        return AUDIO_ERROR_FILENOTFOUND;
    }

    if ((pWave = (PAUDIOWAVE) calloc(1, sizeof(AUDIOWAVE))) == NULL) {
        AIOCloseFile();
        return AUDIO_ERROR_NOMEMORY;
    }

    /* read RIFF/WAVE header structure */
    AIOReadLong(&Header.fccId);
    AIOReadLong(&Header.dwSize);
    AIOReadLong(&Header.fccType);
    Header.dwSize += (Header.dwSize & 1);
    if (Header.fccId != FOURCC_RIFF || Header.fccType != FOURCC_WAVE) {
        AIOCloseFile();
        AFreeWaveFile(pWave);
        return AUDIO_ERROR_BADFILEFORMAT;
    }

    memset(&Fmt, 0, sizeof(Fmt));
    Header.dwSize -= sizeof(Header.fccType);
    while (Header.dwSize != 0) {
        /* read RIFF chunk header structure */
        AIOReadLong(&Chunk.fccId);
        AIOReadLong(&Chunk.dwSize);
        Chunk.dwSize += (Chunk.dwSize & 1);
        Header.dwSize -= sizeof(Chunk) + Chunk.dwSize;
        if (Chunk.fccId == FOURCC_FMT) {
            /* read RIFF/WAVE format chunk */
            AIOReadShort(&Fmt.wFormatTag);
            AIOReadShort(&Fmt.nChannels);
            AIOReadLong(&Fmt.nSamplesPerSec);
            AIOReadLong(&Fmt.nAvgBytesPerSec);
            AIOReadShort(&Fmt.nBlockAlign);
            AIOReadShort(&Fmt.wBitsPerSample);
            AIOSeekFile(Chunk.dwSize - sizeof(Fmt), SEEK_CUR);
        }
        else if (Chunk.fccId == FOURCC_DATA) {
            /* read RIFF/WAVE data chunk */
            if (Fmt.wFormatTag != WAVE_FORMAT_PCM) {
                AIOCloseFile();
                AFreeWaveFile(pWave);
                return AUDIO_ERROR_BADFILEFORMAT;
            }
            pWave->dwLength = Chunk.dwSize;
            pWave->nSampleRate = Fmt.nSamplesPerSec;
            pWave->wFormat = AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO;
            if (Fmt.wBitsPerSample != 8)
                pWave->wFormat |= AUDIO_FORMAT_16BITS;
            if (Fmt.nChannels != 1)
                pWave->wFormat |= AUDIO_FORMAT_STEREO;
            if ((nErrorCode = ACreateAudioData(pWave)) != AUDIO_ERROR_NONE) {
                AIOCloseFile();
                AFreeWaveFile(pWave);
                return nErrorCode;
            }
            AIOReadFile(pWave->pData, pWave->dwLength);
            if (!(pWave->wFormat & AUDIO_FORMAT_16BITS)) {
                for (n = 0; n < pWave->dwLength; n++)
                    pWave->pData[n] ^= 0x80;
            }
            AWriteAudioData(pWave, 0, pWave->dwLength);
            AIOCloseFile();
            *ppWave = pWave;
            return AUDIO_ERROR_NONE;
        }
        else {
            /* skip unknown RIFF/WAVE chunks */
            AIOSeekFile(Chunk.dwSize, SEEK_CUR);
        }
    }

    AIOCloseFile();
    return AUDIO_ERROR_BADFILEFORMAT;
}


UINT AIAPI AFreeWaveFile(PAUDIOWAVE pWave)
{
    return ADestroyAudioData(pWave);
}
