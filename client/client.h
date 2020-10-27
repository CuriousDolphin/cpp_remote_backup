#pragma once
#ifndef CLIENT_H
#define CLIENT_H
#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include "../shared/const.h"
#include "node.h"
#include "request.h"

using boost::asio::ip::tcp;
using namespace std;
const std::map<std::string, int> commands = {{"LOGIN", 1},
                                             {"GET", 2},
                                             {"PUT", 3},
                                             {"PATCH", 4}};
//const int LEN_BUFF = 1024;

class client
{
public:
    client(boost::asio::io_context &io_context,
           const tcp::resolver::results_type &endpoints, const string name, const string pwd);
    tcp::socket &socket();
    void do_write_str(std::string str);
    std::string read_sync();
    std::string read_sync_n(int len);
    bool do_put_sync(Node n);
    void handle_response();
    void do_get_snapshot_sync();
    size_t do_write_str_sync(string str);
    void handle_request(Request req);

private:
    array<char, LEN_BUFF> _data;
    boost::asio::io_context &io_context_;
    tcp::socket socket_;
    ifstream _file;
    void read_response();
    void login(const string name, const string pwd);
    void connect(const tcp::resolver::results_type &endpoints, const string name, const string pwd); // called in costructor


};

#endif