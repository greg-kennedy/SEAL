/* check.c - misc. routines from the reference guide */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#ifndef linux
#include <conio.h>
#endif
#include <audio.h>


/* enable filtering */
#define FILTER


/* soy paranoico! */
#define Assert(x) AssertFailed((x),#x,__LINE__,__FILE__)

void AssertFailed(UINT rc, LPSTR lpszExpr, int nLine, LPSTR lpszFileName)
{
    CHAR szText[128];

    if (rc != AUDIO_ERROR_NONE) {
        AGetErrorText(rc, szText, sizeof(szText) - 1);
        fprintf(stderr, "ASSERT(%s:%d): %s\nERROR: %s\n",
            lpszFileName, nLine, lpszExpr, szText);
        ACloseAudio();
        exit(1);
    }
}



/* APingAudio, AGetAudioDevCaps */
VOID DetectAudioDevice(VOID)
{
    AUDIOCAPS caps;
    UINT nDeviceId;
    if (APingAudio(&nDeviceId) != AUDIO_ERROR_NONE)
        printf("no audio found\n");
    else {
        AGetAudioDevCaps(nDeviceId, &caps);
        printf("%s device found\n", caps.szProductName);
    }
}


/* AGetAudioDevCaps, AGetAudioNumDevs */
VOID PrintAudioDevs(VOID)
{
    AUDIOCAPS caps;
    UINT nDeviceId;
    for (nDeviceId = 0; nDeviceId < AGetAudioNumDevs(); nDeviceId++) {
        Assert(AGetAudioDevCaps(nDeviceId, &caps));
        printf("nDeviceId=%d wProductId=%d szProductName=%s\n",
            nDeviceId, caps.wProductId, caps.szProductName);
    }
}


/* AOpenAudio, AGetErrorText */
VOID InitializeAudio(VOID)
{
    AUDIOINFO info;
    CHAR szText[128];
    UINT rc;

    info.nDeviceId = AUDIO_DEVICE_MAPPER;
    info.wFormat = AUDIO_FORMAT_16BITS | AUDIO_FORMAT_STEREO;
    #ifdef FILTER
    info.wFormat |= AUDIO_FORMAT_FILTER;
    #endif
    info.nSampleRate = 44100;
    if ((rc = AOpenAudio(&info)) != AUDIO_ERROR_NONE) {
        AGetErrorText(rc, szText, sizeof(szText) - 1);
        printf("ERROR: %s\n", szText);
        exit(1);
    }
    else {
        printf("Audio device initialized at %d bits %s %u Hz\n",
            info.wFormat & AUDIO_FORMAT_16BITS ? 16 : 8,
            info.wFormat & AUDIO_FORMAT_STEREO ?
                "stereo" : "mono", info.nSampleRate);
    }
}


/* ASetAudioTimerProc, ASetAudioTimerRate */
volatile UINT nTickCounter = 0;

VOID AIAPI TimerHandler(VOID)
{
    nTickCounter++;
}

VOID InitTimerHandler(VOID)
{
    Assert(ASetAudioTimerProc(TimerHandler));
    Assert(ASetAudioTimerRate(125));
}

VOID DoneTimerHandler(VOID)
{
    Assert(ASetAudioTimerProc(NULL));
}

VOID TestTimerServices(VOID)
{
    InitTimerHandler();
    do {
        #ifndef WIN32
        Assert(AUpdateAudio());
        #endif
        fprintf(stderr, "Elapsed: %2.2f secs\r", nTickCounter / 50.0F);
    } while (nTickCounter < 2*50);
    DoneTimerHandler();
}


/* ACreateAudioVoice, ADestroyAudioVoice, APlayVoice, ASetVoiceXXX */
VOID PlayWaveform(LPAUDIOWAVE lpWave)
{
    HAC hVoice;
    BOOL stopped;
    Assert(ACreateAudioVoice(&hVoice));
    Assert(APlayVoice(hVoice, lpWave));
    Assert(ASetVoiceVolume(hVoice, 64));
    Assert(ASetVoicePanning(hVoice, 128));
    printf("press any key to stop\n");
    while (!kbhit()) {
        #ifndef WIN32
        Assert(AUpdateAudio());
        #endif
        Assert(AGetVoiceStatus(hVoice, &stopped));
        if (stopped) break;
    }
    if (kbhit()) getch();
    Assert(AStopVoice(hVoice));
    Assert(ADestroyAudioVoice(hVoice));
}

