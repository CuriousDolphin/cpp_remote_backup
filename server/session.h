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

const int LEN_BUFF = 1024;
const std::map<std::string, int> commands = {{"LOGIN", 1},
                                             {"GET",   2},
                                             {"PUT",   3},
                                             {"PATCH", 4}};
using boost::asio::ip::tcp;
using namespace std;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, Db *db)
            : socket_(std::move(socket)), user_(""), db_(db){
        cout << " new session " << this_thread::get_id() << endl;
    }

    void start() {
        read_request();
    }

    ~Session() {
        socket_.cancel();
        cout << "DELETED SESSION "<<user_ << endl;
    }

private:
    void read_request() {
        auto self(shared_from_this());
        socket_.async_read_some(boost::asio::buffer(data_, LEN_BUFF),
                                [this, self](std::error_code ec, std::size_t length) {
                                    if (!ec) {
                                        // std::cout<<std::this_thread::get_id()<<" READ :"<<data_<<std::endl;
                                        handle_request();
                                    } else
                                        std::cout << std::this_thread::get_id() << " ERROR :" << ec.message()
                                                  << " CODE " << ec.value() << std::endl;
                                });
    }

    // TODO ADD HASHING
    bool login(const string &user, const string& pwd) {
        string savedpwd = db_->get(user);
        if (savedpwd == pwd) {
            user_ = user; // !!important!!
            return true;
        } else {
            return false;
        }
    }

    void handle_request() {
        vector<string> params(4); // parsed values
        vector<string> tmp1(4); // support
        boost::split(tmp1, data_, boost::is_any_of("\n")); // take one line
        boost::split(params, tmp1[0], boost::is_any_of(" ")); // split by space

        cout << " REQUEST: " << std::endl;

        for (int i = 0; i < params.size(); i++) {
            cout << "\t\t" << params[i] << std::endl;
        }
        int action = -1;
        try {
            action = commands.at(params[0]);
        }
        catch (const std::exception &) {
            cout << "UNKNOWN COMMAND" << std::endl;
            write_str("Unknown command...Bye!");
            return;
        }

        switch (action) {
            case 1: { // LOGIN
                if (params.size() < 3) {
                    write_str("Wrong number arguments...Bye!");
                    return;
                }
                string user = params.at(1);
                string pwd = params.at(2);
                bool res = login(user, pwd);
                if (res) {
                    std::cout << "====== WELCOME " << user_ << std::endl;
                    write_str("OK\n");
                    read_request();
                } else {
                    std::cout << "WRONG CREDENTIALS" << std::endl;
                    write_str("Wrong credentials...Bye!");
                }

            }
                break;
            case 2:  // GET
            {
                read_request();
            }

                break;
            case 3: // PUT
            {
                read_request();
            }
                break;
            case 4: // PATCH
            {
                read_request();
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

    void write_str(std::string str) {
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

    char data_[LEN_BUFF]{};
    Db *db_;
    std::string user_;

};
