{*
 * $Id: audio.h 1.16 1996/09/25 13:09:09 chasan released $
 *
 * SEAL Synthetic Audio Library API Interface
 *
 * Copyright (C) 1995-1999 Carlos Hasan
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *}

unit Audio;

interface
uses
  SysUtils, Windows;

const
  { audio system version number }
  AUDIO_SYSTEM_VERSION          = $0101;

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
  AUDIO_PRODUCT_DSOUND          = $0104;

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
  TAudioCallback = procedure(nSyncMark: Byte; nOrder, nRow: Integer); stdcall;

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
function AGetVoicePanning(hVoice: HAC; var pnPanning: Integer): Integer; stdcall;
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
function ASetModuleCallback(pfnAudioCallback: TAudioCallback): Integer; stdcall;

function ALoadModuleFile(pszFileName: PChar; var ppModule: PAudioModule; FileOffset: Longint): Integer; stdcall;
function AFreeModuleFile(pModule: PAudioModule): Integer; stdcall;

function ALoadWaveFile(pszFileName: PChar; var ppWave: PAudioWave; FileOffset: Longint): Integer; stdcall;
function AFreeWaveFile(pWave: PAudioWave): Integer; stdcall;


type
  TAudio = class(TObject)
    constructor Create(Format: Word; SampleRate: Word);
    destructor Destroy; override;
    procedure Update;
  private
    FInfo: TAudioInfo;
    function GetProductId: Integer;
    function GetProductName: String;
  public
    property DeviceId: Integer read FInfo.nDeviceId;
    property ProductId: Integer read GetProductId;
    property ProductName: String read GetProductName;
    property Format: Word read FInfo.wFormat;
    property SampleRate: Word read FInfo.nSampleRate;
  end;

  TWaveform = class(TObject)
    constructor Create(Format: Word; SampleRate: Word;
      Length, LoopStart, LoopEnd: Longint);
    constructor LoadFromFile(FileName: String);
    destructor Destroy; override;
    procedure Write(var Buffer; Count: Integer);
  private
    FHandle: TAudioWave;
    PHandle: PAudioWave;
    FVolume: Integer;
    FPanning: Integer;
    FPosition: Longint;
    function GetHandle: PAudioWave;
    procedure SetSampleRate(Value: Word);
    procedure SetVolume(Value: Integer);
    procedure SetPanning(Value: Integer);
    procedure SetPosition(Value: LongInt);
  public
    property Handle: PAudioWave read GetHandle;
    property Format: Word read FHandle.wFormat;
    property Length: Longint read FHandle.dwLength;
    property LoopStart: Longint read FHandle.dwLoopStart;
    property LoopEnd: Longint read FHandle.dwLoopEnd;
    property SampleRate: Word read FHandle.nSampleRate write SetSampleRate;
    property Volume: Integer read FVolume write SetVolume;
    property Panning: Integer read FPanning write SetPanning;
    property Position: Longint read FPosition write SetPosition;
  end;
  
  TVoice = class(TObject)
    constructor Create;
    destructor Destroy; override;
    procedure Prime(Wave: TWaveform);
    procedure Play(Wave: TWaveform);
    procedure Start;
    procedure Stop;
  private
    FHandle: HAC;
    FWave: TWaveform;
    function GetHandle: HAC;
    function GetWaveform: TWaveform;
    function GetPosition: Longint;
    procedure SetPosition(Value: Longint);
    function GetFrequency: Longint;
    procedure SetFrequency(Value: Longint);
    function GetVolume: Integer;
    procedure SetVolume(Value: Integer);
    function GetPanning: Integer;
    procedure SetPanning(Value: Integer);
    function GetStopped: Boolean;
  public
    property Handle: HAC read GetHandle;
    property Waveform: TWaveform read GetWaveform;
    property Position: Longint read GetPosition write SetPosition;
    property Frequency: Longint read GetFrequency write SetFrequency;
    property Volume: Integer read GetVolume write SetVolume;
    property Panning: Integer read GetPanning write SetPanning;
    property Stopped: Boolean read GetStopped;
  end;

  TModule = class(TObject)
    constructor LoadFromFile(FileName: String);
    destructor Destroy; override;
    procedure Play;
    procedure Stop;
    procedure Pause;
    procedure Resume;
  private
    FHandle: PAudioModule;
    FCallback: TAudioCallback;
    function GetVolume: Integer;
    procedure SetVolume(Value: Integer);
    function GetOrder: Integer;
    procedure SetOrder(Value: Integer);
    function GetRow: Integer;
    procedure SetRow(Value: Integer);
    function GetStopped: Boolean;
    procedure SetCallback(Value: TAudioCallback);
  public
    property Handle: PAudioModule read FHandle;
    property Volume: Integer read GetVolume write SetVolume;
    property Order: Integer read GetOrder write SetOrder;
    property Row: Integer read GetRow write SetRow;
    property Stopped: Boolean read GetStopped;
    property OnSync: TAudioCallback read FCallback write SetCallback;
  private
    function GetName: String;
    function GetNumOrders: Integer;
    function GetNumTracks: Integer;
    function GetNumPatterns: Integer;
    function GetNumPatches: Integer;
    function GetPatch(Index: Integer): PAudioPatch;
  public
    property Name: String read GetName;
    property NumOrders: Integer read GetNumOrders;
    property NumTracks: Integer read GetNumTracks;
    property NumPatterns: Integer read GetNumPatterns;
    property NumPatches: Integer read GetNumPatches;
    property Patch[Index: Integer]: PAudioPatch read GetPatch;
  end;


