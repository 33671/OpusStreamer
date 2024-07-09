#include "circular_buffer.h"
#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <iostream>
#include <memory>
#include "opus_frame.hpp"
#ifndef  CLIENTSESSION
#define CLIENTSESSION
using boost::asio::ip::tcp;
class ClientSession : public std::enable_shared_from_this<ClientSession> {
public:
    ClientSession(boost::asio::io_context& io_context, std::shared_ptr<CircularBufferBroadcast<OpusFrame>> audio_buffer_broadcaster, std::function<void(std::shared_ptr<ClientSession>)> on_disconnect)
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
        boost::asio::post(thread_pool_, [this, self]() mutable {
            auto a = 1;
            OpusFrame audio_20ms = audio_buffer_->pop();
            if (audio_20ms.size() == 0)
            {
                audio_buffer_broadcaster_->unsubscire(audio_buffer_);
            }
            boost::asio::post(io_context_, boost::bind(&ClientSession::handle_pop_result_async, self, audio_20ms));
        });
    }

    void handle_pop_result_async(OpusFrame audio_20ms)
    {
        if (sending_) {
            std::vector<boost::asio::const_buffer> audio_buffers_prepared;
            audio_buffers_prepared.push_back(boost::asio::buffer({'B','E','G','I','N'}));
            audio_buffers_prepared.push_back(boost::asio::buffer({ static_cast<uint8_t>(audio_20ms.size()) }));
            audio_buffers_prepared.push_back(boost::asio::buffer(audio_20ms.vector()));
            audio_buffers_prepared.push_back(boost::asio::buffer({'E','N','D'}));
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
    std::shared_ptr<CircularBuffer<OpusFrame>> audio_buffer_;
    std::shared_ptr<CircularBufferBroadcast<OpusFrame>> audio_buffer_broadcaster_;
    boost::asio::io_context& io_context_;
    tcp::socket socket_;
    std::string input_buffer_;
    bool sending_;
    std::function<void(std::shared_ptr<ClientSession>)> on_disconnect_;

    static boost::asio::thread_pool thread_pool_;
};
#endif