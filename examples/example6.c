/* example6.c - play a streamed sample (sine wave) using triple buffering */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <audio.h>

#if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__WATCOMC__) || defined(__DJGPP__)
#include <conio.h>
#else
#define kbhit() 0
#endif

int main(void)
{
    AUDIOINFO info;
    AUDIOCAPS caps;
    AUDIOWAVE wave;
    HAC voice;
    int i;
    float t, dt;
    long position, chunkSize, chunkPosition;

    /* initialize library */
    AInitialize();

    /* open audio device */
    info.nDeviceId = AUDIO_DEVICE_MAPPER;
    info.wFormat = AUDIO_FORMAT_16BITS | AUDIO_FORMAT_STEREO;
    info.nSampleRate = 44100;
    AOpenAudio(&info);

    /* show device name */
    AGetAudioDevCaps(info.nDeviceId, &caps);
    printf("%s detected. Press any key to exit.\n", caps.szProductName);

    /* open audio voice */
    AOpenVoices(1);
    ACreateAudioVoice(&voice);
    ASetVoiceVolume(voice, 64);
    ASetVoicePanning(voice, 128);

    /* setup buffer length to 1/60th of a second */
    chunkPosition = 0;
    chunkSize = info.nSampleRate / 60;

    /* create a looped sound buffer of 3 x 1/60th secs */
    wave.nSampleRate = info.nSampleRate;
    wave.dwLength = 3 * chunkSize;
    wave.dwLoopStart = 0;
    wave.dwLoopEnd = wave.dwLength;
    wave.wFormat = AUDIO_FORMAT_8BITS | AUDIO_FORMAT_MONO | AUDIO_FORMAT_LOOP;
    ACreateAudioData(&wave);

    /* clean up sound buffer */
    memset(wave.lpData, 0, wave.dwLength);
    AWriteAudioData(&wave, 0, wave.dwLength);

    /* setup 200 Hz sine wave parameters */
    t = 0.0;
    dt = 2.0 * M_PI * 200.0 / wave.nSampleRate;

    printf("%d-bit %s %u Hz, buffer size = %ld, chunk size = %ld\n",
           info.wFormat & AUDIO_FORMAT_16BITS ? 16 : 8,
           info.wFormat & AUDIO_FORMAT_STEREO ? "stereo" : "mono",
           info.nSampleRate, wave.dwLength, chunkSize);
 
    /* start playing the sound buffer */ 
    APlayVoice(voice, &wave);

    while (!kbhit()) {
      /* do not fill more than 'chunkSize' samples */
      AUpdateAudioEx(chunkSize);

      /* update the chunk of samples at 'chunkPosition' */
      AGetVoicePosition(voice, &position);
      if (position < chunkPosition || position >= chunkPosition + chunkSize) {
        for (i = 0; i < chunkSize; i++)
          wave.lpData[chunkPosition++] = 64.0 * sin(t += dt);
        AWriteAudioData(&wave, chunkPosition - chunkSize, chunkSize);
        if (chunkPosition >= wave.dwLength)
          chunkPosition = 0;
      }
    }

    /* stop playing the buffer */
    AStopVoice(voice);

    /* release sound buffer */
    ADestroyAudioData(&wave);

    /* release audio voices */
    ADestroyAudioVoice(voice);
    ACloseVoices();

    /* close audio device */
    ACloseAudio();
    return 0;
}
