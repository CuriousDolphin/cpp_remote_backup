//
// Created by frenk on 20/10/2020.
//

#include "db.h"

Db::Db(int port) : port(port) {
    connect();
}

void Db::set(const std::string &key, const std::string &value) {
    redis_client.set(key, value);
    redis_client.sync_commit();
    std::cout<<"[redis]"<<" stored key: "<<key<<std::endl;

}

void Db::save_user_file_hash(const std::string user,const std::string path,const std::string sha){
    redis_client.hset("snapshot:"+user,path, sha);
    redis_client.sync_commit();
    std::cout<<"[redis]"<<" stored  "<<"snapshot:"+user<<std::endl<<path<<":"<<sha<<std::endl;
}

void Db::set_user_pwd(const std::string user,const std::string pwd){
    redis_client.hset("users:",user, pwd);
    redis_client.sync_commit();
    std::cout<<"[redis]"<<" set user  "<<user<<std::endl;
}

std::string Db::get_user_pwd(const std::string &user) {
    std::future<cpp_redis::reply> future = redis_client.hget("users:",user);
    redis_client.sync_commit();
    future.wait();
    cpp_redis::reply reply = future.get();

    if (reply.is_error() || reply.is_null()) {
        return "";
    } else {
        std::cout<<"[redis]"<<" get user pwd: "<<user<<std::endl;
        return reply.as_string();

    }


}

std::string Db::get(const std::string &key) {
    std::future<cpp_redis::reply> future = redis_client.get(key);
    redis_client.sync_commit();
    future.wait();
    cpp_redis::reply reply = future.get();

    if (reply.is_error() || reply.is_null()) {
        return "";
    } else {
        std::cout<<"[redis]"<<" get key: "<<key<<std::endl;
        return reply.as_string();

    }


}

// { path: hash }
std::map<std::string, std::string>  Db::get_user_snapshot(const std::string &user) {
    std::cout<<"[redis]"<<" GET snapshot: "<<user<<std::endl;
    std::future<cpp_redis::reply> future = redis_client.hgetall("snapshot:"+user);
    redis_client.sync_commit();
    future.wait();
    cpp_redis::reply reply = future.get();
    std::map<std::string, std::string>  tmp = {};
    if (reply.is_error() || reply.is_null()) {
        std::cout << "error get user snapshot " << user << std::endl;
        return tmp;
    } else {
        if (reply.is_array()) {
            const auto& array = reply.as_array();
            // chiavi e valori sono ritornati come un vettore [k1,v1,k2,v2...] cosi  lo trasformo in una mappa per comodita
            if(array.size()>0){
                for(int i=0;i<array.size()-1;i+=2){
                    tmp[array[i].as_string()]=array[i+1].as_string();
                }
            }
        }
    }
    return tmp;

}

void Db::connect() {
    redis_client.connect("127.0.0.1", port,
                         [](const std::string &host, std::size_t port, cpp_redis::connect_state status) {
                             if (status == cpp_redis::connect_state::dropped) {
                                 std::cout << "  Redis_client disconnected from " << host << ":" << port
                                           << std::endl;
                             }
                             if (status == cpp_redis::connect_state::ok) {
                                 std::cout << "Redis_client connected to " << host << ":" << port << std::endl;
                             }
                         });
}