implementation

function AInitialize: Integer; stdcall; external 'AUDIOW32.DLL';
function AGetVersion: Integer; stdcall; external 'AUDIOW32.DLL';
function AGetAudioNumDevs: Integer; stdcall; external 'AUDIOW32.DLL';
function AGetAudioDevCaps(nDeviceId: Integer; var pCaps: TAudioCaps): Integer; stdcall; external 'AUDIOW32.DLL';
function AGetErrorText(nErrorCode: Integer; pText: PChar; nSize: Integer): Integer; stdcall; external 'AUDIOW32.DLL';

function APingAudio(var pnDeviceId: Integer): Integer; stdcall; external 'AUDIOW32.DLL';
function AOpenAudio(var pInfo: TAudioInfo): Integer; stdcall; external 'AUDIOW32.DLL';
function ACloseAudio: Integer; stdcall; external 'AUDIOW32.DLL';
function AUpdateAudio: Integer; stdcall; external 'AUDIOW32.DLL';

function AOpenVoices(nVoices: Integer): Integer; stdcall; external 'AUDIOW32.DLL';
function ACloseVoices: Integer; stdcall; external 'AUDIOW32.DLL';

function ASetAudioCallback(pfnWaveProc: TAudioWaveProc): Integer; stdcall; external 'AUDIOW32.DLL';
function ASetAudioTimerProc(pfnTimerProc: TAudioTimerProc): Integer; stdcall; external 'AUDIOW32.DLL';
function ASetAudioTimerRate(nTimerRate: Integer): Integer; stdcall; external 'AUDIOW32.DLL';

function AGetAudioDataAvail: Longint; stdcall; external 'AUDIOW32.DLL';
function ACreateAudioData(pWave: PAudioWave): Integer; stdcall; external 'AUDIOW32.DLL';
function ADestroyAudioData(pWave: PAudioWave): Integer; stdcall; external 'AUDIOW32.DLL';
function AWriteAudioData(pWave: PAudioWave; dwOffset: Longint; nCount: Integer): Integer; stdcall; external 'AUDIOW32.DLL';

function ACreateAudioVoice(var phVoice: HAC): Integer; stdcall; external 'AUDIOW32.DLL';
function ADestroyAudioVoice(hVoice: HAC): Integer; stdcall; external 'AUDIOW32.DLL';

function APlayVoice(hVoice: HAC; pWave: PAudioWave): Integer; stdcall; external 'AUDIOW32.DLL';
function APrimeVoice(hVoice: HAC; pWave: PAudioWave): Integer; stdcall; external 'AUDIOW32.DLL';
function AStartVoice(hVoice: HAC): Integer; stdcall; external 'AUDIOW32.DLL';
function AStopVoice(hVoice: HAC): Integer; stdcall; external 'AUDIOW32.DLL';

