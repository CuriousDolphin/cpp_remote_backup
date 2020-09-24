//
// Created by isnob on 24/09/2020.
//

#ifndef SERVER_SESSION_H
#define SERVER_SESSION_H

#endif //SERVER_SESSION_H

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

using boost::asio::ip::tcp;
using namespace std;
class Session: public std::enable_shared_from_this<Session>

{
public:
    Session(tcp::socket socket)
            : socket_(std::move(socket))
    {
        cout<<" new session "<<std::this_thread::get_id()<<endl;
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
        socket_.async_read_some(boost::asio::buffer(data_, max_length),
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
        boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
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

        boost::asio::async_write(socket_, boost::asio::buffer(str, 10),
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
