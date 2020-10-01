#include <boost/asio.hpp>
#include <iostream>
#include "../shared/const.h"

using boost::asio::ip::tcp;
using namespace std;
const std::map<std::string, int> commands = {{"LOGIN", 1},
                                             {"GET",   2},
                                             {"PUT",   3},
                                             {"PATCH", 4}};
//const int LEN_BUFF = 1024;

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

    std::string read_sync(){
        socket_.read_some(boost::asio::buffer(_data.data(), LEN_BUFF));
        return _data.data();
    }

    void do_put_sync(Node n) {
        string str = "PUT"+PARAM_DELIMITER + n.toPathSizeTimeHash()+REQUEST_DELIMITER;
        size_t ris = socket_.write_some(boost::asio::buffer(str, str.length()));
        _file.open(n.getPath(), ios::out | ios::app | ios::binary);
        if (_file.fail()) {
            cout << "failed to open file" << endl;
            return;
        }
        _file.seekg(0, _file.beg);

        int size = n.getSize();
        int n_to_send;
        while (size > 0) {

            if (size > LEN_BUFF)
                n_to_send = LEN_BUFF;
            else
                n_to_send = size;
            _file.read(_data.data(), n_to_send);

            int r = socket_.write_some(boost::asio::buffer(_data, n_to_send));
           // cout <<this_thread::get_id()<< "SEND [" << n_to_send << "]" << "REC [" << r << "]"  << endl;

            size -= r;

        }
        _file.close();


    }

    void handle_response(){
        vector<string> tmp1(4); // support
        boost::split(tmp1, _data, boost::is_any_of(REQUEST_DELIMITER));
        cout<<"RESPONSE: "<<tmp1[0]<<endl;
    }



private:
    array<char,LEN_BUFF> _data   ;
    boost::asio::io_context &io_context_;
    tcp::socket socket_;
    ifstream _file;

    void read_response() {
        string tmp;

        _data.fill(0);
        socket_.async_read_some(boost::asio::buffer(_data, LEN_BUFF),
                                [this](std::error_code ec, std::size_t length) {
                                    if (!ec) {
                                        std::cout << std::this_thread::get_id() << " READ_RESPONSE :" << _data.data()
                                                  << std::endl;

                                    } else
                                        std::cout << std::this_thread::get_id() << " ERROR :" << ec.message()
                                                  << " CODE " << ec.value() << std::endl;
                                });
    }

    void login(const string name, const string pwd) {

        string tmp = "LOGIN"+PARAM_DELIMITER + name  +PARAM_DELIMITER + pwd+REQUEST_DELIMITER ;


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
                                   [this, name, pwd](boost::system::error_code ec, tcp::endpoint) {
                                       if (!ec) {
                                           std::cout <<this_thread::get_id() << "CONNECTED TO SERVER" << std::endl;
                                           login(name, pwd);
                                           //   do_read_header();
                                       } else {
                                           std::cout << "FAILED CONNECTION" << ec << std::endl;
                                       }
                                   });
    }


};