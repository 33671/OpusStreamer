#include "inc/circular_buffer.hpp"
#include "inc/server1.hpp"
#include "inc/utils.h"
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <opus/opus.h>
#include <portaudio.h>
#include <samplerate.h>
#include <thread>
#include "inc/opus_frame.hpp"

boost::asio::thread_pool ClientSession::thread_pool_(4);
using namespace std::chrono_literals;
int main(int argc, char* argv[])
{
    //encode_opus("filename.wav","test.bin");
    //return 0;
    auto buffer = std::make_shared<CircularBufferBroadcast<OpusFrame>>(MILLS_AHEAD / 20);
    // std::thread producer(encode_producer, "test.bin", buffer);
    std::thread producer(mp3_encode_producer, "test.mp3", buffer);
    //auto bufffer_reader = buffer.get()->subscribe();
    //std::thread consumer(audio_consumer, bufffer_reader);
    boost::asio::io_context io_context;
    AsyncAudioServer server(io_context, 8080, buffer);
    io_context.run();
   /* try {
        
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }*/

    producer.join();
    //  consumer.join();
    return 0;
}