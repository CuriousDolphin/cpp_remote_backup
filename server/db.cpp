//
// Created by frenk on 20/10/2020.
//

#include "db.h"


Db::Db(int port) : port(port)
{
    connect();
}

void Db::set(const std::string &key, const std::string &value)
{
    redis_client.set(key, value);
    redis_client.sync_commit();
    std::stringstream ss ;
    ss << " stored key: " << key ;
    Db::log("Server", "db", ss.str());
}

void Db::save_user_file_hash(const std::string &user, const std::string &path, const std::string &sha)
{
    std::string user_hash = Hasher::pswSHA(user);
    redis_client.hset("snapshot:" + user_hash, path, sha);
    redis_client.sync_commit();
    std::stringstream ss ;
    ss << " stored snapshot" << std::endl << path << ":" << sha ;
    Db::log(user, "db", ss.str());
}

void Db::set_user_pwd(const std::string &user, const std::string &pwd)
{
    std::string user_hash = Hasher::pswSHA(user);
    redis_client.hset("users:", user_hash, pwd);
    redis_client.sync_commit();
    std::stringstream ss ;
    ss << " set user  " << user << "@" << user_hash;
    Db::log("Server", "db", ss.str());
}

std::string Db::get_user_pwd(const std::string &user)
{
    std::string user_hash = Hasher::pswSHA(user);
    std::future<cpp_redis::reply> future = redis_client.hget("users:", user_hash);
    redis_client.sync_commit();
    future.wait();
    cpp_redis::reply reply = future.get();

    if (reply.is_error() || reply.is_null())
    {
        return "";
    }
    else
    {
        std::stringstream ss ;
        ss << " get user pwd ";
        Db::log(user, "db", ss.str());
        return reply.as_string();
    }
}

std::string Db::get(const std::string &key)
{
    std::future<cpp_redis::reply> future = redis_client.get(key);
    redis_client.sync_commit();
    future.wait();
    cpp_redis::reply reply = future.get();
    std::cout << "-----------------" << key << "-----------------" << std::endl;
    if (reply.is_error() || reply.is_null())
    {
        return "";
    }
    else
    {
        std::stringstream ss ;
        ss << " get key: " << key ;
        Db::log("Server", "db", ss.str());
        return reply.as_string();
    }
}

// { path: hash }
std::map<std::string, std::string> Db::get_user_snapshot(const std::string &user)
{
    std::string user_hash = Hasher::pswSHA(user);
    std::stringstream ss ;
    ss << " GET snapshot ";
    Db::log(user, "db", ss.str());
    std::future<cpp_redis::reply> future = redis_client.hgetall("snapshot:" + user_hash);
    redis_client.sync_commit();
    future.wait();
    cpp_redis::reply reply = future.get();
    std::map<std::string, std::string> tmp = {};
    if (reply.is_error() || reply.is_null())
    {
        ss.clear();
        ss << "error get user snapshot " << user;
        Db::log("Server", "error", ss.str());
        return tmp;
    }
    else
    {
        if (reply.is_array())
        {
            const auto &array = reply.as_array();
            // chiavi e valori sono ritornati come un vettore [k1,v1,k2,v2...] cosi  lo trasformo in una mappa per comodita
            if (array.size() > 0)
            {
                for (int i = 0; i < array.size() - 1; i += 2)
                {
                    tmp[array[i].as_string()] = array[i + 1].as_string();
                }
            }
        }
    }
    return tmp;
}
bool Db::delete_file_from_snapshot(const std::string &user, const std::string &path)
{
    std::string user_hash = Hasher::pswSHA(user);
    std::stringstream ss ;
    std::vector<std::string> v;
    v.push_back(path);
    std::future<cpp_redis::reply> future = redis_client.hdel("snapshot:" + user_hash, v);
    redis_client.sync_commit();
    future.wait();
    cpp_redis::reply reply = future.get();
    if (reply.is_error() || reply.is_null())
    {
        ss <<"error get user snapshot file " << user << " " << path  ;
        Db::log("Server", "error", ss.str());
        return false;
    }
    ss << " DELETE FILE " << path << " RES:" << reply.as_integer();
    Db::log(user, "db", ss.str());
    if (reply.is_integer() && reply.as_integer() != 0)
        return true;
    return false;
}
void Db::connect()
{
    redis_client.connect("localhost", port,
                         [this](const std::string &host, std::size_t port, cpp_redis::connect_state status) {
                             std::stringstream ss ;
                             if (status == cpp_redis::connect_state::dropped)
                             {
                                 ss << "  Redis_client disconnected from " << host << ":" << port;
                                 Db::log("Server", "db", ss.str());
                             }
                             if (status == cpp_redis::connect_state::ok)
                             {
                                 ss << "Redis_client connected to " << host << ":" << port;
                                 Db::log("Server", "db", ss.str());
                             }
                         });
}

std::string Db::get_user_file_hash(const std::string &user, const std::string &path)
{
    std::string user_hash = Hasher::pswSHA(user);
    std::future<cpp_redis::reply> future = redis_client.hget("snapshot:" + user_hash, path);
    redis_client.sync_commit();
    future.wait();
    cpp_redis::reply reply = future.get();
    if (reply.is_error() || reply.is_null())
    {
        return "";
    }
    else
    {
        std::stringstream ss ;
        ss << " get file hash :" << path << "@" << reply.as_string() ;
        Db::log(user, "db", ss.str());
        return reply.as_string();
    }
}

void Db::log(const std::string &arg1, const std::string &arg2,const std::string &message){ //move(message)
    std::ofstream log_file;
    /*** console logging ***/
    if (arg2 == "db") {
        std::cout << GREEN << "[" << arg1 << "] " << RESET << YELLOW << "[" << arg2 << "] " << RESET << message << std::endl;
    }
    if (arg2 == "error") {
        std::cout << GREEN << "[" << arg1 << "] " << RESET << RED << "[" << arg2 << "] " << RESET << message << std::endl;
    }
    if (arg2 == "info") {
        std::cout << GREEN << "[" << arg1 << "] " << RESET << BLUE << "[" << arg2 << "] " << RESET << message << std::endl;
    }
    /*if (arg2 == "server") {
        std::cout << BLUE << "[" << arg2 << "]: " << RESET << message << std::endl;
    }*/
    /*** file logging ***/
    log_file.open("../../log.txt", ios::out | ios::app);
    if (log_file.is_open()) {
        /*if (arg2 == "server") {
            log_file << "[" << arg2 << "]: " << message << "\n";
            log_file.close();
        }
        else{*/
        log_file << "[" << arg1 << "] " << "[" << arg2 << "] " << message << "\n";
        log_file.close();
    }
    else
        std::cout << RED << "Error during logging on file!"<< RESET << std::endl;

}
