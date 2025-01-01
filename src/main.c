#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <portaudio.h>

#define PI acos(-1)
#define SAMPLE_RATE 44100
#define AMPLITUDE_SCALING 3000 // 16-bit amplitude scaling factor

typedef enum TONE_WAVEFORM
{
    TONE_WAVEFORM_SINE,
    TONE_WAVEFORM_SQUARE,
    TONE_WAVEFORM_TRIANGLE,
    TONE_WAVEFORM_SAWTOOTH,
} TONE_WAVEFORM;

typedef struct Tone
{
    int frequency;
    float duration;
    TONE_WAVEFORM waveform;
} Tone;

typedef struct Melody
{
    int numTones;
    Tone *tones;
} Melody;

typedef struct Signal
{
    short *samples; // Use short for 16-bit PCM audio
    int length;
} Signal;

Signal generate_tone(int frequency, float duration_seconds, TONE_WAVEFORM waveform)
{
    int toneLengthInSamples = duration_seconds * SAMPLE_RATE;
    short *tone = calloc(toneLengthInSamples, sizeof(short)); // Dynamically allocate memory with zero initialization

    Signal signal = {
        .samples = tone,
        .length = toneLengthInSamples,
    };

    // Generate the tone
    for (int i = 0; i < toneLengthInSamples; i++)
    {
        if (waveform == TONE_WAVEFORM_SINE)
        {
            tone[i] = (short)(sin(2 * PI * frequency * i / SAMPLE_RATE) * AMPLITUDE_SCALING); // Use amplitude scaling factor
        }
        else if (waveform == TONE_WAVEFORM_SQUARE)
        {
            tone[i] = (short)(sin(2 * PI * frequency * i / SAMPLE_RATE) > 0 ? AMPLITUDE_SCALING : -AMPLITUDE_SCALING);
        }
        else if (waveform == TONE_WAVEFORM_TRIANGLE)
        {
            float period = (float)SAMPLE_RATE / frequency;
            float currentPeriod = fmod(i / period, 1.0);
            float triangle = 2.0 * fabs(2.0 * currentPeriod - 1.0) - 1.0;
            tone[i] = (short)(triangle * AMPLITUDE_SCALING);
        }
        else if (waveform == TONE_WAVEFORM_SAWTOOTH)
        {
            float period = SAMPLE_RATE / frequency;
            float currentPeriod = i / period;
            float sawtooth = 2 * (currentPeriod - floor(currentPeriod)) - 1;
            tone[i] = (short)(sawtooth * AMPLITUDE_SCALING);
        }
    }

    return signal;
}

int play_signal(Signal *signal)
{
    PaError err;
    err = Pa_Initialize();
    if (err != paNoError)
    {
        printf("PortAudio error: %s\n", Pa_GetErrorText(err));
        return 1;
    }

    PaStream *stream;
    int framesPerBuffer = 256;
    err = Pa_OpenDefaultStream(&stream, 0, 1, paInt16, SAMPLE_RATE, framesPerBuffer, NULL, NULL);
    if (err != paNoError)
    {
        printf("PortAudio error: %s\n", Pa_GetErrorText(err));
        return 1;
    }

    err = Pa_StartStream(stream);
    if (err != paNoError)
    {
        printf("PortAudio error: %s\n", Pa_GetErrorText(err));
        return 1;
    }

    // Write the audio stream, chunk by chunk
    int frameIndex = 0;
    int numFrames = signal->length;
    while (frameIndex < numFrames)
    {
        short sample = signal->samples[frameIndex++];
        err = Pa_WriteStream(stream, &sample, 1);
        if (err != paNoError)
        {
            printf("PortAudio error: %s\n", Pa_GetErrorText(err));
            return 1;
        }
    }

    // Stop and close the stream
    err = Pa_StopStream(stream);
    if (err != paNoError)
    {
        printf("PortAudio error: %s\n", Pa_GetErrorText(err));
        return 1;
    }

    err = Pa_CloseStream(stream);
    if (err != paNoError)
    {
        printf("PortAudio error: %s\n", Pa_GetErrorText(err));
        return 1;
    }

    Pa_Terminate();

    return 0;
}

int main(void)
{
    printf("\tHello Sailor!\n");

    Melody melody = {
        .numTones = 4, // @Note(Victor): Remember to change this value when adding or removing tones
        .tones = (Tone[]){
            {440, 0.5, TONE_WAVEFORM_SINE},
            {440, 0.5, TONE_WAVEFORM_SAWTOOTH},
            {440, 0.5, TONE_WAVEFORM_SQUARE},
            {440, 0.5, TONE_WAVEFORM_TRIANGLE},
        },
    };

    Signal result = {
        .samples = NULL,
        .length = 0,
    };

    for (int i = 0; i < melody.numTones; i++)
    {
        Signal tone = generate_tone(melody.tones[i].frequency, melody.tones[i].duration, melody.tones[i].waveform);
        if (tone.samples == NULL)
        {
            printf("\tFailed to generate tone\n");
            return 1;
        }

        // Concatenate the tones
        int oldLength = result.length;
        result.length += tone.length;
        result.samples = realloc(result.samples, result.length * sizeof(short));
        for (int j = 0; j < tone.length; j++)
        {
            result.samples[oldLength + j] = tone.samples[j];
        }

        free(tone.samples); // Free the allocated memory after concatenation
    }

    if (result.samples == NULL)
    {
        printf("\tFailed to generate melody\n");
        return 1;
    }

    printf("\tGenerated melody\n");

    int playResult = play_signal(&result);
    if (playResult != 0)
    {
        printf("\tFailed to play melody\n");
        free(result.samples); // Free the allocated memory in case of failure
        return 1;
    }

    free(result.samples); // Free the allocated memory after playing

    return 0;
}
