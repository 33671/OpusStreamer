#include "inc/tcp_client.hpp"
#include "inc/utils.h"
int main(int argc, char* argv[])
{
    auto buffer = std::make_shared<CircularBuffer<OpusFrame>>(10);
    std::thread consumer(audio_consumer, buffer);
    boost::asio::io_service io_service;
    TcpClient client(io_service, "127.0.0.1", "8080", buffer);
    client.send("send\n");
    io_service.run();
    printf("Disconnected");
    return 0;
}
