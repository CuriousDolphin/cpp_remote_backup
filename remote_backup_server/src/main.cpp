//
// server.cpp
// ~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <ctime>
#include <iostream>
#include <string>
#include <boost/bind/bind.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/array.hpp>

using boost::asio::ip::tcp;

std::string make_daytime_string()
{
    using namespace std; // For time_t, time and ctime;
    time_t now = time(0);
    return ctime(&now);
}
// We will use shared_ptr and enable_shared_from_this because
// we want to keep the tcp_connection object alive as long as there is an operation that refers to it.

class tcp_connection : public boost::enable_shared_from_this<tcp_connection>
{
public:
    typedef boost::shared_ptr<tcp_connection> pointer;

    static pointer create(boost::asio::io_context &io_context)
    {
        std::cout << std::this_thread::get_id() << " CREATE NEW CONNECTION" << std::endl;
        return pointer(new tcp_connection(io_context));
    }

    tcp::socket &socket()
    {
        return socket_;
    }
    // In the function start(), we call boost::asio::async_write() to serve the data to the client. Note that we are using boost::asio::async_write(),
    // rather than ip::tcp::socket::async_write_some(), to ensure that the entire block of data is sent.
    void start()
    {
        message_ = make_daytime_string();
        // std::cout << std::this_thread::get_id() << " START WRITE " << message_ << std::endl;
        /* boost::asio::async_write(socket_, boost::asio::buffer(message_),
                                 boost::bind(&tcp_connection::handle_write, shared_from_this(),
                                             boost::asio::placeholders::error,
                                             boost::asio::placeholders::bytes_transferred)); */

        boost::asio::async_read(socket_, boost::asio::buffer(buf), boost::bind(&tcp_connection::handler, shared_from_this(), boost::asio::placeholders::error));
    }

    void handler(
        const boost::system::error_code &error // Result of operation.

        /* std::size_t bytes_transferred */) // Number of bytes copied into the
                                             // buffers. If an error occurred,
                                             // this will be the  number of
                                             // bytes successfully transferred
                                             // prior to the error.
    {
        std::string s = socket().remote_endpoint().address().to_string();

        std::cout << "REQ FROM  " << s << std::endl;
        if (!error)
        {
            std::cout << "RECEIVED " << buf.data() << std::endl;
        };
    }

private:
    tcp_connection(boost::asio::io_context &io_context)
        : socket_(io_context)
    {
    }

    void handle_write(const boost::system::error_code & /*error*/,
                      size_t /*bytes_transferred*/)
    {
    }

    boost::array<char, 1> buf;

    tcp::socket socket_;
    std::string message_;
};

class tcp_server
{
public:
    tcp_server(boost::asio::io_context &io_context)
        : io_context_(io_context),
          acceptor_(io_context, tcp::endpoint(tcp::v4(), 4444))
    {
        start_accept();
    }

private:
    // creates a socket and initiates an asynchronous accept operation to wait for a new connection.
    void start_accept()
    {
        tcp_connection::pointer new_connection = tcp_connection::create(io_context_);

        acceptor_.async_accept(new_connection->socket(),
                               boost::bind(&tcp_server::handle_accept, this, new_connection, boost::asio::placeholders::error));
    }

    // The function handle_accept() is called when the asynchronous accept operation initiated by start_accept() finishes.
    // It services the client request, and then calls start_accept() to initiate the next accept operation.
    void handle_accept(tcp_connection::pointer new_connection,
                       const boost::system::error_code &error)
    {
        if (!error)
        {
            new_connection->start();
        }

        start_accept();
    }

    boost::asio::io_context &io_context_;
    tcp::acceptor acceptor_;
};

int main()
{
    try
    {
        std::cout << std::this_thread::get_id() << " START SERVER " << std::endl;
        boost::asio::io_context io_context;
        tcp_server server(io_context);
        io_context.run();
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}