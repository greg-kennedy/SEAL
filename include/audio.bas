Attribute VB_Name = "Audio"
'
' $Id: audio.h 1.17 1996/09/25 17:13:02 chasan released $
'
' SEAL Synthetic Audio Library API Interface
'
' Copyright (C) 1995, 1996, 1997, 1998, 1999 Carlos Hasan
'
' This program is free software; you can redistribute it and/or modify
' it under the terms of the GNU Lesser General Public License as published
' by the Free Software Foundation; either version 2 of the License, or
' (at your option) any later version.
'
' MICROSOFT VISUAL BASIC INTERFACE FOR WINDOWS 95/NT
'

Option Explicit

' audio system version number
Public Const AUDIO_SYSTEM_VERSION = &H101

' audio capabilities bit fields definitions
Public Const AUDIO_FORMAT_1M08 = &H1
Public Const AUDIO_FORMAT_1S08 = &H2
Public Const AUDIO_FORMAT_1M16 = &H4
Public Const AUDIO_FORMAT_1S16 = &H8
Public Const AUDIO_FORMAT_2M08 = &H10
Public Const AUDIO_FORMAT_2S08 = &H20
Public Const AUDIO_FORMAT_2M16 = &H40
Public Const AUDIO_FORMAT_2S16 = &H80
Public Const AUDIO_FORMAT_4M08 = &H100
Public Const AUDIO_FORMAT_4S08 = &H200
Public Const AUDIO_FORMAT_4M16 = &H400
Public Const AUDIO_FORMAT_4S16 = &H800

' audio format bit fields defines for devices and waveforms
Public Const AUDIO_FORMAT_8BITS = &H0
Public Const AUDIO_FORMAT_16BITS = &H1
Public Const AUDIO_FORMAT_LOOP = &H10
Public Const AUDIO_FORMAT_BIDILOOP = &H20
Public Const AUDIO_FORMAT_REVERSE = &H80
Public Const AUDIO_FORMAT_MONO = &H0
Public Const AUDIO_FORMAT_STEREO = &H100
Public Const AUDIO_FORMAT_FILTER = &H8000

' audio resource limits defines
Public Const AUDIO_MAX_VOICES = 32
Public Const AUDIO_MAX_SAMPLES = 16
Public Const AUDIO_MAX_PATCHES = 128
Public Const AUDIO_MAX_PATTERNS = 256
Public Const AUDIO_MAX_ORDERS = 256
Public Const AUDIO_MAX_NOTES = 96
Public Const AUDIO_MAX_POINTS = 12
Public Const AUDIO_MIN_PERIOD = 1
Public Const AUDIO_MAX_PERIOD = 31999
Public Const AUDIO_MIN_VOLUME = 0
Public Const AUDIO_MAX_VOLUME = 64
Public Const AUDIO_MIN_PANNING = 0
Public Const AUDIO_MAX_PANNING = 255
Public Const AUDIO_MIN_POSITION = 0
Public Const AUDIO_MAX_POSITION = 1048576
Public Const AUDIO_MIN_FREQUENCY = 512
Public Const AUDIO_MAX_FREQUENCY = 524288

' audio error code defines
Public Const AUDIO_ERROR_NONE = 0
Public Const AUDIO_ERROR_INVALHANDLE = 1
Public Const AUDIO_ERROR_INVALPARAM = 2
Public Const AUDIO_ERROR_NOTSUPPORTED = 3
Public Const AUDIO_ERROR_BADDEVICEID = 4
Public Const AUDIO_ERROR_NODEVICE = 5
Public Const AUDIO_ERROR_DEVICEBUSY = 6
Public Const AUDIO_ERROR_BADFORMAT = 7
Public Const AUDIO_ERROR_NOMEMORY = 8
Public Const AUDIO_ERROR_NODRAMMEMORY = 9
Public Const AUDIO_ERROR_FILENOTFOUND = 10
Public Const AUDIO_ERROR_BADFILEFORMAT = 11
Public Const AUDIO_LAST_ERROR = 11

