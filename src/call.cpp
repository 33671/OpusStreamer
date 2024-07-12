#include "../include/tcp_client.hpp"
#include "../include/audio_recorder.hpp"
#include <chrono>
#include <memory>
#include <thread>
void record(TcpClient& client)
{
    auto audio_broadcaster = std::make_shared<CircularBufferBroadcast<std::optional<OpusFrame>>>(MILLS_AHEAD / 20);
    AudioRecorder recorder(audio_broadcaster);
    std::this_thread::sleep_for(std::chrono::milliseconds(5000));
    recorder.start();
    auto buffer_reader = audio_broadcaster->subscribe();
    while (true) {
        // printf("here:%zu\n",test_sub->queue_size());
        auto result = buffer_reader->pop();
        if (result.has_value()) {
            std::vector<boost::asio::const_buffer> audio_buffers_prepared;
            audio_buffers_prepared.push_back(boost::asio::buffer({ 'S', 'T', 'A' }));
            audio_buffers_prepared.push_back(boost::asio::buffer({ static_cast<uint8_t>(result->size()) }));
            printf("%zu ",result->size());
            audio_buffers_prepared.push_back(boost::asio::buffer(result->vector()));
            audio_buffers_prepared.push_back(boost::asio::buffer({ 'E', 'N', 'D' }));
            client.send(audio_buffers_prepared);
        } else {
            break;
        }
    }
    // recorder.stop();
}
int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cerr << "Usage: call <ip> <port>\n";
        return 1;
    }
    auto received_audio_buffer = std::make_shared<CircularBuffer<OpusFrame>>(10);
    std::thread consumer(audio_consumer, received_audio_buffer);
    boost::asio::io_service io_service;
    TcpClient client(io_service, argv[1], argv[2], received_audio_buffer);
    std::thread record_producer(record, std::ref(client));
    // record_producer.detach();
    io_service.run();
    printf("Disconnected");
    return 0;
}
