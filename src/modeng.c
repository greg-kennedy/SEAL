/*
 * $Id: modeng.c 1.11 1996/09/13 15:10:01 chasan released $
 *
 * Extended module player engine.
 *
 * Copyright (C) 1995-1999 Carlos Hasan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 */

#ifdef __GNUC__
#include <memory.h>
#endif
#include <string.h>
#include <malloc.h>
#include "audio.h"
#include "tables.h"


/*
 * Module player state control bit fields
 */
#define AUDIO_PLAYER_JUMP       0x01
#define AUDIO_PLAYER_BREAK      0x02
#define AUDIO_PLAYER_DELAY      0x04
#define AUDIO_PLAYER_BPM        0x08
#define AUDIO_PLAYER_VOLUME     0x10
#define AUDIO_PLAYER_PAUSE      0x20
#define AUDIO_PLAYER_ACTIVE     0x80


/*
 * Tracks control bit fields
 */
#define AUDIO_CTRL_PITCH        0x01
#define AUDIO_CTRL_VOLUME       0x02
#define AUDIO_CTRL_PANNING      0x04
#define AUDIO_CTRL_KEYON        0x08
#define AUDIO_CTRL_KEYOFF       0x10
#define AUDIO_CTRL_TOUCH        0x20

/*
 * Some useful macro defines
 */
#define LOPARAM(x)              ((BYTE)(x)&0x0F)
#define HIPARAM(x)              ((BYTE)(x)>>4)
#define CLIP(x,a,b)             ((x)<(a)?(a):((x)>(b)?(b):(x)))
#define ABS(x)                  ((x)>0?(x):-(x))


/*
 * Module player track structure
 */
typedef struct {
    BYTE    nNote;              /* note index (1-96) */
    BYTE    nPatch;             /* patch number (1-128) */
    BYTE    nVolume;            /* volume command */
    BYTE    nCommand;           /* effect */
    BYTE    bParams;            /* parameters */
} NOTE, *LPNOTE;

typedef struct {
    BYTE    fKeyOn;             /* key on flag */
    BYTE    bControl;           /* control bits */
    BYTE    nVolumeCmd;         /* volume command */
    BYTE    nCommand;           /* command */
    BYTE    bParams;            /* parameters */
    BYTE    nPatch;             /* patch number */
    BYTE    nSample;            /* sample number */
    BYTE    nNote;              /* current note */
    int     nFinetune;          /* current finetune */
    int     nRelativeNote;      /* relative note */
    int     nVolume;            /* current volume */
    int     nOutVolume;         /* output volume */
    int     nFinalVolume;       /* final volume */
    int     nPanning;           /* current panning */
    int     nFinalPanning;      /* final panning */
    int     nPeriod;            /* current period */
    int     nOutPeriod;         /* output period */
    int     nFinalPeriod;       /* final period */
    LONG    dwFrequency;        /* frequency */

    LPAUDIOPATCH lpPatch;       /* current patch */
    LPAUDIOSAMPLE lpSample;     /* current sample */

    /* waves & gliss control */
    BYTE    bWaveCtrl;          /* vibrato & tremolo control bits */
    BYTE    bGlissCtrl;         /* glissando control bits */

    /* vibrato & tremolo waves */
    int     nVibratoFrame;      /* vibrato frame */
    int     nVibratoDepth;      /* vibrato depth */
    int     nVibratoRate;       /* vibrato rate */

    int     nTremoloFrame;      /* tremolo frame */
    int     nTremoloDepth;      /* tremolo depth */
    int     nTremoloRate;       /* tremolo rate */

    /* command parameters */
    int     nPortaUp;           /* portamento up rate */
    int     nPortaDown;         /* portamento down rate */
    int     nTonePorta;         /* tone portamento rate */
    int     nWantedPeriod;      /* tone portamento target */
    BYTE    bVolumeSlide;       /* volume slide rate */
    BYTE    bPanningSlide;      /* panning slide rate */
    BYTE    nFinePortaUp;       /* fine portamento up rate */
    BYTE    nFinePortaDown;     /* fine portamento down rate */
    BYTE    nExtraPortaUp;      /* extra fine porta up rate */
    BYTE    nExtraPortaDown;    /* extra fine porta down rate */
    BYTE    nRetrigType;        /* multi retrig type */
    BYTE    nRetrigInterval;    /* multi retrig interval */
    BYTE    nRetrigFrame;       /* multi retrig frame */
    BYTE    bTremorParms;       /* tremor parameters */
    BYTE    bTremorOnOff;       /* tremor on/off state */
    BYTE    nTremorFrame;       /* tremor frame */
    LONG    dwSampleOffset;     /* last sample offset */

    /* volume fadeout */
    int     nVolumeFade;        /* volume fadeout level */
    int     nVolumeFadeout;     /* volume fadeout rate */

    /* volume envelope */
    int     nVolumeFrame;       /* volume envelope frame */
    int     nVolumeValue;       /* volume envelope value */
    int     nVolumePoint;       /* volume envelope point index */
    int     nVolumeSlope;       /* volume envelope slope */

    /* panning envelope */
    int     nPanningFrame;      /* panning envelope frame */
    int     nPanningValue;      /* panning envelope value */
    int     nPanningPoint;      /* panning envelope point index */
    int     nPanningSlope;      /* panning envelope slope */

    /* automatic vibrato */
    int     nAutoVibratoFrame;  /* auto vibrato frame */
    int     nAutoVibratoValue;  /* auto vibrato envelope */
    int     nAutoVibratoSlope;  /* auto vibrato slope */

    /* pattern loop variables */
    int     nPatternRow;        /* pattern loop row */
    int     nPatternLoop;       /* pattern loop counter */
} TRACK, *LPTRACK;

/*
 * Module player run-time state structure
 */
static struct {
    LPAUDIOMODULE lpModule;     /* current module */
    LPBYTE  lpData;             /* pattern data pointer */
    TRACK   aTracks[32];        /* array of tracks */
    HAC     aVoices[32];        /* array of voices */
    WORD    wControl;           /* player control bits */
    WORD    wFlags;             /* module control bits */
    int     nTracks;            /* number of channels */
    int     nFrame;             /* current frame */
    int     nRow;               /* pattern row */
    int     nRows;              /* number of rows */
    int     nPattern;           /* pattern number */
    int     nOrder;             /* order number */
    int     nTempo;             /* current tempo */
    int     nBPM;               /* current BPM */
    int     nVolume;            /* global volume */
    int     nVolumeRate;        /* global volume slide */
    int     nJumpOrder;         /* position jump */
    int     nJumpRow;           /* break pattern */
    int     nPatternDelay;      /* pattern delay counter */
    LPFNAUDIOCALLBACK lpfnCallback; /* sync callback routine */
} Player;


static VOID PlayNote(LPTRACK lpTrack);
static VOID StopNote(LPTRACK lpTrack);
static VOID RetrigNote(LPTRACK lpTrack);

/*
 * Low-level extended module player routines
 */
static LONG GetFrequencyValue(int nPeriod)
{
    UINT nNote, nOctave;

    if (nPeriod > 0) {
        if (Player.wFlags & AUDIO_MODULE_LINEAR) {
            nOctave = nPeriod / (12 * 16 * 4);
            nNote = nPeriod % (12 * 16 * 4);
            return aFrequencyTable[nNote] >> nOctave;
        }
        else {
            return (4L * 8363L * 428L) / nPeriod;
        }
    }
    return 0L;
}

static int GetPeriodValue(int nNote, int nRelativeNote, int nFinetune)
{
    int nOctave;

    nNote = ((nNote + nRelativeNote - 1) << 6) + (nFinetune >> 1);
    if (nNote >= 0 && nNote < 10 * 12 * 16 * 4) {
        if (Player.wFlags & AUDIO_MODULE_LINEAR) {
            return (10 * 12 * 16 * 4 - nNote);
        }
        else {
            nOctave = nNote / (12 * 16 * 4);
            nNote = nNote % (12 * 16 * 4);
            return aPeriodTable[nNote >> 2] >> nOctave;
        }
    }
    return 0;
}

static VOID OnArpeggio(LPTRACK lpTrack)
{
    int nNote;

    if (lpTrack->bParams) {
        nNote = lpTrack->nNote;
        switch (Player.nFrame % 3) {
        case 1:
            nNote += HIPARAM(lpTrack->bParams);
            break;
        case 2:
            nNote += LOPARAM(lpTrack->bParams);
            break;
        }
        lpTrack->nOutPeriod = GetPeriodValue(nNote,
					     lpTrack->nRelativeNote, lpTrack->nFinetune);
        lpTrack->bControl |= AUDIO_CTRL_PITCH;
    }
}

