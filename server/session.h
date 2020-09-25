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
#include <string>
#include <boost/algorithm/string.hpp>
#include "db.h"
const std::map<std::string, int> commands = {{"LOGIN", 1},
                                             {"GET",   2},
                                             {"PUT",   3},
                                             {"PATCH", 4}};
using boost::asio::ip::tcp;
using namespace std;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket,Db* db)
            : socket_(std::move(socket)),user_("") {
        cout << " new session " << std::this_thread::get_id() << endl;
        db_=db;

    }

    void start() {
        //  do_write_str("BENVENUTO STRONZO");

        read_command();
    }

    ~Session() {
        socket_.cancel();
        cout << "DELETED SESSION" << endl;
    }

private:
    void read_command() {
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(data_, max_length),
                                [this, self](std::error_code ec, std::size_t length) {
                                    if (!ec) {
                                        // std::cout<<std::this_thread::get_id()<<" READ :"<<data_<<std::endl;
                                        handle_request(length);
                                        //do_write(length);
                                    } else
                                        std::cout << std::this_thread::get_id() << " ERROR :" << ec.message()
                                                  << " CODE " << ec.value() << std::endl;
                                });
    }

    // TODO ADD HASHING
    bool login(const std::string user,const std::string pwd){
        string savedpwd=db_->get(user);
        if(savedpwd==pwd){
            user_=user; // !!important!!
            return true;
        }else{
            return false;
        }
    }

    void handle_request(std::size_t length) {
        std::cout << "LENGTH REQUEST: " <<length<< std::endl;
        std::vector<std::string> tmp(4); // parsed values
        std::vector<std::string> tmp1(4); // support


        boost::split(tmp1, data_, boost::is_any_of("\n")); // take one line
        boost::split(tmp, tmp1[0], boost::is_any_of(" ")); // split by space

        std::cout << " REQUEST: " << std::endl;

        for (int i = 0; i < tmp.size(); i++) {
            std::cout << "\t\t"  << tmp[i] << std::endl;
        }
        int command = -1;
        try {
            command = commands.at(tmp[0]);
        }
        catch (const std::exception &) {
            std::cout << "UNKNOWN COMMAND" << std::endl;
            do_write_str("Unknown command...Bye!");
            return;
        }

        switch (command) {
            case 1: {
                std::cout << "DO LOGIN" << std::endl;
                if (tmp.size() < 3) {
                    do_write_str("Wrong number arguments...Bye!");
                    return;
                }
                std::string user = tmp.at(1);
                std::string pwd = tmp.at(2);
                bool res = login(user,pwd);
                if(res){
                    std::cout<<"====== WELCOME "<<user_<<std::endl;
                    do_write_str("OK\n");
                    read_command();
                }else{
                    std::cout<<"WRONG CREDENTIALS"<<std::endl;
                    do_write_str("Wrong credentials...Bye!");
                }

            }
                break;
            case 2: {
                std::cout << "DO GET" << std::endl;
                read_command();
            }

                break;
            case 3: {
                std::cout << "DO PUT" << std::endl;
                read_command();
            }

                break;
            case 4: {
                std::cout << "PATCH" << std::endl;
                read_command();
            }
                break;
        }


    }

    void do_write(std::size_t length) {
        auto self(shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
                                 [this, self](std::error_code ec, std::size_t /*length*/) {
                                     if (!ec) {
                                         //do_read();
                                     }
                                 });
    }

    void do_write_str(std::string str) {
        auto self(shared_from_this());
        boost::asio::async_write(socket_, boost::asio::buffer(str, str.size()),
                                 [this, self](std::error_code ec, std::size_t /*length*/) {
                                     if (!ec) {
                                        // do_read();
                                     } else
                                         std::cout << std::this_thread::get_id() << " ERROR :" << ec.message()
                                                   << " CODE " << ec.value() << std::endl;
                                 });
    }


    tcp::socket socket_;
    enum {
        max_length = 1024
    };
    char data_[max_length];
    Db* db_;
    std::string user_;

};
