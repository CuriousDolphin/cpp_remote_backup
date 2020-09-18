#include <cstdlib>
#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

using boost::asio::ip::tcp;
int cont = 0;
class session
{
public:
    session(boost::asio::io_service &io_service)
        : socket_(io_service), id(++cont)
    {
    }

    tcp::socket &socket()
    {
        return socket_;
    }

    void start()
    {
        socket_.async_read_some(boost::asio::buffer(data_, max_length),
                                boost::bind(&session::handle_read, this,
                                            boost::asio::placeholders::error,
                                            boost::asio::placeholders::bytes_transferred));
    }

    void delete_session()
    {
        std::cout << "DELETE SESSION:" << id << std::endl;
        delete this;
    }

    void handle_read(const boost::system::error_code &error,
                     size_t bytes_transferred)
    {
        unsigned short uiClientPort = socket().remote_endpoint().port();

        std::cout << "SESSION # " << id <<" thread"<<std::this_thread::get_id() <<" read " << data_ << " from " << socket().remote_endpoint().address().to_string() << ":" << uiClientPort << std::endl;
        if (!error)
        {
            
            boost::asio::async_write(socket_,
                                     boost::asio::buffer("ciao", 4),
                                     boost::bind(&session::handle_write, this,
                                                 boost::asio::placeholders::error));
        }
        else
        {
            delete_session();
        }
    }

    void handle_write(const boost::system::error_code &error)
    {
        //   std::cout << " write " << data_ << " from " << socket().remote_endpoint().address().to_string() << std::endl;
        if (!error)
        {
            start();
           // socket_.async_read_some(boost::asio::buffer(data_, max_length),
                                //    boost::bind(&session::handle_read, this,
                                     //           boost::asio::placeholders::error,
           //boost::asio::placeholders::bytes_transferred));
        }
        else
        {
            delete_session();
        }
    }

private:
    int id;
    tcp::socket socket_;
    enum
    {
        max_length = 5
    };
    char data_[max_length];
};

class server
{
public:
    server(boost::asio::io_service &io_service, short port)
        : io_service_(io_service),
          acceptor_(io_service, tcp::endpoint(tcp::v4(), 4444))
    {
        session *new_session = new session(io_service_);
        acceptor_.async_accept(new_session->socket(),
                               boost::bind(&server::handle_accept, this, new_session,
                                           boost::asio::placeholders::error));
    }

    void handle_accept(session *new_session,
                       const boost::system::error_code &error)
    {
        if (!error)
        {
            std::cout << "HANDLE ACCEPT" << std::endl;
            new_session->start();
            new_session = new session(io_service_);
            acceptor_.async_accept(new_session->socket(),
                                   boost::bind(&server::handle_accept, this, new_session,
                                               boost::asio::placeholders::error));
        }
        else
        {
            delete new_session;
        }
    }

private:
    boost::asio::io_service &io_service_;
    tcp::acceptor acceptor_;
};

int main()
{
    // todo take from env docker
    std::cout << "START SERVER" << std::endl;
    int port = 4444;
    try
    {
        boost::asio::io_service io_service;
        using namespace std; // For atoi.
        server s(io_service, 4444);

        io_service.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}