static VOID OnPortaUp(LPTRACK lpTrack)
{
    if (!Player.nFrame) {
        if (lpTrack->bParams != 0x00) {
            lpTrack->nPortaUp = (int) lpTrack->bParams << 2;
        }
    }
    else {
        lpTrack->nPeriod -= lpTrack->nPortaUp;
        if (lpTrack->nPeriod < AUDIO_MIN_PERIOD)
            lpTrack->nPeriod = AUDIO_MIN_PERIOD;
        lpTrack->nOutPeriod = lpTrack->nPeriod;
        lpTrack->bControl |= AUDIO_CTRL_PITCH;
    }
}

static VOID OnPortaDown(LPTRACK lpTrack)
{
    if (!Player.nFrame) {
        if (lpTrack->bParams != 0x00) {
            lpTrack->nPortaDown = (int) lpTrack->bParams << 2;
        }
    }
    else {
        lpTrack->nPeriod += lpTrack->nPortaDown;
        if (lpTrack->nPeriod > AUDIO_MAX_PERIOD)
            lpTrack->nPeriod = AUDIO_MAX_PERIOD;
        lpTrack->nOutPeriod = lpTrack->nPeriod;
        lpTrack->bControl |= AUDIO_CTRL_PITCH;
    }
}

static VOID OnTonePorta(LPTRACK lpTrack)
{
    if (!Player.nFrame) {
        if (lpTrack->bParams != 0x00) {
            lpTrack->nTonePorta = (int) lpTrack->bParams << 2;
        }
        lpTrack->nWantedPeriod = GetPeriodValue(lpTrack->nNote,
						lpTrack->nRelativeNote, lpTrack->nFinetune);
        lpTrack->bControl &= ~(AUDIO_CTRL_PITCH | AUDIO_CTRL_KEYON);
    }
    else {
        if (lpTrack->nPeriod < lpTrack->nWantedPeriod) {
            lpTrack->nPeriod += lpTrack->nTonePorta;
            if (lpTrack->nPeriod > lpTrack->nWantedPeriod) {
                lpTrack->nPeriod = lpTrack->nWantedPeriod;
            }
        }
        else if (lpTrack->nPeriod > lpTrack->nWantedPeriod) {
            lpTrack->nPeriod -= lpTrack->nTonePorta;
            if (lpTrack->nPeriod < lpTrack->nWantedPeriod) {
                lpTrack->nPeriod = lpTrack->nWantedPeriod;
            }
        }
        /* TODO: glissando not implemented */
        lpTrack->nOutPeriod = lpTrack->nPeriod;
        lpTrack->bControl |= AUDIO_CTRL_PITCH;
    }
}

static VOID OnVibrato(LPTRACK lpTrack)
{
    int nDelta, nFrame;

    if (!Player.nFrame) {
        if (LOPARAM(lpTrack->bParams)) {
            lpTrack->nVibratoDepth = LOPARAM(lpTrack->bParams);
        }
        if (HIPARAM(lpTrack->bParams)) {
            lpTrack->nVibratoRate = HIPARAM(lpTrack->bParams) << 2;
        }
    }
    else {
        nFrame = (lpTrack->nVibratoFrame >> 2) & 0x1F;
        switch (lpTrack->bWaveCtrl & 0x03) {
        case 0x00:
            nDelta = aSineTable[nFrame];
            break;
        case 0x01:
            nDelta = nFrame << 3;
            if (lpTrack->nVibratoFrame & 0x80)
                nDelta ^= 0xFF;
            break;
        case 0x02:
            nDelta = 0xFF;
            break;
        default:
            /* TODO: random waveform not implemented */
            nDelta = 0x00;
            break;
        }
        nDelta = ((nDelta * lpTrack->nVibratoDepth) >> 5);
        lpTrack->nOutPeriod = lpTrack->nPeriod;
        if (lpTrack->nVibratoFrame & 0x80) {
            lpTrack->nOutPeriod -= nDelta;
            if (lpTrack->nOutPeriod < AUDIO_MIN_PERIOD)
                lpTrack->nOutPeriod = AUDIO_MIN_PERIOD;
        }
        else {
            lpTrack->nOutPeriod += nDelta;
            if (lpTrack->nOutPeriod > AUDIO_MAX_PERIOD)
                lpTrack->nOutPeriod = AUDIO_MAX_PERIOD;
        }
        lpTrack->bControl |= AUDIO_CTRL_PITCH;
        lpTrack->nVibratoFrame += lpTrack->nVibratoRate;
    }
}

static VOID OnFineVibrato(LPTRACK lpTrack)
{
    int nDelta, nFrame;

    if (!Player.nFrame) {
        if (LOPARAM(lpTrack->bParams)) {
            lpTrack->nVibratoDepth = LOPARAM(lpTrack->bParams);
        }
        if (HIPARAM(lpTrack->bParams)) {
            lpTrack->nVibratoRate = HIPARAM(lpTrack->bParams) << 2;
        }
    }
    else {
        nFrame = (lpTrack->nVibratoFrame >> 2) & 0x1F;
        switch (lpTrack->bWaveCtrl & 0x03) {
        case 0x00:
            nDelta = aSineTable[nFrame];
            break;
        case 0x01:
            nDelta = nFrame << 3;
            if (lpTrack->nVibratoFrame & 0x80)
                nDelta ^= 0xFF;
            break;
        case 0x02:
            nDelta = 0xFF;
            break;
        default:
            /* TODO: random waveform not implemented */
            nDelta = 0x00;
            break;
        }
        nDelta = ((nDelta * lpTrack->nVibratoDepth) >> 7);
        lpTrack->nOutPeriod = lpTrack->nPeriod;
        if (lpTrack->nVibratoFrame & 0x80) {
            lpTrack->nOutPeriod -= nDelta;
            if (lpTrack->nOutPeriod < AUDIO_MIN_PERIOD)
                lpTrack->nOutPeriod = AUDIO_MIN_PERIOD;
        }
        else {
            lpTrack->nOutPeriod += nDelta;
            if (lpTrack->nOutPeriod > AUDIO_MAX_PERIOD)
                lpTrack->nOutPeriod = AUDIO_MAX_PERIOD;
        }
        lpTrack->bControl |= AUDIO_CTRL_PITCH;
        lpTrack->nVibratoFrame += lpTrack->nVibratoRate;
    }
}

static VOID OnVolumeSlide(LPTRACK lpTrack)
{
    if (!Player.nFrame) {
        if (lpTrack->bParams != 0x00) {
            lpTrack->bVolumeSlide = lpTrack->bParams;
        }
    }
    else {
        if (HIPARAM(lpTrack->bVolumeSlide)) {
            lpTrack->nVolume += HIPARAM(lpTrack->bVolumeSlide);
            if (lpTrack->nVolume > AUDIO_MAX_VOLUME)
                lpTrack->nVolume = AUDIO_MAX_VOLUME;
        }
        else {
            lpTrack->nVolume -= LOPARAM(lpTrack->bVolumeSlide);
            if (lpTrack->nVolume < AUDIO_MIN_VOLUME)
                lpTrack->nVolume = AUDIO_MIN_VOLUME;
        }
        lpTrack->nOutVolume = lpTrack->nVolume;
        lpTrack->bControl |= AUDIO_CTRL_VOLUME;
    }
}

static VOID OnToneAndSlide(LPTRACK lpTrack)
{
    if (!Player.nFrame) {
        OnVolumeSlide(lpTrack);
        lpTrack->bControl &= ~(AUDIO_CTRL_PITCH | AUDIO_CTRL_KEYON);
    }
    else {
        OnTonePorta(lpTrack);
        OnVolumeSlide(lpTrack);
    }
}

static VOID OnVibratoAndSlide(LPTRACK lpTrack)
{
    if (!Player.nFrame) {
        OnVolumeSlide(lpTrack);
    }
    else {
        OnVibrato(lpTrack);
        OnVolumeSlide(lpTrack);
    }
}

