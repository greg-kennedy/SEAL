{*
 * $Id: audio.h 1.12 1996/06/02 17:02:07 chasan released $
 *
 * SEAL Synthetic Audio Library API Interface
 *
 * Copyright (C) 1995, 1996 Carlos Hasan. All Rights Reserved.
 *
 *}

unit Audio;

interface
uses Windows;

const
  { audio system version number }
  AUDIO_SYSTEM_VERSION          = $0100;

  { audio capabilities bit fields definitions }
  AUDIO_FORMAT_1M08             = $00000001;
  AUDIO_FORMAT_1S08             = $00000002;
  AUDIO_FORMAT_1M16             = $00000004;
  AUDIO_FORMAT_1S16             = $00000008;
  AUDIO_FORMAT_2M08             = $00000010;
  AUDIO_FORMAT_2S08             = $00000020;
  AUDIO_FORMAT_2M16             = $00000040;
  AUDIO_FORMAT_2S16             = $00000080;
  AUDIO_FORMAT_4M08             = $00000100;
  AUDIO_FORMAT_4S08             = $00000200;
  AUDIO_FORMAT_4M16             = $00000400;
  AUDIO_FORMAT_4S16             = $00000800;

  { audio format bit fields defines for devices and waveforms }
  AUDIO_FORMAT_8BITS            = $0000;
  AUDIO_FORMAT_16BITS           = $0001;
  AUDIO_FORMAT_LOOP             = $0010;
  AUDIO_FORMAT_BIDILOOP         = $0020;
  AUDIO_FORMAT_REVERSE          = $0080;
  AUDIO_FORMAT_MONO             = $0000;
  AUDIO_FORMAT_STEREO           = $0100;
  AUDIO_FORMAT_FILTER           = $8000;

  { audio resource limits defines }
  AUDIO_MAX_VOICES              = 32;
  AUDIO_MAX_SAMPLES             = 16;
  AUDIO_MAX_PATCHES             = 128;
  AUDIO_MAX_PATTERNS            = 256;
  AUDIO_MAX_ORDERS              = 256;
  AUDIO_MAX_NOTES               = 96;
  AUDIO_MAX_POINTS              = 12;
  AUDIO_MIN_PERIOD              = 1;
  AUDIO_MAX_PERIOD              = 31999;
  AUDIO_MIN_VOLUME              = $00;
  AUDIO_MAX_VOLUME              = $40;
  AUDIO_MIN_PANNING             = $00;
  AUDIO_MAX_PANNING             = $FF;
  AUDIO_MIN_POSITION            = $00000000;
  AUDIO_MAX_POSITION            = $00100000;
  AUDIO_MIN_FREQUENCY           = $00000200;
  AUDIO_MAX_FREQUENCY           = $00080000;

  { audio error code defines }
  AUDIO_ERROR_NONE              = $0000;
  AUDIO_ERROR_INVALHANDLE       = $0001;
  AUDIO_ERROR_INVALPARAM        = $0002;
  AUDIO_ERROR_NOTSUPPORTED      = $0003;
  AUDIO_ERROR_BADDEVICEID       = $0004;
  AUDIO_ERROR_NODEVICE          = $0005;
  AUDIO_ERROR_DEVICEBUSY        = $0006;
  AUDIO_ERROR_BADFORMAT         = $0007;
  AUDIO_ERROR_NOMEMORY          = $0008;
  AUDIO_ERROR_NODRAMMEMORY      = $0009;
  AUDIO_ERROR_FILENOTFOUND      = $000A;
  AUDIO_ERROR_BADFILEFORMAT     = $000B;
  AUDIO_LAST_ERROR              = $000B;

  { audio device identifiers }
  AUDIO_DEVICE_NONE             = $0000;
  AUDIO_DEVICE_MAPPER           = $FFFF;

  { audio product identifiers }
  AUDIO_PRODUCT_NONE            = $0000;
  AUDIO_PRODUCT_SB              = $0001;
  AUDIO_PRODUCT_SB15            = $0002;
  AUDIO_PRODUCT_SB20            = $0003;
  AUDIO_PRODUCT_SBPRO           = $0004;
  AUDIO_PRODUCT_SB16            = $0005;
  AUDIO_PRODUCT_AWE32           = $0006;
  AUDIO_PRODUCT_WSS             = $0007;
  AUDIO_PRODUCT_ESS             = $0008;
  AUDIO_PRODUCT_GUS             = $0009;
  AUDIO_PRODUCT_GUSDB           = $000A;
  AUDIO_PRODUCT_GUSMAX          = $000B;
  AUDIO_PRODUCT_IWAVE           = $000C;
  AUDIO_PRODUCT_PAS             = $000D;
  AUDIO_PRODUCT_PAS16           = $000E;
  AUDIO_PRODUCT_ARIA            = $000F;
  AUDIO_PRODUCT_WINDOWS         = $0100;
  AUDIO_PRODUCT_LINUX           = $0101;
  AUDIO_PRODUCT_SPARC           = $0102;
  AUDIO_PRODUCT_SGI             = $0103;

  { audio envelope bit fields }
  AUDIO_ENVELOPE_ON             = $0001;
  AUDIO_ENVELOPE_SUSTAIN        = $0002;
  AUDIO_ENVELOPE_LOOP           = $0004;

  { audio pattern bit fields }
  AUDIO_PATTERN_PACKED          = $0080;
  AUDIO_PATTERN_NOTE            = $0001;
  AUDIO_PATTERN_SAMPLE          = $0002;
  AUDIO_PATTERN_VOLUME          = $0004;
  AUDIO_PATTERN_COMMAND         = $0008;
  AUDIO_PATTERN_PARAMS          = $0010;

  { audio module bit fields }
  AUDIO_MODULE_AMIGA            = $0000;
  AUDIO_MODULE_LINEAR           = $0001;
  AUDIO_MODULE_PANNING          = $8000;