function ASetVoicePosition(hVoice: HAC; dwPosition: Longint): Integer; stdcall; external 'AUDIOW32.DLL';
function ASetVoiceFrequency(hVoice: HAC; dwFrequency: Longint): Integer; stdcall; external 'AUDIOW32.DLL';
function ASetVoiceVolume(hVoice: HAC; nVolume: Integer): Integer; stdcall; external 'AUDIOW32.DLL';
function ASetVoicePanning(hVoice: HAC; nPanning: Integer): Integer; stdcall; external 'AUDIOW32.DLL';

function AGetVoicePosition(hVoice: HAC; var pdwPosition: Longint): Integer; stdcall; external 'AUDIOW32.DLL';
function AGetVoiceFrequency(hVoice: HAC; var pdwFrequency: Longint): Integer; stdcall; external 'AUDIOW32.DLL';
function AGetVoiceVolume(hVoice: HAC; var pnVolume: Integer): Integer; stdcall; external 'AUDIOW32.DLL';
function AGetVoicePanning(hVoice: HAC; var pnPanning: Integer): Integer; stdcall; external 'AUDIOW32.DLL';
function AGetVoiceStatus(hVoice: HAC; var pnStatus: Integer): Integer; stdcall; external 'AUDIOW32.DLL';

function APlayModule(pModule: PAudioModule): Integer; stdcall; external 'AUDIOW32.DLL';
function AStopModule: Integer; stdcall; external 'AUDIOW32.DLL';
function APauseModule: Integer; stdcall; external 'AUDIOW32.DLL';
function AResumeModule: Integer; stdcall; external 'AUDIOW32.DLL';
function ASetModuleVolume(nVolume: Integer): Integer; stdcall; external 'AUDIOW32.DLL';
function ASetModulePosition(nOrder, nRow: Integer): Integer; stdcall; external 'AUDIOW32.DLL';
function AGetModuleVolume(var pnVolume: Integer): Integer; stdcall; external 'AUDIOW32.DLL';
function AGetModulePosition(var pnOrder, pnRow: Integer): Integer; stdcall; external 'AUDIOW32.DLL';
function AGetModuleStatus(var pnStatus: Integer): Integer; stdcall; external 'AUDIOW32.DLL';
function ASetModuleCallback(pfnAudioCallback: TAudioCallback): Integer; stdcall; external 'AUDIOW32.DLL';

function ALoadModuleFile(pszFileName: PChar; var ppModule: PAudioModule; FileOffset: Longint): Integer; stdcall; external 'AUDIOW32.DLL';
function AFreeModuleFile(pModule: PAudioModule): Integer; stdcall; external 'AUDIOW32.DLL';

function ALoadWaveFile(pszFileName: PChar; var ppWave: PAudioWave; FileOffset: Longint): Integer; stdcall; external 'AUDIOW32.DLL';
function AFreeWaveFile(pWave: PAudioWave): Integer; stdcall; external 'AUDIOW32.DLL';


const
  Semaphore: LongBool = False;
  PlayingModule: PAudioModule = nil;

function SetPlayingModule(Value: PAudioModule): Boolean; pascal; assembler;
asm
  mov  eax,True
  xchg eax,Semaphore
  cmp  eax,False
  jne  @@1
  mov  eax,PlayingModule
  test eax,eax
  jne  @@0
  mov  eax,Value
  mov  PlayingModule,eax
@@0:
  mov  Semaphore,False
@@1:
  push edx
  xor  eax,eax
  mov  edx,PlayingModule
  cmp  edx,Value
  sete al
  pop  edx
end;

procedure Assert(Header: String; ErrorCode: Integer);
var
  szText: Array [0..255] of Char;
begin
  if ErrorCode <> AUDIO_ERROR_NONE then
  begin
    AGetErrorText(ErrorCode, szText, sizeof(szText) - 1);
    raise Exception.Create(Header + ': ' + StrPas(szText));
  end;
end;


