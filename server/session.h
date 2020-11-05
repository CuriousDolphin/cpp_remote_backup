//
// Created by isnob on 24/09/2020.
//

#ifndef SERVER_SESSION_H
#define SERVER_SESSION_H

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
#include "../shared/const.h"
#include "../shared/hasher.h"
#include <boost/algorithm/string_regex.hpp>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>

//const int LEN_BUFF = 1024;
const std::map<std::string, int> commands = {{"LOGIN", 1}, // "LOGIN USER PWD"
                                             {"GET",   2}, //  "GET FILPATH"
                                             {"PUT",   3},
                                             {"PATCH", 4},
                                             {"SNAPSHOT", 5},
                                             {"DELETE", 6}
};
const std::string DATA_DIR="../data/";
using boost::asio::ip::tcp;
using namespace std;

class Session : public std::enable_shared_from_this<Session> {
public:
    Session(tcp::socket socket, Db *db);
    void start();
    ~Session();

private:
    void read_request(); //async
    void read_and_save_file(std::string const & effectivePath,std::string const & relativePath, int len, std::string const & reqHash);

    // TODO ADD HASHING
    bool login(const string &user, const string &pwd);
    void success_response_sync(); // reponse 'OK'
    void success_response_sync(std::string param); // 'OK param1'
    void success_response_sync(std::string param1,std::string param2);  // 'OK param1 param2'
    void error_response_sync(int cod_error); // 'ERROR COD_ERROR'
    void handle_request();
    // create directories if doesnt  exist
    void create_dirs(string path);
    void read_user_snapshot();
    void do_write(std::size_t length);
    void write_str(std::string str);
    int write_str_sync(std::string&& str);

    tcp::socket _socket;
    std::ofstream _outfile;
    array<char,LEN_BUFF> _data;
    Db *_db;
    std::string _user;

    bool delete_file(const string &effectivePath, const string &relativePath);


};

#endif