/*
 * $Id: mtmfile.c 1.1 1996/09/25 17:19:14 chasan released $
 *
 * Multitracker 1.0 module file loader
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
 * MTM file header signature defines
 */
#define MTM_SIGN_PATTERN    0x104D544DL
#define MTM_SIGN_MASK       0xF0FFFFFFL

/*
 * MTM module header structure
 */
typedef struct {
    DWORD   dwMTM;
    CHAR    szModuleName[20];
    WORD    nSeqTracks;
    BYTE    nPatterns;
    BYTE    nOrders;
    WORD    nMesgSize;
    BYTE    nSamples;
    BYTE    bFlags;
    BYTE    nBeatsPerTrack;
    BYTE    nChannels;
    BYTE    aPanningTable[32];
} MTMHEADER, *LPMTMHEADER;

/*
 * MTM sample header structure
 */
typedef struct {
    CHAR    szSampleName[22];
    DWORD   dwLength;
    DWORD   dwLoopStart;
    DWORD   dwLoopEnd;
    BYTE    nFinetune;
    BYTE    nVolume;
    BYTE    bFlags;
} MTMSAMPLE, *LPMTMSAMPLE;

/*
 * MTM track data structure
 */
typedef struct {
    BYTE    aData[3*64];
} MTMTRACK, *LPMTMTRACK;


static UINT MTMMakePattern(UINT nTracks, UINT nSeqTracks,
			   LPAUDIOPATTERN lpPattern, WORD aPatSeqTable[32], LPMTMTRACK lpTrackTable)
{
    LPBYTE lpData, lpEvent;
    UINT nRowOfs, nTrack;
    UINT fFlags, nNote, nSample, nCommand, nParams;

    if ((lpData = (LPBYTE) malloc(64*5*nTracks)) == NULL)
        return AUDIO_ERROR_NOMEMORY;

    lpPattern->lpData = lpData;
    lpPattern->nPacking = 0;
    lpPattern->nTracks = nTracks;
    lpPattern->nRows = 64;
    for (nRowOfs = 0; nRowOfs < 3*64; nRowOfs += 3) {
        for (nTrack = 0; nTrack < nTracks; nTrack++) {
            if (aPatSeqTable[nTrack] >= 1 &&
                aPatSeqTable[nTrack] <= nSeqTracks) {
                lpEvent = &lpTrackTable[aPatSeqTable[nTrack]-1].aData[nRowOfs];
                nNote = (lpEvent[0] >> 2) & 0x3F;
                nSample = ((lpEvent[0] & 0x03) << 4) | (lpEvent[1] >> 4);
                nCommand = (lpEvent[1] & 0x0F);
                nParams = lpEvent[2];
                fFlags = AUDIO_PATTERN_PACKED;
                if (nNote) {
                    nNote += 12*2 + 1;
                    fFlags |= AUDIO_PATTERN_NOTE;
                }
                if (nSample)
                    fFlags |= AUDIO_PATTERN_SAMPLE;
                if (nCommand)
                    fFlags |= AUDIO_PATTERN_COMMAND;
                if (nParams)
                    fFlags |= AUDIO_PATTERN_PARAMS;
                *lpData++ = fFlags;
                if (fFlags & AUDIO_PATTERN_NOTE)
                    *lpData++ = nNote;
                if (fFlags & AUDIO_PATTERN_SAMPLE)
                    *lpData++ = nSample;
                if (fFlags & AUDIO_PATTERN_COMMAND)
                    *lpData++ = nCommand;
                if (fFlags & AUDIO_PATTERN_PARAMS)
                    *lpData++ = nParams;
            }
            else {
                *lpData++ = AUDIO_PATTERN_PACKED;
            }
        }
    }

    lpPattern->nSize = lpData - lpPattern->lpData;
    if ((lpPattern->lpData = (LPBYTE) 
	 realloc(lpPattern->lpData, lpPattern->nSize)) == NULL)
        return AUDIO_ERROR_NOMEMORY;

    return AUDIO_ERROR_NONE;
}

