/* example3.c - play a waveform file */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <audio.h>

#if defined(_MSC_VER) || defined(__WATCOMC__) || defined(__BORLANDC__) || defined(__DJGPP__)
#include <conio.h>
#else
#define kbhit() 0
#endif

#define NUMVOICES 3*3

#define FREQ(nPeriod) (((LONG) lpWave->nSampleRate * 428) / nPeriod)

UINT aPeriodTable[48] =
{ /* C   C#  D   D#  E   F   F#  G   G#  A   A#  B */
    856,808,762,720,678,640,604,570,538,508,480,453,
    428,404,381,360,339,320,302,285,269,254,240,226,
    214,202,190,180,170,160,151,143,135,127,120,113,
    107,101,95,90,85,80,75,71,67,63,60,56
};

int main(void)
{
    AUDIOINFO info;
    LPAUDIOWAVE lpWave;
    HAC hVoice[NUMVOICES];
    BOOL stopped;
    UINT n, m;

    /* initialize audio library */
    AInitialize();

    /* open audio device */
    info.nDeviceId = AUDIO_DEVICE_MAPPER;
    info.wFormat = AUDIO_FORMAT_16BITS | AUDIO_FORMAT_STEREO;
    info.nSampleRate = 44100;
    AOpenAudio(&info);

    /* load waveform file */
    ALoadWaveFile("test.wav", &lpWave, 0);

    /* open and allocate voices */
    AOpenVoices(NUMVOICES);
    for (n = 0; n < NUMVOICES; n++) {
        ACreateAudioVoice(&hVoice[n]);
        ASetVoiceVolume(hVoice[n], 64);
        ASetVoicePanning(hVoice[n], n & 1 ? 0 : 255);
    }

    /* program main execution loop */
    printf("Playing waveform, press any key to stop.\n");
    for (n = m = 0; !kbhit() && n < 48 - 7; n++) {
        /* play chord C-E-G */
        APlayVoice(hVoice[m+0], lpWave);
        APlayVoice(hVoice[m+1], lpWave);
        APlayVoice(hVoice[m+2], lpWave);
        ASetVoiceFrequency(hVoice[m+0], FREQ(aPeriodTable[n+0]));
        ASetVoiceFrequency(hVoice[m+1], FREQ(aPeriodTable[n+4]));
        ASetVoiceFrequency(hVoice[m+2], FREQ(aPeriodTable[n+7]));
        m = (m + 3) % NUMVOICES;

        /* wait until note finishes */
        do {
            /* update audio system */
            AUpdateAudio();
            AGetVoiceStatus(hVoice[0], &stopped);
        } while (!stopped);
    }

    /* stop and release voices */
    for (n = 0; n < NUMVOICES; n++) {
        AStopVoice(hVoice[n]);
        ADestroyAudioVoice(hVoice[n]);
    }
    ACloseVoices();

    /* release the waveform file */
    AFreeWaveFile(lpWave);

    /* close audio device */
    ACloseAudio();
    return 0;
}
