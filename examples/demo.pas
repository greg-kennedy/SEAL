program Demo;
uses
  SysUtils, Audio;

var
  Caps: TAudioCaps;
  Info: TAudioInfo;
  pModule: PAudioModule;
  szFileName : Array [0..127] of Char;
  bStatus: Integer;
begin
  if ParamCount <> 1 then
  begin
    Writeln('use: demo filename[.mod|.s3m|.xm]');
    Halt(0);
  end;
  Info.nDeviceId := AUDIO_DEVICE_MAPPER;
  Info.wFormat := AUDIO_FORMAT_16BITS or AUDIO_FORMAT_STEREO or AUDIO_FORMAT_FILTER;
  Info.nSampleRate := 44100;
  if AOpenAudio(Info) <> 0 then
  begin
    Writeln('Audio initialization failed');
    Halt(1);
  end;
  AGetAudioDevCaps(Info.nDeviceId, Caps);
  Write(Caps.szProductName,' playing at ');
  if Info.wFormat and AUDIO_FORMAT_16BITS <> 0 then
    Write('16-bit ') else Write('8-bit ');
  if Info.wFormat and AUDIO_FORMAT_STEREO <> 0 then
    Write('stereo ') else Write('mono ');
  Writeln(Info.nSampleRate,' Hz');
  if ALoadModuleFile(StrPCopy(szFileName, ParamStr(1)), pModule, 0) <> 0 then
  begin
    Writeln('Cant load module file');
    ACloseAudio;
    Halt(1);
  end;
  AOpenVoices(pModule^.nTracks);
  APlayModule(pModule);
  while (AGetModuleStatus(bStatus) = 0) do
  begin
    if bStatus <> 0 then break;
    AUpdateAudio;
  end;
  AStopModule;
  ACloseVoices;
  AFreeModuleFile(pModule);
  ACloseAudio;
end.