static UINT MTMMakeSample(LPAUDIOPATCH lpPatch, LPMTMSAMPLE lpMTMSample)
{
    LPAUDIOSAMPLE lpSample;
    LPBYTE lpData;
    DWORD dwCount;
    UINT rc;

    strncpy(lpPatch->szPatchName, lpMTMSample->szSampleName,
	    sizeof(lpMTMSample->szSampleName));
    if (lpMTMSample->dwLength) {
        if ((lpSample = (LPAUDIOSAMPLE) calloc(1, sizeof(AUDIOSAMPLE))) == NULL)
            return AUDIO_ERROR_NOMEMORY;

        lpPatch->nSamples = 1;
        lpPatch->aSampleTable = lpSample;
        lpSample->Wave.wFormat = AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO;
        lpSample->Wave.dwLength = lpMTMSample->dwLength;
        lpSample->nVolume = lpMTMSample->nVolume;
        lpSample->nFinetune = lpMTMSample->nFinetune;
        lpSample->nPanning = 0x80;
        if (lpMTMSample->dwLoopStart + 2 < lpMTMSample->dwLoopEnd &&
            lpMTMSample->dwLoopEnd <= lpMTMSample->dwLength) {
            lpSample->Wave.wFormat |= AUDIO_FORMAT_LOOP;
            lpSample->Wave.dwLoopStart = lpMTMSample->dwLoopStart;
            lpSample->Wave.dwLoopEnd = lpMTMSample->dwLoopEnd;
        }
        if ((rc = ACreateAudioData(&lpSample->Wave)) != AUDIO_ERROR_NONE)
            return rc;
        AIOReadFile(lpSample->Wave.lpData, lpSample->Wave.dwLength);
        lpData = lpSample->Wave.lpData;
        dwCount = lpSample->Wave.dwLength;
        while (dwCount--)
            *lpData++ ^= 0x80;
        AWriteAudioData(&lpSample->Wave, 0, lpSample->Wave.dwLength);
    }
    return AUDIO_ERROR_NONE;
}

