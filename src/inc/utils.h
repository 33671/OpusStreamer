#ifndef UTILS
#define UTILS
#include "../circular_buffer.hpp"
#include "../inc/opus_frame.hpp"
#include <optional>
#define SAMPLE_RATE 16000
#define CHANNELS 1
#define FRAME_SIZE (SAMPLE_RATE / 50) // 20ms frames
constexpr auto MILLS_AHEAD = 1000;
using string = std::string;
std::string gen_random(const int len);
void encode_producer(const char* opus_filename, std::shared_ptr<CircularBufferBroadcast<OpusFrame>> broadcaster);
void audio_consumer(std::shared_ptr<CircularBuffer<OpusFrame>> frames_receiver);
void mp3_encode_producer(const char* filename, std::shared_ptr<CircularBufferBroadcast<std::optional<OpusFrame>>> broadcaster);
#endif
