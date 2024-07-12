#include "../include/tcp_client.hpp"
#include "../include/utils.hpp"
int main(int argc, char* argv[])
{
    if (argc != 3) {
        std::cerr << "Usage: call <ip> <port>\n";
        return 1;
    }
    auto buffer = std::make_shared<CircularBuffer<OpusFrame>>(10);
    std::thread consumer(audio_consumer, buffer);
    boost::asio::io_service io_service;
    TcpClient client(io_service, argv[1], argv[2], buffer);
    client.send("send\n");
    io_service.run();
    printf("Disconnected");
    return 0;
}
