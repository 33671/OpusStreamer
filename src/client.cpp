#include "./inc/utils.h"
#include "inc/circular_buffer.h"
#include <iostream>
#include <fstream>
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>

using boost::asio::ip::tcp;

class AsyncTCPClient {
public:
    AsyncTCPClient(boost::asio::io_context& io_context, const std::string& server, short port)
        : io_context_(io_context),
          socket_(io_context),
          server_(server),
          port_(port) {
        start_connect();
    }
    void send(const std::string& message) {
        boost::asio::post(io_context_, boost::bind(&AsyncTCPClient::do_send, this, message));
    }
    std::ofstream file_{"output.bin", std::ios::binary};
private:
    void start_connect() {
        tcp::resolver resolver(io_context_);
        tcp::resolver::results_type endpoints = resolver.resolve(server_, std::to_string(port_));
        boost::asio::async_connect(socket_, endpoints,
            boost::bind(&AsyncTCPClient::handle_connect, this, boost::asio::placeholders::error));
    }

    void handle_connect(const boost::system::error_code& error) {
        if (!error) {
            send(string("send\n"));
            start_read();
        } else {
            std::cerr << "Connect error: " << error.message() << std::endl;
        }
    }
    void do_send(const std::string& message) {
        std::vector<boost::asio::const_buffer> buffers;
        buffers.push_back(boost::asio::buffer(message));
        boost::asio::async_write(socket_, buffers,
            boost::bind(&AsyncTCPClient::handle_write, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }
    void handle_write(const boost::system::error_code& error, std::size_t /*bytes_transferred*/) {
        if (!error) {
            std::cout << "Data sent to server." << std::endl;
        } else {
            std::cerr << "Write error: " << error.message() << std::endl;
            socket_.close();
        }
    }

    void start_read() {
        boost::asio::async_read(socket_, boost::asio::buffer(read_buffer_),
            boost::asio::transfer_at_least(1),
            boost::bind(&AsyncTCPClient::handle_read, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred) {
        if (!error) {
            file_.write(read_buffer_.data(), bytes_transferred);
            start_read();
        } else if (error != boost::asio::error::eof) {
            std::cerr << "Read error: " << error.message() << std::endl;
            io_context_.stop();
        }
    }

    boost::asio::io_context& io_context_;
    tcp::socket socket_;
    std::string server_;
    short port_;
    std::array<char, 1024> read_buffer_;
    
};

int main() {
    try {
        boost::asio::io_context io_context;
        AsyncTCPClient client(io_context, "127.0.0.1", 8080);
        io_context.run();
        client.file_.flush();
        client.file_.close();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
    }

    return 0;
}
