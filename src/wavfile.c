/*
 * $Id: wavfile.c 1.7 1996/09/13 15:10:22 chasan released $
 *
 * Windows RIFF/WAVE PCM file loader routines.
 *
 * Copyright (C) 1995-1999 Carlos Hasan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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
    WORD    wFormatTag;
    WORD    nChannels;
    DWORD   nSamplesPerSec;
    DWORD   nAvgBytesPerSec;
    WORD    nBlockAlign;
    WORD    wBitsPerSample;
} PCMWAVEFORMAT;

typedef struct {
    DWORD   fccId;
    DWORD   dwSize;
} CHUNKHEADER;

typedef struct {
    DWORD   fccId;
    DWORD   dwSize;
    DWORD   fccType;
} RIFFHEADER;


UINT AIAPI ALoadWaveFile(LPSTR lpszFileName, 
			 LPAUDIOWAVE* lplpWave, DWORD dwFileOffset)
{
    static RIFFHEADER Header;
    static CHUNKHEADER Chunk;
    static PCMWAVEFORMAT Fmt;
    LPAUDIOWAVE lpWave;
    UINT n, nErrorCode;

    if (lplpWave == NULL) {
        return AUDIO_ERROR_INVALPARAM;
    }
    *lplpWave = NULL;

    if (AIOOpenFile(lpszFileName)) {
        return AUDIO_ERROR_FILENOTFOUND;
    }
    AIOSeekFile(dwFileOffset, SEEK_SET);

    if ((lpWave = (LPAUDIOWAVE) calloc(1, sizeof(AUDIOWAVE))) == NULL) {
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
        AFreeWaveFile(lpWave);
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
                AFreeWaveFile(lpWave);
                return AUDIO_ERROR_BADFILEFORMAT;
            }
            lpWave->dwLength = Chunk.dwSize;
            lpWave->nSampleRate = (WORD) Fmt.nSamplesPerSec;
            lpWave->wFormat = AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO;
            if (Fmt.wBitsPerSample != 8)
                lpWave->wFormat |= AUDIO_FORMAT_16BITS;
            if (Fmt.nChannels != 1)
                lpWave->wFormat |= AUDIO_FORMAT_STEREO;
            if ((nErrorCode = ACreateAudioData(lpWave)) != AUDIO_ERROR_NONE) {
                AIOCloseFile();
                AFreeWaveFile(lpWave);
                return nErrorCode;
            }
            AIOReadFile(lpWave->lpData, lpWave->dwLength);
            if (!(lpWave->wFormat & AUDIO_FORMAT_16BITS)) {
                for (n = 0; n < lpWave->dwLength; n++)
                    lpWave->lpData[n] ^= 0x80;
            }
            AWriteAudioData(lpWave, 0, lpWave->dwLength);
            AIOCloseFile();
            *lplpWave = lpWave;
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


UINT AIAPI AFreeWaveFile(LPAUDIOWAVE lpWave)
{
    UINT rc;
    rc = ADestroyAudioData(lpWave);  /*** FIX: 04/12/98 ***/
    free(lpWave);
    return rc;
}
