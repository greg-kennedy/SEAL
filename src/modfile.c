/*
 * $Id: modfile.c 1.5 1996/09/13 15:10:01 chasan released $
 *
 * Protracker/Fastracker module file loader routines.
 *
 * Copyright (c) 1995-1999 Carlos Hasan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#include <malloc.h>
#include <string.h>
#include "audio.h"
#include "iofile.h"


/*
 * Protracker module file structures
 */
typedef struct {
    CHAR    aSampleName[22];
    WORD    wLength;
    BYTE    nFinetune;
    BYTE    nVolume;
    WORD    wLoopStart;
    WORD    wLoopLength;
} MODSAMPLEHEADER;

typedef struct {
    CHAR    aModuleName[20];
    MODSAMPLEHEADER aSampleTable[31];
    BYTE    nSongLength;
    BYTE    nRestart;
    BYTE    aOrderTable[128];
    CHAR    aMagic[4];
} MODFILEHEADER;


/*
 * Extended Protracker logarithmic period table
 */
static WORD aMODPeriodTable[96] =
{
    6848, 6464, 6096, 5760, 5424, 5120, 4832, 4560, 4304, 4064, 3840, 3624,
    3424, 3232, 3048, 2880, 2712, 2560, 2416, 2280, 2152, 2032, 1920, 1812,
    1712, 1616, 1524, 1440, 1356, 1280, 1208, 1140, 1076, 1016,  960,  906,
    856,  808,  762,  720,  678,  640,  604,  570,  538,  508,  480,  453,
    428,  404,  381,  360,  339,  320,  302,  285,  269,  254,  240,  226,
    214,  202,  190,  180,  170,  160,  151,  143,  135,  127,  120,  113,
    107,  101,   95,   90,   85,   80,   75,   71,   67,   63,   60,   56,
    53,   50,   47,   45,   42,   40,   37,   35,   33,   31,   30,   28
};

/*
 * Protracker module header signatures table
 */
static struct {
    CHAR    aMagic[4];
    UINT    nTracks;
} aFmtTable[] = {
    { "M.K.",  4 }, { "M!K!",  4 },
    { "M&K!",  4 }, { "OCTA",  4 },
    { "FLT4",  4 }, { "6CHN",  6 },
    { "8CHN",  8 }, { "FLT8",  8 },
    { "10CH", 10 }, { "12CH", 12 },
    { "14CH", 14 }, { "16CH", 16 },
    { "18CH", 18 }, { "20CH", 20 },
    { "22CH", 22 }, { "24CH", 24 },
    { "26CH", 26 }, { "28CH", 28 },
    { "30CH", 30 }, { "32CH", 32 }
};


/*
 * Protracker module file loader routines
 */
static UINT MODGetNoteValue(UINT nPeriod)
{
    UINT nNote;

    if (nPeriod != 0) {
        for (nNote = 0; nNote < 96; nNote++)
            if (nPeriod >= aMODPeriodTable[nNote])
                return nNote + 1;
    }
    return 0;
}

