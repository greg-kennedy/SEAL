/*
 * $Id: s3mfile.c 1.7 1996/09/13 15:10:01 chasan released $
 *
 * Scream Tracker 3.0 module file loader routines.
 *
 * Copyright (c) 1995-1999 Carlos Hasan
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
 * ScreamTracker 3.0 module file structures
 */

#define S3M_MAX_TRACKS          32
#define S3M_MAX_SAMPLES         100
#define S3M_MAX_PATTERNS        256
#define S3M_MAX_ORDERS          256
#define S3M_MAX_ROWS            64
#define S3M_SCRM_MAGIC          0x4D524353L
#define S3M_SCRS_MAGIC          0x53524353L
#define S3M_SCRS_PCM            0x01
#define S3M_SCRS_LOOPED         0x01

typedef struct {
    CHAR    aModuleName[28];
    BYTE    bPadding;
    BYTE    nFileType;
    WORD    wReserved;
    WORD    nSongLength;
    WORD    nSamples;
    WORD    nPatterns;
    WORD    wFlags;
    WORD    wVersion;
    WORD    nSampleType;
    DWORD   dwSCRM;
    BYTE    nGlobalVolume;
    BYTE    nTempo;
    BYTE    nBPM;
    BYTE    nMasterVolume;
    BYTE    nUltraClick;
    BYTE    nDefaultPanning;
    BYTE    aReserved[8];
    WORD    wSpecial;
    BYTE    aChannelTable[32];
} S3MFILEHEADER;

typedef struct {
    BYTE    nType;
    CHAR    aFileName[13];
    WORD    wDataSegPtr;
    DWORD   dwLength;
    DWORD   dwLoopStart;
    DWORD   dwLoopEnd;
    BYTE    nVolume;
    BYTE    nReserved;
    BYTE    nPacking;
    BYTE    bFlags;
    DWORD   nSampleRate;
    BYTE    aReserved[12];
    CHAR    aSampleName[28];
    DWORD   dwSCRS;
} S3MSAMPLEHEADER;

typedef struct {
    WORD    nSize;
    LPBYTE  lpData;
} S3MPATTERNHEADER;

typedef struct {
    BYTE    nNote;
    BYTE    nSample;
    BYTE    nVolume;
    BYTE    nCommand;
    BYTE    nParams;
} S3MTRACKDATA;


#define ACC 11
#define MUL(a,b) (((a)*(b)+(1<<(ACC-1)))>>ACC)
#define A (LONG)(12.0 * -2.4991805816500 * (1 << ACC))
#define B (LONG)(12.0 * +4.0305172865900 * (1 << ACC))
#define C (LONG)(12.0 * -2.0791605988100 * (1 << ACC))
#define D (LONG)(12.0 * +0.6262992372690 * (1 << ACC))
#define E (LONG)(12.0 * -0.0784753434027 * (1 << ACC))

static LONG S3MGetRelativeNote(LONG dwSampleRate)
{
    LONG dwRelativeNote;

    /*
     * Compute the relative note value given a sampling frequency:
     *    RelativeNote = 12.0 * log2(SampleRate / 8363.0)
     *
     * The following algorithm uses 21.11 fixed-point arithmetic
     * with an accuracy of about 1/128 for sampling frequencies
     * between 522 and 65535 Hertz. (Thanks Zed!)
     */
    dwSampleRate = (dwSampleRate << (ACC + 4)) / 8363;
    dwRelativeNote = -48L << ACC;
    while (dwSampleRate > (2L << ACC)) {
        dwSampleRate = (dwSampleRate + 1) >> 1;
        dwRelativeNote += (12L << ACC);
    }
    dwRelativeNote += A + MUL(B + MUL(C + MUL(D + MUL(E, dwSampleRate),
					      dwSampleRate), dwSampleRate), dwSampleRate);
    return dwRelativeNote >> (ACC - 7);
}

