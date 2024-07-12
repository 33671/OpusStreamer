#include "../include/server.hpp"
#include "../include/audio_recorder.hpp"
boost::asio::thread_pool ClientSession::thread_pool_(4);
int main()
{
    auto buffer = std::make_shared<CircularBufferBroadcast<std::optional<OpusFrame>>>(MILLS_AHEAD / 20);
    AudioRecorder recorder(buffer);
    recorder.start();
    boost::asio::io_context io_context;
    AsyncAudioServer server(io_context, 8080, buffer);
    io_context.run();
    return 0;
}