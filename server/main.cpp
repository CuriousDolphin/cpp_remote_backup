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




int main()
{
    try
    {


        Db db(6379);


        //db.set_user_pwd("ivan","mimmo");
        db.set_user_pwd("ivan","82d13e09a23511bf7906910e238ad09c339be756ebb083c6d1d03434418e089f");
        db.set_user_pwd("francesco","82d13e09a23511bf7906910e238ad09c339be756ebb083c6d1d03434418e089f");
        db.set_user_pwd("alberto","82d13e09a23511bf7906910e238ad09c339be756ebb083c6d1d03434418e089f");

        //! or client.commit(); for asynchronous call
        boost::asio::io_context io_context;
        server s(io_context, 5555,&db);
        std::vector<std::thread> threads;
        for(int n = 0; n < NUM_THREADS; ++n)
            threads.emplace_back([&io_context](){
                std::stringstream ss ;
                ss << "START THREAD " << std::this_thread::get_id();
                Session::log("Server", "info", ss.str());
                io_context.run();
            });

        for(auto& thread : threads)
        {
            if(thread.joinable())
            {
                std::stringstream ss ;
                ss << "JOINED THREAD " << std::this_thread::get_id();
                Session::log("Server", "info", ss.str());
                thread.join();
            }
        }
        //io_context.run();
        //s.run();


    }
    catch (std::exception& e)
    {
        std::stringstream ss ;
        ss << "Exception: " << e.what();
        Session::log("Server", "error", ss.str());
    }

    return 0;
}