' audio device identifiers
Public Const AUDIO_DEVICE_NONE = 0
Public Const AUDIO_DEVICE_MAPPER = 65535

' audio product identifiers
Public Const AUDIO_PRODUCT_NONE = 0
Public Const AUDIO_PRODUCT_SB = 1
Public Const AUDIO_PRODUCT_SB15 = 2
Public Const AUDIO_PRODUCT_SB20 = 3
Public Const AUDIO_PRODUCT_SBPRO = 4
Public Const AUDIO_PRODUCT_SB16 = 5
Public Const AUDIO_PRODUCT_AWE32 = 6
Public Const AUDIO_PRODUCT_WSS = 7
Public Const AUDIO_PRODUCT_ESS = 8
Public Const AUDIO_PRODUCT_GUS = 9
Public Const AUDIO_PRODUCT_GUSDB = 10
Public Const AUDIO_PRODUCT_GUSMAX = 11
Public Const AUDIO_PRODUCT_IWAVE = 12
Public Const AUDIO_PRODUCT_PAS = 13
Public Const AUDIO_PRODUCT_PAS16 = 14
Public Const AUDIO_PRODUCT_ARIA = 15
Public Const AUDIO_PRODUCT_WINDOWS = 256
Public Const AUDIO_PRODUCT_LINUX = 257
Public Const AUDIO_PRODUCT_SPARC = 258
Public Const AUDIO_PRODUCT_SGI = 259
Public Const AUDIO_PRODUCT_DSOUND = 260

' audio envelope bit fields
Public Const AUDIO_ENVELOPE_ON = &H1
Public Const AUDIO_ENVELOPE_SUSTAIN = &H2
Public Const AUDIO_ENVELOPE_LOOP = &H4

' audio pattern bit fields
Public Const AUDIO_PATTERN_PACKED = &H80
Public Const AUDIO_PATTERN_NOTE = &H1
Public Const AUDIO_PATTERN_SAMPLE = &H2
Public Const AUDIO_PATTERN_VOLUME = &H4
Public Const AUDIO_PATTERN_COMMAND = &H8
Public Const AUDIO_PATTERN_PARAMS = &H10

' audio module bit fields
Public Const AUDIO_MODULE_AMIGA = &H0
Public Const AUDIO_MODULE_LINEAR = &H1
Public Const AUDIO_MODULE_PANNING = &H8000

' FIXME: structures should be byte aligned

' audio capabilities structure
Public Type AudioCaps
    wProductId          As Integer      ' product identifier
    szProductName       As String * 30  ' product name
    dwFormats           As Long         ' formats supported
End Type


' audio format structure
Public Type AudioInfo
    nDeviceId           As Long         ' device identifier
    wFormat             As Integer      ' playback format
    nSampleRate         As Integer      ' sampling frequency
End Type

' audio waveform structure
Public Type AudioWave
    lpData              As Long         ' data pointer
    dwHandle            As Long         ' waveform handle
    dwLength            As Long         ' waveform length
    dwLoopStart         As Long         ' loop start point
    dwLoopEnd           As Long         ' loop end point
    nSampleRate         As Integer      ' sampling rate
    wFormat             As Integer      ' format bits
End Type


' audio envelope point structure
Public Type AudioPoint
    nFrame              As Integer      ' envelope frame
    nValue              As Integer      ' envelope value
End Type

' audio envelope structure
Public Type AudioEnvelope
    aEnvelope(0 To 11)  As AudioPoint   ' envelope points
    nPoints             As Byte         ' number of points
    nSustain            As Byte         ' sustain point
    nLoopStart          As Byte         ' loop start point
    nLoopEnd            As Byte         ' loop end point
    wFlags              As Integer      ' envelope flags
    nSpeed              As Integer      ' envelope speed
End Type