VOID TestPlayWaveform(VOID)
{
    LPAUDIOWAVE lpWave;
    Assert(ALoadWaveFile("test.wav", &lpWave, 0));
    lpWave->wFormat |= AUDIO_FORMAT_LOOP;
    lpWave->dwLoopStart = 0;
    lpWave->dwLoopEnd = lpWave->dwLength;
    Assert(AOpenVoices(1));
    PlayWaveform(lpWave);
    Assert(ACloseVoices());
    Assert(AFreeWaveFile(lpWave));
}


/* APrimeVoice, AStartVoice, ASetVoiceXXX */
VOID PlayChord(HAC aVoices[3], LPAUDIOWAVE lpWave, LONG aFreqs[3])
{
    UINT n;
    for (n = 0; n < 3; n++) {
        Assert(APrimeVoice(aVoices[n], lpWave));
        Assert(ASetVoiceFrequency(aVoices[n], aFreqs[n]));
        Assert(ASetVoiceVolume(aVoices[n], 64));
    }
    for (n = 0; n < 3; n++) {
        Assert(AStartVoice(aVoices[n]));
    }
}

VOID TestPlayChord(VOID)
{
    LPAUDIOWAVE lpWave;
    HAC aVoices[3];
    LONG aFreqs[3] = { (8*6000)/8, (8*6000)/6, (8*6000)/5 };
    UINT n;
    Assert(ALoadWaveFile("test.wav", &lpWave, 0));
    Assert(AOpenVoices(3));
    for (n = 0; n < 3; n++)
        Assert(ACreateAudioVoice(&aVoices[n]));
    printf("press any key to stop\n");
    InitTimerHandler();
    while (!kbhit()) {
        #ifndef WIN32
        Assert(AUpdateAudio());
        #endif
        /* play chord two times per second */
        if (nTickCounter >= 25) {
            PlayChord(aVoices, lpWave, aFreqs);
            nTickCounter -= 25;
        }
    }
    if (kbhit()) getch();
    DoneTimerHandler();
    for (n = 0; n < 3; n++) {
        Assert(AStopVoice(aVoices[n]));
        Assert(ADestroyAudioVoice(aVoices[n]));
    }
    Assert(ACloseVoices());
    Assert(AFreeWaveFile(lpWave));
}


/* ASetVoicePosition, AGetVoicePosition */
VOID PlayEchoVoices(HAC aVoices[2], LPAUDIOWAVE lpWave, LONG dwDelay)
{
    Assert(APrimeVoice(aVoices[0], lpWave));
    Assert(APrimeVoice(aVoices[1], lpWave));
    Assert(ASetVoiceFrequency(aVoices[0], lpWave->nSampleRate / 2));
    Assert(ASetVoiceFrequency(aVoices[1], lpWave->nSampleRate / 2));
    Assert(ASetVoiceVolume(aVoices[0], 64));
    Assert(ASetVoiceVolume(aVoices[1], 48));
    Assert(ASetVoicePosition(aVoices[1], dwDelay));
    Assert(AStartVoice(aVoices[0]));
    Assert(AStartVoice(aVoices[1]));
}

VOID TestPlayEcho(VOID)
{
    LPAUDIOWAVE lpWave;
    HAC aVoices[2];
    UINT n;
    Assert(ALoadWaveFile("test.wav", &lpWave, 0));
    Assert(AOpenVoices(2));
    for (n = 0; n < 2; n++)
        Assert(ACreateAudioVoice(&aVoices[n]));
    printf("press any key to stop\n");
    InitTimerHandler();
    while (!kbhit()) {
        #ifndef WIN32
        Assert(AUpdateAudio());
        #endif
        /* play voices two times per second */
        if (nTickCounter >= 25) {
            PlayEchoVoices(aVoices, lpWave, 800);
            nTickCounter -= 25;
        }
    }
    if (kbhit()) getch();
    DoneTimerHandler();
    for (n = 0; n < 2; n++) {
        Assert(AStopVoice(aVoices[n]));
        Assert(ADestroyAudioVoice(aVoices[n]));
    }
    Assert(ACloseVoices());
    Assert(AFreeWaveFile(lpWave));
}


