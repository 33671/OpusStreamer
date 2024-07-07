#include <boost/asio.hpp>
#include <boost/bind/bind.hpp>
#include <chrono>
#include <iostream>
#include <set>
#include <memory>
#include "circular_buffer.h"
using boost::asio::ip::tcp;

class AsyncAudioServer {
public:
    AsyncAudioServer(boost::asio::io_context& io_context, short port, CircularBuffer<std::vector<uint8_t>>* audio_buffer)
        : io_context_(io_context)
        , acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
        , socket_(io_context)
        , timer_(io_context)
        , sending_(false)
        , audio_buffer_broadcaster_(audio_buffer)
        , work_guard_(boost::asio::make_work_guard(io_context))
    {
        start_accept();
    }

private:
    void start_accept()
    {
        acceptor_.async_accept(socket_, boost::bind(&AsyncAudioServer::handle_accept, this, boost::asio::placeholders::error));
    }

    void handle_accept(const boost::system::error_code& error)
    {
        if (!error) {
            start_read();
        } else {
            std::cerr << "Accept error: " << error.message() << std::endl;
            start_accept(); 
        }
    }

    void start_read()
    {
        boost::asio::async_read_until(socket_, boost::asio::dynamic_buffer(input_buffer_), "\n",
            boost::bind(&AsyncAudioServer::handle_read, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

    void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        if (!error) {
            std::string message(input_buffer_.substr(0, bytes_transferred));
            input_buffer_.erase(0, bytes_transferred);

            if (message.find("send") != std::string::npos) {
                sending_ = true;
                socket_.cancel();
                start_write();
            } else if (message.find("pause") != std::string::npos) {
                sending_ = false;
            }

            start_read();
        } else {
            std::cerr << "Read error: " << error.message() << std::endl;
            socket_.close();
        }
    }

    void start_write()
    {
        if (sending_) {
            start_buffer_pop_async();
        }
    }

    void handle_write(const boost::system::error_code& error, std::size_t /*bytes_transferred*/)
    {
        if (!error) {
            if (sending_) {
                start_write();
                // set ms timer
                //timer_.expires_after(200ms);
                //timer_.async_wait(boost::bind(&AsyncAudioServer::handle_timer, this, boost::asio::placeholders::error));
            }
        } else {
            std::cerr << "Write error: " << error.message() << std::endl;
            // socket_.close();
            start_accept(); 
        }
    }
    std::vector<uint8_t> buffer_pop()
    {
        auto audio_20ms = audio_buffer_broadcaster_->pop();
        return audio_20ms; 
    }
    void handle_pop_result_async(std::vector<uint8_t> audio_20ms)
    {
        std::cout << "Blocking operation result: " << audio_20ms.size() << std::endl;

        std::vector<boost::asio::const_buffer> audio_buffers;
        audio_buffers.push_back(boost::asio::buffer({static_cast<uint8_t>(audio_20ms.size())}));
        audio_buffers.push_back(boost::asio::buffer(audio_20ms));
        boost::asio::async_write(socket_, audio_buffers,
            boost::bind(&AsyncAudioServer::handle_write, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }
    void start_buffer_pop_async()
    {
        boost::asio::post(thread_pool_, [this]() {
            std::vector<uint8_t> audio_20ms = buffer_pop();
            boost::asio::post(io_context_, boost::bind(&AsyncAudioServer::handle_pop_result_async, this, audio_20ms));
        });
    }

    void handle_timer(const boost::system::error_code& error)
    {
        if (!error && sending_) {
            start_write();
        }
    }

    boost::asio::io_context& io_context_;
    tcp::acceptor acceptor_;
    tcp::socket socket_;
    boost::asio::steady_timer timer_;
    std::string input_buffer_;
    bool sending_;
    CircularBuffer<std::vector<uint8_t>>* audio_buffer_broadcaster_;

    boost::asio::thread_pool thread_pool_ { 4 }; // create thread pool
    boost::asio::executor_work_guard<boost::asio::io_context::executor_type> work_guard_;
};
