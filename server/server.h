//
// Created by isnob on 24/09/2020.
//

#ifndef SERVER_SERVER_H
#define SERVER_SERVER_H

#endif //SERVER_SERVER_H
#include <iostream>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio/ts/buffer.hpp>
#include <boost/asio/ts/internet.hpp>
#include <boost/asio.hpp>
#include <boost/asio/io_service.hpp>
#include <map>
#include <vector>
#include <future>
#include <fstream>
#include <queue>
#include <optional>
#include <condition_variable>
#include <cpp_redis/cpp_redis>
#include "session.h"

using boost::asio::ip::tcp;

class server
{
public:
    server(boost::asio::io_context& io_context, short port)
            : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))

    {
        std::cout<<"START SERVER: THREAD"<<std::this_thread::get_id()<<std::endl;
        //io_context_= &io_context;
        do_accept();
    }

    /* void run(){
         std::vector<std::thread> threads;
         for(int n = 0; n < NUM_THREADS; ++n)
             threads.emplace_back([this](){
                 std::cout<<"START  THREAD"<<std::this_thread::get_id()<<std::endl;
                 io_context_->run();
             });

         for(auto& thread : threads)
         {
             if(thread.joinable())
             {
                 std::cout<<"JOINED  THREAD"<<thread::id()<<std::endl;
                 thread.join();
             }
         }
     } */

private:
    void do_accept()
    {
        std::cout<<std::this_thread::get_id() <<" DO ACCEPT"<<std::endl;
        acceptor_.async_accept(
                [this](std::error_code ec,tcp::socket socket)
                {
                    if (!ec)
                    {
                        std::cout<<std::this_thread::get_id() <<" NEW ACCEPT"<<std::endl;
                        std::make_shared<Session>(std::move(socket))->start();
                    }

                    do_accept();
                });
    }

    tcp::acceptor acceptor_;
    //  boost::asio::io_context *io_context_;

};