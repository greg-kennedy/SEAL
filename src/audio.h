/*
 * $Id: audio.h 1.12 1996/06/16 00:51:03 chasan released $
 *
 * SEAL Synthetic Audio Library API Interface
 *
 * Copyright (C) 1995, 1996 Carlos Hasan. All Rights Reserved.
 *
 */

#ifndef __AUDIO_H
#define __AUDIO_H

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __FLAT__
#define AIAPI
#else
#define AIAPI __stdcall
#endif

#ifndef WINAPI

/* atomic data types definitions */
typedef void            VOID;
typedef char            CHAR;
typedef short           SHORT;
typedef int             INT;
typedef long            LONG;
typedef int             BOOL;
typedef unsigned char   UCHAR;
typedef unsigned short  USHORT;
typedef unsigned int    UINT;
typedef unsigned long   ULONG;
typedef VOID*           PVOID;
typedef CHAR*           PCHAR;
typedef SHORT*          PSHORT;
typedef INT*            PINT;
typedef LONG*           PLONG;
typedef BOOL*           PBOOL;
typedef UCHAR*          PUCHAR;
typedef USHORT*         PUSHORT;
typedef UINT*           PUINT;
typedef ULONG*          PULONG;
typedef CHAR*           PSZ;
typedef ULONG           HANDLE;

/* helper macros */
#define LOBYTE(s)       ((UCHAR)(s))
#define HIBYTE(s)       ((UCHAR)((USHORT)(s)>>8))
#define LOWORD(l)       ((USHORT)(l))
#define HIWORD(l)       ((USHORT)((ULONG)(l)>>16))
#define MAKEWORD(l,h)   ((USHORT)(((UCHAR)(l))|(((USHORT)((UCHAR)(h)))<<8)))
#define MAKELONG(l,h)   ((ULONG)(((USHORT)(l))|(((ULONG)((USHORT)(h)))<<16)))

#else
#ifndef __FLAT__

/* atomic data types definitions */
typedef char            CHAR;
typedef short           SHORT;
typedef unsigned char   UCHAR;
typedef unsigned short  USHORT;
typedef unsigned long   ULONG;
typedef SHORT*          PSHORT;
typedef BOOL*           PBOOL;
typedef UCHAR*          PUCHAR;
typedef USHORT*         PUSHORT;
typedef UINT*           PUINT;
typedef ULONG*          PULONG;
typedef CHAR*           PSZ;

#endif
#endif


/* audio system version number */
#define AUDIO_SYSTEM_VERSION            0x0100

/* audio capabilities bit fields definitions */
#define AUDIO_FORMAT_1M08               0x00000001
#define AUDIO_FORMAT_1S08               0x00000002
#define AUDIO_FORMAT_1M16               0x00000004
#define AUDIO_FORMAT_1S16               0x00000008
#define AUDIO_FORMAT_2M08               0x00000010
#define AUDIO_FORMAT_2S08               0x00000020
#define AUDIO_FORMAT_2M16               0x00000040
#define AUDIO_FORMAT_2S16               0x00000080
#define AUDIO_FORMAT_4M08               0x00000100
#define AUDIO_FORMAT_4S08               0x00000200
#define AUDIO_FORMAT_4M16               0x00000400
#define AUDIO_FORMAT_4S16               0x00000800

/* audio format bit fields defines for devices and waveforms */
#define AUDIO_FORMAT_8BITS              0x0000
#define AUDIO_FORMAT_16BITS             0x0001
#define AUDIO_FORMAT_LOOP               0x0010
#define AUDIO_FORMAT_BIDILOOP           0x0020
#define AUDIO_FORMAT_REVERSE            0x0080
#define AUDIO_FORMAT_MONO               0x0000
#define AUDIO_FORMAT_STEREO             0x0100
#define AUDIO_FORMAT_FILTER             0x8000

