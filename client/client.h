#pragma once
#ifndef CLIENT_H
#define CLIENT_H
#include <boost/asio.hpp>
#include <iostream>
#include <fstream>
#include "../shared/const.h"
#include "../shared/job.h"
#include "../shared/shared_map.h"
#include "../shared/hasher.h"
#include "node.h"
#include "request.h"
#include <boost/algorithm/string_regex.hpp>

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
           const tcp::resolver::results_type &endpoints,
           const string name,
           const string pwd,
           shared_map<Node> *remote_snapshot,
           shared_map<bool> *pending_operations
       );
    tcp::socket &socket();


    void handle_request(Request req);

private:
    array<char, LEN_BUFF> _data;
    shared_map<Node> *_remote_snapshot;
    shared_map<bool> *_pending_operations;
    std::string _input_buffer;
    boost::asio::io_context &io_context_;
    tcp::socket _socket;
    ifstream _file;
    ofstream _ofile;

    size_t do_write_str_sync(string str);
    bool send_file_chunked(Node n);
    void login(string name, string pwd);
    void connect(const tcp::resolver::results_type &endpoints, string name, string pwd); // called in costructor
    string read_sync_until_delimiter();
    static vector<string> extract_params(string &&str);
    vector<string> read_header();
    void handle_response(Request &&req);
    void read_chunked_snapshot_and_set(int len);
    bool read_and_save_file(Node n, int filesize);
};

#endif