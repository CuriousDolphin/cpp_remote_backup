#pragma once
#ifndef CLIENT_H
#define CLIENT_H
#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include "../shared/const.h"
#include "node.h"

using boost::asio::ip::tcp;
using namespace std;
const std::map<std::string, int> commands = {{"LOGIN", 1},
                                             {"GET",   2},
                                             {"PUT",   3},
                                             {"PATCH", 4}};
//const int LEN_BUFF = 1024;

class client {
public:
    client(boost::asio::io_context &io_context,
           const tcp::resolver::results_type &endpoints, const string name, const string pwd);
    tcp::socket &socket();
    void do_write_str(std::string str);
    std::string read_sync();
    std::string read_sync_until_delimiter();
    std::string read_sync_n(int len);
    bool do_put_sync(Node n);
    void handle_response();
    void do_get_snapshot_sync();

    tcp::socket &socket() {
        return socket_;
    }

    void do_write_str(std::string str) {
        boost::asio::async_write(socket_, boost::asio::buffer(str, str.length()),
                                 [this, str](std::error_code ec, std::size_t length) {
                                     if (!ec) {
                                         std::cout << "SUCCESS send " << str << std::endl;
                                     } else {
                                         std::cout << std::this_thread::get_id() << " ERROR :" << ec.message()
                                                   << " CODE " << ec.value() << std::endl;
                                     }

                                 });
    }

    std::string read_sync(){
        _data.fill(0);
        socket_.read_some(boost::asio::buffer(_data.data(), LEN_BUFF));
        return _data.data();
    }



    std::string read_sync_n(int len){
        boost::system::error_code ec;
        int n =socket_.read_some(boost::asio::buffer(_data.data(), len),ec);
        cout<<"~READED B: "<<n<<"\n~ERROR CODE: "<<ec.message() <<endl;
        if(n<=0 ){
            return "";
        }

        return _data.data();
    }


    bool do_put_sync(Node n) {
        string str = "PUT"+PARAM_DELIMITER + n.toPathSizeTimeHash()+REQUEST_DELIMITER;
        _file.open(n.getPath(), ios::out | ios::app | ios::binary);
        if (_file.fail()) {
            cout << "failed to open file" << endl;
            return false;
        }
        size_t ris = socket_.write_some(boost::asio::buffer(str, str.length()));
        _file.seekg(0, _file.beg);

        int size = n.getSize();
        int n_to_send;
        while (size > 0) {
            if (size > LEN_BUFF)
                n_to_send = LEN_BUFF;
            else
                n_to_send = size;
            _file.read(_data.data(), n_to_send);
            int r = socket_.write_some(boost::asio::buffer(_data, n_to_send));
           // cout <<this_thread::get_id()<< "SEND [" << n_to_send << "]" << "REC [" << r << "]"  << endl;
            size -= r;
        }
        _file.close();
        return true;
    }

    void handle_response(){
        vector<string> tmp1(4); // support
        boost::split(tmp1, _data, boost::is_any_of(REQUEST_DELIMITER));
        cout<<"RESPONSE: "<<tmp1[0]<<endl;
    }

    void do_get_snapshot_sync(){
        string str = "SNAPSHOT"+REQUEST_DELIMITER;
        size_t ris = socket_.write_some(boost::asio::buffer(str, str.length()));

    }



private:
    array<char,LEN_BUFF> _data   ;
    boost::asio::io_context &io_context_;
    tcp::socket socket_;
    ifstream _file;

    void read_response();
    void login(const string name, const string pwd);
    void connect(const tcp::resolver::results_type &endpoints, const string name, const string pwd);
};

#endif