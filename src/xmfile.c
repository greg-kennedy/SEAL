/*
 * $Id: xmfile.c 1.6 1996/12/12 16:28:54 chasan Exp $
 *
 * Extended module file loader routines.
 *
 * Copyright (c) 1995-1999 Carlos Hasan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <stdio.h>
#include <limits.h>
#include <malloc.h>
#include <string.h>
#include "audio.h"
#include "iofile.h"


/*
 * Extended module file structures
 */
#define XM_FILE_HEADER_SIZE     276
#define XM_PATTERN_HEADER_SIZE  9
#define XM_PATCH_HEADER_SIZE    29
#define XM_PATCH_BODY_SIZE      214
#define XM_SAMPLE_HEADER_SIZE   40
#define XM_MODULE_LINEAR        0x0001
#define XM_ENVELOPE_ON          0x0001
#define XM_ENVELOPE_SUSTAIN     0x0002
#define XM_ENVELOPE_LOOP        0x0004
#define XM_SAMPLE_LOOP          0x0001
#define XM_SAMPLE_PINGPONG      0x0002
#define XM_SAMPLE_16BITS        0x0010

typedef struct {
    CHAR    aIdText[17];
    CHAR    aModuleName[20];
    BYTE    bPadding;
    CHAR    aTrackerName[20];
    WORD    wVersion;
    DWORD   dwHeaderSize;
    WORD    nSongLength;
    WORD    nRestart;
    WORD    nTracks;
    WORD    nPatterns;
    WORD    nPatches;
    WORD    wFlags;
    WORD    nTempo;
    WORD    nBPM;
    BYTE    aOrderTable[256];
} XMFILEHEADER;

typedef struct {
    DWORD   dwHeaderSize;
    BYTE    nPacking;
    WORD    nRows;
    WORD    nSize;
} XMPATTERNHEADER;

typedef struct {
    DWORD   dwHeaderSize;
    CHAR    aPatchName[22];
    BYTE    nType;
    WORD    nSamples;
    DWORD   dwSampleHeaderSize;
    BYTE    aSampleNumber[96];
    DWORD   aVolumeEnvelope[12];
    DWORD   aPanningEnvelope[12];
    BYTE    nVolumePoints;
    BYTE    nPanningPoints;
    BYTE    nVolumeSustain;
    BYTE    nVolumeLoopStart;
    BYTE    nVolumeLoopEnd;
    BYTE    nPanningSustain;
    BYTE    nPanningLoopStart;
    BYTE    nPanningLoopEnd;
    BYTE    bVolumeFlags;
    BYTE    bPanningFlags;
    BYTE    nVibratoType;
    BYTE    nVibratoSweep;
    BYTE    nVibratoDepth;
    BYTE    nVibratoRate;
    WORD    nVolumeFadeout;
    WORD    wReserved;
} XMPATCHHEADER;

typedef struct {
    DWORD   dwLength;
    DWORD   dwLoopStart;
    DWORD   dwLoopLength;
    BYTE    nVolume;
    BYTE    nFinetune;
    BYTE    bFlags;
    BYTE    nPanning;
    BYTE    nRelativeNote;
    BYTE    bReserved;
    CHAR    aSampleName[22];
} XMSAMPLEHEADER;


/*
 * Extended module loader routines
 */
static VOID XMDecodeSamples(LPAUDIOWAVE lpWave)
{
    LPWORD lpwCode;
    LPBYTE lpbCode;
    UINT nDelta, nCount;

    nDelta = 0;
    nCount = lpWave->dwLength;
    if (lpWave->wFormat & AUDIO_FORMAT_16BITS) {
        lpwCode = (LPWORD) lpWave->lpData;
        nCount >>= 1;
        while (nCount--) {
#ifdef __BIGENDIAN__
            *lpwCode = MAKEWORD(HIBYTE(*lpwCode), LOBYTE(*lpwCode));
#endif
            nDelta = *lpwCode++ += nDelta;
        }
    }
    else {
        lpbCode = (LPBYTE) lpWave->lpData;
        while (nCount--) {
            nDelta = (BYTE) (*lpbCode++ += (BYTE) nDelta);
        }
    }
}

