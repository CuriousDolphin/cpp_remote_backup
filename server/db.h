//
// Created by isnob on 24/09/2020.
//
#pragma once
#ifndef SERVER_DB_H
#define SERVER_DB_H

#endif //SERVER_DB_H
#include <cpp_redis/cpp_redis>
class Db {
public:
    Db(int port):port(port){
        connect();
    }

    void set(const std::string& key,const std::string& value){
        redis_client.set(key, value);
        redis_client.sync_commit();
    }
    std::string get(const std::string& key){
        std::future<cpp_redis::reply> future=redis_client.get(key);
        redis_client.sync_commit();
        future.wait();
        cpp_redis::reply reply=future.get();

        if(reply.is_error() || reply.is_null()){
            return "";
        }else{
            return reply.as_string();
        }


    }

    void exist(const std::string& key){
       // redis_client.exists(key);
    }
private:
    void connect(){
        redis_client.connect("127.0.0.1", port, [](const std::string& host, std::size_t port, cpp_redis::connect_state status) {
            if (status == cpp_redis::connect_state::dropped) {
                std::cout << "  Redis_client disconnected from " << host << ":" << port << std::endl;
            }
            if (status == cpp_redis::connect_state::ok) {
                std::cout << "Redis_client connected to " << host << ":" << port << std::endl;
            }
        });
    }
    int port;
    cpp_redis::client redis_client;
};