{ TAudio }
constructor TAudio.Create(Format: Word; SampleRate: Word);
begin
  inherited Create;
  FInfo.nDeviceId := AUDIO_DEVICE_MAPPER;
  FInfo.wFormat := Format;
  FInfo.nSampleRate := SampleRate;
  Assert('AOpenAudio', AOpenAudio(FInfo));
  Assert('AOpenVoices', AOpenVoices(32));
end;

destructor TAudio.Destroy;
begin
  Assert('ACloseVoices', ACloseVoices);
  Assert('ACloseAudio', ACloseAudio);
  inherited Destroy;
end;

procedure TAudio.Update;
begin
  Assert('AUpdateAudio', AUpdateAudio);
end;

function TAudio.GetProductId: Integer;
var
  Caps: TAudioCaps;
begin
  Assert('AGetAudioDevCaps', AGetAudioDevCaps(FInfo.nDeviceId, Caps));
  Result := Caps.wProductId;
end;

function TAudio.GetProductName: String;
var
  Caps: TAudioCaps;
begin
  Assert('AGetAudioDevCaps', AGetAudioDevCaps(FInfo.nDeviceId, Caps));
  Result := StrPas(Caps.szProductName);
end;


{ TWaveform }
constructor TWaveform.Create(Format: Word; SampleRate: Word;
  Length, LoopStart, LoopEnd: Longint);
begin
  inherited Create;
  FPosition := 0;
  FVolume := 64;
  FPanning := 128;
  FHandle.wFormat := Format;
  FHandle.dwLength := Length;
  FHandle.dwLoopStart := LoopStart;
  FHandle.dwLoopEnd := LoopEnd;
  FHandle.nSampleRate := SampleRate;
  PHandle := nil;
  Assert('ACreateAudioData', ACreateAudioData(@FHandle));
end;

constructor TWaveform.LoadFromFile(FileName: String);
var
  szFileName: Array [0..255] of Char;
begin
  inherited Create;
  FPosition := 0;
  FVolume := 64;
  FPanning := 128;
  FHandle.pData := nil;
  FHandle.wFormat := AUDIO_FORMAT_8BITS or AUDIO_FORMAT_MONO;
  FHandle.dwLength := 0;
  FHandle.dwLoopStart := 0;
  FHandle.dwLoopEnd := 0;
  FHandle.nSampleRate := 22050;
  Assert('ALoadWaveFile', ALoadWaveFile(StrPCopy(szFileName, FileName), PHandle, 0));
  if Assigned(PHandle) then
    FHandle := PHandle^;
end;

destructor TWaveform.Destroy;
begin
  if Assigned(PHandle) then
  begin
    Assert('AFreeWaveFile', AFreeWaveFile(PHandle));
  end
  else if Assigned(FHandle.pData) then
  begin
    Assert('ADestroyAudioData', ADestroyAudioData(@FHandle));
  end;
  inherited Destroy;
end;

procedure TWaveform.Write(var Buffer; Count: Integer);
var
  Size: Integer;
begin
  if Assigned(FHandle.pData) then
  begin
    while (Count > 0) and (FHandle.dwLength > 0) do
    begin
      Size := Count;
      if FPosition + Size > FHandle.dwLength then
        Size := FHandle.dwLength - FPosition;
      Move(Buffer, PChar(FHandle.pData)[FPosition], Size);
      Assert('AWriteAudioData', AWriteAudioData(@FHandle, FPosition, Size));
      Inc(FPosition, Size);
      if FPosition >= FHandle.dwLength then
        Dec(FPosition, FHandle.dwLength);
      Dec(Count, Size);
    end;
  end;
end;

function TWaveform.GetHandle: PAudioWave;
begin
  Result := @FHandle;
end;

procedure TWaveform.SetSampleRate(Value: Word);
begin
  if (Value >= AUDIO_MIN_FREQUENCY) and (Value <= AUDIO_MAX_FREQUENCY) then
    FHandle.nSampleRate := Value;
end;

procedure TWaveform.SetVolume(Value: Integer);
begin
  if (Value >= AUDIO_MIN_VOLUME) and (Value <= AUDIO_MAX_VOLUME) then
    FVolume := Value;
end;

procedure TWaveform.SetPanning(Value: Integer);
begin
  if (Value >= AUDIO_MIN_PANNING) and (Value <= AUDIO_MAX_PANNING) then
    FPanning := Value;
