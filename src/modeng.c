/*
 * $Id: modeng.c 1.7 1996/05/24 08:30:44 chasan released $
 *
 * Extended module player engine.
 *
 * Copyright (C) 1995, 1996 Carlos Hasan. All Rights Reserved.
 *
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
#define LOPARAM(x)              ((UCHAR)(x)&0x0F)
#define HIPARAM(x)              ((UCHAR)(x)>>4)
#define CLIP(x,a,b)             ((x)<(a)?(a):((x)>(b)?(b):(x)))
#define ABS(x)                  ((x)>0?(x):-(x))


/*
 * Module player track structure
 */
typedef struct {
    UCHAR   nNote;              /* note index (1-96) */
    UCHAR   nPatch;             /* patch number (1-128) */
    UCHAR   nVolume;            /* volume command */
    UCHAR   nCommand;           /* effect */
    UCHAR   bParams;            /* parameters */
} NOTE, *PNOTE;

typedef struct {
    UCHAR   fKeyOn;             /* key on flag */
    UCHAR   bControl;           /* control bits */
    UCHAR   nVolumeCmd;         /* volume command */
    UCHAR   nCommand;           /* command */
    UCHAR   bParams;            /* parameters */
    UCHAR   nPatch;             /* patch number */
    UCHAR   nSample;            /* sample number */
    UCHAR   nNote;              /* current note */
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

    PAUDIOPATCH pPatch;         /* current patch */
    PAUDIOSAMPLE pSample;       /* current sample */

    /* waves & gliss control */
    UCHAR   bWaveCtrl;          /* vibrato & tremolo control bits */
    UCHAR   bGlissCtrl;         /* glissando control bits */

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
    UCHAR   bVolumeSlide;       /* volume slide rate */
    UCHAR   bPanningSlide;      /* panning slide rate */
    UCHAR   nFinePortaUp;       /* fine portamento up rate */
    UCHAR   nFinePortaDown;     /* fine portamento down rate */
    UCHAR   nExtraPortaUp;      /* extra fine porta up rate */
    UCHAR   nExtraPortaDown;    /* extra fine porta down rate */
    UCHAR   nRetrigType;        /* multi retrig type */
    UCHAR   nRetrigInterval;    /* multi retrig interval */
    UCHAR   nRetrigFrame;       /* multi retrig frame */
    UCHAR   bTremorParms;       /* tremor parameters */
    UCHAR   bTremorOnOff;       /* tremor on/off state */
    UCHAR   nTremorFrame;       /* tremor frame */
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
} TRACK, *PTRACK;

/*
 * Module player run-time state structure
 */
static struct {
    PAUDIOMODULE pModule;       /* current module */
    PUCHAR  pData;              /* pattern data pointer */
    TRACK   aTracks[32];        /* array of tracks */
    HAC     aVoices[32];        /* array of voices */
    USHORT  wControl;           /* player control bits */
    USHORT  wFlags;             /* module control bits */
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
} Player;


static VOID PlayNote(PTRACK pTrack);
static VOID StopNote(PTRACK pTrack);
static VOID RetrigNote(PTRACK pTrack);

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

static VOID OnArpeggio(PTRACK pTrack)
{
    int nNote;

    if (pTrack->bParams) {
        nNote = pTrack->nNote;
        switch (Player.nFrame % 3) {
        case 1:
            nNote += HIPARAM(pTrack->bParams);
            break;
        case 2:
            nNote += LOPARAM(pTrack->bParams);
            break;
        }
        pTrack->nOutPeriod = GetPeriodValue(nNote,
            pTrack->nRelativeNote, pTrack->nFinetune);
        pTrack->bControl |= AUDIO_CTRL_PITCH;
    }
}

static VOID OnPortaUp(PTRACK pTrack)
{
    if (!Player.nFrame) {
        if (pTrack->bParams != 0x00) {
            pTrack->nPortaUp = (int) pTrack->bParams << 2;
        }
    }
    else {
        pTrack->nPeriod -= pTrack->nPortaUp;
        if (pTrack->nPeriod < AUDIO_MIN_PERIOD)
            pTrack->nPeriod = AUDIO_MIN_PERIOD;
        pTrack->nOutPeriod = pTrack->nPeriod;
        pTrack->bControl |= AUDIO_CTRL_PITCH;
    }
}

static VOID OnPortaDown(PTRACK pTrack)
{
    if (!Player.nFrame) {
        if (pTrack->bParams != 0x00) {
            pTrack->nPortaDown = (int) pTrack->bParams << 2;
        }
    }
    else {
        pTrack->nPeriod += pTrack->nPortaDown;
        if (pTrack->nPeriod > AUDIO_MAX_PERIOD)
            pTrack->nPeriod = AUDIO_MAX_PERIOD;
        pTrack->nOutPeriod = pTrack->nPeriod;
        pTrack->bControl |= AUDIO_CTRL_PITCH;
    }
}

static VOID OnTonePorta(PTRACK pTrack)
{
    if (!Player.nFrame) {
        if (pTrack->bParams != 0x00) {
            pTrack->nTonePorta = (int) pTrack->bParams << 2;
        }
        pTrack->nWantedPeriod = GetPeriodValue(pTrack->nNote,
            pTrack->nRelativeNote, pTrack->nFinetune);
        pTrack->bControl &= ~(AUDIO_CTRL_PITCH | AUDIO_CTRL_KEYON);
    }
    else {
        if (pTrack->nPeriod < pTrack->nWantedPeriod) {
            pTrack->nPeriod += pTrack->nTonePorta;
            if (pTrack->nPeriod > pTrack->nWantedPeriod) {
                pTrack->nPeriod = pTrack->nWantedPeriod;
            }
        }
        else if (pTrack->nPeriod > pTrack->nWantedPeriod) {
            pTrack->nPeriod -= pTrack->nTonePorta;
            if (pTrack->nPeriod < pTrack->nWantedPeriod) {
                pTrack->nPeriod = pTrack->nWantedPeriod;
            }
        }
        /* TODO: glissando not implemented */
        pTrack->nOutPeriod = pTrack->nPeriod;
        pTrack->bControl |= AUDIO_CTRL_PITCH;
    }
}

