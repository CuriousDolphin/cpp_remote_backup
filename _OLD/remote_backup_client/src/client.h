#include <boost/asio.hpp>
using boost::asio::ip::tcp;

class client
{
public:
    client(boost::asio::io_context &io_context,
           const tcp::resolver::results_type &endpoints)
        : io_context_(io_context),
          socket_(io_context, tcp::endpoint(tcp::v4(), 5555))
    {
        do_connect(endpoints);
    }

    tcp::socket &socket()
    {
        return socket_;
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