#include "inc/circular_buffer.h"
#include "inc/server1.hpp"
#include "inc/utils.h"
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <chrono>
#include <fstream>
#include <iostream>
#include <opus/opus.h>
#include <portaudio.h>
#include <samplerate.h>
#include <sndfile.h>
#include <thread>
int encode_opus(const char* file, const char* outputfile);
using namespace std::chrono_literals;
int main(int argc, char* argv[])
{
    // encode_opus("silence.wav","test1.bin");
    // return 0;
    auto buffer = std::make_shared<CircularBufferBroadcast<std::vector<uint8_t>>>(MILLS_AHEAD / 20);
    std::thread producer(encode_producer, "test1.bin", buffer.get());
    // auto bufffer_reader = buffer.get()->subscribe();
    // std::thread consumer(audio_consumer, buffer.get());
    std::this_thread::sleep_for(2s);

    try {
        boost::asio::io_context io_context;
        AsyncAudioServer server(io_context, 8080, buffer.get());
        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    producer.join();
    // consumer.join();
    return 0;
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

    // Prepare to read and encode in chunks
    auto framesPerChunk = static_cast<int>(FRAME_SIZE); // 20ms worth of frames
    std::vector<opus_int16> inputBuffer(framesPerChunk * CHANNELS);
    static unsigned char outputBuffer[255]; // Max size for Opus packet

    sf_count_t readFrames;
    std::ofstream binstream(outputfile, std::ios::binary);
    std::ofstream length_record("length_record.txt");
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