/* ASetVoiceFrequency */
VOID PlayVoiceStereo(HAC aVoices[2], LPAUDIOWAVE lpWave, LONG dwPitchShift)
{
    Assert(APrimeVoice(aVoices[0], lpWave));
    Assert(APrimeVoice(aVoices[1], lpWave));
    Assert(ASetVoiceVolume(aVoices[0], 64));
    Assert(ASetVoiceVolume(aVoices[1], 64));
    Assert(ASetVoiceFrequency(aVoices[0], lpWave->nSampleRate));
    Assert(ASetVoiceFrequency(aVoices[1], lpWave->nSampleRate + dwPitchShift));
    Assert(ASetVoicePanning(aVoices[0], 0));
    Assert(ASetVoicePanning(aVoices[1], 255));
    Assert(AStartVoice(aVoices[0]));
    Assert(AStartVoice(aVoices[1]));
}

VOID TestPlayStereoEnh(VOID)
{
    LPAUDIOWAVE lpWave;
    HAC aVoices[2];
    UINT n;
    Assert(ALoadWaveFile("test.wav", &lpWave, 0));
    Assert(AOpenVoices(2));
    for (n = 0; n < 2; n++)
        Assert(ACreateAudioVoice(&aVoices[n]));
    printf("press any key to stop\n");
    InitTimerHandler();
    while (!kbhit()) {
        #ifndef WIN32
        Assert(AUpdateAudio());
        #endif
        /* play voices two times per second */
        if (nTickCounter >= 25) {
            PlayVoiceStereo(aVoices, lpWave, 100);
            nTickCounter -= 25;
        }
    }
    if (kbhit()) getch();
    DoneTimerHandler();
    for (n = 0; n < 2; n++) {
        Assert(AStopVoice(aVoices[n]));
        Assert(ADestroyAudioVoice(aVoices[n]));
    }
    Assert(ACloseVoices());
    Assert(AFreeWaveFile(lpWave));
}


/* ACreateAudioData, AWriteAudioData */
LPAUDIOWAVE CreateAudio8BitMono(WORD nSampleRate,
    LPBYTE lpData, DWORD dwLength)
{
    LPAUDIOWAVE lpWave;
    if ((lpWave = (LPAUDIOWAVE) malloc(sizeof(AUDIOWAVE))) != NULL) {
        lpWave->wFormat = AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO;
        lpWave->nSampleRate = nSampleRate;
        lpWave->dwLength = dwLength;
        lpWave->dwLoopStart = lpWave->dwLoopEnd = 0L;
        Assert(ACreateAudioData(lpWave));
        memcpy(lpWave->lpData, lpData, dwLength);
        Assert(AWriteAudioData(lpWave, 0L, dwLength));
    }
    return lpWave;
}

VOID TestCreateAudioData(VOID)
{
    LPAUDIOWAVE lpWave;
    HAC hVoice;
    static BYTE aData[4000];
    UINT n;

    /* create 500 Hz sinewave (sampled at 4 kHz) */
    for (n = 0; n < sizeof(aData); n++)
        aData[n] = (BYTE)(127.0 * sin((500.0 * 3.141592653 * n) / sizeof(aData)));

    lpWave = CreateAudio8BitMono(4000, aData, sizeof(aData));
    if (lpWave == NULL) {
        printf("not enough memory\n");
        return;
    }
    Assert(AOpenVoices(1));
    Assert(ACreateAudioVoice(&hVoice));
    printf("press any key to stop\n");
    InitTimerHandler();
    while (!kbhit()) {
        #ifndef WIN32
        Assert(AUpdateAudio());
        #endif
        /* play voices two times per second */
        if (nTickCounter >= 25) {
            Assert(APlayVoice(hVoice, lpWave));
            Assert(ASetVoiceVolume(hVoice, 64));
            nTickCounter -= 25;
        }
    }
    if (kbhit()) getch();
    DoneTimerHandler();
    Assert(AStopVoice(hVoice));
    Assert(ADestroyAudioVoice(hVoice));
    Assert(ACloseVoices());
    Assert(ADestroyAudioData(lpWave));
    free(lpWave);
}