/* audio resource limits defines */
#define AUDIO_MAX_VOICES                32
#define AUDIO_MAX_SAMPLES               16
#define AUDIO_MAX_PATCHES               128
#define AUDIO_MAX_PATTERNS              256
#define AUDIO_MAX_ORDERS                256
#define AUDIO_MAX_NOTES                 96
#define AUDIO_MAX_POINTS                12
#define AUDIO_MIN_PERIOD                1
#define AUDIO_MAX_PERIOD                31999
#define AUDIO_MIN_VOLUME                0x00
#define AUDIO_MAX_VOLUME                0x40
#define AUDIO_MIN_PANNING               0x00
#define AUDIO_MAX_PANNING               0xFF
#define AUDIO_MIN_POSITION              0x00000000L
#define AUDIO_MAX_POSITION              0x00100000L
#define AUDIO_MIN_FREQUENCY             0x00000200L
#define AUDIO_MAX_FREQUENCY             0x00080000L

/* audio error code defines */
#define AUDIO_ERROR_NONE                0x0000
#define AUDIO_ERROR_INVALHANDLE         0x0001
#define AUDIO_ERROR_INVALPARAM          0x0002
#define AUDIO_ERROR_NOTSUPPORTED        0x0003
#define AUDIO_ERROR_BADDEVICEID         0x0004
#define AUDIO_ERROR_NODEVICE            0x0005
#define AUDIO_ERROR_DEVICEBUSY          0x0006
#define AUDIO_ERROR_BADFORMAT           0x0007
#define AUDIO_ERROR_NOMEMORY            0x0008
#define AUDIO_ERROR_NODRAMMEMORY        0x0009
#define AUDIO_ERROR_FILENOTFOUND        0x000A
#define AUDIO_ERROR_BADFILEFORMAT       0x000B
#define AUDIO_LAST_ERROR                0x000B

/* audio device identifiers */
#define AUDIO_DEVICE_NONE               0x0000
#define AUDIO_DEVICE_MAPPER             0xFFFF

/* audio product identifiers */
#define AUDIO_PRODUCT_NONE              0x0000
#define AUDIO_PRODUCT_SB                0x0001
#define AUDIO_PRODUCT_SB15              0x0002
#define AUDIO_PRODUCT_SB20              0x0003
#define AUDIO_PRODUCT_SBPRO             0x0004
#define AUDIO_PRODUCT_SB16              0x0005
#define AUDIO_PRODUCT_AWE32             0x0006
#define AUDIO_PRODUCT_WSS               0x0007
#define AUDIO_PRODUCT_ESS               0x0008
#define AUDIO_PRODUCT_GUS               0x0009
#define AUDIO_PRODUCT_GUSDB             0x000A
#define AUDIO_PRODUCT_GUSMAX            0x000B
#define AUDIO_PRODUCT_IWAVE             0x000C
#define AUDIO_PRODUCT_PAS               0x000D
#define AUDIO_PRODUCT_PAS16             0x000E
#define AUDIO_PRODUCT_ARIA              0x000F
#define AUDIO_PRODUCT_WINDOWS           0x0100
#define AUDIO_PRODUCT_LINUX             0x0101
#define AUDIO_PRODUCT_SPARC             0x0102
#define AUDIO_PRODUCT_SGI               0x0103

/* audio envelope bit fields */
#define AUDIO_ENVELOPE_ON               0x0001
#define AUDIO_ENVELOPE_SUSTAIN          0x0002
#define AUDIO_ENVELOPE_LOOP             0x0004

/* audio pattern bit fields */
#define AUDIO_PATTERN_PACKED            0x0080
#define AUDIO_PATTERN_NOTE              0x0001
#define AUDIO_PATTERN_SAMPLE            0x0002
#define AUDIO_PATTERN_VOLUME            0x0004
#define AUDIO_PATTERN_COMMAND           0x0008
#define AUDIO_PATTERN_PARAMS            0x0010

/* audio module bit fields */
#define AUDIO_MODULE_AMIGA              0x0000
#define AUDIO_MODULE_LINEAR             0x0001
#define AUDIO_MODULE_PANNING            0x8000

#pragma pack(1)

/* audio capabilities structure */
typedef struct {
    USHORT  wProductId;                         /* product identifier */
    CHAR    szProductName[30];                  /* product name */
    ULONG   dwFormats;                          /* formats supported */
} AUDIOCAPS, *PAUDIOCAPS;

