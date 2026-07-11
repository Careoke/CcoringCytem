#include <stdio.h>

#define DR_MP3_IMPLEMENTATION
#include "../headers/dr_mp3.h"

#define ORG_AUDIO "out/music.mp3"
#define INPUT_AUDIO "out/test.mp3"

int main()
{
    drmp3_config conf;
    conf.sampleRate = 44100;
    conf.channels = 1; // mono

    drmp3_uint64 Org__totalSamples = 0;
    drmp3_uint64 Input__totalSamples = 0;

    float *Org__audioData = drmp3_open_file_and_read_pcm_frames_f32(
        ORG_AUDIO,
        &conf,
        &Org__totalSamples,
        NULL);

    float *Input__audioData = drmp3_open_file_and_read_pcm_frames_f32(
        INPUT_AUDIO,
        &conf,
        &Input__totalSamples,
        NULL);

    for (drmp3_uint64 i = 0; i < Org__totalSamples; i++)
        printf("org:%f, %d/%d, inp:%f %d/%d\n", Org__audioData[i], i, Org__totalSamples, Input__audioData[i], i, Org__totalSamples);

    free(Org__audioData);
    free(Input__audioData);

    return 0;
}