/* ACreateAudioData, AWriteAudioData */
VOID StreamData8BitMono(FILE *stream, HAC hVoice, LPAUDIOWAVE lpWave)
{
    static BYTE aBuffer[1024];
    LPBYTE lpChunk;
    UINT nLength, nChunkSize;
    DWORD dwOffset;
    static LONG dwVoicePosition;

    if (2*sizeof(aBuffer) > lpWave->dwLength) {
        printf("the waveform is too small\n");
        return;
    }
    memset(lpWave->lpData, 0x80, lpWave->dwLength);
    Assert(AWriteAudioData(lpWave, 0L, lpWave->dwLength));
    lpWave->wFormat |= AUDIO_FORMAT_LOOP;
    lpWave->dwLoopStart = 0L;
    lpWave->dwLoopEnd = lpWave->dwLength;
    Assert(APlayVoice(hVoice, lpWave));
    Assert(ASetVoiceVolume(hVoice, 64));
    dwOffset = 0L;
    while ((nLength = fread(aBuffer, 1, sizeof(aBuffer), stream)) != 0) {
        if (kbhit()) break;

        #ifndef SIGNED
        {
            UINT n;
            for (n = 0; n < nLength; n++)
                aBuffer[n] ^= 0x80;
        }
        #endif

        lpChunk = aBuffer;
        while (nLength > 0) {
            nChunkSize = nLength;
            if (dwOffset + nChunkSize > lpWave->dwLength)
                nChunkSize = lpWave->dwLength - dwOffset;
            for (;;) {
                #ifndef WIN32
                Assert(AUpdateAudio());
                #endif
                Assert(AGetVoicePosition(hVoice, &dwVoicePosition));
                if (dwOffset + nChunkSize > lpWave->dwLength) {
                    if (dwVoicePosition < dwOffset &&
                        dwVoicePosition > dwOffset +
                            nChunkSize - lpWave->dwLength)
                            break;
                }
                else {
                    if (dwVoicePosition < dwOffset ||
                        dwVoicePosition > dwOffset + nChunkSize)
                            break;
                }
            }
            memcpy(lpWave->lpData + dwOffset, lpChunk, nChunkSize);
            Assert(AWriteAudioData(lpWave, dwOffset, nChunkSize));
            if ((dwOffset += nChunkSize) >= lpWave->dwLength)
                dwOffset = 0L;
            lpChunk += nChunkSize;
            nLength -= nChunkSize;
        }
    }
    if (kbhit()) getch();
}

VOID TestAudioStream(VOID)
{
    FILE *stream;
    HAC hVoice;
    AUDIOWAVE Wave;

    /* open .wav file and skip header structure */
    if ((stream = fopen("8mono.wav", "rb")) == NULL) {
        printf("cant open raw 8-bit mono file\n");
        return;
    }
    fseek(stream, 48L, SEEK_SET);

    /* start playing the "data" chunk of the .wav file */
    Assert(AOpenVoices(1));
    Assert(ACreateAudioVoice(&hVoice));
    Wave.wFormat = AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO;
    Wave.nSampleRate = 11025;
    Wave.dwLength = Wave.dwLoopEnd = 10000;
    Wave.dwLoopStart = 0;
    Assert(ACreateAudioData(&Wave));
    printf("press any key to stop\n");
    StreamData8BitMono(stream, hVoice, &Wave);
    Assert(AStopVoice(hVoice));
    Assert(ADestroyAudioVoice(hVoice));
    Assert(ACloseVoices());
    Assert(ADestroyAudioData(&Wave));
    fclose(stream);
}


void main(void)
{
    #ifndef WIN32
    AInitialize();
    #endif

    printf("------------ DetectAudioDevice() ------------\n");
    DetectAudioDevice();
    printf("------------ PrintAudioDevs() ---------------\n");
    PrintAudioDevs();
    printf("------------ InitializeAudio() --------------\n");
    InitializeAudio();
    printf("------------ TestTimerServices() ------------\n");
    TestTimerServices();
    printf("------------ TestPlayWaveform() -------------\n");
    TestPlayWaveform();
    printf("------------ TestPlayChord() ----------------\n");
    TestPlayChord();
    printf("------------ TestPlayEcho() -----------------\n");
    TestPlayEcho();
    printf("------------ TestPlayStereoEnh() ------------\n");
    TestPlayStereoEnh();
    printf("------------ TestCreateAudioData() ----------\n");
    TestCreateAudioData();
    printf("------------ TestAudioStream() --------------\n");
    TestAudioStream();
    printf("------------ ACloseAudio() ------------------\n");
    Assert(ACloseAudio());
}
