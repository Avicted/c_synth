#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <portaudio.h>

// Write straight to the NULL pointer to crash the program
#define ASSERT(condition, message, param) \
    if (!(condition))                     \
    {                                     \
        printf(message, param);           \
        *((int *)0) = 0;                  \
    }

#define PI acos(-1)
#define SAMPLE_RATE 44100
#define AMPLITUDE_SCALING 3000 // 16-bit amplitude scaling factor

unsigned long long CPU_MEMORY_ALLOCATED_IN_BYTES = 0;

typedef enum note_WAVEFORM
{
    note_WAVEFORM_SINE,
    note_WAVEFORM_SQUARE,
    note_WAVEFORM_TRIANGLE,
    note_WAVEFORM_SAWTOOTH,
} note_WAVEFORM;

typedef struct Note
{
    int frequency;
    float duration;
    note_WAVEFORM waveform;
} Note;

typedef struct Melody
{
    int numnotes;
    Note *notes;
} Melody;

typedef struct Signal
{
    short *samples; // Use short for 16-bit PCM audio
    int length;
} Signal;

Signal *generate_note(int frequency, float duration_seconds, note_WAVEFORM waveform)
{
    const int noteLengthInSamples = duration_seconds * SAMPLE_RATE;

    // Dynamically allocate memory with zero initialization
    short *note = calloc(noteLengthInSamples, sizeof(short));
    CPU_MEMORY_ALLOCATED_IN_BYTES += noteLengthInSamples * sizeof(short);

    Signal *signal = calloc(1, sizeof(Signal));
    CPU_MEMORY_ALLOCATED_IN_BYTES += sizeof(Signal);

    if (signal == NULL)
    {
        CPU_MEMORY_ALLOCATED_IN_BYTES -= noteLengthInSamples * sizeof(short);
        free(note);
        return NULL;
    }

    // Set the signal properties
    signal->samples = note;
    signal->length = noteLengthInSamples;

    // Generate the note
    for (int i = 0; i < noteLengthInSamples; i++)
    {
        if (waveform == note_WAVEFORM_SINE)
        {
            note[i] = (short)(sin(2 * PI * frequency * i / SAMPLE_RATE) * AMPLITUDE_SCALING); // Use amplitude scaling factor
        }
        else if (waveform == note_WAVEFORM_SQUARE)
        {
            note[i] = (short)(sin(2 * PI * frequency * i / SAMPLE_RATE) > 0 ? AMPLITUDE_SCALING : -AMPLITUDE_SCALING);
        }
        else if (waveform == note_WAVEFORM_TRIANGLE)
        {
            const float period = (float)SAMPLE_RATE / frequency;
            const float currentPeriod = fmod(i / period, 1.0);
            const float triangle = 2.0 * fabs(2.0 * currentPeriod - 1.0) - 1.0;
            note[i] = (short)(triangle * AMPLITUDE_SCALING);
        }
        else if (waveform == note_WAVEFORM_SAWTOOTH)
        {
            const float period = SAMPLE_RATE / frequency;
            const float currentPeriod = i / period;
            const float sawtooth = 2 * (currentPeriod - floor(currentPeriod)) - 1;
            note[i] = (short)(sawtooth * AMPLITUDE_SCALING);
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

    // Create the melody
    unsigned int noteCount = 4;
    Note *notes = calloc(noteCount, sizeof(Note));

    Melody *melody = calloc(1, sizeof(Melody));
    CPU_MEMORY_ALLOCATED_IN_BYTES += sizeof(Melody);

    notes[0].frequency = 440;
    notes[0].duration = 0.5;
    notes[0].waveform = note_WAVEFORM_SINE;

    notes[1].frequency = 440;
    notes[1].duration = 0.5;
    notes[1].waveform = note_WAVEFORM_SAWTOOTH;

    notes[2].frequency = 440;
    notes[2].duration = 0.5;
    notes[2].waveform = note_WAVEFORM_SQUARE;

    notes[3].frequency = 440;
    notes[3].duration = 0.5;
    notes[3].waveform = note_WAVEFORM_TRIANGLE;

    melody->numnotes = noteCount;
    melody->notes = notes;

    CPU_MEMORY_ALLOCATED_IN_BYTES += noteCount * sizeof(Note);

    Signal *result = calloc(1, sizeof(Signal));
    CPU_MEMORY_ALLOCATED_IN_BYTES += sizeof(Signal);

    // Free the initial memory allocated for result->samples (1 short)
    CPU_MEMORY_ALLOCATED_IN_BYTES -= sizeof(short);
    free(result->samples);

    for (int i = 0; i < melody->numnotes; i++)
    {
        Signal *signal = generate_note(melody->notes[i].frequency, melody->notes[i].duration, melody->notes[i].waveform);
        if (signal->samples == NULL)
        {
            printf("\tFailed to generate note\n");
            return 1;
        }

        // Concatenate the notes
        int oldLength = result->length;
        result->length += signal->length;

        // Reallocate memory for the samples array
        result->samples = realloc(result->samples, result->length * sizeof(short));
        CPU_MEMORY_ALLOCATED_IN_BYTES += signal->length * sizeof(short);

        // Copy the new samples
        for (int j = 0; j < signal->length; j++)
        {
            result->samples[oldLength + j] = signal->samples[j];
        }

        // Free the individual signal's samples and the signal struct
        CPU_MEMORY_ALLOCATED_IN_BYTES -= signal->length * sizeof(short);
        free(signal->samples);
        free(signal);
    }

    if (result->samples == NULL)
    {
        printf("\tFailed to generate melody\n");
        return 1;
    }

    printf("\tGenerated melody\n");

    int playResult = play_signal(result);
    if (playResult != 0)
    {
        printf("\tFailed to play melody\n");
        CPU_MEMORY_ALLOCATED_IN_BYTES -= result->length * sizeof(short);
        free(result->samples);
        free(result);
        return 1;
    }

    // Free the result's samples and struct
    CPU_MEMORY_ALLOCATED_IN_BYTES -= result->length * sizeof(short);
    free(result->samples);

    CPU_MEMORY_ALLOCATED_IN_BYTES -= sizeof(Signal);
    free(result);

    // Free the melody's notes and struct
    for (int i = 0; i < melody->numnotes; i++)
    {
        CPU_MEMORY_ALLOCATED_IN_BYTES -= sizeof(Note);
    }

    CPU_MEMORY_ALLOCATED_IN_BYTES -= melody->numnotes * sizeof(Note);

    free(melody->notes);
    free(melody);

    CPU_MEMORY_ALLOCATED_IN_BYTES -= sizeof(Melody);

    printf("\tCPU_MEMORY_ALLOCATED in kilobytes: %llu\n", CPU_MEMORY_ALLOCATED_IN_BYTES / 1024);

    // Final check
    const int MEMORY_LEAK_THRESHOLD = 256;
    ASSERT(CPU_MEMORY_ALLOCATED_IN_BYTES <= MEMORY_LEAK_THRESHOLD, "\tERROR: Memory leak detected! with a threshold of %d bytes", MEMORY_LEAK_THRESHOLD);

    return 0;
}
