#include "circular_buffer.h"
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <chrono>
#include <memory>
#include <set>
using boost::asio::ip::tcp;

class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
    ClientSession(boost::asio::io_context& io_context, CircularBufferBroadcast<std::vector<uint8_t>>* audio_buffer_broadcaster, std::function<void(std::shared_ptr<ClientSession>)> on_disconnect)
        : audio_buffer_broadcaster_(audio_buffer_broadcaster)
        , io_context_(io_context)
        , socket_(io_context)
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
        audio_buffer_ = audio_buffer_broadcaster_->subscribe();
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
            if (audio_20ms.size() == 0)
                close();
            boost::asio::post(io_context_, boost::bind(&ClientSession::handle_pop_result_async, self, audio_20ms));
        });
    }

    void handle_pop_result_async(std::vector<uint8_t> audio_20ms)
    {
        if (sending_) {
            std::vector<boost::asio::const_buffer> audio_buffers_prepared;
            audio_buffers_prepared.push_back(boost::asio::buffer("BEGIN "));
            audio_buffers_prepared.push_back(boost::asio::buffer({ static_cast<uint8_t>(audio_20ms.size()) }));
            audio_buffers_prepared.push_back(boost::asio::buffer(audio_20ms));
            boost::asio::async_write(socket_, audio_buffers_prepared,
                boost::bind(&ClientSession::handle_write, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
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
        audio_buffer_broadcaster_->unsubscire(audio_buffer_);
        if (on_disconnect_) {
            on_disconnect_(shared_from_this());
        }
    }
    CircularBuffer<std::vector<uint8_t>>* audio_buffer_;
    CircularBufferBroadcast<std::vector<uint8_t>>* audio_buffer_broadcaster_;
    boost::asio::io_context& io_context_;
    tcp::socket socket_;
    std::string input_buffer_;
    bool sending_;
    std::function<void(std::shared_ptr<ClientSession>)> on_disconnect_;

    boost::asio::thread_pool thread_pool_ { 1 };
};

// Define the thread pool
// boost::asio::thread_pool ClientSession::thread_pool_(4);

class AsyncAudioServer {
public:
    AsyncAudioServer(boost::asio::io_context& io_context, short port, CircularBufferBroadcast<std::vector<uint8_t>>* audio_buffer)
        : io_context_(io_context)
        , acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
        , audio_buffer_broadcaster_(audio_buffer)
    {
        start_accept();
    }

private:
    void start_accept()
    {
        auto new_session = std::make_shared<ClientSession>(io_context_, audio_buffer_broadcaster_,
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
        sessions_.erase(session);
        // TODO: ???
        // audio_buffer_broadcaster_->unsubscire(audio_buffer_subscriber_);
        printf("Session Removed,Current Sessions:%ld\n", sessions_.size());
    }

    boost::asio::io_context& io_context_;
    tcp::acceptor acceptor_;
    CircularBufferBroadcast<std::vector<uint8_t>>* audio_buffer_broadcaster_;
    std::set<std::shared_ptr<ClientSession>> sessions_;
    std::shared_mutex mutex_;
};