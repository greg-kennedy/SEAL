/*
 * $Id: modfile.c 1.3 1996/05/24 08:30:44 chasan released $
 *
 * Protracker/Fastracker module file loader routines.
 *
 * Copyright (c) 1995, 1996 Carlos Hasan. All Rights Reserved.
 *
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
    USHORT  wLength;
    UCHAR   nFinetune;
    UCHAR   nVolume;
    USHORT  wLoopStart;
    USHORT  wLoopLength;
} MODSAMPLEHEADER;

typedef struct {
    CHAR    aModuleName[20];
    MODSAMPLEHEADER aSampleTable[31];
    UCHAR   nSongLength;
    UCHAR   nRestart;
    UCHAR   aOrderTable[128];
    CHAR    aMagic[4];
} MODFILEHEADER;


/*
 * Extended Protracker logarithmic period table
 */
static USHORT aPeriodTable[96] =
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
            if (nPeriod >= aPeriodTable[nNote])
                return nNote + 1;
    }
    return 0;
}

static UINT MODLoadPattern(UINT nTracks, PAUDIOPATTERN pPattern)
{
    UINT nSize, fPacking, nNote, nSample, nCommand, nParams;
    PUCHAR pData, pFTData;

    /* initialize pattern header structure */
    nSize = 4 * 64 * nTracks;
    pPattern->nPacking = 0;
    pPattern->nTracks = nTracks;
    pPattern->nRows = 64;
    pPattern->nSize = nSize + 64 * nTracks;
    if ((pPattern->pData = malloc(pPattern->nSize)) == NULL)
        return AUDIO_ERROR_NOMEMORY;

    /* load MOD pattern data from disk */
    pData = pPattern->pData + 64 * nTracks;
    pFTData = pPattern->pData;
    AIOReadFile(pData, nSize);

    while (nSize != 0) {
        /* grab MOD note event from the pattern */
        nNote = MODGetNoteValue(((pData[0] & 0x0F) << 8) + pData[1]);
        nSample = (pData[0] & 0xF0) + ((pData[2] >> 4) & 0x0F);
        nCommand = (pData[2] & 0x0F);
        nParams = pData[3];
        pData += 4;
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
        *pFTData++ = fPacking;
        if (nNote)
            *pFTData++ = nNote;
        if (nSample)
            *pFTData++ = nSample;
        if (nCommand)
            *pFTData++ = nCommand;
        if (nParams)
            *pFTData++ = nParams;
    }

    pPattern->nSize = (pFTData - pPattern->pData);
    if ((pPattern->pData = realloc(pPattern->pData, pPattern->nSize)) == NULL) {
        return AUDIO_ERROR_NOMEMORY;
    }
    return AUDIO_ERROR_NONE;
}

UINT AIAPI ALoadModuleMOD(PSZ pszFileName, PAUDIOMODULE *ppModule)
{
    static MODFILEHEADER Header;
    static MODSAMPLEHEADER Sample;
    PAUDIOMODULE pModule;
    PAUDIOPATTERN pPattern;
    PAUDIOPATCH pPatch;
    PAUDIOSAMPLE pSample;
    UINT n, nErrorCode;

    if (AIOOpenFile(pszFileName)) {
        return AUDIO_ERROR_FILENOTFOUND;
    }

    if ((pModule = (PAUDIOMODULE) calloc(1, sizeof(AUDIOMODULE))) == NULL) {
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
        if (!memcmp(Header.aMagic, aFmtTable[n].aMagic, sizeof(ULONG))) {
            pModule->nTracks = aFmtTable[n].nTracks;
            break;
        }
    }
    if (pModule->nTracks == 0) {
        AFreeModuleFile(pModule);
        AIOCloseFile();
        return AUDIO_ERROR_BADFILEFORMAT;
    }

    /* initialize the module structure */
    strncpy(pModule->szModuleName, Header.aModuleName,
        sizeof(Header.aModuleName));
    pModule->wFlags = AUDIO_MODULE_AMIGA | AUDIO_MODULE_PANNING;
    pModule->nOrders = Header.nSongLength;
    pModule->nRestart = Header.nRestart;
    pModule->nPatches = 31;
    pModule->nTempo = 6;
    pModule->nBPM = 125;
    for (n = 0; n < sizeof(Header.aOrderTable); n++) {
        pModule->aOrderTable[n] = Header.aOrderTable[n];
        if (pModule->nPatterns < Header.aOrderTable[n])
            pModule->nPatterns = Header.aOrderTable[n];
    }
    pModule->nPatterns++;
    for (n = 0; n < pModule->nTracks; n++) {
        pModule->aPanningTable[n] =
            ((n & 3) == 0 || (n & 3) == 3) ? 0x00 : 0xFF;
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

    /* load MOD pattern sheets */
    pPattern = pModule->aPatternTable;
    for (n = 0; n < pModule->nPatterns; n++, pPattern++) {
        if ((nErrorCode = MODLoadPattern(pModule->nTracks, pPattern)) != 0) {
            AFreeModuleFile(pModule);
            AIOCloseFile();
            return nErrorCode;
        }
    }

    /* load MOD sample waveforms */
    pPatch = pModule->aPatchTable;
    for (n = 0; n < pModule->nPatches; n++, pPatch++) {
        memcpy(&Sample, &Header.aSampleTable[n], sizeof(MODSAMPLEHEADER));
        strncpy(pPatch->szPatchName, Sample.aSampleName,
            sizeof(Sample.aSampleName));
        if (Sample.wLength != 0) {
            if ((pSample = (PAUDIOSAMPLE)
                    calloc(1, sizeof(AUDIOSAMPLE))) == NULL) {
                AFreeModuleFile(pModule);
                AIOCloseFile();
                return AUDIO_ERROR_NOMEMORY;
            }
            pPatch->nSamples = 1;
            pPatch->aSampleTable = pSample;

            /* initialize sample structure */
            pSample->Wave.wFormat = AUDIO_FORMAT_8BITS;
            pSample->Wave.dwLength = (ULONG) Sample.wLength << 1;
            pSample->Wave.nSampleRate = 8363;
            if (Sample.wLoopLength > 1 && Sample.wLoopStart < Sample.wLength) {
                pSample->Wave.wFormat |= AUDIO_FORMAT_LOOP;
                pSample->Wave.dwLoopStart = (ULONG) Sample.wLoopStart << 1;
                pSample->Wave.dwLoopEnd = (ULONG) Sample.wLoopStart << 1;
                pSample->Wave.dwLoopEnd += (ULONG) Sample.wLoopLength << 1;
                if (pSample->Wave.dwLoopEnd > pSample->Wave.dwLength)
                    pSample->Wave.dwLoopEnd = pSample->Wave.dwLength;
            }
            pSample->nVolume = Sample.nVolume <= 64 ? Sample.nVolume : 64;
            pSample->nFinetune = (Sample.nFinetune & 0x0F) << 4;
            pSample->nPanning = 0x80;

            nErrorCode = ACreateAudioData(&pSample->Wave);
            if (nErrorCode != AUDIO_ERROR_NONE) {
                AFreeModuleFile(pModule);
                AIOCloseFile();
                return nErrorCode;
            }

            /* upload waveform data */
            AIOReadFile(pSample->Wave.pData, pSample->Wave.dwLength);
            AWriteAudioData(&pSample->Wave, 0, pSample->Wave.dwLength);
        }
    }

    AIOCloseFile();
    *ppModule = pModule;
    return AUDIO_ERROR_NONE;
}
