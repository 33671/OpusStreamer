#include "../include/asio_include.hpp"
#include <iostream>
#include <memory>
#include <array>

using boost::asio::ip::tcp;

class TcpRelaySession : public std::enable_shared_from_this<TcpRelaySession> {
public:
    TcpRelaySession(boost::asio::io_context& io_context)
        : socket1_(io_context), socket2_(io_context) {}

    tcp::socket& socket1() { return socket1_; }
    tcp::socket& socket2() { return socket2_; }

    void start() {
        do_read(socket1_, buffer1_, socket2_);
        do_read(socket2_, buffer2_, socket1_);
    }

private:
    void do_read(tcp::socket& socket, std::array<char, 8192>& buffer, tcp::socket& other_socket) {
        auto self(shared_from_this());
        socket.async_read_some(boost::asio::buffer(buffer),
            [this, self, &socket, &buffer, &other_socket](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    do_write(other_socket, buffer, length);
                    do_read(socket, buffer, other_socket);
                }
            });
    }

    void do_write(tcp::socket& socket, std::array<char, 8192>& buffer, std::size_t length) {
        auto self(shared_from_this());
        boost::asio::async_write(socket, boost::asio::buffer(buffer, length),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    // Continue writing
                }
            });
    }

    tcp::socket socket1_;
    tcp::socket socket2_;
    std::array<char, 8192> buffer1_;
    std::array<char, 8192> buffer2_;
};

class TcpRelayServer {
public:
    TcpRelayServer(boost::asio::io_context& io_context, short port)
        : io_context_(io_context),acceptor_(io_context, tcp::endpoint(tcp::v4(), port)) {
        start_accept();
    }

private:
    void start_accept() {
        auto session = std::make_shared<TcpRelaySession>(io_context_);
        acceptor_.async_accept(session->socket1(),
            [this, session](boost::system::error_code ec) {
                if (!ec) {
                    acceptor_.async_accept(session->socket2(),
                        [this, session](boost::system::error_code ec) {
                            if (!ec) {
                                session->start();
                            }
                            start_accept();
                        });
                } else {
                    start_accept();
                }
            });
    }

    tcp::acceptor acceptor_;
    boost::asio::io_context& io_context_;
};

int main(int argc, char* argv[]) {
    try {
        if (argc != 2) {
            std::cerr << "Usage: tcp_relay_server <port>\n";
            return 1;
        }

        boost::asio::io_context io_context;
        TcpRelayServer server(io_context, std::atoi(argv[1]));
        io_context.run();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