static UINT MODLoadPattern(UINT nTracks, LPAUDIOPATTERN lpPattern)
{
    UINT nSize, fPacking, nNote, nSample, nCommand, nParams;
    LPBYTE lpData, lpFTData;

    /* initialize pattern header structure */
    nSize = 4 * 64 * nTracks;
    lpPattern->nPacking = 0;
    lpPattern->nTracks = nTracks;
    lpPattern->nRows = 64;
    lpPattern->nSize = nSize + 64 * nTracks;
    if ((lpPattern->lpData = malloc(lpPattern->nSize)) == NULL)
        return AUDIO_ERROR_NOMEMORY;

    /* load MOD pattern data from disk */
    lpData = lpPattern->lpData + 64 * nTracks;
    lpFTData = lpPattern->lpData;
    AIOReadFile(lpData, nSize);

    while (nSize != 0) {
        /* grab MOD note event from the pattern */
        nNote = MODGetNoteValue(((lpData[0] & 0x0F) << 8) + lpData[1]);
        nSample = (lpData[0] & 0xF0) + ((lpData[2] >> 4) & 0x0F);
        nCommand = (lpData[2] & 0x0F);
        nParams = lpData[3];
        lpData += 4;
        nSize -= 4;

        /* remove some dummy MOD command effects */
        switch ((nCommand << 8) + nParams) {
        case 0x100:
        case 0x200:
        case 0xA00:
        case 0xE10:
        case 0xE20:
        case 0xEA0:
        case 0xEB0:
            nCommand = nParams = 0x00;
            break;
        }

        /* convert DMP-style panning command */
        if (nCommand == 0x8) {
            if (nParams > 0x80)
                nParams = 0x80;
            else if (nParams < 0x80)
                nParams <<= 1;
            else
                nParams = 0xFF;
        }

        /* insert packed note event */
        fPacking = AUDIO_PATTERN_PACKED;
        if (nNote)
            fPacking |= AUDIO_PATTERN_NOTE;
        if (nSample)
            fPacking |= AUDIO_PATTERN_SAMPLE;
        if (nCommand)
            fPacking |= AUDIO_PATTERN_COMMAND;
        if (nParams)
            fPacking |= AUDIO_PATTERN_PARAMS;
        *lpFTData++ = fPacking;
        if (nNote)
            *lpFTData++ = nNote;
        if (nSample)
            *lpFTData++ = nSample;
        if (nCommand)
            *lpFTData++ = nCommand;
        if (nParams)
            *lpFTData++ = nParams;
    }

    lpPattern->nSize = (lpFTData - lpPattern->lpData);
    if ((lpPattern->lpData = realloc(lpPattern->lpData, lpPattern->nSize)) == NULL) {
        return AUDIO_ERROR_NOMEMORY;
    }
    return AUDIO_ERROR_NONE;
}