static VOID OnTremolo(LPTRACK lpTrack)
{
    int nDelta, nFrame;

    if (!Player.nFrame) {
        if (LOPARAM(lpTrack->bParams)) {
            lpTrack->nTremoloDepth = LOPARAM(lpTrack->bParams);
        }
        if (HIPARAM(lpTrack->bParams)) {
            lpTrack->nTremoloRate = HIPARAM(lpTrack->bParams) << 2;
        }
    }
    else {
        nFrame = (lpTrack->nTremoloFrame >> 2) & 0x1F;
        switch (lpTrack->bWaveCtrl & 0x30) {
        case 0x00:
            nDelta = aSineTable[nFrame];
            break;
        case 0x10:
            nDelta = nFrame << 3;
            if (lpTrack->nTremoloFrame & 0x80)
                nDelta ^= 0xFF;
            break;
        case 0x20:
            nDelta = 0xFF;
            break;
        default:
            /* TODO: random waveform not implemented */
            nDelta = 0x00;
            break;
        }
        nDelta = (nDelta * lpTrack->nTremoloDepth) >> 6;
        lpTrack->nOutVolume = lpTrack->nVolume;
        if (lpTrack->nTremoloFrame & 0x80) {
            lpTrack->nOutVolume -= nDelta;
            if (lpTrack->nOutVolume < AUDIO_MIN_VOLUME)
                lpTrack->nOutVolume = AUDIO_MIN_VOLUME;
        }
        else {
            lpTrack->nOutVolume += nDelta;
            if (lpTrack->nOutVolume > AUDIO_MAX_VOLUME)
                lpTrack->nOutVolume = AUDIO_MAX_VOLUME;
        }
        lpTrack->bControl |= AUDIO_CTRL_VOLUME;
        lpTrack->nTremoloFrame += lpTrack->nTremoloRate;
    }
}

static VOID OnSetPanning(LPTRACK lpTrack)
{
    if (!Player.nFrame) {
        lpTrack->nPanning = lpTrack->bParams;
        lpTrack->bControl |= AUDIO_CTRL_PANNING;
    }
}

static VOID OnSampleOffset(LPTRACK lpTrack)
{
    if (!Player.nFrame) {
        lpTrack->dwSampleOffset = (LONG) lpTrack->bParams << 8;
        lpTrack->bControl |= AUDIO_CTRL_KEYON | AUDIO_CTRL_TOUCH;
    }
}

static VOID OnJumpPosition(LPTRACK lpTrack)
{
    if (!Player.nFrame) {
        if (lpTrack->bParams < Player.lpModule->nOrders) {
            Player.nJumpOrder = lpTrack->bParams;
            Player.wControl |= AUDIO_PLAYER_JUMP;
        }
    }
}

static VOID OnSetVolume(LPTRACK lpTrack)
{
    if (!Player.nFrame) {
        if ((lpTrack->nVolume = lpTrack->bParams) > AUDIO_MAX_VOLUME)
            lpTrack->nVolume = AUDIO_MAX_VOLUME;
        lpTrack->nOutVolume = lpTrack->nVolume;
        lpTrack->bControl |= AUDIO_CTRL_VOLUME;
    }
}

static VOID OnPatternBreak(LPTRACK lpTrack)
{
    if (!Player.nFrame) {
        Player.nJumpRow = 10 * HIPARAM(lpTrack->bParams) +
            LOPARAM(lpTrack->bParams);
        Player.wControl |= AUDIO_PLAYER_BREAK;
    }
}

static VOID OnExtraCommand(LPTRACK lpTrack)
{
    switch (HIPARAM(lpTrack->bParams)) {
    case 0x0:
        /* TODO: filter control not implemented */
        break;
    case 0x1:
        /* fine portamento up */
        if (!Player.nFrame) {
            if (LOPARAM(lpTrack->bParams))
                lpTrack->nFinePortaUp = LOPARAM(lpTrack->bParams) << 2;
            lpTrack->nPeriod -= lpTrack->nFinePortaUp;
            if (lpTrack->nPeriod < AUDIO_MIN_PERIOD)
                lpTrack->nPeriod = AUDIO_MIN_PERIOD;
            lpTrack->nOutPeriod = lpTrack->nPeriod;
            lpTrack->bControl |= AUDIO_CTRL_PITCH;
        }
        break;
    case 0x2:
        /* fine portamento down */
        if (!Player.nFrame) {
            if (LOPARAM(lpTrack->bParams))
                lpTrack->nFinePortaDown = LOPARAM(lpTrack->bParams) << 2;
            lpTrack->nPeriod += lpTrack->nFinePortaDown;
            if (lpTrack->nPeriod > AUDIO_MAX_PERIOD)
                lpTrack->nPeriod = AUDIO_MAX_PERIOD;
            lpTrack->nOutPeriod = lpTrack->nPeriod;
            lpTrack->bControl |= AUDIO_CTRL_PITCH;
        }
        break;
    case 0x3:
        /* set glissando control */
        if (!Player.nFrame) {
            lpTrack->bGlissCtrl = LOPARAM(lpTrack->bParams);
        }
        break;
    case 0x4:
        /* set vibrato wave control */
        if (!Player.nFrame) {
            lpTrack->bWaveCtrl &= 0xF0;
            lpTrack->bWaveCtrl |= LOPARAM(lpTrack->bParams);
        }
        break;
    case 0x5:
        /* set finetune */
        if (!Player.nFrame) {
            lpTrack->nFinetune = ((int) LOPARAM(lpTrack->bParams) << 4) - 0x80;
            lpTrack->nOutPeriod = GetPeriodValue(lpTrack->nNote,
						 lpTrack->nRelativeNote, lpTrack->nFinetune);
            lpTrack->bControl |= AUDIO_CTRL_PITCH;
        }
        break;
    case 0x6:
        /* set/start pattern loop */
        if (!Player.nFrame) {
            if (LOPARAM(lpTrack->bParams)) {
                if (lpTrack->nPatternLoop)
                    lpTrack->nPatternLoop--;
                else
                    lpTrack->nPatternLoop = LOPARAM(lpTrack->bParams);
                if (lpTrack->nPatternLoop) {
                    Player.nJumpRow = lpTrack->nPatternRow;
                    Player.nJumpOrder = Player.nOrder;
                    Player.wControl |= AUDIO_PLAYER_JUMP;
                }
            }
            else {
                lpTrack->nPatternRow = Player.nRow;
            }
        }
        break;
    case 0x7:
        /* set tremolo wave control */
        if (!Player.nFrame) {
            lpTrack->bWaveCtrl &= 0x0F;
            lpTrack->bWaveCtrl |= LOPARAM(lpTrack->bParams) << 4;
        }
        break;
    case 0x8:
        /* set stereo panning control */
        if (!Player.nFrame) {
            lpTrack->nPanning = LOPARAM(lpTrack->bParams) << 4;
            lpTrack->bControl |= AUDIO_CTRL_PANNING;
        }
        break;
    case 0x9:
        /* retrig note */
        if (!Player.nFrame) {
            RetrigNote(lpTrack);
        }
        else if (LOPARAM(lpTrack->bParams)) {
            if (!(Player.nFrame % LOPARAM(lpTrack->bParams))) {
                RetrigNote(lpTrack);
            }
        }
        break;
    case 0xA:
        /* fine volume slide up */
        if (!Player.nFrame) {
            lpTrack->nVolume += LOPARAM(lpTrack->bParams);
            if (lpTrack->nVolume > AUDIO_MAX_VOLUME)
                lpTrack->nVolume = AUDIO_MAX_VOLUME;
            lpTrack->nOutVolume = lpTrack->nVolume;
            lpTrack->bControl |= AUDIO_CTRL_VOLUME;
        }
        break;
    case 0xB:
        /* fine volume slide down */
        if (!Player.nFrame) {
            lpTrack->nVolume -= LOPARAM(lpTrack->bParams);
            if (lpTrack->nVolume < AUDIO_MIN_VOLUME)
                lpTrack->nVolume = AUDIO_MIN_VOLUME;
            lpTrack->nOutVolume = lpTrack->nVolume;
            lpTrack->bControl |= AUDIO_CTRL_VOLUME;
        }
        break;
    case 0xC:
        /* note cut */
        if (Player.nFrame == LOPARAM(lpTrack->bParams)) {
            lpTrack->nVolume = lpTrack->nOutVolume = 0;
            lpTrack->bControl |= AUDIO_CTRL_VOLUME;
        }
        break;
    case 0xD:
        /* note delay */
        lpTrack->bControl &= AUDIO_CTRL_KEYOFF;
        if (Player.nFrame == LOPARAM(lpTrack->bParams)) {
            RetrigNote(lpTrack);
            lpTrack->bControl |= (AUDIO_CTRL_VOLUME | AUDIO_CTRL_PANNING);
        }
        break;
    case 0xE:
        /* pattern delay */
        if (!Player.nFrame) {
            if (!(Player.wControl & AUDIO_PLAYER_DELAY)) {
                Player.nPatternDelay = LOPARAM(lpTrack->bParams) + 1;
                Player.wControl |= AUDIO_PLAYER_DELAY;
            }
        }
        break;
    case 0xF:
        /* TODO: funk it! not implemented */
        break;
    }
}