UINT AIAPI ALoadModuleXM(LPSTR lpszFileName, 
			 LPAUDIOMODULE *lplpModule, DWORD dwFileOffset)
{
    static XMFILEHEADER Header;
    static XMPATTERNHEADER Pattern;
    static XMPATCHHEADER Patch;
    static XMSAMPLEHEADER Sample;
    LPAUDIOMODULE lpModule;
    LPAUDIOPATTERN lpPattern;
    LPAUDIOPATCH lpPatch;
    LPAUDIOSAMPLE lpSample;
    UINT n, m, nErrorCode;

    /* open XM module file */
    if (AIOOpenFile(lpszFileName)) {
        return AUDIO_ERROR_FILENOTFOUND;
    }
    AIOSeekFile(dwFileOffset, SEEK_SET);

    if ((lpModule = (LPAUDIOMODULE) calloc(1, sizeof(AUDIOMODULE))) == NULL) {
        AIOCloseFile();
        return AUDIO_ERROR_NOMEMORY;
    }

    /* load XM module header structure */
    AIOReadFile(Header.aIdText, sizeof(Header.aIdText));
    AIOReadFile(Header.aModuleName, sizeof(Header.aModuleName));
    AIOReadChar(&Header.bPadding);
    AIOReadFile(Header.aTrackerName, sizeof(Header.aTrackerName));
    AIOReadShort(&Header.wVersion);
    AIOReadLong(&Header.dwHeaderSize);
    AIOReadShort(&Header.nSongLength);
    AIOReadShort(&Header.nRestart);
    AIOReadShort(&Header.nTracks);
    AIOReadShort(&Header.nPatterns);
    AIOReadShort(&Header.nPatches);
    AIOReadShort(&Header.wFlags);
    AIOReadShort(&Header.nTempo);
    AIOReadShort(&Header.nBPM);
    AIOReadFile(&Header.aOrderTable, sizeof(Header.aOrderTable));
    AIOSeekFile(Header.dwHeaderSize - XM_FILE_HEADER_SIZE, SEEK_CUR);
    if (memcmp(Header.aIdText, "Extended Module: ", sizeof(Header.aIdText)) ||
        Header.wVersion != 0x104 ||
        Header.nSongLength > AUDIO_MAX_ORDERS ||
        Header.nPatterns > AUDIO_MAX_PATTERNS ||
        Header.nPatches > AUDIO_MAX_PATCHES ||
        Header.nTracks > AUDIO_MAX_VOICES) {
        AFreeModuleFile(lpModule);
        AIOCloseFile();
        return AUDIO_ERROR_BADFILEFORMAT;
    }

    /* initialize module structure */
    strncpy(lpModule->szModuleName, Header.aModuleName,
	    sizeof(Header.aModuleName));
    if (Header.wFlags & XM_MODULE_LINEAR)
        lpModule->wFlags |= AUDIO_MODULE_LINEAR;
    lpModule->nOrders = Header.nSongLength;
    lpModule->nRestart = Header.nRestart;
    lpModule->nTracks = Header.nTracks;
    lpModule->nPatterns = Header.nPatterns;
    lpModule->nPatches = Header.nPatches;
    lpModule->nTempo = Header.nTempo;
    lpModule->nBPM = Header.nBPM;
    for (n = 0; n < lpModule->nOrders; n++) {
        lpModule->aOrderTable[n] = Header.aOrderTable[n];
    }
    for (n = 0; n < lpModule->nTracks; n++) {
        lpModule->aPanningTable[n] = (AUDIO_MIN_PANNING + AUDIO_MAX_PANNING)/2;
    }

    if ((lpModule->aPatternTable = (LPAUDIOPATTERN)
	 calloc(lpModule->nPatterns, sizeof(AUDIOPATTERN))) == NULL) {
        AFreeModuleFile(lpModule);
        AIOCloseFile();
        return AUDIO_ERROR_NOMEMORY;
    }
    if ((lpModule->aPatchTable = (LPAUDIOPATCH)
	 calloc(lpModule->nPatches, sizeof(AUDIOPATCH))) == NULL) {
        AFreeModuleFile(lpModule);
        AIOCloseFile();
        return AUDIO_ERROR_NOMEMORY;
    }

    lpPattern = lpModule->aPatternTable;
    for (n = 0; n < lpModule->nPatterns; n++, lpPattern++) {
        /* load XM pattern header structure */
        AIOReadLong(&Pattern.dwHeaderSize);
        AIOReadChar(&Pattern.nPacking);
        AIOReadShort(&Pattern.nRows);
        AIOReadShort(&Pattern.nSize);
        AIOSeekFile(Pattern.dwHeaderSize - XM_PATTERN_HEADER_SIZE, SEEK_CUR);
        if (Pattern.nPacking != 0) {
            AFreeModuleFile(lpModule);
            AIOCloseFile();
            return AUDIO_ERROR_BADFILEFORMAT;
        }

        /* initialize pattern structure */
        lpPattern->nPacking = Pattern.nPacking;
        lpPattern->nTracks = lpModule->nTracks;
        lpPattern->nRows = Pattern.nRows;
        lpPattern->nSize = Pattern.nSize;
        if (lpPattern->nSize != 0) {
            /* allocate and load pattern data */
            if ((lpPattern->lpData = malloc(lpPattern->nSize)) == NULL) {
                AFreeModuleFile(lpModule);
                AIOCloseFile();
                return AUDIO_ERROR_NOMEMORY;
            }
            AIOReadFile(lpPattern->lpData, lpPattern->nSize);
        }
    }

    lpPatch = lpModule->aPatchTable;
    for (n = 0; n < lpModule->nPatches; n++, lpPatch++) {
        /* load XM patch header structure */
        AIOReadLong(&Patch.dwHeaderSize);
        AIOReadFile(Patch.aPatchName, sizeof(Patch.aPatchName));
        AIOReadChar(&Patch.nType);
        AIOReadShort(&Patch.nSamples);
        if (Patch.nSamples != 0) {
            AIOReadLong(&Patch.dwSampleHeaderSize);
            AIOReadFile(Patch.aSampleNumber, sizeof(Patch.aSampleNumber));
            for (m = 0; m < AUDIO_MAX_POINTS; m++) {
                AIOReadLong(&Patch.aVolumeEnvelope[m]);
            }
            for (m = 0; m < AUDIO_MAX_POINTS; m++) {
                AIOReadLong(&Patch.aPanningEnvelope[m]);
            }
            AIOReadChar(&Patch.nVolumePoints);
            AIOReadChar(&Patch.nPanningPoints);
            AIOReadChar(&Patch.nVolumeSustain);
            AIOReadChar(&Patch.nVolumeLoopStart);
            AIOReadChar(&Patch.nVolumeLoopEnd);
            AIOReadChar(&Patch.nPanningSustain);
            AIOReadChar(&Patch.nPanningLoopStart);
            AIOReadChar(&Patch.nPanningLoopEnd);
            AIOReadChar(&Patch.bVolumeFlags);
            AIOReadChar(&Patch.bPanningFlags);
            AIOReadChar(&Patch.nVibratoType);
            AIOReadChar(&Patch.nVibratoSweep);
            AIOReadChar(&Patch.nVibratoDepth);
            AIOReadChar(&Patch.nVibratoRate);
            AIOReadShort(&Patch.nVolumeFadeout);
            AIOReadShort(&Patch.wReserved);
            Patch.dwHeaderSize -= XM_PATCH_BODY_SIZE;
        }
        AIOSeekFile(Patch.dwHeaderSize - XM_PATCH_HEADER_SIZE, SEEK_CUR);

        /* HACK: clamp envelope's number of points (12/12/96) */
	if (Patch.nVolumePoints > AUDIO_MAX_POINTS)
	    Patch.nVolumePoints = AUDIO_MAX_POINTS;
        if (Patch.nPanningPoints > AUDIO_MAX_POINTS)
            Patch.nPanningPoints = AUDIO_MAX_POINTS;

        if (Patch.nSamples > AUDIO_MAX_SAMPLES ||
            Patch.nVolumePoints > AUDIO_MAX_POINTS ||
            Patch.nPanningPoints > AUDIO_MAX_POINTS) {
            AFreeModuleFile(lpModule);
            AIOCloseFile();
            return AUDIO_ERROR_BADFILEFORMAT;
        }

        /* initialize patch structure */
        strncpy(lpPatch->szPatchName, Patch.aPatchName,
		sizeof(Patch.aPatchName));
        for (m = 0; m < AUDIO_MAX_NOTES; m++) {
            lpPatch->aSampleNumber[m] = Patch.aSampleNumber[m];
            if (lpPatch->aSampleNumber[m] >= Patch.nSamples)
                lpPatch->aSampleNumber[m] = 0x00;
        }
        lpPatch->nSamples = Patch.nSamples;
        lpPatch->nVibratoType = Patch.nVibratoType;
        lpPatch->nVibratoSweep = Patch.nVibratoSweep;
        lpPatch->nVibratoDepth = Patch.nVibratoDepth;
        lpPatch->nVibratoRate = Patch.nVibratoRate;
        lpPatch->nVolumeFadeout = Patch.nVolumeFadeout;
        lpPatch->Volume.nPoints = Patch.nVolumePoints;
        lpPatch->Volume.nSustain = Patch.nVolumeSustain;
        lpPatch->Volume.nLoopStart = Patch.nVolumeLoopStart;
        lpPatch->Volume.nLoopEnd = Patch.nVolumeLoopEnd;
        if (Patch.bVolumeFlags & XM_ENVELOPE_ON)
            lpPatch->Volume.wFlags |= AUDIO_ENVELOPE_ON;
        if (Patch.bVolumeFlags & XM_ENVELOPE_SUSTAIN)
            lpPatch->Volume.wFlags |= AUDIO_ENVELOPE_SUSTAIN;
        if (Patch.bVolumeFlags & XM_ENVELOPE_LOOP)
            lpPatch->Volume.wFlags |= AUDIO_ENVELOPE_LOOP;
        for (m = 0; m < lpPatch->Volume.nPoints; m++) {
            lpPatch->Volume.aEnvelope[m].nFrame =
                LOWORD(Patch.aVolumeEnvelope[m]);
            lpPatch->Volume.aEnvelope[m].nValue =
                HIWORD(Patch.aVolumeEnvelope[m]);
        }
        lpPatch->Panning.nPoints = Patch.nPanningPoints;
        lpPatch->Panning.nSustain = Patch.nPanningSustain;
        lpPatch->Panning.nLoopStart = Patch.nPanningLoopStart;
        lpPatch->Panning.nLoopEnd = Patch.nPanningLoopEnd;
        if (Patch.bPanningFlags & XM_ENVELOPE_ON)
            lpPatch->Panning.wFlags |= AUDIO_ENVELOPE_ON;
        if (Patch.bPanningFlags & XM_ENVELOPE_SUSTAIN)
            lpPatch->Panning.wFlags |= AUDIO_ENVELOPE_SUSTAIN;
        if (Patch.bPanningFlags & XM_ENVELOPE_LOOP)
            lpPatch->Panning.wFlags |= AUDIO_ENVELOPE_LOOP;
        for (m = 0; m < lpPatch->Panning.nPoints; m++) {
            lpPatch->Panning.aEnvelope[m].nFrame =
                LOWORD(Patch.aPanningEnvelope[m]);
            lpPatch->Panning.aEnvelope[m].nValue =
                HIWORD(Patch.aPanningEnvelope[m]);
        }
        if (lpPatch->nSamples != 0) {
            if ((lpPatch->aSampleTable = (LPAUDIOSAMPLE)
		 calloc(lpPatch->nSamples, sizeof(AUDIOSAMPLE))) == NULL) {
                AFreeModuleFile(lpModule);
                AIOCloseFile();
                return AUDIO_ERROR_NOMEMORY;
            }
        }

        /* load XM multi-sample header structures */
        lpSample = lpPatch->aSampleTable;
        for (m = 0; m < Patch.nSamples; m++, lpSample++) {
            AIOReadLong(&Sample.dwLength);
            AIOReadLong(&Sample.dwLoopStart);
            AIOReadLong(&Sample.dwLoopLength);
            AIOReadChar(&Sample.nVolume);
            AIOReadChar(&Sample.nFinetune);
            AIOReadChar(&Sample.bFlags);
            AIOReadChar(&Sample.nPanning);
            AIOReadChar(&Sample.nRelativeNote);
            AIOReadChar(&Sample.bReserved);
            AIOReadFile(Sample.aSampleName, sizeof(Sample.aSampleName));
            AIOSeekFile(Patch.dwSampleHeaderSize -
			XM_SAMPLE_HEADER_SIZE, SEEK_CUR);
            strncpy(lpSample->szSampleName, Sample.aSampleName,
		    sizeof(Sample.aSampleName));
            lpSample->Wave.dwLength = Sample.dwLength;
            lpSample->Wave.dwLoopStart = Sample.dwLoopStart;
            lpSample->Wave.dwLoopEnd = Sample.dwLoopStart + Sample.dwLoopLength;
            lpSample->Wave.nSampleRate = 8363;
            lpSample->nVolume = Sample.nVolume;
            lpSample->nPanning = Sample.nPanning;
            lpSample->nRelativeNote = Sample.nRelativeNote;
            lpSample->nFinetune = Sample.nFinetune;
            if (Sample.bFlags & XM_SAMPLE_16BITS) {
                lpSample->Wave.wFormat |= AUDIO_FORMAT_16BITS;
            }
            if (Sample.bFlags & XM_SAMPLE_LOOP)
                lpSample->Wave.wFormat |= AUDIO_FORMAT_LOOP;
            if (Sample.bFlags & XM_SAMPLE_PINGPONG)
                lpSample->Wave.wFormat |= AUDIO_FORMAT_BIDILOOP;
        }

        /* load XM multi-sample waveform data */
        lpSample = lpPatch->aSampleTable;
        for (m = 0; m < Patch.nSamples; m++, lpSample++) {
            if (lpSample->Wave.dwLength == 0)
                continue;
            nErrorCode = ACreateAudioData(&lpSample->Wave);
            if (nErrorCode != AUDIO_ERROR_NONE) {
                AFreeModuleFile(lpModule);
                AIOCloseFile();
                return nErrorCode;
            }
            AIOReadFile(lpSample->Wave.lpData, lpSample->Wave.dwLength);
            XMDecodeSamples(&lpSample->Wave);
            AWriteAudioData(&lpSample->Wave, 0, lpSample->Wave.dwLength);
        }
    }

    AIOCloseFile();
    *lplpModule = lpModule;
    return AUDIO_ERROR_NONE;
}
