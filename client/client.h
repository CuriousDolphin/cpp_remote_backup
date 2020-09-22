#include <boost/asio.hpp>
#include <iostream>
using boost::asio::ip::tcp;

class client
{
public:
    client(boost::asio::io_context &io_context,
           const tcp::resolver::results_type &endpoints)
        : io_context_(io_context),
          socket_(io_context, tcp::endpoint(tcp::v4(),  4444))
    {
        do_connect(endpoints);
    }

    tcp::socket &socket()
    {
        return socket_;
    }

    void do_write_str(std::string str){
        boost::asio::async_write(socket_, boost::asio::buffer(str, str.length()),
                                 [this,str](std::error_code ec, std::size_t /*length*/)
                                 {
                                     if (!ec)
                                     {
                                         std::cout<<"SUCCESS send "<< str <<std::endl;
                                     } else std::cout<<std::this_thread::get_id()<<" ERROR :"<<ec.message()<<" CODE "<<ec.value() << std::endl;
                                 });
    }

private:
    boost::asio::io_context &io_context_;
    tcp::socket socket_;
    void do_connect(const tcp::resolver::results_type &endpoints)
    {
        unsigned short uiClientPort = socket().local_endpoint().port();
        std::cout << "DO CONNECT " << uiClientPort << std::endl;
        boost::asio::async_connect(socket_, endpoints,
                                   [this](boost::system::error_code ec, tcp::endpoint) {
                                       if (!ec)
                                       {
                                           std::cout << "CONNECTED TO SERVER" << std::endl;
                                           //   do_read_header();
                                       }
                                       else
                                       {
                                           std::cout << "FAILED CONNECTION" << ec << std::endl;
                                       }
                                   });
    }


};