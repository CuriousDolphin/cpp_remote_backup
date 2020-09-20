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
#include <map>
#include <vector>
#include <future>
#include <fstream>
#include <queue>
#include <optional>
#include <condition_variable>

using asio::ip::tcp;
using namespace std;
int MAX_QUEUE_LEN = 20;


template<class T>
class Jobs {
    mutex m;
    condition_variable cv; // conditional get

    queue<T> coda;
    std::atomic<bool> completed = false;
public:
    // inserisce un job in coda in attesa di essere processato, può
    // essere bloccante se la coda dei job è piena
    void put(T&& job) {
        unique_lock<mutex> lg(m);
        coda.push(move(job));
        cv.notify_all(); // notifica i consumers
    };

    // legge un job dalla coda e lo rimuove
    // si blocca se non ci sono valori in coda
    optional<T> get() {
        unique_lock<mutex> lg(m);
        cv.wait(lg, [this]() { return !coda.empty() || completed.load(); });  // true sblocca la wait
        if (completed.load() && coda.empty()) {
            return std::nullopt;
        }
        T data = move(coda.front());
        coda.pop();
        return move(data);

    };

    bool is_completed() {
        return completed.load();
    }

    bool is_empty() {
        return coda.empty();
    }

    // per notificare la chiusura
    void complete() {
        completed.store(true);
        cv.notify_all();
        // std::cout << this_thread::get_id() << "COMPLETED " << std::endl;
    }
};


class Session

{
public:
    Session(tcp::socket socket,asio::io_context& io_context)
            : socket_(std::move(socket))
    {
        std::cout<<" new session "<<std::this_thread::get_id()<<std::endl;
    }

    void start()
    {
        std::cout<<" session start"<<std::this_thread::get_id()<<std::endl;
        do_read();
    }

private:
    void do_read()
    {
        std::cout<<" do read "<<std::this_thread::get_id()<<std::endl;
        socket_.async_read_some(asio::buffer(data_, max_length),
                                [this](std::error_code ec, std::size_t length)
                                {
                                    if (!ec)
                                    {
                                        std::cout<<std::this_thread::get_id()<<" READ :"<<data_<<std::endl;

                                        do_write(length);
                                    }
                                    else std::cout<<std::this_thread::get_id()<<" ERROR :"<<ec.message()<<std::endl;
                                });
    }

    void do_write(std::size_t length)
    {
        //auto self(shared_from_this());
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


Jobs<tcp::socket> jobs;


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
                                        std::cout<<"NEW ACCEPT"<<std::endl;
                                        //jobs.put(Session(std::move(socket_)));
                                        jobs.put(std::move(socket_));

                                       //std::make_shared<Session>(std::move(socket_))->start();
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

        asio::io_service io_service io_service_;

         asio::io_context io_context;

        std::cout<<"NUM THREADs"<<std::thread::hardware_concurrency()<<std::endl;
        // Prepare things
        std::vector<std::thread> threads;
        auto count = std::thread::hardware_concurrency() * 2;

        for(int n = 0; n < count; ++n)
            threads.emplace_back([&io_context](){
                //io_context.run();
                // std::cout<<"START THREAD"<<std::this_thread::get_id()<<std::endl;
               // server s(io_context, 5555);

                while (true) {
                    optional<tcp::socket> sock = jobs.get();
                    if (!sock.has_value()) {
                        std::cout<<"NO VALUE"<<std::endl;
                        break;
                    }
                    std::cout<<"NEW socket"<<std::endl;

                    Session s(std::move(sock.value()),io_context);
                    io_context.run();
                    s.start();

                }

                });









        std::cout<<"START SERVER: THREAD"<<std::this_thread::get_id()<<std::endl;

        server s(io_context, 5555);

        io_context.run();

        for(auto& thread : threads)
        {
            if(thread.joinable())
            {
                thread.join();
            }
        } */
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
