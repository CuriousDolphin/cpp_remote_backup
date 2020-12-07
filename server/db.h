//
// Created by isnob on 24/09/2020.
//
#pragma once
#ifndef SERVER_DB_H
#define SERVER_DB_H

#include <cpp_redis/cpp_redis>

class Db {
public:
    Db(int port);
    void set(const std::string &key, const std::string &value);


    void save_user_file_hash(const std::string user,const std::string path,const std::string sha);
    void set_user_pwd(const std::string user,const std::string pwd);
    std::string get_user_pwd(const std::string &user);
    std::string get(const std::string &key);

    // { path: hash }
    std::map<std::string, std::string>  get_user_snapshot(const std::string &user);

    bool delete_file_from_snapshot(const std::string &user, const std::string &path);

    std::string get_user_file_hash(const std::string user,const std::string path);

private:
    void connect();
    int port;
    cpp_redis::client redis_client;

};

#endif