//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

#include <iostream>

#include <boost/asio/ts/buffer.hpp>
#include <boost/asio/ts/internet.hpp>

#include <vector>
#include <future>

#include <optional>

#include "server.h"
#include "db.h"


using namespace std;
int NUM_THREADS=8;




int main(int argc, char *argv[])
{
    /*
     * parameters:
     *
     * [0] -> db_host (localhost)
     * [1] -> db_port (6379)
     *
     */

    std::string db_host;
    int db_port;
    std::vector<std::thread> threads;
    if(argc == 3){
        db_host = argv[1];
        db_port = std::atoi(argv[2]);
        std::cout<<"parametri linea di comando"<<std::endl;
    }else{
        db_host = "localhost";
        db_port = 6379;
    }

    try
    {

        Db db(db_port,db_host);



        //db.set_user_pwd("ivan","mimmo");
        db.set_user_pwd("ivan","82d13e09a23511bf7906910e238ad09c339be756ebb083c6d1d03434418e089f");
        db.set_user_pwd("francesco","82d13e09a23511bf7906910e238ad09c339be756ebb083c6d1d03434418e089f");
        db.set_user_pwd("alberto","82d13e09a23511bf7906910e238ad09c339be756ebb083c6d1d03434418e089f");
        boost::asio::io_context io_context;
        server s(io_context, 5555,&db);

        for(int n = 0; n < NUM_THREADS; n++)
            threads.emplace_back([&io_context](){
                std::stringstream ss ;
                ss << "START IO THREAD " << std::this_thread::get_id();
                Session::log("Server", "info", ss.str());
                io_context.run();
            });

        for(auto& thread : threads)
        {
            if(thread.joinable())
            {
                //std::stringstream ss ;
                //ss << "JOINED THREAD " << std::this_thread::get_id();
                //Session::log("Server", "info", ss.str());
                thread.join();
            }
        }
    }
    catch (std::exception& e)
    {
        std::stringstream ss ;
        ss << "Exception: " << e.what();
        Session::log("Server", "error", ss.str());
    }



    return 0;
}