static VOID OnSetSpeed(LPTRACK lpTrack)
{
    if (!Player.nFrame) {
        if (lpTrack->bParams >= 0x20) {
            Player.nBPM = lpTrack->bParams;
            Player.wControl |= AUDIO_PLAYER_BPM;
        }
        else if (lpTrack->bParams >= 0x01) {
            Player.nTempo = lpTrack->bParams;
        }
    }
}

static VOID OnSetGlobalVolume(LPTRACK lpTrack)
{
    if (!Player.nFrame) {
        if ((Player.nVolume = lpTrack->bParams) > AUDIO_MAX_VOLUME)
            Player.nVolume = AUDIO_MAX_VOLUME;
        Player.wControl |= AUDIO_PLAYER_VOLUME;
    }
}

static VOID OnGlobalVolumeSlide(LPTRACK lpTrack)
{
    if (!Player.nFrame) {
        if (lpTrack->bParams != 0x00) {
            Player.nVolumeRate = lpTrack->bParams;
        }
    }
    else {
        if (HIPARAM(Player.nVolumeRate)) {
            Player.nVolume += HIPARAM(Player.nVolumeRate);
            if (Player.nVolume > AUDIO_MAX_VOLUME)
                Player.nVolume = AUDIO_MAX_VOLUME;
        }
        else {
            Player.nVolume -= LOPARAM(Player.nVolumeRate);
            if (Player.nVolume < AUDIO_MIN_VOLUME)
                Player.nVolume = AUDIO_MIN_VOLUME;
        }
        Player.wControl |= AUDIO_PLAYER_VOLUME;
    }
}

static VOID OnKeyOff(LPTRACK lpTrack)
{
    if (Player.nFrame == lpTrack->bParams) {
        StopNote(lpTrack);
    }
}

static VOID OnSetEnvelope(LPTRACK lpTrack)
{
    /* TODO: set envelope position not implemented */
    if (lpTrack != NULL) {
    }
}

static VOID OnPanningSlide(LPTRACK lpTrack)
{
    if (!Player.nFrame) {
        if (lpTrack->bParams != 0x00) {
            lpTrack->bPanningSlide = lpTrack->bParams;
        }
    }
    else {
        if (HIPARAM(lpTrack->bPanningSlide)) {
            lpTrack->nPanning += HIPARAM(lpTrack->bPanningSlide);
            if (lpTrack->nPanning > AUDIO_MAX_PANNING)
                lpTrack->nPanning = AUDIO_MAX_PANNING;
        }
        else {
            lpTrack->nPanning -= LOPARAM(lpTrack->bPanningSlide);
            if (lpTrack->nPanning < AUDIO_MIN_PANNING)
                lpTrack->nPanning = AUDIO_MIN_PANNING;
        }
        lpTrack->bControl |= AUDIO_CTRL_PANNING;
    }
}

static VOID OnMultiRetrig(LPTRACK lpTrack)
{
    if (!Player.nFrame) {
        if (HIPARAM(lpTrack->bParams))
            lpTrack->nRetrigType = HIPARAM(lpTrack->bParams);
        if (LOPARAM(lpTrack->bParams))
            lpTrack->nRetrigInterval = LOPARAM(lpTrack->bParams);
    }
    else if (++lpTrack->nRetrigFrame >= lpTrack->nRetrigInterval) {
        lpTrack->nRetrigFrame = 0;
        lpTrack->nVolume += aRetrigTable[lpTrack->nRetrigType];
        lpTrack->nVolume *= aRetrigTable[lpTrack->nRetrigType + 16];
        lpTrack->nVolume >>= 4;
        lpTrack->nVolume = CLIP(lpTrack->nVolume,
				AUDIO_MIN_VOLUME, AUDIO_MAX_VOLUME);
        if (lpTrack->nVolumeCmd >= 0x10 && lpTrack->nVolumeCmd <= 0x50) {
            lpTrack->nVolume = lpTrack->nVolumeCmd - 0x10;
            lpTrack->bControl |= AUDIO_CTRL_VOLUME;
        }
        if (lpTrack->nVolumeCmd >= 0xC0 && lpTrack->nVolumeCmd <= 0xCF) {
            lpTrack->nPanning = LOPARAM(lpTrack->nVolumeCmd) << 4;
            lpTrack->bControl |= AUDIO_CTRL_PANNING;
        }
        lpTrack->nOutVolume = lpTrack->nVolume;
        lpTrack->bControl |= AUDIO_CTRL_VOLUME;
        PlayNote(lpTrack);
    }
}

static VOID OnTremor(LPTRACK lpTrack)
{
    if (!Player.nFrame) {
        if (lpTrack->bParams) {
            lpTrack->bTremorParms = lpTrack->bParams;
        }
    }
    if (!lpTrack->nTremorFrame--) {
        lpTrack->nTremorFrame = lpTrack->bTremorOnOff ?
            LOPARAM(lpTrack->bTremorParms) :
            HIPARAM(lpTrack->bTremorParms);
        lpTrack->bTremorOnOff = !lpTrack->bTremorOnOff;
    }
    lpTrack->nOutVolume = lpTrack->bTremorOnOff ? lpTrack->nVolume : 0x00;
    lpTrack->bControl |= AUDIO_CTRL_VOLUME;
}

static VOID OnExtraFinePorta(LPTRACK lpTrack)
{
    if (!Player.nFrame) {
        if (HIPARAM(lpTrack->bParams) == 0x1) {
            if (LOPARAM(lpTrack->bParams))
                lpTrack->nExtraPortaUp = LOPARAM(lpTrack->bParams);
            lpTrack->nPeriod -= lpTrack->nExtraPortaUp;
            if (lpTrack->nPeriod < AUDIO_MIN_PERIOD)
                lpTrack->nPeriod = AUDIO_MIN_PERIOD;
            lpTrack->nOutPeriod = lpTrack->nPeriod;
            lpTrack->bControl |= AUDIO_CTRL_PITCH;
        }
        else if (HIPARAM(lpTrack->bParams) == 0x2) {
            if (LOPARAM(lpTrack->bParams))
                lpTrack->nExtraPortaDown = LOPARAM(lpTrack->bParams);
            lpTrack->nPeriod += lpTrack->nExtraPortaDown;
            if (lpTrack->nPeriod > AUDIO_MAX_PERIOD)
                lpTrack->nPeriod = AUDIO_MAX_PERIOD;
            lpTrack->nOutPeriod = lpTrack->nPeriod;
            lpTrack->bControl |= AUDIO_CTRL_PITCH;
        }
    }
}

static VOID OnSyncMark(LPTRACK lpTrack)
{
    if (!Player.nFrame && Player.lpfnCallback)
        Player.lpfnCallback(lpTrack->bParams, Player.nOrder, Player.nRow);
}

static VOID OnNothing(LPTRACK lpTrack)
{
    /* does nothing, just avoid compiler warnings */
    if (lpTrack != NULL) {
    }
}

