#ifndef AUDIO_DEFINES
#define AUDIO_DEFINES
#define SAMPLE_RATE 16000
#define CHANNELS 1
#define FRAME_SIZE (SAMPLE_RATE / 50) // 20ms frames
constexpr auto MILLS_AHEAD = 1000;
#endif