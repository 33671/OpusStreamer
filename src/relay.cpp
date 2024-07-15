#include "../include/asio_include.hpp"
#include <array>
#include <cstdio>
#include <iostream>
#include <memory>
#include <optional>

using boost::asio::ip::tcp;

class TcpRelaySession : public std::enable_shared_from_this<TcpRelaySession> {
public:
    TcpRelaySession(boost::asio::io_context& io_context, std::list<std::shared_ptr<TcpRelaySession>>& sessions)
        : socket_(io_context)
        , sessions_(sessions)
    {
    }

    std::optional<std::shared_ptr<TcpRelaySession>> get_another_session()
    {
        for (auto elem : sessions_) {
            if (elem != shared_from_this()) {
                return std::optional(elem); 
            }
        }
        return std::nullopt;
    }

    tcp::socket& socket() { return socket_; }
    // tcp::socket& socket2() { return socket2_; }

    void start()
    {
        do_read_async();
        auto session1 = get_another_session();
        if (!session1.has_value()) {
            return;
        }
        (*session1)->do_read_async();
    }
    tcp::socket socket_;
    std::array<char, 8192> buffer_;

private:
    void do_read_async()
    {
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(buffer_),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    auto session1 = get_another_session();
                    do_read_async();
                    if (!session1.has_value()) {
                        return;
                    }
                    do_write_async((*session1)->socket_, buffer_, length);
                } else {
                    // printf("Read Error\n");
                    handle_disconnect(socket_);
                }
            });
    }

    void do_write_async(tcp::socket& socket, std::array<char, 8192>& buffer, std::size_t length)
    {
        auto self(shared_from_this());
        boost::asio::async_write(socket, boost::asio::buffer(buffer, length),
            [this, self, &socket](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    // Continue writing
                } else {
                    // printf("Write Error\n");
                    handle_disconnect(socket);
                }
            });
    }

    void handle_disconnect(tcp::socket& socket)
    {
        sessions_.remove(shared_from_this());
        socket.close();
        // Implement logic to handle reconnections if needed
        // For example, you could notify the server to accept a new connection
    }

    std::list<std::shared_ptr<TcpRelaySession>>& sessions_;
};

class TcpRelayServer {
public:
    TcpRelayServer(boost::asio::io_context& io_context, short port)
        : io_context_(io_context)
        , acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
    {
        start_accept();
    }

private:
    void start_accept()
    {
        auto session = std::make_shared<TcpRelaySession>(io_context_,sessions_);
        acceptor_.async_accept(session->socket(),
            [this, session](boost::system::error_code ec) {
                if (!ec) {
                    std::cout << "Client connected\n";
                    if (sessions_.size() >= 2) {
                        while (sessions_.size() > 1) {
                            sessions_.front()->socket_.close();
                            sessions_.pop_front();
                        }
                    }
                    sessions_.push_back(session);
                    // if (sessions_.size() == 2) {
                    // }
                    session->start();
                    start_accept();
                } else {
                    std::cout << "Error accepting Client 1: " << ec.message() << "\n";
                    start_accept();
                }
            });
    }

    tcp::acceptor acceptor_;
    boost::asio::io_context& io_context_;
    std::list<std::shared_ptr<TcpRelaySession>> sessions_;
};

int main(int argc, char* argv[])
{
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