type
  { audio capabilities structure }
  PAudioCaps = ^TAudioCaps;
  TAudioCaps = record
    wProductId : Word;                          { product identifier }
    szProductName : Array [0..29] of Char;      { product name }
    dwFormats : Longint;                        { formats supported }
  end;

  { audio format structure }
  PAudioInfo = ^TAudioInfo;
  TAudioInfo = record
    nDeviceId : Integer;                        { device identifier }
    wFormat : Word;                             { playback format }
    nSampleRate : Word;                         { sampling rate }
  end;

  { audio waveform structure }
  PAudioWave = ^TAudioWave;
  TAudioWave = record
    pData : Pointer;                            { data pointer }
    dwHandle : Longint;                         { waveform handle }
    dwLength : Longint;                         { waveform length }
    dwLoopStart : Longint;                      { loop start point }
    dwLoopEnd : Longint;                        { loop end point }
    nSampleRate : Word;                         { sampling rate }
    wFormat : Word;                             { format bits }
  end;


  { audio envelope point structure }
  PAudioPoint = ^TAudioPoint;
  TAudioPoint = record
    nFrame : Word;                              { envelope frame }
    nValue : Word;                              { envelope value }
  end;

  { audio envelope structure }
  PAudioEnvelope = ^TAudioEnvelope;
  TAudioEnvelope = record
    aEnvelope : Array [1..AUDIO_MAX_POINTS] of TAudioPoint; { envelope points }
    nPoints : Byte;                             { number of points }
    nSustain : Byte;                            { sustain point }
    nLoopStart : Byte;                          { loop start point }
    nLoopEnd : Byte;                            { loop end point }
    wFlags : Word;                              { envelope flags }
    nSpeed : Word;                              { envelope speed }
  end;

  { audio sample structure }
  PAudioSample = ^TAudioSample;
  TAudioSample = record
    szSampleName : Array [0..31] of Char;       { sample name }
    nVolume : Byte;                             { default volume }
    nPanning : Byte;                            { default panning }
    nRelativeNote : Byte;                       { relative note }
    nFinetune : Byte;                           { finetune }
    Wave : TAudioWave;                          { waveform handle }
  end;

  { audio patch structure }
  PAudioPatch = ^TAudioPatch;
  TAudioPatch = record
    szPatchName : Array [0..31] of Char;        { patch name }
    aSampleNumber : Array [1..AUDIO_MAX_NOTES] of Byte; { multi-sample table }
    nSamples : Word;                            { number of samples }
    nVibratoType : Byte;                        { vibrato type }
    nVibratoSweep : Byte;                       { vibrato sweep }
    nVibratoDepth : Byte;                       { vibrato depth }
    nVibratoRate : Byte;                        { vibrato rate }
    nVolumeFadeout : Word;                      { volume fadeout }
    Volume : TAudioEnvelope;                    { volume envelope }
    Panning : TAudioEnvelope;                   { panning envelope }
    aSampleTable : PAudioSample;                { sample table }
  end;

  { audio pattern structure }
  PAudioPattern = ^TAudioPattern;
  TAudioPattern = record
    nPacking : Word;                            { packing type }
    nTracks : Word;                             { number of tracks }
    nRows : Word;                               { number of rows }
    nSize : Word;                               { data size }
    pData : Pointer;                            { data pointer }
  end;

  { audio module structure }
  PAudioModule = ^TAudioModule;
  TAudioModule = record
    szModuleName : Array [0..31] of Char;       { module name }
    wFlags : Word;                              { module flags }
    nOrders : Word;                             { number of orders }
    nRestart : Word;                            { restart position }
    nTracks : Word;                             { number of tracks }
    nPatterns : Word;                           { number of patterns }
    nPatches : Word;                            { number of patches }
    nTempo : Word;                              { initial tempo }
    nBPM : Word;                                { initial BPM }
    aOrderTable : Array [1..AUDIO_MAX_ORDERS] of Byte; { order table }
    aPanningTable : Array [1..AUDIO_MAX_VOICES] of Byte; { panning table }
    aPatternTable : PAudioPattern;              { pattern table }
    aPatchTable : PAudioPatch;                  { patch table }
  end;

  { audio callback function defines }
  TAudioWaveProc = procedure (pData: Pointer; nCount: Integer); stdcall;
  TAudioTimerProc = procedure; stdcall;

  { audio handle defines }
  PHAC = ^HAC;
  HAC = Integer;


