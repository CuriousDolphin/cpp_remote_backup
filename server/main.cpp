//
// async_tcp_echo_server.cpp
// ~~~~~~~~~~~~~~~~~~~~~~~~~
//
// Copyright (c) 2003-2020 Christopher M. Kohlhoff (chris at kohlhoff dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//

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
#include "server.h"
#include "db.h"


using namespace std;
int NUM_THREADS=8;




int main()
{
    try
    {
        Db db(6379);

        db.set("ivan","mypwd");
        db.get("ivan");


        //! or client.commit(); for asynchronous call
        boost::asio::io_context io_context;
        server s(io_context, 5555);
        std::vector<std::thread> threads;
        for(int n = 0; n < NUM_THREADS; ++n)
            threads.emplace_back([&io_context](){
                std::cout<<"START  THREAD"<< std::this_thread::get_id()<<std::endl;
                io_context.run();
            });

        for(auto& thread : threads)
        {
            if(thread.joinable())
            {
                std::cout<<"JOINED  THREAD"<<thread::id()<<std::endl;
                thread.join();
            }
        }
        //io_context.run();
        //s.run();


    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}
