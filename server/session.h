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
            : _socket(std::move(socket)), _user(""), _db(db) {
        cout << " new session " << this_thread::get_id() << endl;
    }

    void start() {
        read_request();
    }

    ~Session() {
        _socket.cancel();
        cout << "DELETED SESSION " << _user << endl;
    }

private:
    void read_request() {
        auto self(shared_from_this());



        //std::size_t read  = _socket.read_some(boost::asio::buffer(_data, LEN_BUFF));
        //cout<<"READ REQUEST:"<<read<<endl;


       _socket.async_read_some(boost::asio::buffer(_data, LEN_BUFF),
                                [this, self](std::error_code ec, std::size_t length) {
                                    if (!ec) {
                                        // std::cout<<std::this_thread::get_id()<<" READ :"<<_data<<std::endl;
                                        handle_request();
                                    } else
                                        std::cout << std::this_thread::get_id() << " ERROR :" << ec.message()
                                                  << " CODE " << ec.value() << std::endl;
                                });
    }
    void read_and_save_file( int len) {
        auto self(shared_from_this());
       // std::cout<<std::this_thread::get_id()<<" READ FILE LEN:"<<len<<std::endl;

        if(len > 0)
        {
            _socket.async_read_some(boost::asio::buffer(_data, len),
                                    [this, self,len](std::error_code ec, std::size_t length) {
                                        if (!ec) {
                                         //   std::cout<<std::this_thread::get_id()<<" READ :"<<_data.data()<<std::endl;


                                            _outfile.write(_data.data(),length);
                                            read_and_save_file(len-length);




                                        } else
                                            std::cout << std::this_thread::get_id() << " ERROR :" << ec.message()
                                                      << " CODE " << ec.value() << std::endl;
                                    });
        }else{

            _outfile.close();
            if(_outfile.fail()){
             cout<<"FAILED CREATING FILE"<<endl;
             }
            std::cout<<std::this_thread::get_id()<<" SAVED FILE"<<std::endl;
            read_request();
        }

    }

    // TODO ADD HASHING
    bool login(const string &user, const string &pwd) {
        string savedpwd = _db->get(user);
        if (savedpwd == pwd) {
            _user = user; // !!important!!
            return true;
        } else {
            return false;
        }
    }

    void handle_request() {
        vector<string> params(5); // parsed values
        vector<string> tmp1(4); // support
        boost::split(tmp1, _data, boost::is_any_of("\n")); // take one line
        boost::split(params, tmp1[0], boost::is_any_of(" ")); // split by space
        cout << "-------------------------------------" << std::endl;
        cout<<this_thread::get_id() << " REQUEST "<<"FROM "<<_user <<": "<< std::endl;
        cout << "-------------------------------------" << std::endl;

        for (int i = 0; i < params.size(); i++) {
            cout <<i<< "\t" << params[i] << std::endl;
        }
        cout << "-------------------------------------" << std::endl;
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
                    write_str_sync("Wrong number arguments...Bye!");
                    return;
                }
                string user = params.at(1);
                string pwd = params.at(2);
                bool res = login(user, pwd);
                if (res) {
                    cout << "RESPONSE: OK" << std::endl;
                    write_str_sync("OK\n");
                    cout << "-------------------------------------" << std::endl;
                    read_request();
                } else {
                    cout<<"RESPONSE: WRONG CREDENTIALS"<<endl;
                    cout << "-------------------------------------" << std::endl;
                    write_str_sync("Wrong credentials...Bye!");
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
                if (params.size() < 3) {
                    write_str_sync("Wrong number arguments...Bye!");
                    return;
                }
                string path = params.at(1);
                int len = std::stoi(params.at(2));
                int time = std::stoi(params.at(3));
                 _outfile.open(path);
                read_and_save_file(len);
               // read_request();


               // read_request();
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
        boost::asio::async_write(_socket, boost::asio::buffer(_data, length),
                                 [this, self](std::error_code ec, std::size_t /*length*/) {
                                     if (!ec) {
                                         //do_read();
                                     }
                                 });
    }

    void write_str(std::string str) {
        auto self(shared_from_this());
        boost::asio::async_write(_socket, boost::asio::buffer(str, str.size()),
                                 [this, self](std::error_code ec, std::size_t /*length*/) {
                                     if (!ec) {
                                         // do_read();
                                     } else
                                         std::cout << std::this_thread::get_id() << " ERROR :" << ec.message()
                                                   << " CODE " << ec.value() << std::endl;
                                 });
    }
    void write_str_sync(std::string str) {
        _socket.write_some(boost::asio::buffer(str, str.size()));

    }


    tcp::socket _socket;
    std::ofstream _outfile;
    array<char,LEN_BUFF> _data;
    Db *_db;
    std::string _user;

};
