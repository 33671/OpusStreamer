#ifndef AUDIO_SERVER
#define AUDIO_SERVER
#include "client_session.hpp"
#include "circular_buffer.hpp"
#include "opus_frame.hpp"
#include <iostream>
#include <memory>
#include <optional>
#include <set>
using boost::asio::ip::tcp;

class AsyncAudioServer {
public:
    AsyncAudioServer(boost::asio::io_context& io_context, short port,std::shared_ptr<CircularBufferBroadcast<std::optional<OpusFrame>>> audio_buffer)
        : io_context_(io_context)
        , acceptor_(io_context, tcp::endpoint(tcp::v4(), port))
        , audio_buffer_broadcaster_(audio_buffer)
    {
        start_accept();
    }

private:
    void start_accept()
    {
        auto session = std::make_shared<ClientSession>(io_context_, audio_buffer_broadcaster_);
        acceptor_.async_accept(session->socket(),
            boost::bind(&AsyncAudioServer::handle_accept, this, session, boost::asio::placeholders::error));
    }

    void handle_accept(std::shared_ptr<ClientSession> session, const boost::system::error_code& error)
    {
        if (!error) {
            // {
            //     // std::unique_lock<std::shared_mutex> lock(mutex_);
            //     sessions_.insert(session);
            // }
            session->start_read();
            start_accept();
        } else {
            std::cerr << "Accept error: " << error.message() << std::endl;
        }
    }

    // void remove_session(std::shared_ptr<ClientSession> session)
    // {
    //     // std::unique_lock<std::shared_mutex> lock(mutex_);
    //     sessions_.erase(session);
    //     printf("Session Removed,Current Sessions:%zu\n", sessions_.size());
    // }

    boost::asio::io_context& io_context_;
    tcp::acceptor acceptor_;
    std::shared_ptr<CircularBufferBroadcast<std::optional<OpusFrame>>> audio_buffer_broadcaster_;
    std::set<std::shared_ptr<ClientSession>> sessions_;
    // std::shared_mutex mutex_;
};
#endif