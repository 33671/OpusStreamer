#include "circular_buffer.h"
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <chrono>
#include <memory>
#include <set>
using boost::asio::ip::tcp;
using namespace std::chrono_literals;

class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
    ClientSession(boost::asio::io_context& io_context, CircularBuffer<std::vector<uint8_t>>* audio_buffer, std::function<void(std::shared_ptr<ClientSession>)> on_disconnect)
        : io_context_(io_context)
        , socket_(io_context)
        , audio_buffer_(audio_buffer)
        , sending_(false)
        , on_disconnect_(on_disconnect)
    {
    }

    tcp::socket& socket()
    {
        return socket_;
    }

    void start()
    {
        start_read();
    }

    void start_sending()
    {
        socket_.cancel();
        sending_ = true;
        start_write();
    }

    void stop_sending()
    {
        sending_ = false;
    }

private:
    void start_read()
    {
        boost::asio::async_read_until(socket_, boost::asio::dynamic_buffer(input_buffer_), "\n",
            boost::bind(&ClientSession::handle_read, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        if (!error) {
            std::string message(input_buffer_.substr(0, bytes_transferred));
            input_buffer_.erase(0, bytes_transferred);

            if (message.find("send") != std::string::npos) {
                start_sending();
            } else if (message.find("pause") != std::string::npos) {
                stop_sending();
            }

            start_read();
        } else {
            std::cerr << "Read error: " << error.message() << std::endl;
            close();
        }
    }

    void start_write()
    {
        if (sending_) {
            start_buffer_pop_async();
        }
    }

    void start_buffer_pop_async()
    {
        auto self = shared_from_this();
        boost::asio::post(thread_pool_, [this, self]() {
            std::vector<uint8_t> audio_20ms = audio_buffer_->pop();
            boost::asio::post(io_context_, boost::bind(&ClientSession::handle_pop_result_async, self, audio_20ms));
        });
    }

    void handle_pop_result_async(std::vector<uint8_t> audio_20ms)
    {
        if (sending_) {
            std::vector<boost::asio::const_buffer> audio_buffers;
            audio_buffers.push_back(boost::asio::buffer("FRAME BEGIN "));
            audio_buffers.push_back(boost::asio::buffer({ static_cast<uint8_t>(audio_20ms.size()) }));
            audio_buffers.push_back(boost::asio::buffer(audio_20ms));
            boost::asio::async_write(socket_, audio_buffers,
                boost::bind(&ClientSession::handle_write, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
        }
    }

    void handle_write(const boost::system::error_code& error, std::size_t /*bytes_transferred*/)
    {
        if (!error && sending_) {
            start_write();
        } else {
            std::cerr << "Write error: " << error.message() << std::endl;
            close();
        }
    }

    void close()
    {
        socket_.close();
        if (on_disconnect_) {
            on_disconnect_(shared_from_this());
        }
    }
    boost::asio::io_context& io_context_;
    tcp::socket socket_;
    std::string input_buffer_;
    bool sending_;
    CircularBuffer<std::vector<uint8_t>>* audio_buffer_;
    std::function<void(std::shared_ptr<ClientSession>)> on_disconnect_;

    boost::asio::thread_pool thread_pool_{1};
};

// Define the thread pool
// boost::asio::thread_pool ClientSession::thread_pool_(4);

class AsyncAudioServer {
public:
    AsyncAudioServer(boost::asio::io_context& io_context, short port, CircularBuffer<std::vector<uint8_t>>* audio_buffer)
        : io_context_(io_context)
        , acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
        , audio_buffer_(audio_buffer)
    {
        start_accept();
    }

private:
    void start_accept()
    {
        auto new_session = std::make_shared<ClientSession>(io_context_, audio_buffer_,
            std::bind(&AsyncAudioServer::remove_session, this, std::placeholders::_1));
        acceptor_.async_accept(new_session->socket(),
            boost::bind(&AsyncAudioServer::handle_accept, this, new_session, boost::asio::placeholders::error));
    }

    void handle_accept(std::shared_ptr<ClientSession> session, const boost::system::error_code& error)
    {
        if (!error) {
            {
                std::unique_lock<std::shared_mutex> lock(mutex_);
                sessions_.insert(session);
            }
            session->start();
            start_accept();
        } else {
            std::cerr << "Accept error: " << error.message() << std::endl;
        }
    }

    void remove_session(std::shared_ptr<ClientSession> session)
    {
        std::unique_lock<std::shared_mutex> lock(mutex_);
        printf("removing session,Current Sessions:%d\n",sessions_.size());
        sessions_.erase(session);
    }

    boost::asio::io_context& io_context_;
    tcp::acceptor acceptor_;
    CircularBuffer<std::vector<uint8_t>>* audio_buffer_;
    std::set<std::shared_ptr<ClientSession>> sessions_;
    std::shared_mutex mutex_;
};