static VOID OnVibrato(PTRACK pTrack)
{
    int nDelta, nFrame;

    if (!Player.nFrame) {
        if (LOPARAM(pTrack->bParams)) {
            pTrack->nVibratoDepth = LOPARAM(pTrack->bParams);
        }
        if (HIPARAM(pTrack->bParams)) {
            pTrack->nVibratoRate = HIPARAM(pTrack->bParams) << 2;
        }
    }
    else {
        nFrame = (pTrack->nVibratoFrame >> 2) & 0x1F;
        switch (pTrack->bWaveCtrl & 0x03) {
        case 0x00:
            nDelta = aSineTable[nFrame];
            break;
        case 0x01:
            nDelta = nFrame << 3;
            if (pTrack->nVibratoFrame & 0x80)
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
        nDelta = ((nDelta * pTrack->nVibratoDepth) >> 5);
        pTrack->nOutPeriod = pTrack->nPeriod;
        if (pTrack->nVibratoFrame & 0x80) {
            pTrack->nOutPeriod -= nDelta;
            if (pTrack->nOutPeriod < AUDIO_MIN_PERIOD)
                pTrack->nOutPeriod = AUDIO_MIN_PERIOD;
        }
        else {
            pTrack->nOutPeriod += nDelta;
            if (pTrack->nOutPeriod > AUDIO_MAX_PERIOD)
                pTrack->nOutPeriod = AUDIO_MAX_PERIOD;
        }
        pTrack->bControl |= AUDIO_CTRL_PITCH;
        pTrack->nVibratoFrame += pTrack->nVibratoRate;
    }
}

static VOID OnFineVibrato(PTRACK pTrack)
{
    int nDelta, nFrame;

    if (!Player.nFrame) {
        if (LOPARAM(pTrack->bParams)) {
            pTrack->nVibratoDepth = LOPARAM(pTrack->bParams);
        }
        if (HIPARAM(pTrack->bParams)) {
            pTrack->nVibratoRate = HIPARAM(pTrack->bParams) << 2;
        }
    }
    else {
        nFrame = (pTrack->nVibratoFrame >> 2) & 0x1F;
        switch (pTrack->bWaveCtrl & 0x03) {
        case 0x00:
            nDelta = aSineTable[nFrame];
            break;
        case 0x01:
            nDelta = nFrame << 3;
            if (pTrack->nVibratoFrame & 0x80)
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
        nDelta = ((nDelta * pTrack->nVibratoDepth) >> 7);
        pTrack->nOutPeriod = pTrack->nPeriod;
        if (pTrack->nVibratoFrame & 0x80) {
            pTrack->nOutPeriod -= nDelta;
            if (pTrack->nOutPeriod < AUDIO_MIN_PERIOD)
                pTrack->nOutPeriod = AUDIO_MIN_PERIOD;
        }
        else {
            pTrack->nOutPeriod += nDelta;
            if (pTrack->nOutPeriod > AUDIO_MAX_PERIOD)
                pTrack->nOutPeriod = AUDIO_MAX_PERIOD;
        }
        pTrack->bControl |= AUDIO_CTRL_PITCH;
        pTrack->nVibratoFrame += pTrack->nVibratoRate;
    }
}

static VOID OnVolumeSlide(PTRACK pTrack)
{
    if (!Player.nFrame) {
        if (pTrack->bParams != 0x00) {
            pTrack->bVolumeSlide = pTrack->bParams;
        }
    }
    else {
        if (HIPARAM(pTrack->bVolumeSlide)) {
            pTrack->nVolume += HIPARAM(pTrack->bVolumeSlide);
            if (pTrack->nVolume > AUDIO_MAX_VOLUME)
                pTrack->nVolume = AUDIO_MAX_VOLUME;
        }
        else {
            pTrack->nVolume -= LOPARAM(pTrack->bVolumeSlide);
            if (pTrack->nVolume < AUDIO_MIN_VOLUME)
                pTrack->nVolume = AUDIO_MIN_VOLUME;
        }
        pTrack->nOutVolume = pTrack->nVolume;
        pTrack->bControl |= AUDIO_CTRL_VOLUME;
    }
}

static VOID OnToneAndSlide(PTRACK pTrack)
{
    if (!Player.nFrame) {
        OnVolumeSlide(pTrack);
        pTrack->bControl &= ~(AUDIO_CTRL_PITCH | AUDIO_CTRL_KEYON);
    }
    else {
        OnTonePorta(pTrack);
        OnVolumeSlide(pTrack);
    }
}

static VOID OnVibratoAndSlide(PTRACK pTrack)
{
    if (!Player.nFrame) {
        OnVolumeSlide(pTrack);
    }
    else {
        OnVibrato(pTrack);
        OnVolumeSlide(pTrack);
    }
}

static VOID OnTremolo(PTRACK pTrack)
{
    int nDelta, nFrame;

    if (!Player.nFrame) {
        if (LOPARAM(pTrack->bParams)) {
            pTrack->nTremoloDepth = LOPARAM(pTrack->bParams);
        }
        if (HIPARAM(pTrack->bParams)) {
            pTrack->nTremoloRate = HIPARAM(pTrack->bParams) << 2;
        }
    }
    else {
        nFrame = (pTrack->nTremoloFrame >> 2) & 0x1F;
        switch (pTrack->bWaveCtrl & 0x30) {
        case 0x00:
            nDelta = aSineTable[nFrame];
            break;
        case 0x10:
            nDelta = nFrame << 3;
            if (pTrack->nTremoloFrame & 0x80)
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
        nDelta = (nDelta * pTrack->nTremoloDepth) >> 6;
        pTrack->nOutVolume = pTrack->nVolume;
        if (pTrack->nTremoloFrame & 0x80) {
            pTrack->nOutVolume -= nDelta;
            if (pTrack->nOutVolume < AUDIO_MIN_VOLUME)
                pTrack->nOutVolume = AUDIO_MIN_VOLUME;
        }
        else {
            pTrack->nOutVolume += nDelta;
            if (pTrack->nOutVolume > AUDIO_MAX_VOLUME)
                pTrack->nOutVolume = AUDIO_MAX_VOLUME;
        }
        pTrack->bControl |= AUDIO_CTRL_VOLUME;
        pTrack->nTremoloFrame += pTrack->nTremoloRate;
    }
}

static VOID OnSetPanning(PTRACK pTrack)
{
    if (!Player.nFrame) {
        pTrack->nPanning = pTrack->bParams;
        pTrack->bControl |= AUDIO_CTRL_PANNING;
    }
}

static VOID OnSampleOffset(PTRACK pTrack)
{
    if (!Player.nFrame) {
        pTrack->dwSampleOffset = (LONG) pTrack->bParams << 8;
        pTrack->bControl |= AUDIO_CTRL_KEYON | AUDIO_CTRL_TOUCH;
    }
}

static VOID OnJumpPosition(PTRACK pTrack)
{
    if (!Player.nFrame) {
        if (pTrack->bParams < Player.pModule->nOrders) {
            Player.nJumpOrder = pTrack->bParams;
            Player.wControl |= AUDIO_PLAYER_JUMP;
        }
    }
}

static VOID OnSetVolume(PTRACK pTrack)
{
    if (!Player.nFrame) {
        if ((pTrack->nVolume = pTrack->bParams) > AUDIO_MAX_VOLUME)
            pTrack->nVolume = AUDIO_MAX_VOLUME;
        pTrack->nOutVolume = pTrack->nVolume;
        pTrack->bControl |= AUDIO_CTRL_VOLUME;
    }
}

static VOID OnPatternBreak(PTRACK pTrack)
{
    if (!Player.nFrame) {
        Player.nJumpRow = 10 * HIPARAM(pTrack->bParams) +
            LOPARAM(pTrack->bParams);
        Player.wControl |= AUDIO_PLAYER_BREAK;
    }
}

static VOID OnExtraCommand(PTRACK pTrack)
{
    switch (HIPARAM(pTrack->bParams)) {
    case 0x0:
        /* TODO: filter control not implemented */
        break;
    case 0x1:
        /* fine portamento up */
        if (!Player.nFrame) {
            if (LOPARAM(pTrack->bParams))
                pTrack->nFinePortaUp = LOPARAM(pTrack->bParams) << 2;
            pTrack->nPeriod -= pTrack->nFinePortaUp;
            if (pTrack->nPeriod < AUDIO_MIN_PERIOD)
                pTrack->nPeriod = AUDIO_MIN_PERIOD;
            pTrack->nOutPeriod = pTrack->nPeriod;
            pTrack->bControl |= AUDIO_CTRL_PITCH;
        }
        break;
    case 0x2:
        /* fine portamento down */
        if (!Player.nFrame) {
            if (LOPARAM(pTrack->bParams))
                pTrack->nFinePortaDown = LOPARAM(pTrack->bParams) << 2;
            pTrack->nPeriod += pTrack->nFinePortaDown;
            if (pTrack->nPeriod > AUDIO_MAX_PERIOD)
                pTrack->nPeriod = AUDIO_MAX_PERIOD;
            pTrack->nOutPeriod = pTrack->nPeriod;
            pTrack->bControl |= AUDIO_CTRL_PITCH;
        }
        break;
    case 0x3:
        /* set glissando control */
        if (!Player.nFrame) {
            pTrack->bGlissCtrl = LOPARAM(pTrack->bParams);
        }
        break;
    case 0x4:
        /* set vibrato wave control */
        if (!Player.nFrame) {
            pTrack->bWaveCtrl &= 0xF0;
            pTrack->bWaveCtrl |= LOPARAM(pTrack->bParams);
        }
        break;
    case 0x5:
        /* set finetune */
        if (!Player.nFrame) {
            pTrack->nFinetune = ((int) LOPARAM(pTrack->bParams) << 4) - 0x80;
            pTrack->nOutPeriod = GetPeriodValue(pTrack->nNote,
                pTrack->nRelativeNote, pTrack->nFinetune);
            pTrack->bControl |= AUDIO_CTRL_PITCH;
        }
        break;
    case 0x6:
        /* set/start pattern loop */
        if (!Player.nFrame) {
            if (LOPARAM(pTrack->bParams)) {
                if (pTrack->nPatternLoop)
                    pTrack->nPatternLoop--;
                else
                    pTrack->nPatternLoop = LOPARAM(pTrack->bParams);
                if (pTrack->nPatternLoop) {
                    Player.nJumpRow = pTrack->nPatternRow;
                    Player.nJumpOrder = Player.nOrder;
                    Player.wControl |= AUDIO_PLAYER_JUMP;
                }
            }
            else {
                pTrack->nPatternRow = Player.nRow;
            }
        }
        break;
    case 0x7:
        /* set tremolo wave control */
        if (!Player.nFrame) {
            pTrack->bWaveCtrl &= 0x0F;
            pTrack->bWaveCtrl |= LOPARAM(pTrack->bParams) << 4;
        }
        break;
    case 0x8:
        /* set stereo panning control */
        if (!Player.nFrame) {
            pTrack->nPanning = LOPARAM(pTrack->bParams) << 4;
            pTrack->bControl |= AUDIO_CTRL_PANNING;
        }
        break;
    case 0x9:
        /* retrig note */
        if (!Player.nFrame) {
            RetrigNote(pTrack);
        }
        else if (LOPARAM(pTrack->bParams)) {
            if (!(Player.nFrame % LOPARAM(pTrack->bParams))) {
                RetrigNote(pTrack);
            }
        }
        break;
    case 0xA:
        /* fine volume slide up */
        if (!Player.nFrame) {
            pTrack->nVolume += LOPARAM(pTrack->bParams);
            if (pTrack->nVolume > AUDIO_MAX_VOLUME)
                pTrack->nVolume = AUDIO_MAX_VOLUME;
            pTrack->nOutVolume = pTrack->nVolume;
            pTrack->bControl |= AUDIO_CTRL_VOLUME;
        }
        break;
    case 0xB:
        /* fine volume slide down */
        if (!Player.nFrame) {
            pTrack->nVolume -= LOPARAM(pTrack->bParams);
            if (pTrack->nVolume < AUDIO_MIN_VOLUME)
                pTrack->nVolume = AUDIO_MIN_VOLUME;
            pTrack->nOutVolume = pTrack->nVolume;
            pTrack->bControl |= AUDIO_CTRL_VOLUME;
        }
        break;
    case 0xC:
        /* note cut */
        if (Player.nFrame == LOPARAM(pTrack->bParams)) {
            pTrack->nVolume = pTrack->nOutVolume = 0;
            pTrack->bControl |= AUDIO_CTRL_VOLUME;
        }
        break;
    case 0xD:
        /* note delay */
        pTrack->bControl &= AUDIO_CTRL_KEYOFF;
        if (Player.nFrame == LOPARAM(pTrack->bParams)) {
            RetrigNote(pTrack);
            pTrack->bControl |= (AUDIO_CTRL_VOLUME | AUDIO_CTRL_PANNING);
        }
        break;
    case 0xE:
        /* pattern delay */
        if (!Player.nFrame) {
            if (!(Player.wControl & AUDIO_PLAYER_DELAY)) {
                Player.nPatternDelay = LOPARAM(pTrack->bParams) + 1;
                Player.wControl |= AUDIO_PLAYER_DELAY;
            }
        }
        break;
    case 0xF:
        /* TODO: funk it! not implemented */
        break;
    }
}

static VOID OnSetSpeed(PTRACK pTrack)
{
    if (!Player.nFrame) {
        if (pTrack->bParams >= 0x20) {
            Player.nBPM = pTrack->bParams;
            Player.wControl |= AUDIO_PLAYER_BPM;
        }
        else if (pTrack->bParams >= 0x01) {
            Player.nTempo = pTrack->bParams;
        }
    }
}

static VOID OnSetGlobalVolume(PTRACK pTrack)
{
    if (!Player.nFrame) {
        if ((Player.nVolume = pTrack->bParams) > AUDIO_MAX_VOLUME)
            Player.nVolume = AUDIO_MAX_VOLUME;
        Player.wControl |= AUDIO_PLAYER_VOLUME;
    }
}

static VOID OnGlobalVolumeSlide(PTRACK pTrack)
{
    if (!Player.nFrame) {
        if (pTrack->bParams != 0x00) {
            Player.nVolumeRate = pTrack->bParams;
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

static VOID OnKeyOff(PTRACK pTrack)
{
    if (Player.nFrame == pTrack->bParams) {
        StopNote(pTrack);
    }
}

static VOID OnSetEnvelope(PTRACK pTrack)
{
    /* TODO: set envelope position not implemented */
    if (pTrack != NULL) {
    }
}

static VOID OnPanningSlide(PTRACK pTrack)
{
    if (!Player.nFrame) {
        if (pTrack->bParams != 0x00) {
            pTrack->bPanningSlide = pTrack->bParams;
        }
    }
    else {
        if (HIPARAM(pTrack->bPanningSlide)) {
            pTrack->nPanning += HIPARAM(pTrack->bPanningSlide);
            if (pTrack->nPanning > AUDIO_MAX_PANNING)
                pTrack->nPanning = AUDIO_MAX_PANNING;
        }
        else {
            pTrack->nPanning -= LOPARAM(pTrack->bPanningSlide);
            if (pTrack->nPanning < AUDIO_MIN_PANNING)
                pTrack->nPanning = AUDIO_MIN_PANNING;
        }
        pTrack->bControl |= AUDIO_CTRL_PANNING;
    }
}

static VOID OnMultiRetrig(PTRACK pTrack)
{
    if (!Player.nFrame) {
        if (HIPARAM(pTrack->bParams))
            pTrack->nRetrigType = HIPARAM(pTrack->bParams);
        if (LOPARAM(pTrack->bParams))
            pTrack->nRetrigInterval = LOPARAM(pTrack->bParams);
    }
    else if (++pTrack->nRetrigFrame >= pTrack->nRetrigInterval) {
        pTrack->nRetrigFrame = 0;
        pTrack->nVolume += aRetrigTable[pTrack->nRetrigType];
        pTrack->nVolume *= aRetrigTable[pTrack->nRetrigType + 16];
        pTrack->nVolume >>= 4;
        pTrack->nVolume = CLIP(pTrack->nVolume,
            AUDIO_MIN_VOLUME, AUDIO_MAX_VOLUME);
        if (pTrack->nVolumeCmd >= 0x10 && pTrack->nVolumeCmd <= 0x50) {
            pTrack->nVolume = pTrack->nVolumeCmd - 0x10;
            pTrack->bControl |= AUDIO_CTRL_VOLUME;
        }
        if (pTrack->nVolumeCmd >= 0xC0 && pTrack->nVolumeCmd <= 0xCF) {
            pTrack->nPanning = LOPARAM(pTrack->nVolumeCmd) << 4;
            pTrack->bControl |= AUDIO_CTRL_PANNING;
        }
        pTrack->nOutVolume = pTrack->nVolume;
        pTrack->bControl |= AUDIO_CTRL_VOLUME;
        PlayNote(pTrack);
    }
}

static VOID OnTremor(PTRACK pTrack)
{
    if (!Player.nFrame) {
        if (pTrack->bParams) {
            pTrack->bTremorParms = pTrack->bParams;
        }
    }
    if (!pTrack->nTremorFrame--) {
        pTrack->nTremorFrame = pTrack->bTremorOnOff ?
            LOPARAM(pTrack->bTremorParms) :
            HIPARAM(pTrack->bTremorParms);
        pTrack->bTremorOnOff = !pTrack->bTremorOnOff;
    }
    pTrack->nOutVolume = pTrack->bTremorOnOff ? pTrack->nVolume : 0x00;
    pTrack->bControl |= AUDIO_CTRL_VOLUME;
}

static VOID OnExtraFinePorta(PTRACK pTrack)
{
    if (!Player.nFrame) {
        if (HIPARAM(pTrack->bParams) == 0x1) {
            if (LOPARAM(pTrack->bParams))
                pTrack->nExtraPortaUp = LOPARAM(pTrack->bParams);
            pTrack->nPeriod -= pTrack->nExtraPortaUp;
            if (pTrack->nPeriod < AUDIO_MIN_PERIOD)
                pTrack->nPeriod = AUDIO_MIN_PERIOD;
            pTrack->nOutPeriod = pTrack->nPeriod;
            pTrack->bControl |= AUDIO_CTRL_PITCH;
        }
        else if (HIPARAM(pTrack->bParams) == 0x2) {
            if (LOPARAM(pTrack->bParams))
                pTrack->nExtraPortaDown = LOPARAM(pTrack->bParams);
            pTrack->nPeriod += pTrack->nExtraPortaDown;
            if (pTrack->nPeriod > AUDIO_MAX_PERIOD)
                pTrack->nPeriod = AUDIO_MAX_PERIOD;
            pTrack->nOutPeriod = pTrack->nPeriod;
            pTrack->bControl |= AUDIO_CTRL_PITCH;
        }
    }
}

static VOID OnNothing(PTRACK pTrack)
{
    /* does nothing, just avoid compiler warnings */
    if (pTrack != NULL) {
    }
}

static VOID ExecNoteCmd(PTRACK pTrack)
{
    static VOID (*CommandProcTable[36])(PTRACK) =
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
        OnNothing               /* Zxx */
    };

    if ((pTrack->nCommand || pTrack->bParams) && pTrack->nCommand < 36) {
        (*CommandProcTable[pTrack->nCommand]) (pTrack);
    }
}

static VOID ExecVolumeCmd(PTRACK pTrack)
{
    UINT nCommand;

    nCommand = pTrack->nVolumeCmd;
    if (nCommand >= 0x10 && nCommand <= 0x50) {
        /* set volume */
        if (!Player.nFrame) {
            pTrack->nVolume = nCommand - 0x10;
            pTrack->nOutVolume = pTrack->nVolume;
            pTrack->bControl |= AUDIO_CTRL_VOLUME;
        }
    }
    else if (nCommand >= 0x60 && nCommand <= 0x6F) {
        /* volume slide down */
        if (Player.nFrame) {
            pTrack->nVolume -= LOPARAM(nCommand);
            if (pTrack->nVolume < AUDIO_MIN_VOLUME)
                pTrack->nVolume = AUDIO_MIN_VOLUME;
            pTrack->nOutVolume = pTrack->nVolume;
            pTrack->bControl |= AUDIO_CTRL_VOLUME;
        }
    }
    else if (nCommand >= 0x70 && nCommand <= 0x7F) {
        /* volume slide up */
        if (Player.nFrame) {
            pTrack->nVolume += LOPARAM(nCommand);
            if (pTrack->nVolume > AUDIO_MAX_VOLUME)
                pTrack->nVolume = AUDIO_MAX_VOLUME;
            pTrack->nOutVolume = pTrack->nVolume;
            pTrack->bControl |= AUDIO_CTRL_VOLUME;
        }
    }
    else if (nCommand >= 0x80 && nCommand <= 0x8F) {
        /* fine volume slide down */
        if (!Player.nFrame) {
            pTrack->nVolume -= LOPARAM(nCommand);
            if (pTrack->nVolume < AUDIO_MIN_VOLUME)
                pTrack->nVolume = AUDIO_MIN_VOLUME;
            pTrack->nOutVolume = pTrack->nVolume;
            pTrack->bControl |= AUDIO_CTRL_VOLUME;
        }
    }
    else if (nCommand >= 0x90 && nCommand <= 0x9F) {
        /* fine volume slide up */
        if (!Player.nFrame) {
            pTrack->nVolume += LOPARAM(nCommand);
            if (pTrack->nVolume > AUDIO_MAX_VOLUME)
                pTrack->nVolume = AUDIO_MAX_VOLUME;
            pTrack->nOutVolume = pTrack->nVolume;
            pTrack->bControl |= AUDIO_CTRL_VOLUME;
        }
    }
    else if (nCommand >= 0xA0 && nCommand <= 0xAF) {
        /* set vibrato speed */
        if (!Player.nFrame) {
            if (LOPARAM(nCommand)) {
                pTrack->nVibratoRate = LOPARAM(nCommand) << 2;
            }
        }
    }
    else if (nCommand >= 0xB0 && nCommand <= 0xBF) {
        /* vibrato */
        if (!Player.nFrame) {
            if (LOPARAM(nCommand)) {
                pTrack->nVibratoDepth = LOPARAM(nCommand);
            }
        }
        else {
            OnVibrato(pTrack);
        }
    }
    else if (nCommand >= 0xC0 && nCommand <= 0xCF) {
        /* set coarse panning */
        if (!Player.nFrame) {
            pTrack->nPanning = LOPARAM(nCommand) << 4;
            pTrack->bControl |= AUDIO_CTRL_PANNING;
        }
    }
    else if (nCommand >= 0xD0 && nCommand <= 0xDF) {
        /* panning slide left */
        if (Player.nFrame) {
            pTrack->nPanning -= LOPARAM(nCommand);
            if (pTrack->nPanning < AUDIO_MIN_PANNING)
                pTrack->nPanning = AUDIO_MIN_PANNING;
            pTrack->bControl |= AUDIO_CTRL_PANNING;
        }
    }
    else if (nCommand >= 0xE0 && nCommand <= 0xEF) {
        /* panning slide right */
        if (Player.nFrame) {
            pTrack->nPanning += LOPARAM(nCommand);
            if (pTrack->nPanning > AUDIO_MAX_PANNING)
                pTrack->nPanning = AUDIO_MAX_PANNING;
            pTrack->bControl |= AUDIO_CTRL_PANNING;
        }
    }
    else if (nCommand >= 0xF0 && nCommand <= 0xFF) {
        /* tone portamento */
        if (Player.nFrame)
            OnTonePorta(pTrack);
    }
}

static VOID StartEnvelopes(PTRACK pTrack)
{
    PAUDIOPATCH pPatch;

    /* reset vibrato and tremolo waves */
    if (!(pTrack->bWaveCtrl & 0x04))
        pTrack->nVibratoFrame = 0;
    if (!(pTrack->bWaveCtrl & 0x40))
        pTrack->nTremoloFrame = 0;

    /* reset retrig and tremor frames */
    pTrack->nRetrigFrame = 0;
    pTrack->nTremorFrame = 0;
    pTrack->bTremorOnOff = 0;

    pPatch = pTrack->pPatch;

    /* start volume envelope */
    if (pPatch != NULL && (pPatch->Volume.wFlags & AUDIO_ENVELOPE_ON)) {
        pTrack->nVolumeFrame = -1;
        pTrack->nVolumePoint = 0;
    }

    /* start panning envelope */
    if (pPatch != NULL && (pPatch->Panning.wFlags & AUDIO_ENVELOPE_ON)) {
        pTrack->nPanningFrame = -1;
        pTrack->nPanningPoint = 0;
    }

    /* start volume fadeout */
    if (pPatch != NULL)
        pTrack->nVolumeFadeout = pPatch->nVolumeFadeout;
    pTrack->nVolumeFade = 0x7FFF;

    /* start automatic vibrato */
    if (pPatch != NULL && pPatch->nVibratoDepth) {
        pTrack->nAutoVibratoFrame = 0;
        if (pPatch->nVibratoSweep) {
            pTrack->nAutoVibratoSlope =
                ((int) pPatch->nVibratoDepth << 8) / pPatch->nVibratoSweep;
            pTrack->nAutoVibratoValue = 0;
        }
        else {
            pTrack->nAutoVibratoSlope = 0;
            pTrack->nAutoVibratoValue =
                ((int) pPatch->nVibratoDepth << 8);
        }
    }
}

static VOID UpdateEnvelopes(PTRACK pTrack)
{
    PAUDIOPATCH pPatch;
    PAUDIOPOINT pPoints;
    int nFrames, nValue;

    /* get patch structure alias */
    pPatch = pTrack->pPatch;

    /* process volume fadeout */
    if (pPatch != NULL && !pTrack->fKeyOn) {
        if ((pTrack->nVolumeFade -= pTrack->nVolumeFadeout) < 0) {
            pTrack->nVolumeFadeout = 0;
            pTrack->nVolumeFade = 0;
        }
        pTrack->bControl |= AUDIO_CTRL_VOLUME;
    }

    /* process volume envelope */
    if (pPatch != NULL && (pPatch->Volume.wFlags & AUDIO_ENVELOPE_ON)) {
        pPoints = pPatch->Volume.aEnvelope;
        if (++pTrack->nVolumeFrame >= pPoints[pTrack->nVolumePoint].nFrame) {
            if ((pPatch->Volume.wFlags & AUDIO_ENVELOPE_SUSTAIN) &&
                (pTrack->nVolumePoint == pPatch->Volume.nSustain) &&
                pTrack->fKeyOn) {
                pTrack->nVolumeFrame = pPoints[pTrack->nVolumePoint].nFrame;
                pTrack->nVolumeValue = (int) pPoints[pTrack->nVolumePoint].nValue << 8;
            }
            else {
                if ((pPatch->Volume.wFlags & AUDIO_ENVELOPE_LOOP) &&
                    (pTrack->nVolumePoint == pPatch->Volume.nLoopEnd)) {
                    pTrack->nVolumePoint = pPatch->Volume.nLoopStart;
                }
                pTrack->nVolumeFrame = pPoints[pTrack->nVolumePoint].nFrame;
                pTrack->nVolumeValue = (int) pPoints[pTrack->nVolumePoint].nValue << 8;
                if (pTrack->nVolumePoint + 1 >= pPatch->Volume.nPoints) {
                    pTrack->nVolumeSlope = 0;
                }
                else {
                    if ((nFrames = pPoints[pTrack->nVolumePoint + 1].nFrame -
                            pPoints[pTrack->nVolumePoint].nFrame) <= 0)
                        pTrack->nVolumeSlope = 0;
                    else {
                        pTrack->nVolumeSlope =
                            (((int) pPoints[pTrack->nVolumePoint + 1].nValue -
                                (int) pPoints[pTrack->nVolumePoint].nValue) << 8) / nFrames;
                    }
                    pTrack->nVolumePoint++;
                }
            }
        }
        else {
            pTrack->nVolumeValue += pTrack->nVolumeSlope;
            pTrack->nVolumeValue = CLIP(pTrack->nVolumeValue, 0, 64 * 256);
        }
        pTrack->nFinalVolume = (((LONG) (pTrack->nVolumeValue >> 8) *
            pTrack->nOutVolume) * pTrack->nVolumeFade) >> 21;
        pTrack->bControl |= AUDIO_CTRL_VOLUME;
    }
    else {
        pTrack->nFinalVolume = pTrack->nOutVolume;
        if (pTrack->nVolumeFade != 0x7FFF)
            pTrack->nFinalVolume = ((LONG) pTrack->nFinalVolume * 
                pTrack->nVolumeFade) >> 15;
    }

    /* process panning envelope */
    if (pPatch != NULL && (pPatch->Panning.wFlags & AUDIO_ENVELOPE_ON)) {
        pPoints = pPatch->Panning.aEnvelope;
        if (++pTrack->nPanningFrame >= pPoints[pTrack->nPanningPoint].nFrame) {
            if ((pPatch->Panning.wFlags & AUDIO_ENVELOPE_SUSTAIN) &&
                (pTrack->nPanningPoint == pPatch->Panning.nSustain) &&
                pTrack->fKeyOn) {
                pTrack->nPanningFrame = pPoints[pTrack->nPanningPoint].nFrame;
                pTrack->nPanningValue = (int) pPoints[pTrack->nPanningPoint].nValue << 8;
            }
            else {
                if ((pPatch->Panning.wFlags & AUDIO_ENVELOPE_LOOP) &&
                    (pTrack->nPanningPoint == pPatch->Panning.nLoopEnd)) {
                    pTrack->nPanningPoint = pPatch->Panning.nLoopStart;
                }
                pTrack->nPanningFrame = pPoints[pTrack->nPanningPoint].nFrame;
                pTrack->nPanningValue = (int) pPoints[pTrack->nPanningPoint].nValue << 8;
                if (pTrack->nPanningPoint + 1 >= pPatch->Panning.nPoints) {
                    pTrack->nPanningSlope = 0;
                }
                else {
                    if ((nFrames = pPoints[pTrack->nPanningPoint + 1].nFrame -
                            pPoints[pTrack->nPanningPoint].nFrame) <= 0)
                        pTrack->nPanningSlope = 0;
                    else {
                        pTrack->nPanningSlope =
                            (((int) pPoints[pTrack->nPanningPoint + 1].nValue -
                                (int) pPoints[pTrack->nPanningPoint].nValue) << 8) / nFrames;
                    }
                    pTrack->nPanningPoint++;
                }
            }
        }
        else {
            pTrack->nPanningValue += pTrack->nPanningSlope;
            pTrack->nPanningValue = CLIP(pTrack->nPanningValue, 0, 64 * 256);
        }
        pTrack->nFinalPanning = pTrack->nPanning +
            ((((128L - ABS(pTrack->nPanning - 128)) << 3) *
                (pTrack->nPanningValue - 32 * 256L)) >> 16);
        pTrack->bControl |= AUDIO_CTRL_PANNING;
    }
    else {
        pTrack->nFinalPanning = pTrack->nPanning;
    }

    /* process automatic vibrato */
    if (pPatch != NULL && pPatch->nVibratoDepth != 0) {
        if (pTrack->fKeyOn && pTrack->nAutoVibratoSlope) {
            pTrack->nAutoVibratoValue += pTrack->nAutoVibratoSlope;
            if (pTrack->nAutoVibratoValue > ((int) pPatch->nVibratoDepth << 8)) {
                pTrack->nAutoVibratoValue = (int) pPatch->nVibratoDepth << 8;
                pTrack->nAutoVibratoSlope = 0;
            }
        }
        pTrack->nAutoVibratoFrame += pPatch->nVibratoRate;
        nFrames = (UCHAR) pTrack->nAutoVibratoFrame;
        switch (pPatch->nVibratoType) {
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
        pTrack->nFinalPeriod = pTrack->nOutPeriod +
            ((((LONG) nValue << 2) * pTrack->nAutoVibratoValue) >> 16);
        pTrack->nFinalPeriod = CLIP(pTrack->nFinalPeriod,
            AUDIO_MIN_PERIOD, AUDIO_MAX_PERIOD);
        pTrack->bControl |= AUDIO_CTRL_PITCH;
    }
    else {
        pTrack->nFinalPeriod = pTrack->nOutPeriod;
    }
}

static VOID PlayNote(PTRACK pTrack)
{
    PAUDIOPATCH pPatch;
    PAUDIOSAMPLE pSample;

    pTrack->fKeyOn = (pTrack->nNote >= 1 && pTrack->nNote <= AUDIO_MAX_NOTES);
    if (pTrack->fKeyOn && (pPatch = pTrack->pPatch) != NULL) {
        pTrack->nSample = pPatch->aSampleNumber[pTrack->nNote - 1];
        if (pTrack->nSample < pPatch->nSamples) {
            pTrack->pSample = &pPatch->aSampleTable[pTrack->nSample];
            pSample = pTrack->pSample;
            pTrack->nRelativeNote = (signed char) pSample->nRelativeNote;
            pTrack->nFinetune = (signed char) pSample->nFinetune;
            if (pTrack->nCommand != 0x03 && pTrack->nCommand != 0x05 &&
                (pTrack->nVolumeCmd & 0xF0) != 0xF0) {
                pTrack->nPeriod = pTrack->nOutPeriod =
                    GetPeriodValue(pTrack->nNote,
                    pTrack->nRelativeNote, pTrack->nFinetune);
                pTrack->bControl |= (AUDIO_CTRL_PITCH | AUDIO_CTRL_KEYON);
            }
        }
        else {
            pTrack->pSample = NULL;
        }
    }
}

static VOID StopNote(PTRACK pTrack)
{
    pTrack->fKeyOn = 0;
    if (pTrack->pPatch != NULL) {
        if (!(pTrack->pPatch->Volume.wFlags & AUDIO_ENVELOPE_ON)) {
            pTrack->nVolume = pTrack->nOutVolume = 0;
            pTrack->bControl |= (AUDIO_CTRL_VOLUME | AUDIO_CTRL_KEYOFF);
        }
    }
    else {
        pTrack->bControl |= AUDIO_CTRL_KEYOFF;
    }
}

static VOID RetrigNote(PTRACK pTrack)
{
    PlayNote(pTrack);
    StartEnvelopes(pTrack);
}

static VOID GetPatternNote(PNOTE pNote)
{
    UCHAR fPacking;

#define GETBYTE *Player.pData++

    fPacking = (Player.pData != NULL) ?
        (Player.pData[0] & AUDIO_PATTERN_PACKED ? GETBYTE : 0xFF) : 0x00;
    pNote->nNote = (fPacking & AUDIO_PATTERN_NOTE) ? GETBYTE : 0x00;
    pNote->nPatch = (fPacking & AUDIO_PATTERN_SAMPLE) ? GETBYTE : 0x00;
    pNote->nVolume = (fPacking & AUDIO_PATTERN_VOLUME) ? GETBYTE : 0x00;
    pNote->nCommand = (fPacking & AUDIO_PATTERN_COMMAND) ? GETBYTE : 0x00;
    pNote->bParams = (fPacking & AUDIO_PATTERN_PARAMS) ? GETBYTE : 0x00;
}

static VOID GetTrackNote(PTRACK pTrack)
{
    static NOTE Note;

    /* read next packed note from pattern */
    GetPatternNote(&Note);

    /* reset frequency for vibrato and arpeggio commands */
    if (pTrack->nCommand == 0x04 || pTrack->nCommand == 0x06) {
        if (Note.nCommand != 0x04 && Note.nCommand != 0x06) {
            pTrack->nOutPeriod = pTrack->nPeriod;
            pTrack->bControl |= AUDIO_CTRL_PITCH;
        }
    }
    else if (pTrack->nCommand == 0x00 && pTrack->bParams != 0x00) {
        pTrack->nOutPeriod = pTrack->nPeriod;
        pTrack->bControl |= AUDIO_CTRL_PITCH;
    }

    /* assign volume and effect commands */
    pTrack->nVolumeCmd = Note.nVolume;
    pTrack->nCommand = Note.nCommand;
    pTrack->bParams = Note.bParams;

    /* change default patch instrument */
    if (Note.nPatch >= 1 && Note.nPatch <= Player.pModule->nPatches) {
        pTrack->nPatch = Note.nPatch;
        pTrack->pPatch = &Player.pModule->aPatchTable[pTrack->nPatch - 1];
    }

    /* new note pressed? */
    if (Note.nNote >= 1 && Note.nNote <= AUDIO_MAX_NOTES) {
        pTrack->nNote = Note.nNote;
        PlayNote(pTrack);
        if (Note.nPatch != 0 && pTrack->pSample != NULL)
            StartEnvelopes(pTrack);
    }
    else if (Note.nNote != 0) {
        StopNote(pTrack);
    }

    /* use default sample's volume and panning? */
    if (Note.nPatch != 0 && pTrack->pSample != NULL) {
        pTrack->nVolume = pTrack->nOutVolume = pTrack->pSample->nVolume;
        pTrack->bControl |= AUDIO_CTRL_VOLUME;
        if (!(Player.wFlags & AUDIO_MODULE_PANNING)) {
            pTrack->nPanning = pTrack->pSample->nPanning;
            pTrack->bControl |= AUDIO_CTRL_PANNING;
        }
    }
}

static VOID SendNoteMesg(HAC hVoice, PTRACK pTrack)
{
    if (pTrack->bControl & (AUDIO_CTRL_KEYON | AUDIO_CTRL_TOUCH)) {
        if (pTrack->pSample != NULL) {
            APrimeVoice(hVoice, &pTrack->pSample->Wave);
            if (pTrack->bControl & AUDIO_CTRL_TOUCH)
                ASetVoicePosition(hVoice, pTrack->dwSampleOffset);
        }
    }
    if (pTrack->bControl & AUDIO_CTRL_KEYOFF) {
        AStopVoice(hVoice);
    }
    if (pTrack->bControl & AUDIO_CTRL_PITCH) {
        ASetVoiceFrequency(hVoice, GetFrequencyValue(pTrack->nFinalPeriod));
    }
    if (pTrack->bControl & AUDIO_CTRL_VOLUME) {
        ASetVoiceVolume(hVoice, (pTrack->nFinalVolume * Player.nVolume) >> 6);
    }
    if (pTrack->bControl & AUDIO_CTRL_PANNING) {
        ASetVoicePanning(hVoice, pTrack->nFinalPanning);
    }
    if (pTrack->bControl & AUDIO_CTRL_KEYON) {
        AStartVoice(hVoice);
    }
    pTrack->bControl = 0x00;
}

static VOID GetNextPatternRow(VOID)
{
    static NOTE Note;
    PAUDIOPATTERN pPattern;
    UINT n, m;

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
        if (Player.nOrder >= Player.pModule->nOrders) {
            Player.nOrder = Player.pModule->nRestart;
            if (Player.nOrder >= Player.pModule->nOrders) {
                Player.nOrder = 0x00;
                Player.wControl |= AUDIO_PLAYER_PAUSE;
                return;
            }
        }
        Player.nPattern = Player.pModule->aOrderTable[Player.nOrder];
        if (Player.nPattern < Player.pModule->nPatterns) {
            pPattern = &Player.pModule->aPatternTable[Player.nPattern];
            Player.nRows = pPattern->nRows;
            Player.pData = pPattern->pData;
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
            Player.pData = NULL;
        }
    }
    for (n = 0; n < Player.nTracks; n++) {
        GetTrackNote(&Player.aTracks[n]);
    }
}

static VOID AIAPI PlayNextFrame(VOID)
{
    UINT n;

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
UINT AIAPI APlayModule(PAUDIOMODULE pModule)
{
    UINT n;

    if (!(Player.wControl & AUDIO_PLAYER_ACTIVE)) {
        if (pModule != NULL) {
            memset(&Player, 0, sizeof(Player));
            Player.pModule = pModule;
            Player.nTracks = pModule->nTracks;
            Player.wFlags = pModule->wFlags;
            Player.nTempo = pModule->nTempo;
            Player.nBPM = pModule->nBPM;
            Player.nVolume = AUDIO_MAX_VOLUME;
            Player.wControl = AUDIO_PLAYER_ACTIVE | AUDIO_PLAYER_JUMP;
            for (n = 0; n < Player.nTracks; n++) {
                if (ACreateAudioVoice(&Player.aVoices[n]) != AUDIO_ERROR_NONE) {
                    AStopModule();
                    return AUDIO_ERROR_NOMEMORY;
                }
                Player.aTracks[n].nPanning = pModule->aPanningTable[n];
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
    UINT n;

    if (Player.wControl & AUDIO_PLAYER_ACTIVE) {
        for (n = 0; n < Player.nTracks; n++) {
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
    UINT n;

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

UINT AIAPI AGetModuleVolume(PUINT pnVolume)
{
    if (Player.wControl & AUDIO_PLAYER_ACTIVE) {
        if (pnVolume != NULL) {
            *pnVolume = Player.nVolume;
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_NOTSUPPORTED;
}

UINT AIAPI AGetModulePosition(PUINT pnOrder, PUINT pnRow)
{
    if (Player.wControl & AUDIO_PLAYER_ACTIVE) {
        if (pnOrder != NULL && pnRow != NULL) {
            *pnOrder = Player.nOrder;
            *pnRow = Player.nRow;
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_NOTSUPPORTED;
}

UINT AIAPI AGetModuleStatus(PBOOL pnStatus)
{
    if (Player.wControl & AUDIO_PLAYER_ACTIVE) {
        if (pnStatus != NULL) {
            *pnStatus = ((Player.wControl & AUDIO_PLAYER_PAUSE) != 0);
            return AUDIO_ERROR_NONE;
        }
        return AUDIO_ERROR_INVALPARAM;
    }
    return AUDIO_ERROR_NOTSUPPORTED;
}



UINT AIAPI AFreeModuleFile(PAUDIOMODULE pModule)
{
    PAUDIOPATTERN pPattern;
    PAUDIOPATCH pPatch;
    PAUDIOSAMPLE pSample;
    UINT n, m;

    if (pModule != NULL) {
        if ((pPattern = pModule->aPatternTable) != NULL) {
            for (n = 0; n < pModule->nPatterns; n++, pPattern++) {
                if (pPattern->pData != NULL)
                    free(pPattern->pData);
            }
            free(pModule->aPatternTable);
        }
        if ((pPatch = pModule->aPatchTable) != NULL) {
            for (n = 0; n < pModule->nPatches; n++, pPatch++) {
                pSample = pPatch->aSampleTable;
                for (m = 0; m < pPatch->nSamples; m++, pSample++) {
                    ADestroyAudioData(&pSample->Wave);
                }
                free(pPatch->aSampleTable);
            }
            free(pModule->aPatchTable);
        }
        free(pModule);
        return AUDIO_ERROR_NONE;
    }
    return AUDIO_ERROR_INVALPARAM;
}

UINT AIAPI ALoadModuleFile(PSZ pszFileName, PAUDIOMODULE *ppModule)
{
    extern UINT AIAPI ALoadModuleXM(PSZ, PAUDIOMODULE*);
    extern UINT AIAPI ALoadModuleS3M(PSZ, PAUDIOMODULE*);
    extern UINT AIAPI ALoadModuleMOD(PSZ, PAUDIOMODULE*);
    UINT nErrorCode;

    if (pszFileName != NULL && ppModule != NULL) {
        *ppModule = NULL;
        nErrorCode = ALoadModuleXM(pszFileName, ppModule);
        if (nErrorCode == AUDIO_ERROR_BADFILEFORMAT) {
            nErrorCode = ALoadModuleS3M(pszFileName, ppModule);
            if (nErrorCode == AUDIO_ERROR_BADFILEFORMAT) {
                nErrorCode = ALoadModuleMOD(pszFileName, ppModule);
            }
        }
        return nErrorCode;
    }
    return AUDIO_ERROR_INVALPARAM;
}