end;

procedure TWaveform.SetPosition(Value: LongInt);
begin
  if (Value >= 0) and (Value < FHandle.dwLength) then
    FPosition := Value;
end;


{ TVoice}
constructor TVoice.Create;
begin
  inherited Create;
  FWave := nil;
  Assert('ACreateAudioVoice', ACreateAudioVoice(FHandle));
end;

destructor TVoice.Destroy;
begin
  if FHandle <> 0 then
    Assert('ADestroyAudioVoice', ADestroyAudioVoice(FHandle));
  inherited Destroy;
end;

procedure TVoice.Prime(Wave: TWaveform);
begin
  if Assigned(Wave) then
  begin
    FWave := Wave;
    Assert('APrimeVoice', APrimeVoice(FHandle, FWave.Handle));
    Assert('ASetVoiceFrequency', ASetVoiceFrequency(FHandle, FWave.SampleRate));
    Assert('ASetVoiceVolume', ASetVoiceVolume(FHandle, FWave.Volume));
    Assert('ASetVoicePanning', ASetVoicePanning(FHandle, FWave.Panning));
  end;
end;

procedure TVoice.Play(Wave: TWaveform);
begin
  if Assigned(Wave) then
  begin
    FWave := Wave;
    Assert('APrimeVoice', APrimeVoice(FHandle, FWave.Handle));
    Assert('ASetVoiceFrequency', ASetVoiceFrequency(FHandle, FWave.SampleRate));
    Assert('ASetVoiceVolume', ASetVoiceVolume(FHandle, FWave.Volume));
    Assert('ASetVoicePanning', ASetVoicePanning(FHandle, FWave.Panning));
    Assert('AStartVoice', AStartVoice(FHandle));
  end;
end;

procedure TVoice.Start;
begin
  Assert('AStartVoice', AStartVoice(FHandle));
end;

procedure TVoice.Stop;
begin
  Assert('AStopVoice', AStopVoice(FHandle));
end;

function TVoice.GetHandle: HAC;
begin
  Result := FHandle;
end;

function TVoice.GetWaveform: TWaveform;
begin
  Result := FWave;
end;

function TVoice.GetPosition: Longint;
var
  Value: Longint;
begin
  Assert('AGetVoicePosition', AGetVoicePosition(FHandle, Value));
  Result := Value;
end;

procedure TVoice.SetPosition(Value: Longint);
begin
  Assert('ASetVoicePosition', ASetVoicePosition(FHandle, Value));
end;

function TVoice.GetFrequency: Longint;
var
  Value: Longint;
begin
  Assert('AGetVoiceFrequency', AGetVoiceFrequency(FHandle, Value));
  Result := Value;
end;

procedure TVoice.SetFrequency(Value: Longint);
begin
  Assert('ASetVoiceFrequency', ASetVoiceFrequency(FHandle, Value));
end;

function TVoice.GetVolume: Integer;
var
  Value: Integer;
begin
  Assert('AGetVoiceVolume', AGetVoiceVolume(FHandle, Value));
  Result := Value;
end;

procedure TVoice.SetVolume(Value: Integer);
begin
  Assert('ASetVoiceVolume', ASetVoiceVolume(FHandle, Value));
end;

function TVoice.GetPanning: Integer;
var
  Value: Integer;
begin
  Assert('AGetVoicePanning', AGetVoicePanning(FHandle, Value));
  Result := Value;
end;

procedure TVoice.SetPanning(Value: Integer);
begin
  Assert('ASetVoicePanning', ASetVoicePanning(FHandle, Value));
end;

function TVoice.GetStopped: Boolean;
var
  Value: Integer;
begin
  Assert('AGetVoiceStatus', AGetVoiceStatus(FHandle, Value));
  Result := Value <> 0;
end;


{ TModule }
constructor TModule.LoadFromFile(FileName: String);
var
  szFileName: Array [0..255] of Char;
begin
  inherited Create;
  FHandle := nil;
  FCallback := nil;
  Assert('ALoadModuleFile', ALoadModuleFile(StrPCopy(szFileName, FileName), FHandle, 0));
end;

