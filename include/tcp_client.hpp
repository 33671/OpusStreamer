﻿#ifndef TCP_AUDIO_CLIENT
#define TCP_AUDIO_CLIENT
#include "asio_include.hpp"
#include "circular_buffer.hpp"
#include "opus_frame.hpp"

#include <iostream>
#include <iterator>
#include <memory>
#include <ostream>
#define DebugBreak() (throw std::runtime_error("Break Point"))
using boost::asio::ip::tcp;


class TcpClient {
public:
    TcpClient(boost::asio::io_service& io_service, const std::string& host, const std::string& port, std::shared_ptr<CircularBuffer<OpusFrame>> frames_buffer)
        : io_service_(io_service)
        , socket_(io_service)
        , frames_buffer_(frames_buffer) // Initial buffer size
    {
        tcp::resolver resolver(io_service_);
        tcp::resolver::query query(host, port);
        tcp::resolver::iterator endpoint_iterator = resolver.resolve(query);

        boost::asio::async_connect(socket_, endpoint_iterator,
            boost::bind(&TcpClient::handle_connect, this,
                boost::asio::placeholders::error));
    }
    void send(const std::string& message)
    {
        boost::asio::post(io_service_, boost::bind(&TcpClient::do_send, this, message));
    }
    void send(const std::vector<boost::asio::const_buffer>& buffer)
    {
        boost::asio::write(socket_, buffer);
        // boost::asio::post(io_service_, boost::bind(&TcpClient::do_send_buffer, this, std::ref(buffer)));
    }
    enum class ReadState {
        ToSearchStart,
        ToReadLength,
        ToReadData,
        EndRead,
    };

private:
    void do_send(const std::string& message)
    {
        std::vector<boost::asio::const_buffer> buffers;
        buffers.push_back(boost::asio::buffer(message));
        boost::asio::async_write(socket_, buffers,
            boost::bind(&TcpClient::handle_write, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }
    void do_send_buffer(const std::vector<boost::asio::const_buffer>& message)
    {
        boost::asio::async_write(socket_, message,
            boost::bind(&TcpClient::handle_write, this, boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }
    void start_read_to_end()
    {
        boost::asio::async_read_until(socket_, buffer_, end_marker,
            boost::bind(&TcpClient::handle_read, this,
                boost::asio::placeholders::error,
                boost::asio::placeholders::bytes_transferred));
    }
    void handle_write(const boost::system::error_code& error, std::size_t /*bytes_transferred*/)
    {
        if (!error) {
            std::cout << "1" << " ";
        } else {
            std::cerr << "Write error: " << error.message() << std::endl;
            socket_.close();
        }
    }
    void handle_connect(const boost::system::error_code& error)
    {
        if (!error) {
            // Start reading data
            start_read_to_end();
        } else {
            throw std::runtime_error("TCP Not Connected");
        }
    }

    void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred)
    {
        if (!error) {
            // Process the received data
            process_data(bytes_transferred);

            // Continue reading data
            start_read_to_end();
        } else {
            std::cerr << "Error on receive: " << error.message() << "\n";
            socket_.close();
        }
    }

    void process_data(std::size_t bytes_transferred)
    {
        // std::cout << "Reading Data" << std::endl;
        boost::asio::streambuf::const_buffers_type bufs = buffer_.data();
        auto buf_begin = boost::asio::buffers_begin(bufs);
        auto buf_end = boost::asio::buffers_end(bufs);
        for (auto it = buf_begin; it < buf_end;) {
            if (read_state_ == ReadState::ToReadLength) {
                frame_length = *it;
                int i = frame_length;
                // std::cerr << "Reading Length:" << (int)frame_length << std::endl;
                if (it + 1 < buf_end) {
                    it++;
                } else {
                    read_state_ = ReadState::ToSearchStart;
                    buffer_.consume(buffer_.size());
                    break;
                }
                read_state_ = ReadState::ToReadData;
                if (frame_length == 0) {
                    std::cerr << "Read Frame 0 Length" << std::endl;
                    read_state_ = ReadState::ToSearchStart;
                    buffer_.consume(buffer_.size());
                    break;
                    // DebugBreak();
                }
            } else if (read_state_ == ReadState::ToSearchStart) {
                // std::cout << "Searching Start" << std::endl;
                if (std::equal(start_marker.begin(), start_marker.end(), it)) {
                    read_state_ = ReadState::ToReadLength;
                    auto position = std::distance(buf_begin, it);
                    // std::cout << "Header Detected at " << position << " Length:" << static_cast<int>(*(it + 5)) << std::endl;
                    if (it + start_marker.size() < buf_end) {
                        it += start_marker.size();
                    }
                    else{
                        buffer_.consume(buffer_.size());
                        break; 
                    }
                } else if (it + 1 < buf_end) {
                    buffer_.consume(1);
                    break;
                } else {
                    std::cerr << "Searching Start Failed" << std::endl;
                    buffer_.consume(buffer_.size());
                    break;
                }
            } else if (read_state_ == ReadState::ToReadData) {
                OpusFrame opus_frame;
                // std::cerr << "opus_frame Length"<<opus_frame.size() << std::endl;
                int read_times = frame_length;
                while (true) {
                    opus_frame.push_back(*it);
                    read_times--;
                    if (read_times <= 0) {
                        // std::cout << "Read Times Reached" << std::endl;
                        break;
                    } else if (it + 1 < buf_end) {
                        it++;
                    } else {
                        std::cerr << "Read Failed Before Frame Length Reached" << std::endl;
                        break;
                    }
                }
                if (read_times != 0) {
                    std::cerr << "Dirty Data Read:" << buffer_.size() << std::endl;
                    read_state_ = ReadState::ToSearchStart;
                    buffer_.consume(buffer_.size());
                    // DebugBreak();s
                    break;
                }
                if (opus_frame.size() == frame_length) {
                    frames_buffer_->push_no_wait(opus_frame);
                } else {
                    std::cerr << "Read Data Size Not Match, Buffer:" << opus_frame.size() << " Frame Length:" << (int)frame_length << std::endl;
                }
                buffer_.consume(frame_length + start_marker.size() + end_marker.size() + 1);
                read_state_ = ReadState::ToSearchStart;
                break;
            }
        }
    }

    boost::asio::io_service& io_service_;
    tcp::socket socket_;
    boost::asio::streambuf buffer_;
    ReadState read_state_ = ReadState::ToSearchStart;
    const std::string start_marker = "STA";
    const std::string end_marker = "END";
    std::uint8_t frame_length = 0;
    std::shared_ptr<CircularBuffer<OpusFrame>> frames_buffer_;
};
#endif