' audio sample structure
Public Type AudioSample
    szSampleName        As String * 32  ' sample name
    nVolume             As Byte         ' default volume
    nPanning            As Byte         ' default panning
    nRelativeNote       As Byte         ' relative note
    nFinetune           As Byte         ' finetune
    Wave                As AudioWave    ' waveform handle
End Type

' audio patch structure
Public Type AudioPatch
    szPatchName         As String * 32  ' patch name
    aSampleNumber(0 To 95) As Byte      ' multi-sample table
    nSamples            As Integer      ' number of samples
    nVibratoType        As Byte         ' vibrato type
    nVibratoSweep       As Byte         ' vibrato sweep
    nVibratoDepth       As Byte         ' vibrato depth
    nVibratoRate        As Byte         ' vibrato rate
    nVolumeFadeout      As Integer      ' volume fadeout
    Volume              As AudioEnvelope ' volume envelope
    Panning             As AudioEnvelope ' panning envelope
    aSampleTable        As Long         ' sample table (pointer)
End Type

' audio pattern structure
Public Type AudioPattern
    nPacking            As Integer      ' packing type
    nTracks             As Integer      ' number of tracks
    nRows               As Integer      ' number of rows
    nSize               As Integer      ' data size
    lpData              As Long         ' data pointer
End Type

' audio module structure
Public Type AudioModule
    szModuleName        As String * 32  ' module name
    wFlags              As Integer      ' module flags
    nOrders             As Integer      ' number of orders
    nRestart            As Integer      ' restart position
    nTracks             As Integer      ' number of tracks
    nPatterns           As Integer      ' number of patterns
    nPatches            As Integer      ' number of patches
    nTempo              As Integer      ' initial tempo
    nBPM                As Integer      ' initial BPM
    aOrderTable(0 To 255) As Byte       ' order table
    aPanningTable(0 To 31) As Byte      ' panning table
    aPatternTable       As Long         ' pattern table (pointer)
    aPatchTable         As Long         ' patch table (pointer)
End Type

' FIXME: how do I define callback functions?
'typedef VOID (AIAPI* LPFNAUDIOWAVE)(LPBYTE, UINT);
'typedef VOID (AIAPI* LPFNAUDIOTIMER)(VOID);
'typedef VOID (AIAPI* LPFNAUDIOCALLBACK)(BYTE, UINT, UINT);

' audio interface API prototypes
Public Declare Function AInitialize Lib "AudioW32" () As Long
Public Declare Function AGetVersion Lib "AudioW32" () As Long
Public Declare Function AGetAudioNumDevs Lib "AudioW32" () As Long
Public Declare Function AGetAudioDevCaps Lib "AudioW32" (ByVal nDeviceId As Long, ByRef lpCaps As AudioCaps) As Long
Public Declare Function AGetErrorText Lib "AudioW32" (ByVal nErrorCode As Long, ByVal lpszText As String, ByVal nSize As Long) As Long

Public Declare Function APingAudio Lib "AudioW32" (ByRef lpnDeviceId As Long) As Long
Public Declare Function AOpenAudio Lib "AudioW32" (ByRef lpInfo As AudioInfo) As Long
Public Declare Function ACloseAudio Lib "AudioW32" () As Long
Public Declare Function AUpdateAudio Lib "AudioW32" () As Long

Public Declare Function AOpenVoices Lib "AudioW32" (ByVal nVoices As Long) As Long
Public Declare Function ACloseVoices Lib "AudioW32" () As Long

Public Declare Function ASetAudioCallback Lib "AudioW32" (ByVal lpfnAudioWave As Long) As Long
Public Declare Function ASetAudioTimerProc Lib "AudioW32" (ByVal lpfnAudioTimer As Long) As Long
Public Declare Function ASetAudioTimerRate Lib "AudioW32" (ByVal nTimerRate As Long) As Long