destructor TModule.Destroy;
begin
  if Assigned(FHandle) then
  begin
    if not Stopped then Stop;
    Assert('AFreeModuleFile', AFreeModuleFile(FHandle));
  end;
  inherited Destroy;
end;

procedure TModule.Play;
begin
  if Assigned(FHandle) and SetPlayingModule(FHandle) then
    Assert('APlayModule', APlayModule(FHandle));
end;

procedure TModule.Stop;
begin
  if Assigned(FHandle) and (PlayingModule = FHandle) then
  begin
    Assert('AStopModule', AStopModule);
    PlayingModule := nil;
  end;
end;

procedure TModule.Pause;
begin
  if Assigned(FHandle) and (PlayingModule = FHandle) then
    Assert('APauseModule', APauseModule);
end;

procedure TModule.Resume;
begin
  if Assigned(FHandle) and (PlayingModule = FHandle) then
    Assert('AResumeModule', AResumeModule);
end;

function TModule.GetVolume: Integer;
var
  Value: Integer;
begin
  Result := 0;
  if Assigned(FHandle) and (PlayingModule = FHandle) then
  begin
    Assert('AGetModuleVolume', AGetModuleVolume(Value));
    Result := Value;
  end;
end;

procedure TModule.SetVolume(Value: Integer);
begin
  if Assigned(FHandle) and (PlayingModule = FHandle) then
    Assert('ASetModuleVolume', ASetModuleVolume(Value));
end;

function TModule.GetOrder: Integer;
var
  Order, Row: Integer;
begin
  Result := 0;
  if Assigned(FHandle) and (PlayingModule = FHandle) then
  begin
    Assert('AGetModulePosition', AGetModulePosition(Order, Row));
    Result := Order;
  end;
end;

procedure TModule.SetOrder(Value: Integer);
begin
  if Assigned(FHandle) and (PlayingModule = FHandle) then
    Assert('ASetModulePosition', ASetModulePosition(Value, Row));
end;

function TModule.GetRow: Integer;
var
  Order, Row: Integer;
begin
  Result := 0;
  if Assigned(FHandle) and (PlayingModule = FHandle) then
  begin
    Assert('AGetModulePosition', AGetModulePosition(Order, Row));
    Result := Row;
  end;
end;

procedure TModule.SetRow(Value: Integer);
begin
  if Assigned(FHandle) and (PlayingModule = FHandle) then
    Assert('ASetModulePosition', ASetModulePosition(Order, Value));
end;

function TModule.GetStopped: Boolean;
var
  Value: Integer;
begin
  Result := True;
  if Assigned(FHandle) and (PlayingModule = FHandle) then
  begin
    Assert('AGetModuleStatus', AGetModuleStatus(Value));
    Result := Value <> 0;
  end;
end;

procedure TModule.SetCallback(Value: TAudioCallback);
begin
  if Assigned(FHandle) and (PlayingModule = FHandle) then
  begin
    FCallback := Value;
    Assert('ASetModuleCallback', ASetModuleCallback(Value));
  end;
end;

function TModule.GetName: String;
begin
  Result := '';
  if Assigned(FHandle) then
    Result := StrPas(FHandle^.szModuleName);
end;

function TModule.GetNumOrders: Integer;
begin
  Result := 0;
  if Assigned(FHandle) then
    Result := FHandle^.nOrders;
end;

function TModule.GetNumTracks: Integer;
begin
  Result := 0;
  if Assigned(FHandle) then
    Result := FHandle^.nTracks;
end;

function TModule.GetNumPatterns: Integer;
begin
  Result := 0;
  if Assigned(FHandle) then
    Result := FHandle^.nPatterns;
end;

function TModule.GetNumPatches: Integer;
begin
  Result := 0;
  if Assigned(FHandle) then
    Result := FHandle^.nPatches;
end;

function TModule.GetPatch(Index: Integer): PAudioPatch;
begin
  Result := nil;
  if Assigned(FHandle) then
  begin
    if (Index >= 1) and (Index <= FHandle^.nPatches) then
      Result := PAudioPatch(@PChar(FHandle^.aPatchTable)[sizeof(TAudioPatch) * Pred(Index)]);
  end;
end;

end.
