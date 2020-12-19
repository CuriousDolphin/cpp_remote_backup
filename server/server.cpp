//
// Created by frenk on 20/10/2020.
//

#include "server.h"

server::server(boost::asio::io_context& io_context, short port,Db* db)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)){
    db_=db;
    std::stringstream ss ;
    ss<<"START SERVER: THREAD "<<std::this_thread::get_id();
    Session::log("Server", "info", ss.str());
    //io_context_= &io_context;
    do_accept();
}

/* void server::run(){
     std::vector<std::thread> threads;
     for(int n = 0; n < NUM_THREADS; ++n)
         threads.emplace_back([this](){
             std::cout<<"START  THREAD"<<std::this_thread::get_id()<<std::endl;
             io_context_->run();
         });

     for(auto& thread : threads)
     {
         if(thread.joinable())
         {
             std::cout<<"JOINED  THREAD"<<thread::id()<<std::endl;
             thread.join();
         }
     }
 } */

void server::do_accept() {
    std::stringstream ss ;
    ss << "THREAD " << std::this_thread::get_id() <<" DO ACCEPT";
    Session::log("Server", "info", ss.str());
    acceptor_.async_accept(
            [this](std::error_code ec,tcp::socket socket)
            {
                if (!ec)
                {
                    std::stringstream ss ;
                    ss<<std::this_thread::get_id() <<" NEW ACCEPT";
                    Session::log("Server", "info", ss.str());
                    std::make_shared<Session>(std::move(socket),db_)->start();
                }

                do_accept();
            });
}
