#ifndef MP3OPSUDECODER
#define MP3OPSUDECODER
#include "utils.h"
#include "opus_frame.hpp"
#include <fstream>
#include <iostream>
#include <optional>
#include <opus/opus.h>
#include <opus/opus_types.h>
#include <samplerate.h>
#include <string>
#define MINIMP3_IMPLEMENTATION
#include <minimp3.h>
#include <minimp3_ex.h>
class Mp3OpusEncoder {
public:
    Mp3OpusEncoder(const std::string& filename)
    {
        // 读取MP3文件
        mp3Data = readFile(filename);

        // 初始化minimp3
        mp3dec_init(&mp3d);
        int load_res = mp3dec_load_buf(&mp3d, mp3Data->data(), mp3Data->size(), &info, NULL, NULL);
        if (load_res) {
            throw std::runtime_error("Error loading MP3 file");
        }

        // 初始化Opus编码器
        int error;
        encoder = opus_encoder_create(SAMPLE_RATE, CHANNELS, OPUS_APPLICATION_VOIP, &error);
        if (error != OPUS_OK) {
            throw std::runtime_error("Failed to create Opus encoder: " + std::string(opus_strerror(error)));
        }

        // 配置Opus编码参数
        // opus_encoder_ctl(encoder, OPUS_SET_BITRATE(bitrate));
        mp3_sample_rate = info.hz;
        audio_channels = info.channels;
        // 初始化索引
        currentSampleIndex = 0;
        frameSize = mp3_sample_rate * 0.02; // 20ms * 48kHz = 960
        resampledFrameSize = SAMPLE_RATE * 0.02; // 20ms * 16kHz = 320
                                                 // 初始化重采样器
        src_state = src_new(SRC_SINC_FASTEST, 1, &error);
        if (src_state == NULL) {
            throw std::runtime_error("Failed to create SRC state: " + std::to_string(error));
        }
    }

    ~Mp3OpusEncoder()
    {
        opus_encoder_destroy(encoder);
        src_delete(src_state);
    }

    std::optional<OpusFrame> getNextOpusFrame()
    {
        if (currentSampleIndex >= info.samples) {
            return std::nullopt; // 返回空向量表示结束
        }

        std::vector<float> monoPcm(frameSize); // 单声道缓冲区
        if (audio_channels == 2) {
            for (int i = 0; i < frameSize; ++i) {
                if (currentSampleIndex + i * 2 < info.samples) {
                    monoPcm[i] = info.buffer[currentSampleIndex + i * 2] / 32768.0f; // encode left channel only
                }
            }
        } else if (audio_channels == 1) {
            for (int i = 0; i < frameSize; ++i) {
                monoPcm[i] = info.buffer[i] / 32768.0f;
            }
        } else {
            throw std::runtime_error("Audio channels not support");
        }
        std::vector<float> resampledPcm(resampledFrameSize);

        SRC_DATA srcData;
        srcData.data_in = monoPcm.data();
        srcData.input_frames = frameSize;
        srcData.data_out = resampledPcm.data();
        srcData.output_frames = resampledFrameSize;
        srcData.src_ratio = (double)SAMPLE_RATE / (double)mp3_sample_rate;
        srcData.end_of_input = 0;

        int error = src_process(src_state, &srcData);
        if (error) {
            auto err_s = string(src_strerror(error));
            throw std::runtime_error("SRC processing failed: " + err_s);
        }

        if (srcData.output_frames_gen < resampledFrameSize) {
            resampledPcm.resize(resampledFrameSize, 0);
        }

        OpusFrame opusData(255);
        int opusBytes = opus_encode_float(encoder, resampledPcm.data(), resampledFrameSize, opusData.data_ptr(), opusData.size());

        if (opusBytes < 0) {
            auto error = string(opus_strerror(opusBytes));
            throw std::runtime_error("Opus encoding failed: " + error);
        }

        currentSampleIndex += frameSize * 2;
        opusData.resize(opusBytes);
        return opusData;
    }

private:
    std::vector<uint8_t>* readFile(const std::string& filename)
    {
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            throw std::runtime_error("Could not open file: " + filename);
        }

        file.seekg(0, std::ios::end);
        std::streamsize size = file.tellg();
        file.seekg(0, std::ios::beg);

        auto buffer = new std::vector<uint8_t>(size);
        if (!file.read(reinterpret_cast<char*>(buffer->data()), size)) {
            throw std::runtime_error("Could not read file: " + filename);
        }

        return buffer;
    }

    mp3dec_t mp3d;
    mp3dec_file_info_t info;
    std::vector<uint8_t>* mp3Data;
    OpusEncoder* encoder;
    SRC_STATE* src_state;

    int currentSampleIndex;
    int frameSize;
    int resampledFrameSize;
    int mp3_sample_rate;
    int audio_channels;
};
#endif