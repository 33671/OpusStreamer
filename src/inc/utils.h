#include "../inc/circular_buffer.h"
#include <iostream>
#include <opus/opus_types.h>

#include <vector>
#ifndef UTILS
#define UTILS
#define SAMPLE_RATE 16000
#define CHANNELS 1
#define FRAME_SIZE (SAMPLE_RATE / 50) // 20ms frames
constexpr auto MILLS_AHEAD = 2000;
typedef std::string string;
std::string gen_random(const int len);
void encode_producer(const char* filename,CircularBuffer<std::vector<uint8_t>>* buffer);
void audio_consumer(CircularBuffer<std::vector<opus_int16>>* buffer); 
#endif