#include "inc/circular_buffer.hpp"
#include "inc/server.hpp"
#include "inc/utils.h"
#include <optional>
#include <thread>

boost::asio::thread_pool ClientSession::thread_pool_(4);
using namespace std::chrono_literals;
int main(int argc, char* argv[])
{
    auto buffer = std::make_shared<CircularBufferBroadcast<std::optional<OpusFrame>>>(MILLS_AHEAD / 20);
    // std::thread producer(encode_producer, "test.bin", buffer);
    // std::thread producer(mp3_encode_producer, "30-seconds-of-silence.mp3", buffer);
    std::thread producer(mp3_encode_producer, "test.mp3", buffer);
    //auto bufffer_reader = buffer.get()->subscribe();
    //std::thread consumer(audio_consumer, bufffer_reader);
    boost::asio::io_context io_context;
    AsyncAudioServer server(io_context, 8080, buffer);
    io_context.run();

    producer.join();
    return 0;
}