Public Declare Function AGetAudioDataAvail Lib "AudioW32" () As Long
Public Declare Function ACreateAudioData Lib "AudioW32" (ByRef lpWave As AudioWave) As Long
Public Declare Function ADestroyAudioData Lib "AudioW32" (ByRef lpWave As AudioWave) As Long
Public Declare Function AWriteAudioData Lib "AudioW32" (ByRef lpWave As AudioWave, ByVal dwOffset As Long, ByVal nCount As Long) As Long

Public Declare Function ACreateAudioVoice Lib "AudioW32" (ByRef lphVoice As Long) As Long
Public Declare Function ADestroyAudioVoice Lib "AudioW32" (ByVal hVoice As Long) As Long

Public Declare Function APlayVoice Lib "AudioW32" (ByVal hVoice As Long, ByVal lpWave As Long) As Long
Public Declare Function APrimeVoice Lib "AudioW32" (ByVal hVoice As Long, ByVal lpWave As Long) As Long
Public Declare Function AStartVoice Lib "AudioW32" (ByVal hVoice As Long) As Long
Public Declare Function AStopVoice Lib "AudioW32" (ByVal hVoice As Long) As Long

Public Declare Function ASetVoicePosition Lib "AudioW32" (ByVal hVoice As Long, ByVal dwPosition As Long) As Long
Public Declare Function ASetVoiceFrequency Lib "AudioW32" (ByVal hVoice As Long, ByVal dwFrequency As Long) As Long
Public Declare Function ASetVoiceVolume Lib "AudioW32" (ByVal hVoice As Long, ByVal nVolume As Long) As Long
Public Declare Function ASetVoicePanning Lib "AudioW32" (ByVal hVoice As Long, ByVal nPanning As Long) As Long

Public Declare Function AGetVoicePosition Lib "AudioW32" (ByVal hVoice As Long, ByRef lpdwPosition As Long) As Long
Public Declare Function AGetVoiceFrequency Lib "AudioW32" (ByVal hVoice As Long, ByRef lpdwFrequency As Long) As Long
Public Declare Function AGetVoiceVolume Lib "AudioW32" (ByVal hVoice As Long, ByRef lpnVolume As Long) As Long
Public Declare Function AGetVoicePanning Lib "AudioW32" (ByVal hVoice As Long, ByRef lpnPanning As Long) As Long
Public Declare Function AGetVoiceStatus Lib "AudioW32" (ByVal hVoice As Long, ByRef lpnStatus As Long) As Long

Public Declare Function APlayModule Lib "AudioW32" (ByVal lpModule As Long) As Long
Public Declare Function AStopModule Lib "AudioW32" () As Long
Public Declare Function APauseModule Lib "AudioW32" () As Long
Public Declare Function AResumeModule Lib "AudioW32" () As Long
Public Declare Function ASetModuleVolume Lib "AudioW32" (ByVal nVolume As Long) As Long
Public Declare Function ASetModulePosition Lib "AudioW32" (ByVal nOrder As Long, ByVal nRow As Long) As Long
Public Declare Function AGetModuleVolume Lib "AudioW32" (ByVal lpnVolume As Long) As Long
Public Declare Function AGetModulePosition Lib "AudioW32" (ByRef pnOrder As Long, ByRef lpnRow As Long) As Long
Public Declare Function AGetModuleStatus Lib "AudioW32" (ByRef lpnStatus As Long) As Long
Public Declare Function ASetModuleCallback Lib "AudioW32" (ByVal lpfnAudioCallback As Long) As Long

' FIXME: how do I define pointers to AudioModule and AudioWave?

Public Declare Function ALoadModuleFile Lib "AudioW32" (ByVal lpszFileName As String, ByRef lplpModule As Long, ByVal dwFileOffset As Long) As Long
Public Declare Function AFreeModuleFile Lib "AudioW32" (ByVal lpModule As Long) As Long

Public Declare Function ALoadWaveFile Lib "AudioW32" (ByVal lpszFileName As String, ByRef lplpWave As Long, ByVal dwFileOffset As Long) As Long
Public Declare Function AFreeWaveFile Lib "AudioW32" (ByVal lpWave As Long) As Long