/* audio format structure */
typedef struct {
    UINT    nDeviceId;                          /* device identifier */
    USHORT  wFormat;                            /* playback format */
    USHORT  nSampleRate;                        /* sampling frequency */
} AUDIOINFO, *PAUDIOINFO;

/* audio waveform structure */
typedef struct {
    PUCHAR  pData;                              /* data pointer */
    ULONG   dwHandle;                           /* waveform handle */
    ULONG   dwLength;                           /* waveform length */
    ULONG   dwLoopStart;                        /* loop start point */
    ULONG   dwLoopEnd;                          /* loop end point */
    USHORT  nSampleRate;                        /* sampling rate */
    USHORT  wFormat;                            /* format bits */
} AUDIOWAVE, *PAUDIOWAVE;


/* audio envelope point structure */
typedef struct {
    USHORT  nFrame;                             /* envelope frame */
    USHORT  nValue;                             /* envelope value */
} AUDIOPOINT, *PAUDIOPOINT;

/* audio envelope structure */
typedef struct {
    AUDIOPOINT aEnvelope[AUDIO_MAX_POINTS];     /* envelope points */
    UCHAR   nPoints;                            /* number of points */
    UCHAR   nSustain;                           /* sustain point */
    UCHAR   nLoopStart;                         /* loop start point */
    UCHAR   nLoopEnd;                           /* loop end point */
    USHORT  wFlags;                             /* envelope flags */
    USHORT  nSpeed;                             /* envelope speed */
} AUDIOENVELOPE, *PAUDIOENVELOPE;

/* audio sample structure */
typedef struct {
    CHAR    szSampleName[32];                   /* sample name */
    UCHAR   nVolume;                            /* default volume */
    UCHAR   nPanning;                           /* default panning */
    UCHAR   nRelativeNote;                      /* relative note */
    UCHAR   nFinetune;                          /* finetune */
    AUDIOWAVE Wave;                             /* waveform handle */
} AUDIOSAMPLE, *PAUDIOSAMPLE;

/* audio patch structure */
typedef struct {
    CHAR    szPatchName[32];                    /* patch name */
    UCHAR   aSampleNumber[AUDIO_MAX_NOTES];     /* multi-sample table */
    USHORT  nSamples;                           /* number of samples */
    UCHAR   nVibratoType;                       /* vibrato type */
    UCHAR   nVibratoSweep;                      /* vibrato sweep */
    UCHAR   nVibratoDepth;                      /* vibrato depth */
    UCHAR   nVibratoRate;                       /* vibrato rate */
    USHORT  nVolumeFadeout;                     /* volume fadeout */
    AUDIOENVELOPE Volume;                       /* volume envelope */
    AUDIOENVELOPE Panning;                      /* panning envelope */
    PAUDIOSAMPLE aSampleTable;                  /* sample table */
} AUDIOPATCH, *PAUDIOPATCH;

/* audio pattern structure */
typedef struct {
    USHORT  nPacking;                           /* packing type */
    USHORT  nTracks;                            /* number of tracks */
    USHORT  nRows;                              /* number of rows */
    USHORT  nSize;                              /* data size */
    PUCHAR  pData;                              /* data pointer */
} AUDIOPATTERN, *PAUDIOPATTERN;

/* audio module structure */
typedef struct {
    CHAR    szModuleName[32];                   /* module name */
    USHORT  wFlags;                             /* module flags */
    USHORT  nOrders;                            /* number of orders */
    USHORT  nRestart;                           /* restart position */
    USHORT  nTracks;                            /* number of tracks */
    USHORT  nPatterns;                          /* number of patterns */
    USHORT  nPatches;                           /* number of patches */
    USHORT  nTempo;                             /* initial tempo */
    USHORT  nBPM;                               /* initial BPM */
    UCHAR   aOrderTable[AUDIO_MAX_ORDERS];      /* order table */
    UCHAR   aPanningTable[AUDIO_MAX_VOICES];    /* panning table */
    PAUDIOPATTERN aPatternTable;                /* pattern table */
    PAUDIOPATCH aPatchTable;                    /* patch table */
} AUDIOMODULE, *PAUDIOMODULE;

