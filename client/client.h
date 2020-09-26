#include <boost/asio.hpp>
#include <iostream>

using boost::asio::ip::tcp;
using namespace std;
const std::map<std::string, int> commands = {{"LOGIN", 1},
                                             {"GET",   2},
                                             {"PUT",   3},
                                             {"PATCH", 4}};
const int LEN_BUFF = 1024;

class client {
public:
    client(boost::asio::io_context &io_context,
           const tcp::resolver::results_type &endpoints, const string name, const string pwd)
            : io_context_(io_context),
              socket_(io_context, tcp::endpoint(tcp::v4(), 4444)) {
        connect(endpoints, name, pwd);
    }

    tcp::socket &socket() {
        return socket_;
    }

    void do_write_str(std::string str) {
        boost::asio::async_write(socket_, boost::asio::buffer(str, str.length()),
                                 [this, str](std::error_code ec, std::size_t length) {
                                     if (!ec) {
                                         std::cout << "SUCCESS send " << str << std::endl;
                                     } else {
                                         std::cout << std::this_thread::get_id() << " ERROR :" << ec.message()
                                                   << " CODE " << ec.value() << std::endl;
                                     }

                                 });
    }

    // TO DO CHUNK ME
    void write_file(Node n) {

        ifstream myfile;
        myfile.open(n.getPath(), ios::out | ios::app | ios::binary);
        if(myfile.fail())
            cout<<"failed to open file"<<endl;
        auto fileSize = myfile.tellg();
        myfile.seekg(0, myfile.beg);
        myfile.read(data_, LEN_BUFF);
        if (myfile.fail() && !myfile.eof()) {
            cout<<"Failed while reading file"<<endl;
        }
        std::stringstream ss;
        ss << "Send " << myfile.gcount() << " bytes, total: "
           << myfile.tellg() << " bytes";
        std::cout << ss.str() << std::endl;

        boost::asio::async_write(socket_, boost::asio::buffer(data_, n.getSize()),
                [this](std::error_code ec, std::size_t length) {
                    if (!ec) {
                        cout<<"SEND OK "<<length<<endl;
                        read_response();
                    } else {
                        std::cout << std::this_thread::get_id() << " ERROR :" << ec.message()
                                  << " CODE " << ec.value() << std::endl;
                    }

                });
    }


private:
    char data_[LEN_BUFF]{};
    boost::asio::io_context &io_context_;
    tcp::socket socket_;

    void read_response() {
        string tmp;
        socket_.async_read_some(boost::asio::buffer(data_, LEN_BUFF),
                                [this](std::error_code ec, std::size_t length) {
                                    if (!ec) {
                                        std::cout << std::this_thread::get_id() << " READ :" << data_ << std::endl;
                                    } else
                                        std::cout << std::this_thread::get_id() << " ERROR :" << ec.message()
                                                  << " CODE " << ec.value() << std::endl;
                                });
    }

    void login(const string name, const string pwd) {

        string tmp = "LOGIN " + name + " " + pwd + "\n";
        boost::asio::async_write(socket_, boost::asio::buffer(tmp, tmp.length()),
                                 [this, tmp](std::error_code ec, std::size_t length) {
                                     if (!ec) {
                                         read_response();
                                     } else {
                                         std::cout << std::this_thread::get_id() << " ERROR :" << ec.message()
                                                   << " CODE " << ec.value() << std::endl;
                                     }

                                 });
    }

    void connect(const tcp::resolver::results_type &endpoints, const string name, const string pwd) {
        boost::asio::async_connect(socket_, endpoints,
                                   [this,  name,  pwd](boost::system::error_code ec, tcp::endpoint) {
                                       if (!ec) {
                                           std::cout << "CONNECTED TO SERVER" << std::endl;
                                           login(name, pwd);
                                           //   do_read_header();
                                       } else {
                                           std::cout << "FAILED CONNECTION" << ec << std::endl;
                                       }
                                   });
    }


};