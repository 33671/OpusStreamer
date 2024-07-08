#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <cstdint>
#include <cstdio>
#include <exception>
#include <iostream>
#include <fstream>
using boost::asio::ip::tcp;

class TcpClient {
public:
    TcpClient(boost::asio::io_service& io_service, const std::string& host, const std::string& port)
        : io_service_(io_service)
        , socket_(io_service)
        , buffer_(1024) // Initial buffer size
    {
        tcp::resolver resolver(io_service_);
        tcp::resolver::query query(host, port);
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

        boost::asio::async_connect(socket_, endpoint_iterator,
            boost::bind(&TcpClient::handle_connect, this,
                boost::asio::placeholders::error));
    }
    void send(const std::string& message) {
        boost::asio::post(io_service_, boost::bind(&TcpClient::do_send, this, message));
    }
    std::ofstream file_{"output.bin", std::ios::binary};

private:
    void do_send(const std::string& message) {
        std::vector<boost::asio::const_buffer> buffers;
        buffers.push_back(boost::asio::buffer(message));
        boost::asio::async_write(socket_, buffers,
            boost::bind(&TcpClient::handle_write, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }
     void handle_write(const boost::system::error_code& error, std::size_t /*bytes_transferred*/) {
        if (!error) {
            std::cout << "Data sent to server." << std::endl;
        } else {
            std::cerr << "Write error: " << error.message() << std::endl;
            socket_.close();
        }
    }
    void handle_connect(const boost::system::error_code& error)
    {
        if (!error) {
            // Start reading data
            boost::asio::async_read(socket_, boost::asio::buffer(buffer_),
                boost::bind(&TcpClient::handle_read, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        }
        else{
            throw std::exception("TCP Not Connected");
        }
    }

    void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        if (!error) {
            // Process the received data
            process_data(bytes_transferred);

            // Continue reading data
            boost::asio::async_read(socket_, boost::asio::buffer(buffer_),
                boost::bind(&TcpClient::handle_read, this,
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred));
        } else {
            std::cerr << "Error on receive: " << error.message() << "\n";
            socket_.close();
        }
    }

    void process_data(std::size_t bytes_transferred)
    {
        static const std::string start_marker = "BEGIN ";
        static std::vector<char> frame_buffer;
        static std::uint8_t frame_length = 0;
        static bool reading_length = false;

        for (std::size_t i = 0; i < bytes_transferred; ++i) {
            if (!reading_length) {
                // Check for the start marker
                frame_buffer.push_back(buffer_[i]);
                if (frame_buffer.size() >= start_marker.size()) {
                    if (std::equal(frame_buffer.end() - start_marker.size(), frame_buffer.end(), start_marker.begin())) {
                        frame_buffer.clear();
                        reading_length = true;
                    } else {
                        frame_buffer.erase(frame_buffer.begin());
                    }
                }
            } else {
                // Read the frame length
                frame_length = static_cast<uint8_t>(buffer_[i]);
                reading_length = false;
                frame_buffer.clear();
                frame_buffer.push_back(buffer_[i]);
                std::cout << "Detected frame start, length: " << (int)frame_length << "\n";
                printf("Transferred Buffer length:%zu",bytes_transferred);
                char s[4];
                sprintf(s, "%02x", frame_length);
                file_.write(s, 2);
                file_.write(" ", 1);
            }
        }
    }

    boost::asio::io_service& io_service_;
    tcp::socket socket_;
    std::vector<char> buffer_;
};

int main(int argc, char* argv[])
{
    try {
        boost::asio::io_service io_service;
        TcpClient client(io_service, "127.0.0.1", "8080");
        client.send("send\n");
        io_service.run();
        printf("Disconnected");
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
