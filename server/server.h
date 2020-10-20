//
// Created by isnob on 24/09/2020.
//

#ifndef SERVER_SERVER_H
#define SERVER_SERVER_H

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
#include "session.h"
#include "db.h"
using boost::asio::ip::tcp;

class server
{
public:
    server(boost::asio::io_context& io_context, short port,Db* db);
    /* void run();*/

private:
    void do_accept();
    tcp::acceptor acceptor_;
    Db* db_;
    //  boost::asio::io_context *io_context_;
};

#endif