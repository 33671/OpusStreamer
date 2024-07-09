#include "../inc/utils.h"
#include "../inc/mp3_opus_encoder.hpp"
#include "../inc/opus_frame.hpp"
#include "minimp3.h"
#include "sndfile.h"
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <memory>
#include <opus/opus.h>
#include <portaudio.h>
#include <thread>
#include <vector>
using namespace std::chrono;
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
void audio_consumer(std::shared_ptr<CircularBuffer<OpusFrame>> frames_receiver)
{
    // std::this_thread::sleep_for(10000ms);
    // Initialize Opus decoder
    int error;
    OpusDecoder* decoder = opus_decoder_create(SAMPLE_RATE, CHANNELS, &error);
    if (error != OPUS_OK) {
        std::cerr << "Failed to create Opus decoder: " << opus_strerror(error) << std::endl;
        return;
    }
    PaError err = Pa_Initialize();
    check_pa_error(err);

    // Open an audio I/O stream
    PaStream* stream;
    err = Pa_OpenDefaultStream(&stream, 0, CHANNELS, paInt16, SAMPLE_RATE, FRAME_SIZE, nullptr, nullptr);
    check_pa_error(err);

    err = Pa_StartStream(stream);
    check_pa_error(err);
    while (true) {
        // std::cout << "Decoding"<< std::endl;
        auto encodedData = frames_receiver->pop();
        std::vector<opus_int16> decodedData(FRAME_SIZE * CHANNELS);
        if (encodedData.size() == 0) {
            std::cerr << "Decoding To 0 Length Buffer" << std::endl;
            break;
        }
        int frame_size = opus_decode(decoder, encodedData.data_ptr(), encodedData.size(), decodedData.data(), FRAME_SIZE, 0);
        if (frame_size < 0) {
            std::cerr << "Decoding failed: " << opus_strerror(frame_size) << std::endl;
            // opus_decoder_ctl(decoder, OPUS_RESET_STATE);
            continue;
            // infile.close();
            // break;
        }
        auto err = Pa_WriteStream(stream, decodedData.data(), frame_size);
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
    // Open Opus file
    std::ifstream infile(filename, std::ios::binary);
    if (!infile) {
        std::cerr << "Failed to open audio file." << std::endl;
        return;
    }

    // Buffer for encoded and decoded data

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
            if (frame_bias < -(MILLS_AHEAD / 20)) // 2000ms/20ms send rate limit
            {
                // printf("Sleeping\n");
                std::this_thread::sleep_for(17ms);
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

    // Cleanup
    infile.close();
    // opus_decoder_destroy(decoder);
}

void mp3_encode_producer(const char* filename, std::shared_ptr<CircularBufferBroadcast<OpusFrame>> broadcaster)
{
    Mp3OpusEncoder converter(filename);
    auto start = steady_clock::now();
    size_t sent_frames_count = 0;
    while (true) {
        OpusFrame opusFrame = converter.getNextOpusFrame();
        broadcaster->broadcast(opusFrame);
        if (opusFrame.size() == 0) {
            auto now = steady_clock::now();
            auto elapsed = duration_cast<milliseconds>(now - start).count();
            printf("Mp3 Encoding Complete,Time Elapsed:%lld secs\n", elapsed / 1000);
            break;
        }
        sent_frames_count++;
        auto now = steady_clock::now();
        auto elapsed = duration_cast<milliseconds>(now - start).count();
        auto frames_shoud_send_count = elapsed / 20;
        long long int frame_bias = frames_shoud_send_count - sent_frames_count;
        if (frame_bias < -(MILLS_AHEAD / 20)) // 2000ms/20ms send rate limit
        {
            // printf("Sleeping\n");
            std::this_thread::sleep_for(17ms);
        }
    }
}
int encode_opus(const char* file, const char* outputfile)
{
    SF_INFO sfInfo;
    SNDFILE* inputSndFile = sf_open(file, SFM_READ, &sfInfo);
    if (!inputSndFile) {
        std::cerr << "Failed to open input file: " << sf_strerror(inputSndFile) << std::endl;
        return 1;
    }
    int sampleRate = sfInfo.samplerate;
    int channels = sfInfo.channels;
    if (channels != CHANNELS || sampleRate != SAMPLE_RATE) {
        std::cerr << "file not match" << std::endl;
        return 1;
    }
    std::cout << "Fs:" << sampleRate << std::endl;
    int error = OPUS_OK;
    OpusEncoder* enc = opus_encoder_create(SAMPLE_RATE, CHANNELS, OPUS_APPLICATION_VOIP, &error);
    if (error != OPUS_OK) {
        std::cerr << "Failed to create decoder" << std::endl;
        return 1;
    }
    // opus_encoder_ctl(enc, OPUS_SET_INBAND_FEC(0));

    // Prepare to read and encode in chunks
    auto framesPerChunk = static_cast<int>(FRAME_SIZE); // 20ms worth of frames
    std::vector<opus_int16> inputBuffer(framesPerChunk * CHANNELS);
    static unsigned char outputBuffer[255]; // Max size for Opus packet

    sf_count_t readFrames;
    std::ofstream binstream(outputfile, std::ios::binary);
    std::ofstream length_record("length_record1.txt");
    while ((readFrames = sf_readf_short(inputSndFile, inputBuffer.data(), framesPerChunk)) > 0) {
        int numBytes = opus_encode(enc, inputBuffer.data(), framesPerChunk, outputBuffer, 255);
        if (numBytes < 0) {
            // std::cerr << "length:" << readFrames << std::endl;
            std::cerr << "Opus encoding error: " << opus_strerror(numBytes) << std::endl;
            opus_encoder_destroy(enc);
            return 1;
        }
        if (numBytes == 0)
            continue;
        binstream.write((const char*)&numBytes, 1);
        binstream.write((const char*)outputBuffer, numBytes);
        char s[4];
        sprintf(s, "%02x", numBytes);
        length_record.write(s, 2);
        length_record.write(" ", 1);
    }

    binstream.flush();
    binstream.close();
    sf_close(inputSndFile);
    opus_encoder_destroy(enc);

    std::cout << "Opus stream written" << std::endl;

    return 0;
}