UINT AIAPI ALoadModuleMTM(LPSTR lpszFileName, 
			  LPAUDIOMODULE* lplpModule, DWORD dwFileOffset)
{
    LPAUDIOMODULE lpModule;
    static MTMHEADER Header;
    LPMTMSAMPLE lpSampleTable;
    LPMTMTRACK lpTrackTable;
    static WORD aPatSeqTable[32];
    UINT n, rc;

    if (AIOOpenFile(lpszFileName)) {
        return AUDIO_ERROR_FILENOTFOUND;
    }
    AIOSeekFile(dwFileOffset, SEEK_SET);

    if ((lpModule = (LPAUDIOMODULE) calloc(1, sizeof(AUDIOMODULE))) == NULL) {
        AIOCloseFile();
        return AUDIO_ERROR_NOMEMORY;
    }

    /* load MTM module header data */
    AIOReadLong(&Header.dwMTM);
    AIOReadFile(Header.szModuleName, sizeof(Header.szModuleName));
    AIOReadShort(&Header.nSeqTracks);
    AIOReadChar(&Header.nPatterns);
    AIOReadChar(&Header.nOrders);
    AIOReadShort(&Header.nMesgSize);
    AIOReadChar(&Header.nSamples);
    AIOReadChar(&Header.bFlags);
    AIOReadChar(&Header.nBeatsPerTrack);
    AIOReadChar(&Header.nChannels);
    AIOReadFile(Header.aPanningTable, sizeof(Header.aPanningTable));

    /* check MTM file header signature and other fields */
    if ((Header.dwMTM & MTM_SIGN_MASK) != MTM_SIGN_PATTERN ||
/***    (Header.nPatterns > AUDIO_MAX_PATTERNS) ||   FIX: avoid warnings ***/
/***    (Header.nOrders > AUDIO_MAX_ORDERS) ||       FIX: avoid warnings ***/
        (Header.nSamples > AUDIO_MAX_PATCHES) ||
        (Header.nChannels > AUDIO_MAX_VOICES) ||
        (Header.nBeatsPerTrack != 64)) {
        AFreeModuleFile(lpModule);
        AIOCloseFile();
        return AUDIO_ERROR_BADFILEFORMAT;
    }

    /* build the local module header structure */
    strncpy(lpModule->szModuleName, Header.szModuleName,
	    sizeof(Header.szModuleName));
    lpModule->wFlags = AUDIO_MODULE_AMIGA | AUDIO_MODULE_PANNING;
    lpModule->nOrders = Header.nOrders + 1;
    lpModule->nRestart = AUDIO_MAX_ORDERS;
    lpModule->nTracks = Header.nChannels;
    lpModule->nPatterns = Header.nPatterns + 1;
    lpModule->nPatches = Header.nSamples;
    lpModule->nTempo = 6;
    lpModule->nBPM = 125;
    for (n = 0; n < lpModule->nTracks; n++) {
        lpModule->aPanningTable[n] = Header.aPanningTable[n] << 4;
    }

    /* allocate space for local patterns and patches */
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

    /* allocate space for MTM samples and track structures */
    if ((lpSampleTable = (LPMTMSAMPLE) 
	 calloc(Header.nSamples, sizeof(MTMSAMPLE))) == NULL) {
        AFreeModuleFile(lpModule);
        AIOCloseFile();
        return AUDIO_ERROR_NOMEMORY;
    }
    if ((lpTrackTable = (LPMTMTRACK)
	 calloc(Header.nSeqTracks, sizeof(MTMTRACK))) == NULL) {
        free(lpSampleTable);
        AFreeModuleFile(lpModule);
        AIOCloseFile();
        return AUDIO_ERROR_NOMEMORY;
    }

    /* load MTM sample header structures */
    for (n = 0; n < lpModule->nPatches; n++) {
        AIOReadFile(lpSampleTable[n].szSampleName, 
		    sizeof(lpSampleTable[n].szSampleName));
        AIOReadLong(&lpSampleTable[n].dwLength);
        AIOReadLong(&lpSampleTable[n].dwLoopStart);
        AIOReadLong(&lpSampleTable[n].dwLoopEnd);
        AIOReadChar(&lpSampleTable[n].nFinetune);
        AIOReadChar(&lpSampleTable[n].nVolume);
        AIOReadChar(&lpSampleTable[n].bFlags);
    }

    /* load the MTM module order list */
    AIOReadFile(lpModule->aOrderTable, 128);

    /* load MTM module tracks from disk */
    AIOReadFile(lpTrackTable, sizeof(MTMTRACK) * Header.nSeqTracks);

    /* build module patterns from tracks */
    for (n = 0; n < lpModule->nPatterns; n++) {
        AIOReadFile(aPatSeqTable, sizeof(aPatSeqTable));
        rc = MTMMakePattern(lpModule->nTracks, Header.nSeqTracks,
			    &lpModule->aPatternTable[n], aPatSeqTable, lpTrackTable);
        if (rc != AUDIO_ERROR_NONE) {
            free(lpTrackTable);
            free(lpSampleTable);
            AFreeModuleFile(lpModule);
            AIOCloseFile();
            return rc;
        }
    }
    free(lpTrackTable);

    /* skip MTM comment message data */
    AIOSeekFile(Header.nMesgSize, SEEK_CUR);

    /* build module patches and samples structures */
    for (n = 0; n < lpModule->nPatches; n++) {
        rc = MTMMakeSample(&lpModule->aPatchTable[n], &lpSampleTable[n]);
        if (rc != AUDIO_ERROR_NONE) {
            free(lpSampleTable);
            AFreeModuleFile(lpModule);
            AIOCloseFile();
            return rc;
        }
    }
    free(lpSampleTable);

    *lplpModule = lpModule;
    return AUDIO_ERROR_NONE;
}
