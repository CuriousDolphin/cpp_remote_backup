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
#include <asio/io_service.hpp>
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
int NUM_THREADS=8;

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


class Session: public std::enable_shared_from_this<Session>

{
public:
    Session(tcp::socket socket)
            : socket_(std::move(socket))
    {
        std::cout<<" new session "<<std::this_thread::get_id()<<std::endl;
    }

    void start()
    {
      //  do_write_str("BENVENUTO STRONZO");
        do_read();
    }
    ~Session(){
        cout<<"DELETED SESSION"<<endl;
    }

private:
    void do_read()
    {   auto self(shared_from_this());
        socket_.async_read_some(asio::buffer(data_, max_length),
                                [this,self](std::error_code ec, std::size_t length)
                                {
                                    if (!ec)
                                    {
                                        std::cout<<std::this_thread::get_id()<<" READ :"<<data_<<std::endl;
                                        do_read();

                                        //do_write(length);
                                    }
                                    else std::cout<<std::this_thread::get_id()<<" ERROR :"<<ec.message()<<" CODE "<<ec.value() << std::endl;
                                });
    }

    void do_write(std::size_t length)
    {
        auto self(shared_from_this());
        asio::async_write(socket_, asio::buffer(data_, length),
                          [this,self](std::error_code ec, std::size_t /*length*/)
                          {
                              if (!ec)
                              {
                                  do_read();
                              }
                          });
    }

    void do_write_str(std::string str)
    {
        auto self(shared_from_this());

        asio::async_write(socket_, asio::buffer(str, 10),
                          [this,self](std::error_code ec, std::size_t /*length*/)
                          {
                              if (!ec)
                              {
                                  do_read();
                              } else std::cout<<std::this_thread::get_id()<<" ERROR :"<<ec.message()<<" CODE "<<ec.value() << std::endl;
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
            : acceptor_(io_context, tcp::endpoint(tcp::v4(), port))

    {
        std::cout<<"START SERVER: THREAD"<<std::this_thread::get_id()<<std::endl;
        io_context_= &io_context;
        do_accept();
    }

    void run(){
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
    }

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
    asio::io_context *io_context_;

};



int main(int argc, char* argv[])
{
    try
    {
       // asio::io_service io_service_;

       // asio::io_service io_service io_service_;

         asio::io_context io_context;





        server s(io_context, 5555);
        std::vector<std::thread> threads;
        for(int n = 0; n < NUM_THREADS; ++n)
            threads.emplace_back([&io_context](){
                //std::cout<<"START  THREAD"<<std::this_thread::get_id()<<std::endl;
                io_context.run();
            });

        for(auto& thread : threads)
        {
            if(thread.joinable())
            {
                std::cout<<"JOINED  THREAD"<<thread::id()<<std::endl;
                thread.join();
            }
        }
        //io_context.run();
        //s.run();


    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