static VOID ExecNoteCmd(LPTRACK lpTrack)
{
    static VOID (*CommandProcTable[36])(LPTRACK) =
    {
        OnArpeggio,             /* 0xy */
        OnPortaUp,              /* 1xx */
        OnPortaDown,            /* 2xx */
        OnTonePorta,            /* 3xx */
        OnVibrato,              /* 4xy */
        OnToneAndSlide,         /* 5xy */
        OnVibratoAndSlide,      /* 6xy */
        OnTremolo,              /* 7xy */
        OnSetPanning,           /* 8xx */
        OnSampleOffset,         /* 9xx */
        OnVolumeSlide,          /* Axy */
        OnJumpPosition,         /* Bxx */
        OnSetVolume,            /* Cxx */
        OnPatternBreak,         /* Dxx */
        OnExtraCommand,         /* Exy */
        OnSetSpeed,             /* Fxx */
        OnSetGlobalVolume,      /* Gxx */
        OnGlobalVolumeSlide,    /* Hxy */
        OnNothing,              /* Ixx */
        OnNothing,              /* Jxx */
        OnKeyOff,               /* Kxx */
        OnSetEnvelope,          /* Lxx */
        OnNothing,              /* Mxx */
        OnNothing,              /* Nxx */
        OnNothing,              /* Oxx */
        OnPanningSlide,         /* Pxy */
        OnNothing,              /* Qxx */
        OnMultiRetrig,          /* Rxy */
        OnNothing,              /* Sxx */
        OnTremor,               /* Txy */
        OnFineVibrato,          /* Uxy */
        OnNothing,              /* Vxx */
        OnNothing,              /* Wxx */
        OnExtraFinePorta,       /* Xxy */
        OnNothing,              /* Yxx */
        OnSyncMark              /* Zxx */
    };

    if ((lpTrack->nCommand || lpTrack->bParams) && lpTrack->nCommand < 36) {
        (*CommandProcTable[lpTrack->nCommand]) (lpTrack);
    }
}

static VOID ExecVolumeCmd(LPTRACK lpTrack)
{
    UINT nCommand;

    nCommand = lpTrack->nVolumeCmd;
    if (nCommand >= 0x10 && nCommand <= 0x50) {
        /* set volume */
        if (!Player.nFrame) {
            lpTrack->nVolume = nCommand - 0x10;
            lpTrack->nOutVolume = lpTrack->nVolume;
            lpTrack->bControl |= AUDIO_CTRL_VOLUME;
        }
    }
    else if (nCommand >= 0x60 && nCommand <= 0x6F) {
        /* volume slide down */
        if (Player.nFrame) {
            lpTrack->nVolume -= LOPARAM(nCommand);
            if (lpTrack->nVolume < AUDIO_MIN_VOLUME)
                lpTrack->nVolume = AUDIO_MIN_VOLUME;
            lpTrack->nOutVolume = lpTrack->nVolume;
            lpTrack->bControl |= AUDIO_CTRL_VOLUME;
        }
    }
    else if (nCommand >= 0x70 && nCommand <= 0x7F) {
        /* volume slide up */
        if (Player.nFrame) {
            lpTrack->nVolume += LOPARAM(nCommand);
            if (lpTrack->nVolume > AUDIO_MAX_VOLUME)
                lpTrack->nVolume = AUDIO_MAX_VOLUME;
            lpTrack->nOutVolume = lpTrack->nVolume;
            lpTrack->bControl |= AUDIO_CTRL_VOLUME;
        }
    }
    else if (nCommand >= 0x80 && nCommand <= 0x8F) {
        /* fine volume slide down */
        if (!Player.nFrame) {
            lpTrack->nVolume -= LOPARAM(nCommand);
            if (lpTrack->nVolume < AUDIO_MIN_VOLUME)
                lpTrack->nVolume = AUDIO_MIN_VOLUME;
            lpTrack->nOutVolume = lpTrack->nVolume;
            lpTrack->bControl |= AUDIO_CTRL_VOLUME;
        }
    }
    else if (nCommand >= 0x90 && nCommand <= 0x9F) {
        /* fine volume slide up */
        if (!Player.nFrame) {
            lpTrack->nVolume += LOPARAM(nCommand);
            if (lpTrack->nVolume > AUDIO_MAX_VOLUME)
                lpTrack->nVolume = AUDIO_MAX_VOLUME;
            lpTrack->nOutVolume = lpTrack->nVolume;
            lpTrack->bControl |= AUDIO_CTRL_VOLUME;
        }
    }
    else if (nCommand >= 0xA0 && nCommand <= 0xAF) {
        /* set vibrato speed */
        if (!Player.nFrame) {
            if (LOPARAM(nCommand)) {
                lpTrack->nVibratoRate = LOPARAM(nCommand) << 2;
            }
        }
    }
    else if (nCommand >= 0xB0 && nCommand <= 0xBF) {
        /* vibrato */
        if (!Player.nFrame) {
            if (LOPARAM(nCommand)) {
                lpTrack->nVibratoDepth = LOPARAM(nCommand);
            }
        }
        else {
            OnVibrato(lpTrack);
        }
    }
    else if (nCommand >= 0xC0 && nCommand <= 0xCF) {
        /* set coarse panning */
        if (!Player.nFrame) {
            lpTrack->nPanning = LOPARAM(nCommand) << 4;
            lpTrack->bControl |= AUDIO_CTRL_PANNING;
        }
    }
    else if (nCommand >= 0xD0 && nCommand <= 0xDF) {
        /* panning slide left */
        if (Player.nFrame) {
            lpTrack->nPanning -= LOPARAM(nCommand);
            if (lpTrack->nPanning < AUDIO_MIN_PANNING)
                lpTrack->nPanning = AUDIO_MIN_PANNING;
            lpTrack->bControl |= AUDIO_CTRL_PANNING;
        }
    }
    else if (nCommand >= 0xE0 && nCommand <= 0xEF) {
        /* panning slide right */
        if (Player.nFrame) {
            lpTrack->nPanning += LOPARAM(nCommand);
            if (lpTrack->nPanning > AUDIO_MAX_PANNING)
                lpTrack->nPanning = AUDIO_MAX_PANNING;
            lpTrack->bControl |= AUDIO_CTRL_PANNING;
        }
    }
    else if (nCommand >= 0xF0 && nCommand <= 0xFF) {
        /* tone portamento */
        if (Player.nFrame)
            OnTonePorta(lpTrack);
    }
}

static VOID StartEnvelopes(LPTRACK lpTrack)
{
    LPAUDIOPATCH lpPatch;

    /* reset vibrato and tremolo waves */
    if (!(lpTrack->bWaveCtrl & 0x04))
        lpTrack->nVibratoFrame = 0;
    if (!(lpTrack->bWaveCtrl & 0x40))
        lpTrack->nTremoloFrame = 0;

    /* reset retrig and tremor frames */
    lpTrack->nRetrigFrame = 0;
    lpTrack->nTremorFrame = 0;
    lpTrack->bTremorOnOff = 0;

    lpPatch = lpTrack->lpPatch;

    /* start volume envelope */
    if (lpPatch != NULL && (lpPatch->Volume.wFlags & AUDIO_ENVELOPE_ON)) {
        lpTrack->nVolumeFrame = -1;
        lpTrack->nVolumePoint = 0;
    }

    /* start panning envelope */
    if (lpPatch != NULL && (lpPatch->Panning.wFlags & AUDIO_ENVELOPE_ON)) {
        lpTrack->nPanningFrame = -1;
        lpTrack->nPanningPoint = 0;
    }

    /* start volume fadeout */
    if (lpPatch != NULL)
        lpTrack->nVolumeFadeout = lpPatch->nVolumeFadeout;
    lpTrack->nVolumeFade = 0x7FFF;

    /* start automatic vibrato */
    if (lpPatch != NULL && lpPatch->nVibratoDepth) {
        lpTrack->nAutoVibratoFrame = 0;
        if (lpPatch->nVibratoSweep) {
            lpTrack->nAutoVibratoSlope =
                ((int) lpPatch->nVibratoDepth << 8) / lpPatch->nVibratoSweep;
            lpTrack->nAutoVibratoValue = 0;
        }
        else {
            lpTrack->nAutoVibratoSlope = 0;
            lpTrack->nAutoVibratoValue =
                ((int) lpPatch->nVibratoDepth << 8);
        }
    }
}