static UINT S3MDecodePattern(UINT nTracks, LPBYTE lpData, UINT nSize,
			     BYTE aMappingTable[], LPAUDIOPATTERN lpPattern)
{
    static S3MTRACKDATA aTrackTable[S3M_MAX_TRACKS];
    static BYTE aParamTable[S3M_MAX_TRACKS];
    UINT nRow, nTrack, nFlags, nNote, nSample, nVolume, nCommand, nParams;
    LPBYTE lpFTData, lpEndData;

    /* initialize the pattern structure */
    lpPattern->nPacking = 0;
    lpPattern->nTracks = nTracks;
    lpPattern->nRows = S3M_MAX_ROWS;
    lpPattern->nSize = 0;
    if ((lpPattern->lpData = malloc(nTracks * 6 * S3M_MAX_ROWS)) == NULL) {
        return AUDIO_ERROR_NOMEMORY;
    }

    lpFTData = lpPattern->lpData;
    lpEndData = lpData + nSize;
    memset(aParamTable, 0, sizeof(aParamTable));
    for (nRow = 0; nRow < S3M_MAX_ROWS; nRow++) {
        /* grab the next row of notes from the S3M pattern */
        memset(aTrackTable, 0, sizeof(aTrackTable));
        while (lpData < lpEndData && (nFlags = *lpData++) != 0x00) {
            /* get the note event */
            nNote = 0xFF;
            nSample = 0x00;
            nVolume = 0xFF;
            nCommand = 0x00;
            nParams = 0x00;
            nTrack = nFlags & 0x1F;
            if (nFlags & 0x20) {
                nNote = *lpData++;
                nSample = *lpData++;
            }
            if (nFlags & 0x40) {
                nVolume = *lpData++;
            }
            if (nFlags & 0x80) {
                nCommand = *lpData++;
                nParams = *lpData++;
            }

            /* skip notes for non-PCM tracks */
            if ((nTrack = aMappingTable[nTrack]) >= nTracks)
                continue;

            /* decode note index */
            if (nNote == 0xFE) {
                nNote = AUDIO_MAX_NOTES + 1;
            }
            else if (nNote == 0xFF) {
                nNote = 0x00;
            }
            else {
                nNote = 12 * (nNote >> 4) + (nNote & 0x0F) + 1;
                if (nNote > AUDIO_MAX_NOTES)
                    nNote = 0;
            }

            /* decode S3M command effect default parameter stuff */
            nCommand += 0x40;
            if (nParams != 0x00) {
                aParamTable[nTrack] = nParams;
            }
            else if (strchr("DEFGKLQ", nCommand)) {
                nParams = aParamTable[nTrack];
            }

            /* decode S3M command effect */
            switch (nCommand) {
            case '@':
                /* null command */
                nCommand = nParams = 0x00;
                break;

            case 'A':
                /* set tempo speed */
                if (nParams < 0x20) {
                    nCommand = 0x0F;
                }
                else {
                    /* WARNING: tempo greater than 31 are not supported! */
                    nCommand = nParams = 0x00;
                }
                break;

            case 'B':
                /* jump position */
                nCommand = 0x0B;
                break;

            case 'C':
                /* break pattern */
                nCommand = 0x0D;
                break;

            case 'D':
                if ((nParams & 0xF0) == 0x00) {
                    /* volume slide down */
                    nCommand = 0x0A;
                    nParams &= 0x0F;
                }
                else if ((nParams & 0x0F) == 0x00) {
                    /* volume slide up */
                    nCommand = 0x0A;
                    nParams &= 0xF0;
                }
                else if ((nParams & 0xF0) == 0xF0) {
                    /* fine volume slide down */
                    nCommand = 0x0E;
                    nParams = 0xB0 | (nParams & 0x0F);
                }
                else if ((nParams & 0x0F) == 0x0F) {
                    /* fine volume slide up */
                    nCommand = 0x0E;
                    nParams = 0xA0 | (nParams >> 4);
                }
                else {
                    nCommand = nParams = 0x00;
                }
                break;

            case 'E':
                if ((nParams & 0xF0) == 0xF0) {
                    /* fine porta down */
                    nCommand = 0x0E;
                    nParams = 0x20 | (nParams & 0x0F);
                }
                else if ((nParams & 0xF0) == 0xE0) {
                    /* extra fine porta down */
                    nCommand = 0x21;
                    nParams = 0x20 | (nParams & 0x0F);
                }
                else {
                    /* portamento down */
                    nCommand = 0x02;
                }
                break;

            case 'F':
                if ((nParams & 0xF0) == 0xF0) {
                    /* fine portamento up */
                    nCommand = 0x0E;
                    nParams = 0x10 | (nParams & 0x0F);
                }
                else if ((nParams & 0xF0) == 0xe0) {
                    /* extra fine portamento up */
                    nCommand = 0x21;
                    nParams = 0x10 | (nParams & 0x0F);
                }
                else {
                    /* portamento up */
                    nCommand = 0x01;
                }
                break;

            case 'G':
                /* tone portamento */
                nCommand = 0x03;
                break;

            case 'H':
                /* vibrato */
                nCommand = 0x04;
                break;

            case 'I':
                /* tremor */
                nCommand = 0x1d;
                break;

            case 'J':
                /* arpeggio */
                nCommand = 0x00;
                break;

            case 'K':
                /* vibrato & volume slide */
                nCommand = 0x06;
                break;

            case 'L':
                /* tone portamento & volume slide */
                nCommand = 0x05;
                break;

            case 'O':
                /* set sample offset */
                nCommand = 0x09;
                break;

            case 'Q':
                /* multi-retrig */
                nCommand = 0x1b;
                break;

            case 'R':
                /* tremolo */
                nCommand = 0x07;
                break;

            case 'S':
                /* misc settings */
                switch (nParams & 0xF0) {
                case 0x00:
                    /* set filter on/off */
                    nCommand = 0x0E;
                    nParams = 0x00 | (nParams & 0x0F);
                    break;

                case 0x10:
                    /* set glissando control */
                    nCommand = 0x0E;
                    nParams = 0x30 | (nParams & 0x0F);
                    break;

                case 0x20:
                    /* set fine-tune */
                    nCommand = 0x0E;
                    nParams = 0x50 | (nParams & 0x0F);
                    break;

                case 0x30:
                    /* set tremolo waveform */
                    nCommand = 0x0E;
                    nParams = 0x70 | (nParams & 0x0F);
                    break;

                case 0x40:
                    /* set vibrato waveform */
                    nCommand = 0x0E;
                    nParams = 0x40 | (nParams & 0x0F);
                    break;

                case 0x80:
                    /* set coarse panning */
                    nCommand = 0x0E;
                    nParams = 0x80 | (nParams & 0x0F);
                    break;

                case 0xA0:
                    /* set stereo control */
                    nCommand = 0x08;
                    switch (nParams & 0x0F) {
                    case 0x00:
                    case 0x02:
                        /* hard left panning */
                        nParams = 0x00;
                        break;

                    case 0x01:
                    case 0x03:
                        /* hard right panning */
                        nParams = 0xFF;
                        break;

                    case 0x04:
                        /* left panning */
                        nParams = 0x40;
                        break;

                    case 0x05:
                        /* right panning */
                        nParams = 0xC0;
                        break;

                    case 0x06:
                    case 0x07:
                        /* middle panning */
                        nParams = 0x80;
                        break;

                    default:
                        nCommand = nParams = 0x00;
                        break;
                    }
                    break;

                case 0xB0:
                    /* set/start pattern loop */
                    nCommand = 0x0E;
                    nParams = 0x60 | (nParams & 0x0F);
                    break;

                case 0xC0:
                    /* note cut */
                    nCommand = 0x0E;
                    nParams = 0xC0 | (nParams & 0x0F);
                    break;

                case 0xD0:
                    /* note delay */
                    nCommand = 0x0E;
                    nParams = 0xD0 | (nParams & 0x0F);
                    break;

                case 0xE0:
                    /* pattern delay */
                    nCommand = 0x0E;
                    nParams = 0xE0 | (nParams & 0x0F);
                    break;

                default:
                    nCommand = nParams = 0x00;
                    break;
                }
                break;

            case 'T':
                /* set BPM speed */
                if (nParams >= 0x20)
                    nCommand = 0x0F;
                else
                    nCommand = nParams = 0x00;
                break;

            case 'U':
                /* fine vibrato */
                /* WARNING: this is not an standard FT2 command! */
                nCommand = 0x1E;
                break;

            case 'V':
                /* set global volume */
                nCommand = 0x10;
                break;

            case 'X':
                /* set DMP-style panning */
                nCommand = 0x08;
                if (nParams > 0x80)
                    nParams = 0x80;
                else if (nParams < 0x80)
                    nParams <<= 1;
                else
                    nParams = 0xFF;
                break;

            case 'Z':
                /* set sync mark */
                /* WARNING: this is not an standard FT2 command! */
                nCommand = 0x23;
                break;

            default:
                /* unknown S3M command value */
                nCommand = nParams = 0x00;
                break;
            }

            /* save note message */
            if (nNote) {
                aTrackTable[nTrack].nNote = nNote;
            }
            if (nSample) {
                aTrackTable[nTrack].nSample = nSample;
            }
            if (nVolume <= 64) {
                aTrackTable[nTrack].nVolume = 0x10 + nVolume;
            }
            if (nCommand | nParams) {
                aTrackTable[nTrack].nCommand = nCommand;
                aTrackTable[nTrack].nParams = nParams;
            }
        }

        /* encode row of notes in our pattern structure */
        for (nTrack = 0; nTrack < nTracks; nTrack++) {
            /* get saved note message */
            nNote = aTrackTable[nTrack].nNote;
            nSample = aTrackTable[nTrack].nSample;
            nVolume = aTrackTable[nTrack].nVolume;
            nCommand = aTrackTable[nTrack].nCommand;
            nParams = aTrackTable[nTrack].nParams;

            /* insert new note message */
            nFlags = AUDIO_PATTERN_PACKED;
            if (nNote)
                nFlags |= AUDIO_PATTERN_NOTE;
            if (nSample)
                nFlags |= AUDIO_PATTERN_SAMPLE;
            if (nVolume)
                nFlags |= AUDIO_PATTERN_VOLUME;
            if (nCommand)
                nFlags |= AUDIO_PATTERN_COMMAND;
            if (nParams)
                nFlags |= AUDIO_PATTERN_PARAMS;
            *lpFTData++ = nFlags;
            if (nNote)
                *lpFTData++ = nNote;
            if (nSample)
                *lpFTData++ = nSample;
            if (nVolume)
                *lpFTData++ = nVolume;
            if (nCommand)
                *lpFTData++ = nCommand;
            if (nParams)
                *lpFTData++ = nParams;
        }
    }
    lpPattern->nSize = (lpFTData - lpPattern->lpData);
    if ((lpPattern->lpData = realloc(lpPattern->lpData,
				     lpPattern->nSize)) == NULL) {
        return AUDIO_ERROR_NOMEMORY;
    }
    return AUDIO_ERROR_NONE;
}

