/*
 * $Id: xmfile.c 1.3 1996/05/24 08:30:44 chasan released $
 *
 * Extended module file loader routines.
 *
 * Copyright (c) 1995, 1996 Carlos Hasan. All Rights Reserved.
 *
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
    CHAR    bPadding;
    CHAR    aTrackerName[20];
    USHORT  wVersion;
    ULONG   dwHeaderSize;
    USHORT  nSongLength;
    USHORT  nRestart;
    USHORT  nTracks;
    USHORT  nPatterns;
    USHORT  nPatches;
    USHORT  wFlags;
    USHORT  nTempo;
    USHORT  nBPM;
    UCHAR   aOrderTable[256];
} XMFILEHEADER;

typedef struct {
    ULONG   dwHeaderSize;
    UCHAR   nPacking;
    USHORT  nRows;
    USHORT  nSize;
} XMPATTERNHEADER;

typedef struct {
    ULONG   dwHeaderSize;
    CHAR    aPatchName[22];
    UCHAR   nType;
    USHORT  nSamples;
    ULONG   dwSampleHeaderSize;
    UCHAR   aSampleNumber[96];
    ULONG   aVolumeEnvelope[12];
    ULONG   aPanningEnvelope[12];
    UCHAR   nVolumePoints;
    UCHAR   nPanningPoints;
    UCHAR   nVolumeSustain;
    UCHAR   nVolumeLoopStart;
    UCHAR   nVolumeLoopEnd;
    UCHAR   nPanningSustain;
    UCHAR   nPanningLoopStart;
    UCHAR   nPanningLoopEnd;
    UCHAR   bVolumeFlags;
    UCHAR   bPanningFlags;
    UCHAR   nVibratoType;
    UCHAR   nVibratoSweep;
    UCHAR   nVibratoDepth;
    UCHAR   nVibratoRate;
    USHORT  nVolumeFadeout;
    USHORT  wReserved;
} XMPATCHHEADER;

typedef struct {
    ULONG   dwLength;
    ULONG   dwLoopStart;
    ULONG   dwLoopLength;
    UCHAR   nVolume;
    UCHAR   nFinetune;
    UCHAR   bFlags;
    UCHAR   nPanning;
    UCHAR   nRelativeNote;
    UCHAR   bReserved;
    CHAR    aSampleName[22];
} XMSAMPLEHEADER;


/*
 * Extended module loader routines
 */
static VOID XMDecodeSamples(PAUDIOWAVE pWave)
{
    PUSHORT pwCode;
    PUCHAR pbCode;
    UINT nDelta, nCount;

    nDelta = 0;
    nCount = pWave->dwLength;
    if (pWave->wFormat & AUDIO_FORMAT_16BITS) {
        pwCode = (PUSHORT) pWave->pData;
        nCount >>= 1;
        while (nCount--) {
#ifdef __BIGENDIAN__
            *pwCode = MAKEWORD(HIBYTE(*pwCode), LOBYTE(*pwCode));
#endif
            nDelta = *pwCode++ += nDelta;
        }
    }
    else {
        pbCode = (PUCHAR) pWave->pData;
        while (nCount--) {
            nDelta = (UCHAR) (*pbCode++ += (UCHAR) nDelta);
        }
    }
}

