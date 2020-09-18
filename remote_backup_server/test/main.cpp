//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <asio/ts/buffer.hpp>
#include <asio/ts/internet.hpp>
#include <asio.hpp>

using asio::ip::tcp;

class session
{
public:
    session(tcp::socket socket)
            : socket_(std::move(socket))
    {
    }

    void start()
    {
        do_read();
    }

private:
    void do_read()
    {
        //auto self(shared_from_this());
        socket_.async_read_some(asio::buffer(data_, max_length),
                                [this](std::error_code ec, std::size_t length)
                                {
                                    if (!ec)
                                    {
                                        std::cout<<std::this_thread::get_id()<<" READ :"<<data_<<std::endl;

                                        do_write(length);
                                    }
                                });
    }

    void do_write(std::size_t length)
    {
       // auto self(shared_from_this());
        asio::async_write(socket_, asio::buffer(data_, length),
                          [this](std::error_code ec, std::size_t /*length*/)
                          {
                              if (!ec)
                              {
                                  do_read();
                              }
                          });
    }

    tcp::socket socket_;
    enum { max_length = 1024 };
    char data_[max_length];
};

class server
{
public:
    server(asio::io_context& io_context, short port)
            : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
              socket_(io_context)
    {
        do_accept();
    }

private:
    void do_accept()
    {
        acceptor_.async_accept(socket_,
                               [this](std::error_code ec)
                               {
                                   if (!ec)
                                   {
                                       std::make_shared<session>(std::move(socket_))->start();
                                      /* std::thread(
                                               [this](){
                                                   std::cout<<"new thread"<<std::this_thread::get_id()<<std::endl;
                                                   std::make_shared<session>(std::move(socket_))->start();
                                               }
                                               ).detach();*/


                                   }

                                   do_accept();
                               });
    }

    tcp::acceptor acceptor_;
    tcp::socket socket_;
};

int main(int argc, char* argv[])
{
    try
    {


        asio::io_context io_context;
        std::cout<<"START SERVER: THREAD"<<std::this_thread::get_id()<<std::endl;

        server s(io_context, 5555);

        io_context.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