static VOID UpdateEnvelopes(LPTRACK lpTrack)
{
    LPAUDIOPATCH lpPatch;
    LPAUDIOPOINT lpPts;
    int nFrames, nValue;

    /* get patch structure alias */
    lpPatch = lpTrack->lpPatch;

    /* process volume fadeout */
    if (lpPatch != NULL && !lpTrack->fKeyOn) {
        if ((lpTrack->nVolumeFade -= lpTrack->nVolumeFadeout) < 0) {
            lpTrack->nVolumeFadeout = 0;
            lpTrack->nVolumeFade = 0;
        }
        lpTrack->bControl |= AUDIO_CTRL_VOLUME;
    }

    /* process volume envelope */
    if (lpPatch != NULL && (lpPatch->Volume.wFlags & AUDIO_ENVELOPE_ON)) {
        lpPts = lpPatch->Volume.aEnvelope;
        if (++lpTrack->nVolumeFrame >= lpPts[lpTrack->nVolumePoint].nFrame) {
            if ((lpPatch->Volume.wFlags & AUDIO_ENVELOPE_SUSTAIN) &&
                (lpTrack->nVolumePoint == lpPatch->Volume.nSustain) &&
                lpTrack->fKeyOn) {
                lpTrack->nVolumeFrame = lpPts[lpTrack->nVolumePoint].nFrame;
                lpTrack->nVolumeValue = (int) lpPts[lpTrack->nVolumePoint].nValue << 8;
            }
            else {
                if ((lpPatch->Volume.wFlags & AUDIO_ENVELOPE_LOOP) &&
                    (lpTrack->nVolumePoint == lpPatch->Volume.nLoopEnd)) {
                    lpTrack->nVolumePoint = lpPatch->Volume.nLoopStart;
                }
                lpTrack->nVolumeFrame = lpPts[lpTrack->nVolumePoint].nFrame;
                lpTrack->nVolumeValue = (int) lpPts[lpTrack->nVolumePoint].nValue << 8;
                if (lpTrack->nVolumePoint + 1 >= lpPatch->Volume.nPoints) {
                    lpTrack->nVolumeSlope = 0;
                }
                else {
                    if ((nFrames = lpPts[lpTrack->nVolumePoint + 1].nFrame -
			 lpPts[lpTrack->nVolumePoint].nFrame) <= 0)
                        lpTrack->nVolumeSlope = 0;
                    else {
                        lpTrack->nVolumeSlope =
                            (((int) lpPts[lpTrack->nVolumePoint + 1].nValue -
			      (int) lpPts[lpTrack->nVolumePoint].nValue) << 8) / nFrames;
                    }
                    lpTrack->nVolumePoint++;
                }
            }
        }
        else {
            lpTrack->nVolumeValue += lpTrack->nVolumeSlope;
            lpTrack->nVolumeValue = CLIP(lpTrack->nVolumeValue, 0, 64 * 256);
        }
        lpTrack->nFinalVolume = (((LONG) (lpTrack->nVolumeValue >> 8) *
				  lpTrack->nOutVolume) * lpTrack->nVolumeFade) >> 21;
        lpTrack->bControl |= AUDIO_CTRL_VOLUME;
    }
    else {
        lpTrack->nFinalVolume = lpTrack->nOutVolume;
        if (lpTrack->nVolumeFade != 0x7FFF)
            lpTrack->nFinalVolume = ((LONG) lpTrack->nFinalVolume * 
				     lpTrack->nVolumeFade) >> 15;
    }

    /* process panning envelope */
    if (lpPatch != NULL && (lpPatch->Panning.wFlags & AUDIO_ENVELOPE_ON)) {
        lpPts = lpPatch->Panning.aEnvelope;
        if (++lpTrack->nPanningFrame >= lpPts[lpTrack->nPanningPoint].nFrame) {
            if ((lpPatch->Panning.wFlags & AUDIO_ENVELOPE_SUSTAIN) &&
                (lpTrack->nPanningPoint == lpPatch->Panning.nSustain) &&
                lpTrack->fKeyOn) {
                lpTrack->nPanningFrame = lpPts[lpTrack->nPanningPoint].nFrame;
                lpTrack->nPanningValue = (int) lpPts[lpTrack->nPanningPoint].nValue << 8;
            }
            else {
                if ((lpPatch->Panning.wFlags & AUDIO_ENVELOPE_LOOP) &&
                    (lpTrack->nPanningPoint == lpPatch->Panning.nLoopEnd)) {
                    lpTrack->nPanningPoint = lpPatch->Panning.nLoopStart;
                }
                lpTrack->nPanningFrame = lpPts[lpTrack->nPanningPoint].nFrame;
                lpTrack->nPanningValue = (int) lpPts[lpTrack->nPanningPoint].nValue << 8;
                if (lpTrack->nPanningPoint + 1 >= lpPatch->Panning.nPoints) {
                    lpTrack->nPanningSlope = 0;
                }
                else {
                    if ((nFrames = lpPts[lpTrack->nPanningPoint + 1].nFrame -
			 lpPts[lpTrack->nPanningPoint].nFrame) <= 0)
                        lpTrack->nPanningSlope = 0;
                    else {
                        lpTrack->nPanningSlope =
                            (((int) lpPts[lpTrack->nPanningPoint + 1].nValue -
			      (int) lpPts[lpTrack->nPanningPoint].nValue) << 8) / nFrames;
                    }
                    lpTrack->nPanningPoint++;
                }
            }
        }
        else {
            lpTrack->nPanningValue += lpTrack->nPanningSlope;
            lpTrack->nPanningValue = CLIP(lpTrack->nPanningValue, 0, 64 * 256);
        }
        lpTrack->nFinalPanning = lpTrack->nPanning +
            ((((128L - ABS(lpTrack->nPanning - 128)) << 3) *
	      (lpTrack->nPanningValue - 32 * 256L)) >> 16);
        lpTrack->bControl |= AUDIO_CTRL_PANNING;
    }
    else {
        lpTrack->nFinalPanning = lpTrack->nPanning;
    }

    /* process automatic vibrato */
    if (lpPatch != NULL && lpPatch->nVibratoDepth != 0) {
        if (lpTrack->fKeyOn && lpTrack->nAutoVibratoSlope) {
            lpTrack->nAutoVibratoValue += lpTrack->nAutoVibratoSlope;
            if (lpTrack->nAutoVibratoValue > ((int) lpPatch->nVibratoDepth << 8)) {
                lpTrack->nAutoVibratoValue = (int) lpPatch->nVibratoDepth << 8;
                lpTrack->nAutoVibratoSlope = 0;
            }
        }
        lpTrack->nAutoVibratoFrame += lpPatch->nVibratoRate;
        nFrames = (BYTE) lpTrack->nAutoVibratoFrame;
        switch (lpPatch->nVibratoType) {
        case 0x00:
            nValue = aAutoVibratoTable[nFrames];
            break;
        case 0x01:
            nValue = (nFrames & 0x80) ? +64 : -64;
            break;
        case 0x02:
            nValue = ((64 + (nFrames >> 1)) & 0x7f) - 64;
            break;
        case 0x03:
            nValue = ((64 - (nFrames >> 1)) & 0x7f) - 64;
            break;
        default:
            /* unknown vibrato waveform type */
            nValue = 0;
            break;
        }
        lpTrack->nFinalPeriod = lpTrack->nOutPeriod +
            ((((LONG) nValue << 2) * lpTrack->nAutoVibratoValue) >> 16);
        lpTrack->nFinalPeriod = CLIP(lpTrack->nFinalPeriod,
				     AUDIO_MIN_PERIOD, AUDIO_MAX_PERIOD);
        lpTrack->bControl |= AUDIO_CTRL_PITCH;
    }
    else {
        lpTrack->nFinalPeriod = lpTrack->nOutPeriod;
    }
}

static VOID PlayNote(LPTRACK lpTrack)
{
    LPAUDIOPATCH lpPatch;
    LPAUDIOSAMPLE lpSample;

    lpTrack->fKeyOn = (lpTrack->nNote >= 1 && lpTrack->nNote <= AUDIO_MAX_NOTES);
    if (lpTrack->fKeyOn && (lpPatch = lpTrack->lpPatch) != NULL) {
        lpTrack->nSample = lpPatch->aSampleNumber[lpTrack->nNote - 1];
        if (lpTrack->nSample < lpPatch->nSamples) {
            lpTrack->lpSample = &lpPatch->aSampleTable[lpTrack->nSample];
            lpSample = lpTrack->lpSample;
            lpTrack->nRelativeNote = (signed char) lpSample->nRelativeNote;
            lpTrack->nFinetune = (signed char) lpSample->nFinetune;
            if (lpTrack->nCommand != 0x03 && lpTrack->nCommand != 0x05 &&
                (lpTrack->nVolumeCmd & 0xF0) != 0xF0) {
                lpTrack->nPeriod = lpTrack->nOutPeriod =
                    GetPeriodValue(lpTrack->nNote,
				   lpTrack->nRelativeNote, lpTrack->nFinetune);
                lpTrack->bControl |= (AUDIO_CTRL_PITCH | AUDIO_CTRL_KEYON);
            }
        }
        else {
            lpTrack->lpSample = NULL;
        }
    }
}

