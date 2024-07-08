#include "../inc/utils.h"
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <opus/opus.h>
#include <portaudio.h>
#include <thread>
#include <vector>
#include "../inc/opus_frame.hpp"
std::string gen_random(const int len)
{
    srand(time(nullptr));
    static const char alphanum[] = "0123456789"
                                   "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                   "abcdefghijklmnopqrstuvwxyz";
    std::string tmp_s;
    tmp_s.reserve(len);

    for (int i = 0; i < len; ++i) {
        tmp_s += alphanum[rand() % (sizeof(alphanum) - 1)];
    }

    return tmp_s;
}
static void check_pa_error(PaError err)
{
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        exit(1);
    }
}
void audio_consumer(CircularBuffer<std::vector<opus_int16>>* buffer)
{
    PaError err = Pa_Initialize();
    check_pa_error(err);

    // Open an audio I/O stream
    PaStream* stream;
    err = Pa_OpenDefaultStream(&stream, 0, CHANNELS, paInt16, SAMPLE_RATE, FRAME_SIZE, nullptr, nullptr);
    check_pa_error(err);

    err = Pa_StartStream(stream);
    check_pa_error(err);
    // Play audio
    while (true) {
        auto decodedData = buffer->pop();
        if (decodedData.size() == 0)
            break;
        auto err = Pa_WriteStream(stream, decodedData.data(), decodedData.size());
        if (err != paNoError) {
            std::cerr << "PortAudio WriteStream error: " << Pa_GetErrorText(err) << std::endl;
            // break;
        }
    }
    Pa_StopStream(stream);
    Pa_CloseStream(stream);
    Pa_Terminate();
}
void encode_producer(const char* filename, std::shared_ptr<CircularBufferBroadcast<OpusFrame>> broadcaster)
{
    // Initialize Opus decoder
    /*int error;
    OpusDecoder* decoder = opus_decoder_create(SAMPLE_RATE, CHANNELS, &error);
    if (error != OPUS_OK) {
        std::cerr << "Failed to create Opus decoder: " << opus_strerror(error) << std::endl;
        return;
    }*/

    // Open Opus file
    std::ifstream infile(filename, std::ios::binary);
    if (!infile) {
        std::cerr << "Failed to open audio file." << std::endl;
        return;
    }

    // Buffer for encoded and decoded data

    using namespace std::chrono;
    auto start = steady_clock::now();
    size_t sent_frames_count = 0;
    uint8_t size_c[1] {};
    while (true) {
        OpusFrame encodedData(255); // Maximum size for 20ms frame
        if (infile.read((char*)size_c, 1)) {
            auto size_to_read = (uint8_t)size_c[0];
            // printf("Length:%d", size_to_read);
            if (!infile.read(reinterpret_cast<char*>(encodedData.data_ptr()), size_to_read)) {
                std::cerr << "File Read Error" << std::endl;
                ;
                break;
            }
            auto bytesRead = infile.gcount();
            encodedData.resize(bytesRead);
            // buffer->push_no_wait(encodedData);
            broadcaster->broadcast(encodedData);
            // printf("1");
            sent_frames_count++;
            auto now = steady_clock::now();
            auto elapsed = duration_cast<milliseconds>(now - start).count();
            auto frames_shoud_send_count = elapsed / 20;
            long long int frame_bias = frames_shoud_send_count - sent_frames_count;
            if (frame_bias < (MILLS_AHEAD / 20)) // 2000ms/20ms send rate limit
            {
                // printf("Sleeping\n");
                std::this_thread::sleep_for(20ms);
            }
            // printf("Frames bias:%lld\n", );
        } else {
            auto now = steady_clock::now();
            auto elapsed = duration_cast<milliseconds>(now - start).count();
            printf("File Reading Complete,Time Elapsed:%lld secs\n", elapsed / 1000);
            // buffer->push_no_wait(std::vector<uint8_t>(0));
            broadcaster->broadcast(OpusFrame(0));
            break;
        }
    }

    // while () {
    // std::vector<unsigned char> encodedData(255); // Maximum size for 20ms frame
    //  Decode Opus data
    // std::vector<opus_int16> decodedData(FRAME_SIZE * CHANNELS);
    // int frame_size = opus_decode(decoder, encodedData.data(), bytesRead, decodedData.data(), FRAME_SIZE, 0);
    // if (frame_size < 0) {
    //     std::cerr << "Decoding failed: " << opus_strerror(frame_size) << std::endl;
    //     opus_decoder_ctl(decoder, OPUS_RESET_STATE);
    //     continue;
    //     // infile.close();
    //     // break;
    // }
    //}

    // Cleanup
    infile.close();
    // opus_decoder_destroy(decoder);
}
