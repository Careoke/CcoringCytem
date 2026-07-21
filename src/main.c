#include <stdio.h>
#include <math.h>
#include "../headers/Yin.h"

#define DR_MP3_IMPLEMENTATION
#include "../headers/dr_mp3.h"

#define ORG_AUDIO "out/music.mp3"
#define INPUT_AUDIO "out/test2.mp3"

#define BUFFER_SZ 4096
#define HOP_SZ 441

float CorrectPitch(float original, float input)
{
    float bestPitch = input;
    float bestError = fabsf(1200.0f * log2f(input / original));
    float p = input;

    while (1)
    {
        float next = p / 2.0f;
        float err = fabsf(1200.0f * log2f(next / original));

        if (err >= bestError)
            break;

        bestError = err;
        bestPitch = next;
        p = next;
    }

    p = input;

    while (1)
    {
        float next = p * 2.0f;
        float err = fabsf(1200.0f * log2f(next / original));

        if (err >= bestError)
            break;

        bestError = err;
        bestPitch = next;
        p = next;
    }

    return bestPitch;
}

int main()
{
    drmp3_config conf1;
    drmp3_config conf2;

    float total_error = 0.0f;
    drmp3_uint64 total_frames = 0;
    float yin_threshold = 0.15f;
    float pitch_org;
    float pitch_inp;
    float frame_error = 0.0f;
    float frame_score;
    float total_score = 0.0f;

    drmp3_uint64 Org__totalSamples = 0;
    drmp3_uint64 Input__totalSamples = 0;

    Yin yin_org;
    Yin yin_inp;
    Yin_init(&yin_org, BUFFER_SZ, yin_threshold);
    Yin_init(&yin_inp, BUFFER_SZ, yin_threshold);

    drmp3_int16 *tmpOrg__audioData = drmp3_open_file_and_read_pcm_frames_s16(
        ORG_AUDIO,
        &conf1,
        &Org__totalSamples,
        NULL);

    drmp3_int16 *tmpInput__audioData = drmp3_open_file_and_read_pcm_frames_s16(
        INPUT_AUDIO,
        &conf2,
        &Input__totalSamples,
        NULL);

    drmp3_int16 *Input__audioData = malloc(Input__totalSamples * sizeof(drmp3_int16));
    drmp3_int16 *Org__audioData = malloc(Org__totalSamples * sizeof(drmp3_int16));

    if (conf1.channels == 1)
        memcpy(Org__audioData, tmpOrg__audioData, Org__totalSamples * sizeof(int16_t));

    else
        for (drmp3_uint64 i = 0; i < Org__totalSamples; i++)
            Org__audioData[i] = (int16_t)(((int32_t)tmpOrg__audioData[i * 2] + (int32_t)tmpOrg__audioData[i * 2 + 1]) / 2);

    if (conf2.channels == 1)
        memcpy(Input__audioData, tmpInput__audioData, Input__totalSamples * sizeof(int16_t));

    else
        for (drmp3_uint64 i = 0; i < Input__totalSamples; i++)
            Input__audioData[i] = (int16_t)(((int32_t)tmpInput__audioData[i * 2] + (int32_t)tmpInput__audioData[i * 2 + 1]) / 2);

    drmp3_uint64 minSamp;
    if (Org__totalSamples < Input__totalSamples) // take the smallest sample so we don't overflow
        minSamp = Org__totalSamples;
    else if (Org__totalSamples == Input__totalSamples)
        minSamp = Input__totalSamples;
    else
        minSamp = Input__totalSamples;

    size_t maxFrames = minSamp / HOP_SZ;
    float *orgPitchs = malloc(maxFrames * sizeof(float));
    float *inpPitchs = malloc(maxFrames * sizeof(float));
    int validFrames = 0;

    for (drmp3_uint64 pos = 0; pos + BUFFER_SZ < minSamp; pos += HOP_SZ)
    {
        float rms = 0.0f;
        for (int i = 0; i < BUFFER_SZ; i++)
        {
            float s = Org__audioData[pos + i] / 32768.0f;
            rms += s * s;
        }
        rms = sqrtf(rms / BUFFER_SZ);
        if (rms < 0.01f)
            continue;
        rms = 0.0f;
        for (int i = 0; i < BUFFER_SZ; i++)
        {
            float s = Input__audioData[pos + i] / 32768.0f;
            rms += s * s;
        }
        rms = sqrtf(rms / BUFFER_SZ);
        if (rms < 0.01f)
            continue;

        pitch_org = Yin_getPitch(&yin_org, Org__audioData + pos);
        pitch_inp = Yin_getPitch(&yin_inp, Input__audioData + pos);
        if (pitch_org <= 0 || pitch_inp <= 0)
            continue;
        pitch_inp = CorrectPitch(pitch_org, pitch_inp);

        if (Yin_getProbability(&yin_org) < 0.7f ||
            Yin_getProbability(&yin_inp) < 0.7f)
            continue;

        if (pitch_org < 60 || pitch_org > 1200)
            continue;

        if (pitch_inp < 60 || pitch_inp > 1200)
            continue;

        orgPitchs[validFrames] = pitch_org;
        inpPitchs[validFrames] = pitch_inp;
        validFrames++;

        frame_error = fabsf(1200.0f * log2f(pitch_inp / pitch_org)); // in cents
        // printf(
        //     "Org %.2f  Inp %.2f  Error %.2f cents\n",
        //     orgPitchs[validFrames - 1],
        //     inpPitchs[validFrames - 1],
        //     frame_error);

        total_error += frame_error;
        if (frame_error < 25)
            frame_score = 100;
        else if (frame_error < 50)
            frame_score = 90;
        else if (frame_error < 100)
            frame_score = 75;
        else if (frame_error < 200)
            frame_score = 50;
        else if (frame_error < 400)
            frame_score = 20;
        else
            frame_score = 0;

        total_score += frame_score;
        total_frames++;
    }

    if (total_frames > 0)
    {
        float final_score = total_score / total_frames;
        printf("Total Processed Frames: %llu\n", total_frames);
        printf("Average error: %.2f cents\n", total_error / total_frames);
        printf("Final Singing Score: %.2f\n", final_score);
    }

    free(Org__audioData);
    free(Input__audioData);
    free(orgPitchs);
    free(inpPitchs);
    drmp3_free(tmpOrg__audioData, NULL);
    drmp3_free(tmpInput__audioData, NULL);

    return 0;
}