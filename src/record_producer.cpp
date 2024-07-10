#include <iostream>
#include <portaudio.h>

#define SAMPLE_RATE 16000
#define FRAMES_PER_BUFFER 512
#define NUM_CHANNELS 1
#define SAMPLE_SIZE 2 // 16-bit PCM
#define PA_SAMPLE_TYPE paInt16
typedef short SAMPLE;

struct UserData {
    // 你可以在这里添加其他需要的用户数据
};

// 回调函数
static int recordCallback(const void *inputBuffer, void *outputBuffer,
                          unsigned long framesPerBuffer,
                          const PaStreamCallbackTimeInfo *timeInfo,
                          PaStreamCallbackFlags statusFlags,
                          void *userData) {
    const SAMPLE *in = (const SAMPLE *)inputBuffer;
    // 这里你可以处理从麦克风接收到的音频数据
    // 如将数据存储到文件、发送到网络等

    if (inputBuffer != NULL) {
        for (unsigned int i = 0; i < framesPerBuffer; i++) {
            // 处理音频数据，例如打印或存储
            SAMPLE sample = *in++;
            // 例如打印音频数据
            std::cout << sample << std::endl;
        }
    }

    return paContinue;
}

int main() {
    PaError err;
    UserData data;

    // 初始化PortAudio
    err = Pa_Initialize();
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        return 1;
    }

    // 打开音频输入流
    PaStream *stream;
    err = Pa_OpenDefaultStream(&stream,
                               NUM_CHANNELS,  // 输入通道数量
                               0,             // 输出通道数量
                               PA_SAMPLE_TYPE,  // 采样类型
                               SAMPLE_RATE,  // 采样率
                               FRAMES_PER_BUFFER,  // 每缓冲区帧数
                               recordCallback,  // 回调函数
                               &data);  // 用户数据
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        Pa_Terminate();
        return 1;
    }

    // 启动音频流
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
        Pa_CloseStream(stream);
        Pa_Terminate();
        return 1;
    }

    std::cout << "Recording... Press Enter to stop." << std::endl;
    std::cin.get();

    // 停止音频流
    err = Pa_StopStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
    }

    // 关闭音频流
    err = Pa_CloseStream(stream);
    if (err != paNoError) {
        std::cerr << "PortAudio error: " << Pa_GetErrorText(err) << std::endl;
    }

    // 终止PortAudio
    Pa_Terminate();

    return 0;
}