{ audio interface API prototypes }
function AInitialize: Integer; stdcall;
function AGetVersion: Integer; stdcall;
function AGetAudioNumDevs: Integer; stdcall;
function AGetAudioDevCaps(nDeviceId: Integer; var pCaps: TAudioCaps): Integer; stdcall;
function AGetErrorText(nErrorCode: Integer; pText: PChar; nSize: Integer): Integer; stdcall;

function APingAudio(var pnDeviceId: Integer): Integer; stdcall;
function AOpenAudio(var pInfo: TAudioInfo): Integer; stdcall;
function ACloseAudio: Integer; stdcall;
function AUpdateAudio: Integer; stdcall;

function AOpenVoices(nVoices: Integer): Integer; stdcall;
function ACloseVoices: Integer; stdcall;

function ASetAudioCallback(pfnWaveProc: TAudioWaveProc): Integer; stdcall;
function ASetAudioTimerProc(pfnTimerProc: TAudioTimerProc): Integer; stdcall;
function ASetAudioTimerRate(nTimerRate: Integer): Integer; stdcall;

function AGetAudioDataAvail: Longint; stdcall;
function ACreateAudioData(pWave: PAudioWave): Integer; stdcall;
function ADestroyAudioData(pWave: PAudioWave): Integer; stdcall;
function AWriteAudioData(pWave: PAudioWave; dwOffset: Longint; nCount: Integer): Integer; stdcall;

function ACreateAudioVoice(var phVoice: HAC): Integer; stdcall;
function ADestroyAudioVoice(hVoice: HAC): Integer; stdcall;