UINT AIAPI ALoadModuleMOD(LPSTR lpszFileName, 
			  LPAUDIOMODULE *lplpModule, DWORD dwFileOffset)
{
    static MODFILEHEADER Header;
    static MODSAMPLEHEADER Sample;
    LPAUDIOMODULE lpModule;
    LPAUDIOPATTERN lpPattern;
    LPAUDIOPATCH lpPatch;
    LPAUDIOSAMPLE lpSample;
    UINT n, nErrorCode;

    if (AIOOpenFile(lpszFileName)) {
        return AUDIO_ERROR_FILENOTFOUND;
    }
    AIOSeekFile(dwFileOffset, SEEK_SET);

    if ((lpModule = (LPAUDIOMODULE) calloc(1, sizeof(AUDIOMODULE))) == NULL) {
        AIOCloseFile();
        return AUDIO_ERROR_NOMEMORY;
    }

    /* load MOD module header */
    AIOReadFile(Header.aModuleName, sizeof(Header.aModuleName));
    for (n = 0; n < 31; n++) {
        AIOReadFile(Header.aSampleTable[n].aSampleName,
		    sizeof(Header.aSampleTable[n].aSampleName));
        AIOReadShortM(&Header.aSampleTable[n].wLength);
        AIOReadCharM(&Header.aSampleTable[n].nFinetune);
        AIOReadCharM(&Header.aSampleTable[n].nVolume);
        AIOReadShortM(&Header.aSampleTable[n].wLoopStart);
        AIOReadShortM(&Header.aSampleTable[n].wLoopLength);
    }
    AIOReadCharM(&Header.nSongLength);
    AIOReadCharM(&Header.nRestart);
    AIOReadFile(Header.aOrderTable, sizeof(Header.aOrderTable));
    AIOReadFile(Header.aMagic, sizeof(Header.aMagic));
    for (n = 0; n < sizeof(aFmtTable) / sizeof(aFmtTable[0]); n++) {
        if (!memcmp(Header.aMagic, aFmtTable[n].aMagic, sizeof(DWORD))) {
            lpModule->nTracks = aFmtTable[n].nTracks;
            break;
        }
    }
    if (lpModule->nTracks == 0) {
        AFreeModuleFile(lpModule);
        AIOCloseFile();
        return AUDIO_ERROR_BADFILEFORMAT;
    }

    /* initialize the module structure */
    strncpy(lpModule->szModuleName, Header.aModuleName,
	    sizeof(Header.aModuleName));
    lpModule->wFlags = AUDIO_MODULE_AMIGA | AUDIO_MODULE_PANNING;
    lpModule->nOrders = Header.nSongLength;
    lpModule->nRestart = Header.nRestart;
    lpModule->nPatches = 31;
    lpModule->nTempo = 6;
    lpModule->nBPM = 125;
    for (n = 0; n < sizeof(Header.aOrderTable); n++) {
        lpModule->aOrderTable[n] = Header.aOrderTable[n];
        if (lpModule->nPatterns < Header.aOrderTable[n])
            lpModule->nPatterns = Header.aOrderTable[n];
    }
    lpModule->nPatterns++;
    for (n = 0; n < lpModule->nTracks; n++) {
        lpModule->aPanningTable[n] =
            ((n & 3) == 0 || (n & 3) == 3) ? 0x00 : 0xFF;
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

    /* load MOD pattern sheets */
    lpPattern = lpModule->aPatternTable;
    for (n = 0; n < lpModule->nPatterns; n++, lpPattern++) {
        if ((nErrorCode = MODLoadPattern(lpModule->nTracks, lpPattern)) != 0) {
            AFreeModuleFile(lpModule);
            AIOCloseFile();
            return nErrorCode;
        }
    }

    /* load MOD sample waveforms */
    lpPatch = lpModule->aPatchTable;
    for (n = 0; n < lpModule->nPatches; n++, lpPatch++) {
        memcpy(&Sample, &Header.aSampleTable[n], sizeof(MODSAMPLEHEADER));
        strncpy(lpPatch->szPatchName, Sample.aSampleName,
		sizeof(Sample.aSampleName));
        if (Sample.wLength != 0) {
            if ((lpSample = (LPAUDIOSAMPLE)
		 calloc(1, sizeof(AUDIOSAMPLE))) == NULL) {
                AFreeModuleFile(lpModule);
                AIOCloseFile();
                return AUDIO_ERROR_NOMEMORY;
            }
            lpPatch->nSamples = 1;
            lpPatch->aSampleTable = lpSample;

            /* initialize sample structure */
            lpSample->Wave.wFormat = AUDIO_FORMAT_8BITS;
            lpSample->Wave.dwLength = (DWORD) Sample.wLength << 1;
            lpSample->Wave.nSampleRate = 8363;
            if (Sample.wLoopLength > 1 && Sample.wLoopStart < Sample.wLength) {
                lpSample->Wave.wFormat |= AUDIO_FORMAT_LOOP;
                lpSample->Wave.dwLoopStart = (DWORD) Sample.wLoopStart << 1;
                lpSample->Wave.dwLoopEnd = (DWORD) Sample.wLoopStart << 1;
                lpSample->Wave.dwLoopEnd += (DWORD) Sample.wLoopLength << 1;
                if (lpSample->Wave.dwLoopEnd > lpSample->Wave.dwLength)
                    lpSample->Wave.dwLoopEnd = lpSample->Wave.dwLength;
            }
            lpSample->nVolume = Sample.nVolume <= 64 ? Sample.nVolume : 64;
            lpSample->nFinetune = (Sample.nFinetune & 0x0F) << 4;
            lpSample->nPanning = 0x80;

            nErrorCode = ACreateAudioData(&lpSample->Wave);
            if (nErrorCode != AUDIO_ERROR_NONE) {
                AFreeModuleFile(lpModule);
                AIOCloseFile();
                return nErrorCode;
            }

            /* upload waveform data */
            AIOReadFile(lpSample->Wave.lpData, lpSample->Wave.dwLength);
            AWriteAudioData(&lpSample->Wave, 0, lpSample->Wave.dwLength);
        }
    }

    AIOCloseFile();
    *lplpModule = lpModule;
    return AUDIO_ERROR_NONE;
}
