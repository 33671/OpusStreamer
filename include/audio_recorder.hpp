#ifndef AUDIO_RECORDER
#define AUDIO_RECORDER
#include "circular_buffer.hpp"
#include "utils.hpp"
#include <cstdio>
#include <iostream>
#include <memory>
#include <optional>
#include <opus/opus.h>
#include <portaudio.h>
#include <vector>
#define SAMPLE_SIZE 2 // 16-bit PCM
#define PA_SAMPLE_TYPE paInt16
typedef short SAMPLE;

class AudioRecorder {
public:
    AudioRecorder(std::shared_ptr<CircularBufferBroadcast<std::optional<OpusFrame>>> _frameBuffer)
        : stream(nullptr)
        , isRecording(false)
    {
        frameBuffer = _frameBuffer;
        int error;
        encoder = opus_encoder_create(SAMPLE_RATE, CHANNELS, OPUS_APPLICATION_VOIP, &error);
        if (error != OPUS_OK) {
            throw std::runtime_error("Failed to create Opus encoder: " + std::string(opus_strerror(error)));
        }
    }

    bool start()
    {
        PaError err;
        PORTAUDIO_INIT_LOCK.lock();
        if (!IS_PORTAUDIO_INITIALIZED) {
            err = Pa_Initialize();
            if (err != paNoError) {
                std::cerr << "PortAudio init error: " << Pa_GetErrorText(err) << std::endl;
                IS_PORTAUDIO_INITIALIZED = true;
                 PORTAUDIO_INIT_LOCK.unlock();
                return false;
            }
            IS_PORTAUDIO_INITIALIZED = true;
        }
        PORTAUDIO_INIT_LOCK.unlock();

        err = Pa_OpenDefaultStream(&stream, CHANNELS, 0, PA_SAMPLE_TYPE, SAMPLE_RATE, FRAME_SIZE, [](const void* inputBuffer, void* outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo* timeInfo, PaStreamCallbackFlags statusFlags, void* userData) -> int {
                AudioRecorder *recorder = static_cast<AudioRecorder*>(userData);
                return recorder->recordCallback(inputBuffer, framesPerBuffer); }, this);

        if (err != paNoError) {
            std::cerr << "PortAudio open stream error: " << Pa_GetErrorText(err) << std::endl;
            Pa_Terminate();
            return false;
        }

        err = Pa_StartStream(stream);
        if (err != paNoError) {
            std::cerr << "PortAudio start stream error: " << Pa_GetErrorText(err) << std::endl;
            Pa_CloseStream(stream);
            Pa_Terminate();
            return false;
        }

        isRecording = true;
        return true;
    }

    void stop()
    {
        isRecording = false;
        printf("stopping\n");
        frameBuffer->broadcast(std::nullopt);
        PaError err = Pa_StopStream(stream);
        if (err != paNoError) {
            std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        }

        err = Pa_CloseStream(stream);
        if (err != paNoError) {
            std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        }

        Pa_Terminate();
        printf("stopped\n");
    }

private:
    PaStream* stream;
    bool isRecording;
    std::shared_ptr<CircularBufferBroadcast<std::optional<OpusFrame>>> frameBuffer;
    OpusEncoder* encoder;

    int recordCallback(const void* inputBuffer, unsigned long framesPerBuffer)
    {
        if (inputBuffer != nullptr) {
            const SAMPLE* in = static_cast<const SAMPLE*>(inputBuffer);
            std::vector<SAMPLE> frame(in, in + framesPerBuffer);
            OpusFrame opusData(255);
            int opusBytes = opus_encode(encoder, frame.data(), framesPerBuffer, opusData.data_ptr(), opusData.size());
            if (opusBytes < 0) {
                auto error = string(opus_strerror(opusBytes));
                throw std::runtime_error("Opus encoding failed: " + error);
            }
            opusData.resize(opusBytes);
            frameBuffer->broadcast(opusData);
            // printf("broadcasted\n");
        }
        return paContinue;
    }
};
#endif