static VOID StopNote(LPTRACK lpTrack)
{
    lpTrack->fKeyOn = 0;
    if (lpTrack->lpPatch != NULL) {
        if (!(lpTrack->lpPatch->Volume.wFlags & AUDIO_ENVELOPE_ON)) {
            lpTrack->nVolume = lpTrack->nOutVolume = 0;
            lpTrack->bControl |= (AUDIO_CTRL_VOLUME | AUDIO_CTRL_KEYOFF);
        }
    }
    else {
        lpTrack->bControl |= AUDIO_CTRL_KEYOFF;
    }
}

static VOID RetrigNote(LPTRACK lpTrack)
{
    PlayNote(lpTrack);
    StartEnvelopes(lpTrack);
}

static VOID GetPatternNote(LPNOTE lpNote)
{
    BYTE fPacking;

#define GETBYTE *Player.lpData++

    fPacking = (Player.lpData != NULL) ?
        (Player.lpData[0] & AUDIO_PATTERN_PACKED ? GETBYTE : 0xFF) : 0x00;
    lpNote->nNote = (fPacking & AUDIO_PATTERN_NOTE) ? GETBYTE : 0x00;
    lpNote->nPatch = (fPacking & AUDIO_PATTERN_SAMPLE) ? GETBYTE : 0x00;
    lpNote->nVolume = (fPacking & AUDIO_PATTERN_VOLUME) ? GETBYTE : 0x00;
    lpNote->nCommand = (fPacking & AUDIO_PATTERN_COMMAND) ? GETBYTE : 0x00;
    lpNote->bParams = (fPacking & AUDIO_PATTERN_PARAMS) ? GETBYTE : 0x00;
}

static VOID GetTrackNote(LPTRACK lpTrack)
{
    static NOTE Note;

    /* read next packed note from pattern */
    GetPatternNote(&Note);

    /* reset frequency for vibrato and arpeggio commands */
    if (lpTrack->nCommand == 0x04 || lpTrack->nCommand == 0x06) {
        if (Note.nCommand != 0x04 && Note.nCommand != 0x06) {
            lpTrack->nOutPeriod = lpTrack->nPeriod;
            lpTrack->bControl |= AUDIO_CTRL_PITCH;
        }
    }
    else if (lpTrack->nCommand == 0x00 && lpTrack->bParams != 0x00) {
        lpTrack->nOutPeriod = lpTrack->nPeriod;
        lpTrack->bControl |= AUDIO_CTRL_PITCH;
    }

    /* assign volume and effect commands */
    lpTrack->nVolumeCmd = Note.nVolume;
    lpTrack->nCommand = Note.nCommand;
    lpTrack->bParams = Note.bParams;

    /* change default patch instrument */
    if (Note.nPatch >= 1 && Note.nPatch <= Player.lpModule->nPatches) {
        lpTrack->nPatch = Note.nPatch;
        lpTrack->lpPatch = &Player.lpModule->aPatchTable[lpTrack->nPatch - 1];
    }

    /* new note pressed? */
    if (Note.nNote >= 1 && Note.nNote <= AUDIO_MAX_NOTES) {
        lpTrack->nNote = Note.nNote;
        PlayNote(lpTrack);
        if (Note.nPatch != 0 && lpTrack->lpSample != NULL)
            StartEnvelopes(lpTrack);
    }
    else if (Note.nNote != 0) {
        StopNote(lpTrack);
    }

    /* use default sample's volume and panning? */
    if (Note.nPatch != 0 && lpTrack->lpSample != NULL) {
        lpTrack->nVolume = lpTrack->nOutVolume = lpTrack->lpSample->nVolume;
        lpTrack->bControl |= AUDIO_CTRL_VOLUME;
        if (!(Player.wFlags & AUDIO_MODULE_PANNING)) {
            lpTrack->nPanning = lpTrack->lpSample->nPanning;
            lpTrack->bControl |= AUDIO_CTRL_PANNING;
        }
    }
}

static VOID SendNoteMesg(HAC hVoice, LPTRACK lpTrack)
{
    if (lpTrack->bControl & (AUDIO_CTRL_KEYON | AUDIO_CTRL_TOUCH)) {
        if (lpTrack->lpSample != NULL) {
            APrimeVoice(hVoice, &lpTrack->lpSample->Wave);
            if (lpTrack->bControl & AUDIO_CTRL_TOUCH)
                ASetVoicePosition(hVoice, lpTrack->dwSampleOffset);
        }
    }
    if (lpTrack->bControl & AUDIO_CTRL_KEYOFF) {
        AStopVoice(hVoice);
    }
    if (lpTrack->bControl & AUDIO_CTRL_PITCH) {
        ASetVoiceFrequency(hVoice, GetFrequencyValue(lpTrack->nFinalPeriod));
    }
    if (lpTrack->bControl & AUDIO_CTRL_VOLUME) {
        ASetVoiceVolume(hVoice, (lpTrack->nFinalVolume * Player.nVolume) >> 6);
    }
    if (lpTrack->bControl & AUDIO_CTRL_PANNING) {
        ASetVoicePanning(hVoice, lpTrack->nFinalPanning);
    }
    if (lpTrack->bControl & AUDIO_CTRL_KEYON) {
        AStartVoice(hVoice);
    }
    lpTrack->bControl = 0x00;
}

static VOID GetNextPatternRow(VOID)
{
    static NOTE Note;
    LPAUDIOPATTERN lpPattern;
    int n, m;

    Player.nFrame = 0;
    if (Player.wControl & AUDIO_PLAYER_DELAY)
        return;

    if (++Player.nRow >= Player.nRows) {
        Player.wControl |= AUDIO_PLAYER_BREAK;
    }
    if (Player.wControl & AUDIO_PLAYER_JUMP) {
        Player.nRow = Player.nJumpRow;
        Player.nJumpRow = 0;
        Player.nOrder = Player.nJumpOrder;
    }
    else if (Player.wControl & AUDIO_PLAYER_BREAK) {
        Player.nRow = Player.nJumpRow;
        Player.nJumpRow = 0;
        Player.nOrder++;
    }
    if (Player.wControl & (AUDIO_PLAYER_BREAK | AUDIO_PLAYER_JUMP)) {
        Player.wControl &= ~(AUDIO_PLAYER_BREAK | AUDIO_PLAYER_JUMP);
        if (Player.nOrder >= Player.lpModule->nOrders) {
            Player.nOrder = Player.lpModule->nRestart;
            if (Player.nOrder >= Player.lpModule->nOrders) {
                Player.nOrder = 0x00;
                Player.wControl |= AUDIO_PLAYER_PAUSE;
                return;
            }
        }
        Player.nPattern = Player.lpModule->aOrderTable[Player.nOrder];
        if (Player.nPattern < Player.lpModule->nPatterns) {
            lpPattern = &Player.lpModule->aPatternTable[Player.nPattern];
            Player.nRows = lpPattern->nRows;
            Player.lpData = lpPattern->lpData;
            if (Player.nRow >= Player.nRows) {
                Player.nRow = 0x00;
            }
            for (m = 0; m < Player.nRow; m++) {
                for (n = 0; n < Player.nTracks; n++)
                    GetPatternNote(&Note);
            }
        }
        else {
            Player.nRows = 64;
            Player.lpData = NULL;
        }
    }
    for (n = 0; n < Player.nTracks; n++) {
        GetTrackNote(&Player.aTracks[n]);
    }
}

static VOID AIAPI PlayNextFrame(VOID)
{
    int n;

    if (!(Player.wControl & AUDIO_PLAYER_PAUSE)) {
        if (++Player.nFrame >= Player.nTempo) {
            GetNextPatternRow();
        }

        for (n = 0; n < Player.nTracks; n++) {
            ExecVolumeCmd(&Player.aTracks[n]);
            ExecNoteCmd(&Player.aTracks[n]);
            UpdateEnvelopes(&Player.aTracks[n]);
        }

        if (Player.wControl & AUDIO_PLAYER_DELAY) {
            if (!Player.nFrame && !--Player.nPatternDelay)
                Player.wControl &= ~AUDIO_PLAYER_DELAY;
        }

        if (Player.wControl & AUDIO_PLAYER_VOLUME) {
            Player.wControl &= ~AUDIO_PLAYER_VOLUME;
            for (n = 0; n < Player.nTracks; n++)
                Player.aTracks[n].bControl |= AUDIO_CTRL_VOLUME;
        }

        if (Player.wControl & AUDIO_PLAYER_BPM) {
            Player.wControl &= ~AUDIO_PLAYER_BPM;
            ASetAudioTimerRate(Player.nBPM);
        }

        for (n = 0; n < Player.nTracks; n++) {
            SendNoteMesg(Player.aVoices[n], &Player.aTracks[n]);
        }
    }
}