function APlayVoice(hVoice: HAC; pWave: PAudioWave): Integer; stdcall;
function APrimeVoice(hVoice: HAC; pWave: PAudioWave): Integer; stdcall;
function AStartVoice(hVoice: HAC): Integer; stdcall;
function AStopVoice(hVoice: HAC): Integer; stdcall;

function ASetVoicePosition(hVoice: HAC; dwPosition: Longint): Integer; stdcall;
function ASetVoiceFrequency(hVoice: HAC; dwFrequency: Longint): Integer; stdcall;
function ASetVoiceVolume(hVoice: HAC; nVolume: Integer): Integer; stdcall;
function ASetVoicePanning(hVoice: HAC; nPanning: Integer): Integer; stdcall;

function AGetVoicePosition(hVoice: HAC; var pdwPosition: Longint): Integer; stdcall;
function AGetVoiceFrequency(hVoice: HAC; var pdwFrequency: Longint): Integer; stdcall;
function AGetVoiceVolume(hVoice: HAC; var pnVolume: Integer): Integer; stdcall;
function AGetVoicePanning(hVoice: HAC; pnPanning: Integer): Integer; stdcall;
function AGetVoiceStatus(hVoice: HAC; var pnStatus: Integer): Integer; stdcall;

function APlayModule(pModule: PAudioModule): Integer; stdcall;
function AStopModule: Integer; stdcall;
function APauseModule: Integer; stdcall;
function AResumeModule: Integer; stdcall;
function ASetModuleVolume(nVolume: Integer): Integer; stdcall;
function ASetModulePosition(nOrder, nRow: Integer): Integer; stdcall;
function AGetModuleVolume(var pnVolume: Integer): Integer; stdcall;
function AGetModulePosition(var pnOrder, pnRow: Integer): Integer; stdcall;
function AGetModuleStatus(var pnStatus: Integer): Integer; stdcall;

function ALoadModuleFile(pszFileName: PChar; var ppModule: PAudioModule): Integer; stdcall;
function AFreeModuleFile(pModule: PAudioModule): Integer; stdcall;

function ALoadWaveFile(pszFileName: PChar; var ppWave: PAudioWave): Integer; stdcall;
function AFreeWaveFile(pWave: PAudioWave): Integer; stdcall;


implementation

function AInitialize: Integer; stdcall; external 'AUDIOW32' index 100;
function AGetVersion: Integer; stdcall; external 'AUDIOW32' index 101;
function AGetAudioNumDevs: Integer; stdcall; external 'AUDIOW32' index 102;
function AGetAudioDevCaps(nDeviceId: Integer; var pCaps: TAudioCaps): Integer; stdcall; external 'AUDIOW32' index 103;
function AGetErrorText(nErrorCode: Integer; pText: PChar; nSize: Integer): Integer; stdcall; external 'AUDIOW32' index 104;

function APingAudio(var pnDeviceId: Integer): Integer; stdcall; external 'AUDIOW32' index 105;
function AOpenAudio(var pInfo: TAudioInfo): Integer; stdcall; external 'AUDIOW32' index 106;
function ACloseAudio: Integer; stdcall; external 'AUDIOW32' index 107;
function AUpdateAudio: Integer; stdcall; external 'AUDIOW32' index 108;

function AOpenVoices(nVoices: Integer): Integer; stdcall; external 'AUDIOW32' index 109;
function ACloseVoices: Integer; stdcall; external 'AUDIOW32' index 110;

function ASetAudioCallback(pfnWaveProc: TAudioWaveProc): Integer; stdcall; external 'AUDIOW32' index 111;
function ASetAudioTimerProc(pfnTimerProc: TAudioTimerProc): Integer; stdcall; external 'AUDIOW32' index 112;
function ASetAudioTimerRate(nTimerRate: Integer): Integer; stdcall; external 'AUDIOW32' index 113;