/* audio callback function defines */
typedef VOID (AIAPI* PFNAUDIOWAVE)(PUCHAR, UINT);
typedef VOID (AIAPI* PFNAUDIOTIMER)(VOID);

/* audio handle defines */
typedef HANDLE  HAC;
typedef HAC*    PHAC;

#pragma pack()


/* audio interface API prototypes */
UINT AIAPI AInitialize(VOID);
UINT AIAPI AGetVersion(VOID);
UINT AIAPI AGetAudioNumDevs(VOID);
UINT AIAPI AGetAudioDevCaps(UINT nDeviceId, PAUDIOCAPS pCaps);
UINT AIAPI AGetErrorText(UINT nErrorCode, PSZ pText, UINT nSize);

UINT AIAPI APingAudio(PUINT pnDeviceId);
UINT AIAPI AOpenAudio(PAUDIOINFO pInfo);
UINT AIAPI ACloseAudio(VOID);
UINT AIAPI AUpdateAudio(VOID);

UINT AIAPI AOpenVoices(UINT nVoices);
UINT AIAPI ACloseVoices(VOID);

UINT AIAPI ASetAudioCallback(PFNAUDIOWAVE pfnAudioWave);
UINT AIAPI ASetAudioTimerProc(PFNAUDIOTIMER pfnAudioTimer);
UINT AIAPI ASetAudioTimerRate(UINT nTimerRate);

LONG AIAPI AGetAudioDataAvail(VOID);
UINT AIAPI ACreateAudioData(PAUDIOWAVE pWave);
UINT AIAPI ADestroyAudioData(PAUDIOWAVE pWave);
UINT AIAPI AWriteAudioData(PAUDIOWAVE pWave, ULONG dwOffset, UINT nCount);

UINT AIAPI ACreateAudioVoice(PHAC phVoice);
UINT AIAPI ADestroyAudioVoice(HAC hVoice);

UINT AIAPI APlayVoice(HAC hVoice, PAUDIOWAVE pWave);
UINT AIAPI APrimeVoice(HAC hVoice, PAUDIOWAVE pWave);
UINT AIAPI AStartVoice(HAC hVoice);
UINT AIAPI AStopVoice(HAC hVoice);

UINT AIAPI ASetVoicePosition(HAC hVoice, LONG dwPosition);
UINT AIAPI ASetVoiceFrequency(HAC hVoice, LONG dwFrequency);
UINT AIAPI ASetVoiceVolume(HAC hVoice, UINT nVolume);
UINT AIAPI ASetVoicePanning(HAC hVoice, UINT nPanning);

UINT AIAPI AGetVoicePosition(HAC hVoice, PLONG pdwPosition);
UINT AIAPI AGetVoiceFrequency(HAC hVoice, PLONG pdwFrequency);
UINT AIAPI AGetVoiceVolume(HAC hVoice, PUINT pnVolume);
UINT AIAPI AGetVoicePanning(HAC hVoice, PUINT pnPanning);
UINT AIAPI AGetVoiceStatus(HAC hVoice, PBOOL pnStatus);

UINT AIAPI APlayModule(PAUDIOMODULE pModule);
UINT AIAPI AStopModule(VOID);
UINT AIAPI APauseModule(VOID);
UINT AIAPI AResumeModule(VOID);
UINT AIAPI ASetModuleVolume(UINT nVolume);
UINT AIAPI ASetModulePosition(UINT nOrder, UINT nRow);
UINT AIAPI AGetModuleVolume(PUINT pnVolume);
UINT AIAPI AGetModulePosition(PUINT pnOrder, PUINT pnRow);
UINT AIAPI AGetModuleStatus(PBOOL pnStatus);

UINT AIAPI ALoadModuleFile(PSZ pszFileName, PAUDIOMODULE* ppModule);
UINT AIAPI AFreeModuleFile(PAUDIOMODULE pModule);

UINT AIAPI ALoadWaveFile(PSZ pszFileName, PAUDIOWAVE* ppWave);
UINT AIAPI AFreeWaveFile(PAUDIOWAVE pWave);

#ifdef __cplusplus
};
#endif

#endif