/*
 * High-level extended module player routines
 */
UINT AIAPI APlayModule(LPAUDIOMODULE lpModule)
{
    int n;

    if (!(Player.wControl & AUDIO_PLAYER_ACTIVE)) {
        if (lpModule != NULL) {
            memset(&Player, 0, sizeof(Player));
            Player.lpModule = lpModule;
            Player.nTracks = lpModule->nTracks;
            Player.wFlags = lpModule->wFlags;
            Player.nTempo = lpModule->nTempo;
            Player.nBPM = lpModule->nBPM;
            Player.nVolume = AUDIO_MAX_VOLUME;
            Player.wControl = AUDIO_PLAYER_ACTIVE | AUDIO_PLAYER_JUMP;
            for (n = 0; n < Player.nTracks; n++) {
                if (ACreateAudioVoice(&Player.aVoices[n]) != AUDIO_ERROR_NONE) {
                    AStopModule();
                    return AUDIO_ERROR_NOMEMORY;
                }
                Player.aTracks[n].nPanning = lpModule->aPanningTable[n];
                ASetVoicePanning(Player.aVoices[n], Player.aTracks[n].nPanning);
            }
            ASetAudioTimerRate(Player.nBPM);
            ASetAudioTimerProc(PlayNextFrame);
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_NOTSUPPORTED;
}

UINT AIAPI AStopModule(VOID)
{
    int n;

    if (Player.wControl & AUDIO_PLAYER_ACTIVE) {
        for (n = 0; n < Player.nTracks; n++) {
            AStopVoice(Player.aVoices[n]);
            ADestroyAudioVoice(Player.aVoices[n]);
        }
        memset(&Player, 0, sizeof(Player));
        ASetAudioTimerProc(NULL);
        ASetAudioTimerRate(125);
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_NOTSUPPORTED;
}

UINT AIAPI APauseModule(VOID)
{
    int n;

    if (Player.wControl & AUDIO_PLAYER_ACTIVE) {
        Player.wControl |= AUDIO_PLAYER_PAUSE;
        for (n = 0; n < Player.nTracks; n++)
            ASetVoiceVolume(Player.aVoices[n], 0x00);
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_NOTSUPPORTED;
}

UINT AIAPI AResumeModule(VOID)
{
    if (Player.wControl & AUDIO_PLAYER_ACTIVE) {
        Player.wControl &= ~AUDIO_PLAYER_PAUSE;
        Player.wControl |= AUDIO_PLAYER_VOLUME;
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_NOTSUPPORTED;
}

UINT AIAPI ASetModuleVolume(UINT nVolume)
{
    if (Player.wControl & AUDIO_PLAYER_ACTIVE) {
        if (nVolume <= AUDIO_MAX_VOLUME) {
            Player.nVolume = nVolume;
            if (!(Player.wControl & AUDIO_PLAYER_PAUSE)) {
                Player.wControl |= AUDIO_PLAYER_VOLUME;
            }
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_NOTSUPPORTED;
}

UINT AIAPI ASetModulePosition(UINT nOrder, UINT nRow)
{
    if (Player.wControl & AUDIO_PLAYER_ACTIVE) {
        Player.nJumpOrder = nOrder;
        Player.nJumpRow = nRow;
        Player.wControl |= AUDIO_PLAYER_JUMP;
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_NOTSUPPORTED;
}

UINT AIAPI AGetModuleVolume(LPUINT lpnVolume)
{
    if (Player.wControl & AUDIO_PLAYER_ACTIVE) {
        if (lpnVolume != NULL) {
            *lpnVolume = Player.nVolume;
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_NOTSUPPORTED;
}

UINT AIAPI AGetModulePosition(LPUINT lpnOrder, LPUINT lpnRow)
{
    if (Player.wControl & AUDIO_PLAYER_ACTIVE) {
        if (lpnOrder != NULL && lpnRow != NULL) {
            *lpnOrder = Player.nOrder;
            *lpnRow = Player.nRow;
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_NOTSUPPORTED;
}

UINT AIAPI AGetModuleStatus(LPBOOL lpnStatus)
{
    if (Player.wControl & AUDIO_PLAYER_ACTIVE) {
        if (lpnStatus != NULL) {
            *lpnStatus = ((Player.wControl & AUDIO_PLAYER_PAUSE) != 0);
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_NOTSUPPORTED;
}

UINT AIAPI ASetModuleCallback(LPFNAUDIOCALLBACK lpfnAudioCallback)
{
    if (Player.wControl & AUDIO_PLAYER_ACTIVE) {
        Player.lpfnCallback = lpfnAudioCallback;
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_NOTSUPPORTED;
}

UINT AIAPI AFreeModuleFile(LPAUDIOMODULE lpModule)
{
    LPAUDIOPATTERN lpPattern;
    LPAUDIOPATCH lpPatch;
    LPAUDIOSAMPLE lpSample;
    UINT n, m;

    if (lpModule != NULL) {
        if ((lpPattern = lpModule->aPatternTable) != NULL) {
            for (n = 0; n < lpModule->nPatterns; n++, lpPattern++) {
                if (lpPattern->lpData != NULL)
                    free(lpPattern->lpData);
            }
            free(lpModule->aPatternTable);
        }
        if ((lpPatch = lpModule->aPatchTable) != NULL) {
            for (n = 0; n < lpModule->nPatches; n++, lpPatch++) {
                lpSample = lpPatch->aSampleTable;
                for (m = 0; m < lpPatch->nSamples; m++, lpSample++) {
                    ADestroyAudioData(&lpSample->Wave);
                }
                free(lpPatch->aSampleTable);
            }
            free(lpModule->aPatchTable);
        }
        free(lpModule);
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALPARAM;
}

UINT AIAPI ALoadModuleFile(LPSTR lpszFileName, 
			   LPAUDIOMODULE *lplpModule, DWORD dwOffset)
{
    extern UINT AIAPI ALoadModuleXM(LPSTR, LPAUDIOMODULE*, DWORD);
    extern UINT AIAPI ALoadModuleS3M(LPSTR, LPAUDIOMODULE*, DWORD);
    extern UINT AIAPI ALoadModuleMOD(LPSTR, LPAUDIOMODULE*, DWORD);
    extern UINT AIAPI ALoadModuleMTM(LPSTR, LPAUDIOMODULE*, DWORD);
    UINT nErrorCode;

    if (lpszFileName != NULL && lplpModule != NULL) {
        *lplpModule = NULL;
        nErrorCode = ALoadModuleXM(lpszFileName, lplpModule, dwOffset);
        if (nErrorCode == AUDIO_ERROR_BADFILEFORMAT)
            nErrorCode = ALoadModuleS3M(lpszFileName, lplpModule, dwOffset);
        if (nErrorCode == AUDIO_ERROR_BADFILEFORMAT)
            nErrorCode = ALoadModuleMOD(lpszFileName, lplpModule, dwOffset);
        if (nErrorCode == AUDIO_ERROR_BADFILEFORMAT)
            nErrorCode = ALoadModuleMTM(lpszFileName, lplpModule, dwOffset);
        return nErrorCode;
    }
    return AUDIO_ERROR_INVALPARAM;
}

/*** NEW: 04/12/98 ***/
UINT AIAPI AGetModuleTrack(UINT nTrack, LPAUDIOTRACK lpTrack)
{
    if (Player.wControl & AUDIO_PLAYER_ACTIVE) {
        if (nTrack < 32 && lpTrack != NULL) {
            lpTrack->nNote = Player.aTracks[nTrack].nNote;
            lpTrack->nPatch = Player.aTracks[nTrack].nPatch;
            lpTrack->nSample = Player.aTracks[nTrack].nSample;
            lpTrack->nCommand = Player.aTracks[nTrack].nCommand;
            lpTrack->nVolumeCmd = Player.aTracks[nTrack].nVolumeCmd;
            lpTrack->bParams = Player.aTracks[nTrack].bParams;
            lpTrack->nVolume = Player.aTracks[nTrack].nFinalVolume;
            lpTrack->nPanning = Player.aTracks[nTrack].nFinalPanning;
            lpTrack->wPeriod = Player.aTracks[nTrack].nFinalPeriod;
            lpTrack->dwFrequency = Player.aTracks[nTrack].dwFrequency;
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_NOTSUPPORTED;
}