function AGetAudioDataAvail: Longint; stdcall; external 'AUDIOW32' index 114;
function ACreateAudioData(pWave: PAudioWave): Integer; stdcall; external 'AUDIOW32' index 115;
function ADestroyAudioData(pWave: PAudioWave): Integer; stdcall; external 'AUDIOW32' index 116;
function AWriteAudioData(pWave: PAudioWave; dwOffset: Longint; nCount: Integer): Integer; stdcall; external 'AUDIOW32' index 117;

function ACreateAudioVoice(var phVoice: HAC): Integer; stdcall; external 'AUDIOW32' index 118;
function ADestroyAudioVoice(hVoice: HAC): Integer; stdcall; external 'AUDIOW32' index 119;

function APlayVoice(hVoice: HAC; pWave: PAudioWave): Integer; stdcall; external 'AUDIOW32' index 120;
function APrimeVoice(hVoice: HAC; pWave: PAudioWave): Integer; stdcall; external 'AUDIOW32' index 121;
function AStartVoice(hVoice: HAC): Integer; stdcall; external 'AUDIOW32' index 122;
function AStopVoice(hVoice: HAC): Integer; stdcall; external 'AUDIOW32' index 123;

function ASetVoicePosition(hVoice: HAC; dwPosition: Longint): Integer; stdcall; external 'AUDIOW32' index 124;
function ASetVoiceFrequency(hVoice: HAC; dwFrequency: Longint): Integer; stdcall; external 'AUDIOW32' index 125;
function ASetVoiceVolume(hVoice: HAC; nVolume: Integer): Integer; stdcall; external 'AUDIOW32' index 126;
function ASetVoicePanning(hVoice: HAC; nPanning: Integer): Integer; stdcall; external 'AUDIOW32' index 127;

function AGetVoicePosition(hVoice: HAC; var pdwPosition: Longint): Integer; stdcall; external 'AUDIOW32' index 128;
function AGetVoiceFrequency(hVoice: HAC; var pdwFrequency: Longint): Integer; stdcall; external 'AUDIOW32' index 129;
function AGetVoiceVolume(hVoice: HAC; var pnVolume: Integer): Integer; stdcall; external 'AUDIOW32' index 130;
function AGetVoicePanning(hVoice: HAC; pnPanning: Integer): Integer; stdcall; external 'AUDIOW32' index 131;
function AGetVoiceStatus(hVoice: HAC; var pnStatus: Integer): Integer; stdcall; external 'AUDIOW32' index 132;

function APlayModule(pModule: PAudioModule): Integer; stdcall; external 'AUDIOW32' index 133;
function AStopModule: Integer; stdcall; external 'AUDIOW32' index 134;
function APauseModule: Integer; stdcall; external 'AUDIOW32' index 135;
function AResumeModule: Integer; stdcall; external 'AUDIOW32' index 136;
function ASetModuleVolume(nVolume: Integer): Integer; stdcall; external 'AUDIOW32' index 137;
function ASetModulePosition(nOrder, nRow: Integer): Integer; stdcall; external 'AUDIOW32' index 138;
function AGetModuleVolume(var pnVolume: Integer): Integer; stdcall; external 'AUDIOW32' index 139;
function AGetModulePosition(var pnOrder, pnRow: Integer): Integer; stdcall; external 'AUDIOW32' index 140;
function AGetModuleStatus(var pnStatus: Integer): Integer; stdcall; external 'AUDIOW32' index 141;

function ALoadModuleFile(pszFileName: PChar; var ppModule: PAudioModule): Integer; stdcall; external 'AUDIOW32' index 142;
function AFreeModuleFile(pModule: PAudioModule): Integer; stdcall; external 'AUDIOW32' index 143;

function ALoadWaveFile(pszFileName: PChar; var ppWave: PAudioWave): Integer; stdcall; external 'AUDIOW32' index 144;
function AFreeWaveFile(pWave: PAudioWave): Integer; stdcall; external 'AUDIOW32' index 145;

end.