UINT AIAPI ALoadModuleXM(PSZ pszFileName, PAUDIOMODULE *ppModule)
{
    static XMFILEHEADER Header;
    static XMPATTERNHEADER Pattern;
    static XMPATCHHEADER Patch;
    static XMSAMPLEHEADER Sample;
    PAUDIOMODULE pModule;
    PAUDIOPATTERN pPattern;
    PAUDIOPATCH pPatch;
    PAUDIOSAMPLE pSample;
    UINT n, m, nErrorCode;

    /* open XM module file */
    if (AIOOpenFile(pszFileName)) {
        return AUDIO_ERROR_FILENOTFOUND;
    }

    if ((pModule = (PAUDIOMODULE) calloc(1, sizeof(AUDIOMODULE))) == NULL) {
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
        AFreeModuleFile(pModule);
        AIOCloseFile();
        return AUDIO_ERROR_BADFILEFORMAT;
    }

    /* initialize module structure */
    strncpy(pModule->szModuleName, Header.aModuleName,
        sizeof(Header.aModuleName));
    if (Header.wFlags & XM_MODULE_LINEAR)
        pModule->wFlags |= AUDIO_MODULE_LINEAR;
    pModule->nOrders = Header.nSongLength;
    pModule->nRestart = Header.nRestart;
    pModule->nTracks = Header.nTracks;
    pModule->nPatterns = Header.nPatterns;
    pModule->nPatches = Header.nPatches;
    pModule->nTempo = Header.nTempo;
    pModule->nBPM = Header.nBPM;
    for (n = 0; n < pModule->nOrders; n++) {
        pModule->aOrderTable[n] = Header.aOrderTable[n];
    }
    for (n = 0; n < pModule->nTracks; n++) {
        pModule->aPanningTable[n] = (AUDIO_MIN_PANNING + AUDIO_MAX_PANNING)/2;
    }
    if ((pModule->aPatternTable = (PAUDIOPATTERN)
            calloc(pModule->nPatterns, sizeof(AUDIOPATTERN))) == NULL) {
        AFreeModuleFile(pModule);
        AIOCloseFile();
        return AUDIO_ERROR_NOMEMORY;
    }
    if ((pModule->aPatchTable = (PAUDIOPATCH)
            calloc(pModule->nPatches, sizeof(AUDIOPATCH))) == NULL) {
        AFreeModuleFile(pModule);
        AIOCloseFile();
        return AUDIO_ERROR_NOMEMORY;
    }

    pPattern = pModule->aPatternTable;
    for (n = 0; n < pModule->nPatterns; n++, pPattern++) {
        /* load XM pattern header structure */
        AIOReadLong(&Pattern.dwHeaderSize);
        AIOReadChar(&Pattern.nPacking);
        AIOReadShort(&Pattern.nRows);
        AIOReadShort(&Pattern.nSize);
        AIOSeekFile(Pattern.dwHeaderSize - XM_PATTERN_HEADER_SIZE, SEEK_CUR);
        if (Pattern.nPacking != 0) {
            AFreeModuleFile(pModule);
            AIOCloseFile();
            return AUDIO_ERROR_BADFILEFORMAT;
        }

        /* initialize pattern structure */
        pPattern->nPacking = Pattern.nPacking;
        pPattern->nTracks = pModule->nTracks;
        pPattern->nRows = Pattern.nRows;
        pPattern->nSize = Pattern.nSize;
        if (pPattern->nSize != 0) {
            /* allocate and load pattern data */
            if ((pPattern->pData = malloc(pPattern->nSize)) == NULL) {
                AFreeModuleFile(pModule);
                AIOCloseFile();
                return AUDIO_ERROR_NOMEMORY;
            }
            AIOReadFile(pPattern->pData, pPattern->nSize);
        }
    }

    pPatch = pModule->aPatchTable;
    for (n = 0; n < pModule->nPatches; n++, pPatch++) {
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
        if (Patch.nSamples > AUDIO_MAX_SAMPLES ||
            Patch.nVolumePoints > AUDIO_MAX_POINTS ||
            Patch.nPanningPoints > AUDIO_MAX_POINTS) {
            AFreeModuleFile(pModule);
            AIOCloseFile();
            return AUDIO_ERROR_BADFILEFORMAT;
        }

        /* initialize patch structure */
        strncpy(pPatch->szPatchName, Patch.aPatchName,
            sizeof(Patch.aPatchName));
        for (m = 0; m < AUDIO_MAX_NOTES; m++) {
            pPatch->aSampleNumber[m] = Patch.aSampleNumber[m];
            if (pPatch->aSampleNumber[m] >= Patch.nSamples)
                pPatch->aSampleNumber[m] = 0x00;
        }
        pPatch->nSamples = Patch.nSamples;
        pPatch->nVibratoType = Patch.nVibratoType;
        pPatch->nVibratoSweep = Patch.nVibratoSweep;
        pPatch->nVibratoDepth = Patch.nVibratoDepth;
        pPatch->nVibratoRate = Patch.nVibratoRate;
        pPatch->nVolumeFadeout = Patch.nVolumeFadeout;
        pPatch->Volume.nPoints = Patch.nVolumePoints;
        pPatch->Volume.nSustain = Patch.nVolumeSustain;
        pPatch->Volume.nLoopStart = Patch.nVolumeLoopStart;
        pPatch->Volume.nLoopEnd = Patch.nVolumeLoopEnd;
        if (Patch.bVolumeFlags & XM_ENVELOPE_ON)
            pPatch->Volume.wFlags |= AUDIO_ENVELOPE_ON;
        if (Patch.bVolumeFlags & XM_ENVELOPE_SUSTAIN)
            pPatch->Volume.wFlags |= AUDIO_ENVELOPE_SUSTAIN;
        if (Patch.bVolumeFlags & XM_ENVELOPE_LOOP)
            pPatch->Volume.wFlags |= AUDIO_ENVELOPE_LOOP;
        for (m = 0; m < pPatch->Volume.nPoints; m++) {
            pPatch->Volume.aEnvelope[m].nFrame =
                LOWORD(Patch.aVolumeEnvelope[m]);
            pPatch->Volume.aEnvelope[m].nValue =
                HIWORD(Patch.aVolumeEnvelope[m]);
        }
        pPatch->Panning.nPoints = Patch.nPanningPoints;
        pPatch->Panning.nSustain = Patch.nPanningSustain;
        pPatch->Panning.nLoopStart = Patch.nPanningLoopStart;
        pPatch->Panning.nLoopEnd = Patch.nPanningLoopEnd;
        if (Patch.bPanningFlags & XM_ENVELOPE_ON)
            pPatch->Panning.wFlags |= AUDIO_ENVELOPE_ON;
        if (Patch.bPanningFlags & XM_ENVELOPE_SUSTAIN)
            pPatch->Panning.wFlags |= AUDIO_ENVELOPE_SUSTAIN;
        if (Patch.bPanningFlags & XM_ENVELOPE_LOOP)
            pPatch->Panning.wFlags |= AUDIO_ENVELOPE_LOOP;
        for (m = 0; m < pPatch->Panning.nPoints; m++) {
            pPatch->Panning.aEnvelope[m].nFrame =
                LOWORD(Patch.aPanningEnvelope[m]);
            pPatch->Panning.aEnvelope[m].nValue =
                HIWORD(Patch.aPanningEnvelope[m]);
        }
        if (pPatch->nSamples != 0) {
            if ((pPatch->aSampleTable = (PAUDIOSAMPLE)
                    calloc(pPatch->nSamples, sizeof(AUDIOSAMPLE))) == NULL) {
                AFreeModuleFile(pModule);
                AIOCloseFile();
                return AUDIO_ERROR_NOMEMORY;
            }
        }

        /* load XM multi-sample header structures */
        pSample = pPatch->aSampleTable;
        for (m = 0; m < Patch.nSamples; m++, pSample++) {
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
            strncpy(pSample->szSampleName, Sample.aSampleName,
                sizeof(Sample.aSampleName));
            pSample->Wave.dwLength = Sample.dwLength;
            pSample->Wave.dwLoopStart = Sample.dwLoopStart;
            pSample->Wave.dwLoopEnd = Sample.dwLoopStart + Sample.dwLoopLength;
            pSample->Wave.nSampleRate = 8363;
            pSample->nVolume = Sample.nVolume;
            pSample->nPanning = Sample.nPanning;
            pSample->nRelativeNote = Sample.nRelativeNote;
            pSample->nFinetune = Sample.nFinetune;
            if (Sample.bFlags & XM_SAMPLE_16BITS) {
                pSample->Wave.wFormat |= AUDIO_FORMAT_16BITS;
            }
            if (Sample.bFlags & XM_SAMPLE_LOOP)
                pSample->Wave.wFormat |= AUDIO_FORMAT_LOOP;
            if (Sample.bFlags & XM_SAMPLE_PINGPONG)
                pSample->Wave.wFormat |= AUDIO_FORMAT_BIDILOOP;
        }

        /* load XM multi-sample waveform data */
        pSample = pPatch->aSampleTable;
        for (m = 0; m < Patch.nSamples; m++, pSample++) {
            if (pSample->Wave.dwLength == 0)
                continue;
            nErrorCode = ACreateAudioData(&pSample->Wave);
            if (nErrorCode != AUDIO_ERROR_NONE) {
                AFreeModuleFile(pModule);
                AIOCloseFile();
                return nErrorCode;
            }
            AIOReadFile(pSample->Wave.pData, pSample->Wave.dwLength);
            XMDecodeSamples(&pSample->Wave);
            AWriteAudioData(&pSample->Wave, 0, pSample->Wave.dwLength);
        }
    }

    AIOCloseFile();
    *ppModule = pModule;
    return AUDIO_ERROR_NONE;
}