static VOID S3MDecodeSample(LPBYTE lpData, UINT nSize)
{
    /* convert from 8-bit unsigned to 8-bit signed linear */
    while (nSize--) {
        *lpData++ ^= 0x80;
    }
}

UINT AIAPI ALoadModuleS3M(LPSTR lpszFileName, 
			  LPAUDIOMODULE *lplpModule, DWORD dwFileOffset)
{
    static S3MFILEHEADER Header;
    static S3MSAMPLEHEADER Sample;
    static S3MPATTERNHEADER Pattern;
    static WORD aSampleSegPtr[S3M_MAX_SAMPLES];
    static WORD aPatternSegPtr[S3M_MAX_PATTERNS];
    static BYTE aPanningTable[S3M_MAX_TRACKS];
    static BYTE aMappingTable[S3M_MAX_TRACKS];
    LPAUDIOMODULE lpModule;
    LPAUDIOPATTERN lpPattern;
    LPAUDIOPATCH lpPatch;
    LPAUDIOSAMPLE lpSample;
    LONG dwRelativeNote;
    UINT n, nErrorCode;

    if (AIOOpenFile(lpszFileName)) {
        return AUDIO_ERROR_FILENOTFOUND;
    }
    AIOSeekFile(dwFileOffset, SEEK_SET);

    if ((lpModule = (LPAUDIOMODULE) calloc(1, sizeof(AUDIOMODULE))) == NULL) {
        AIOCloseFile();
        return AUDIO_ERROR_NOMEMORY;
    }

    /* load S3M module file header */
    AIOReadFile(Header.aModuleName, sizeof(Header.aModuleName));
    AIOReadChar(&Header.bPadding);
    AIOReadChar(&Header.nFileType);
    AIOReadShort(&Header.wReserved);
    AIOReadShort(&Header.nSongLength);
    AIOReadShort(&Header.nSamples);
    AIOReadShort(&Header.nPatterns);
    AIOReadShort(&Header.wFlags);
    AIOReadShort(&Header.wVersion);
    AIOReadShort(&Header.nSampleType);
    AIOReadLong(&Header.dwSCRM);
    AIOReadChar(&Header.nGlobalVolume);
    AIOReadChar(&Header.nTempo);
    AIOReadChar(&Header.nBPM);
    AIOReadChar(&Header.nMasterVolume);
    AIOReadChar(&Header.nUltraClick);
    AIOReadChar(&Header.nDefaultPanning);
    AIOReadFile(Header.aReserved, sizeof(Header.aReserved));
    AIOReadShort(&Header.wSpecial);
    AIOReadFile(Header.aChannelTable, sizeof(Header.aChannelTable));
    if (Header.dwSCRM != S3M_SCRM_MAGIC ||
        Header.nSongLength > S3M_MAX_ORDERS ||
        Header.nPatterns > S3M_MAX_PATTERNS ||
        Header.nSamples > S3M_MAX_SAMPLES) {
        AFreeModuleFile(lpModule);
        AIOCloseFile();
        return AUDIO_ERROR_BADFILEFORMAT;
    }

    /* load S3M order table and sample/pattern para-pointers */
    AIOReadFile(lpModule->aOrderTable, Header.nSongLength);
    for (n = 0; n < Header.nSamples; n++) {
        AIOReadShort(&aSampleSegPtr[n]);
    }
    for (n = 0; n < Header.nPatterns; n++) {
        AIOReadShort(&aPatternSegPtr[n]);
    }

    /* load S3M panning table if present */
    memset(aPanningTable, 0x00, sizeof(aPanningTable));
    if (Header.nDefaultPanning == 0xFC) {
        AIOReadFile(aPanningTable, sizeof(aPanningTable));
    }

    /* fixup the S3M panning table */
    for (n = 0; n < S3M_MAX_TRACKS; n++) {
        if (aPanningTable[n] & 0x20) {
            aPanningTable[n] = (aPanningTable[n] & 0x0F) << 4;
        }
        else {
            if (Header.aChannelTable[n] <= 7) {
                aPanningTable[n] = 0x00;
            }
            else if (Header.aChannelTable[n] <= 15) {
                aPanningTable[n] = 0xFF;
            }
        }
    }

    /* initialize the module structure */
    strncpy(lpModule->szModuleName, Header.aModuleName,
	    sizeof(lpModule->szModuleName) - 1);
    lpModule->wFlags = AUDIO_MODULE_AMIGA | AUDIO_MODULE_PANNING;
    lpModule->nPatterns = Header.nPatterns;
    lpModule->nPatches = Header.nSamples;
    lpModule->nTempo = Header.nTempo;
    lpModule->nBPM = Header.nBPM;
    for (n = 0; n < Header.nSongLength; n++) {
        if (lpModule->aOrderTable[n] < Header.nPatterns) {
            lpModule->aOrderTable[lpModule->nOrders++] =
                lpModule->aOrderTable[n];
        }
        else {
            lpModule->aOrderTable[n] = 0x00;
        }
    }
    /* lpModule->nRestart = lpModule->nOrders; */
    for (n = 0; n < S3M_MAX_TRACKS; n++) {
        aMappingTable[n] = 0xFF;
        if ((Header.aChannelTable[n] &= 0x7F) <= 15) {
            aMappingTable[n] = (BYTE) lpModule->nTracks;
            lpModule->aPanningTable[lpModule->nTracks++] =
                aPanningTable[n];
        }
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

    /* load S3M pattern sheets */
    lpPattern = lpModule->aPatternTable;
    for (n = 0; n < lpModule->nPatterns; n++, lpPattern++) {
        AIOSeekFile(((LONG) aPatternSegPtr[n] << 4) + dwFileOffset, SEEK_SET);
        AIOReadShort(&Pattern.nSize);
        Pattern.nSize -= sizeof(Pattern.nSize);
        if ((Pattern.lpData = malloc(Pattern.nSize)) == NULL) {
            AFreeModuleFile(lpModule);
            AIOCloseFile();
            return AUDIO_ERROR_NOMEMORY;
        }
        AIOReadFile(Pattern.lpData, Pattern.nSize);
        nErrorCode = S3MDecodePattern(lpModule->nTracks, Pattern.lpData,
				      Pattern.nSize, aMappingTable, lpPattern);
        free(Pattern.lpData);
        if (nErrorCode != AUDIO_ERROR_NONE) {
            AFreeModuleFile(lpModule);
            AIOCloseFile();
            return nErrorCode;
        }
    }

    /* load S3M sample waveforms */
    lpPatch = lpModule->aPatchTable;
    for (n = 0; n < lpModule->nPatches; n++, lpPatch++) {
        /* load S3M sample header structure */
        AIOSeekFile(((LONG) aSampleSegPtr[n] << 4) + dwFileOffset, SEEK_SET);
        AIOReadChar(&Sample.nType);
        AIOReadFile(&Sample.aFileName, sizeof(Sample.aFileName));
        AIOReadShort(&Sample.wDataSegPtr);
        AIOReadLong(&Sample.dwLength);
        AIOReadLong(&Sample.dwLoopStart);
        AIOReadLong(&Sample.dwLoopEnd);
        AIOReadChar(&Sample.nVolume);
        AIOReadChar(&Sample.nReserved);
        AIOReadChar(&Sample.nPacking);
        AIOReadChar(&Sample.bFlags);
        AIOReadLong(&Sample.nSampleRate);
        AIOReadFile(Sample.aReserved, sizeof(Sample.aReserved));
        AIOReadFile(Sample.aSampleName, sizeof(Sample.aSampleName));
        AIOReadLong(&Sample.dwSCRS);
        if (Sample.nType == S3M_SCRS_PCM && Sample.dwSCRS != S3M_SCRS_MAGIC) {
            AFreeModuleFile(lpModule);
            AIOCloseFile();
            return AUDIO_ERROR_BADFILEFORMAT;
        }

        /* initialize patch structure */
        strncpy(lpPatch->szPatchName, Sample.aSampleName,
		sizeof(lpPatch->szPatchName) - 1);
        if (Sample.nType == S3M_SCRS_PCM && Sample.dwLength != 0) {
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
            if (Sample.bFlags & S3M_SCRS_LOOPED)
                lpSample->Wave.wFormat |= AUDIO_FORMAT_LOOP;
            lpSample->Wave.dwLength = Sample.dwLength;
            lpSample->Wave.dwLoopStart = Sample.dwLoopStart;
            lpSample->Wave.dwLoopEnd = Sample.dwLoopEnd;
            lpSample->Wave.nSampleRate = (WORD) Sample.nSampleRate;
            lpSample->nVolume = Sample.nVolume;
            lpSample->nPanning = (AUDIO_MIN_PANNING + AUDIO_MAX_PANNING) / 2;

            /* compute fine tuning for this sample */
            if (Sample.nSampleRate != 0) {
                dwRelativeNote = S3MGetRelativeNote(Sample.nSampleRate);
                lpSample->nRelativeNote = (BYTE) (dwRelativeNote >> 7);
                lpSample->nFinetune = (dwRelativeNote & 0x7F);
            }

            /* allocate sample waveform data */
            nErrorCode = ACreateAudioData(&lpSample->Wave);
            if (nErrorCode != AUDIO_ERROR_NONE) {
                AFreeModuleFile(lpModule);
                AIOCloseFile();
                return nErrorCode;
            }

            /* load sample wavefrom data */
            AIOSeekFile(((LONG) Sample.wDataSegPtr << 4) + dwFileOffset, SEEK_SET);
            AIOReadFile(lpSample->Wave.lpData, lpSample->Wave.dwLength);
            S3MDecodeSample(lpSample->Wave.lpData, lpSample->Wave.dwLength);
            AWriteAudioData(&lpSample->Wave, 0, lpSample->Wave.dwLength);
        }
    }

    AIOCloseFile();
    *lplpModule = lpModule;
    return AUDIO_ERROR_NONE;
}
