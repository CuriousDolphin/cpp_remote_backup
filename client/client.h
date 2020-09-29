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

    void do_put(Node n) {
        string str = "PUT__" + n.toPathSizeTimeHash();
        boost::asio::async_write(socket_, boost::asio::buffer(str, str.length()),
                                 [this, str, n](std::error_code ec, std::size_t length) {
                                     if (!ec) {
                                         std::cout << "Success send file header " << str << std::endl;
                                         write_file(n);
                                     } else {
                                         std::cout << std::this_thread::get_id() << " ERROR :" << ec.message()
                                                   << " CODE " << ec.value() << std::endl;
                                     }
                                 });
    }

    void do_put_sync(Node n) {
        string str = "PUT__" + n.toPathSizeTimeHash();
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
            _file.read(data_, n_to_send);

            int r = socket_.write_some(boost::asio::buffer(data_, n_to_send));
           // cout <<this_thread::get_id()<< "SEND [" << n_to_send << "]" << "REC [" << r << "]"  << endl;

            size -= r;

        }
        _file.close();


    }

    // TO DO CHUNK ME
    void write_file(Node n) {


        _file.open(n.getPath(), ios::out | ios::app | ios::binary);
        if (_file.fail()) {
            cout << "failed to open file" << endl;
            return;
        }

        _file.seekg(0, _file.beg);

        read_file_and_write(n.getSize());


    }

    void read_file_and_write(std::size_t size) {

        cout << "READ AND  WRITE  size [" << size << "]" << endl;

        if (size <= 0) {
            _file.close();
            std::cout << std::this_thread::get_id() << " SEND FILE SUCCESS" << std::endl;
            read_response();
            return;

        }
        size_t n_to_send;
        if (size > LEN_BUFF) {
            n_to_send = LEN_BUFF;

        } else {
            n_to_send = size;

        }
        cout << "N TO SEND [" << n_to_send << "]" << endl;
        try {
            _file.read(data_, n_to_send);
            cout << "READ FILE" << endl;


        } catch (exception e) {
            cout << "error reading file" << endl;
        }


        if (_file.fail() && !_file.eof()) {
            cout << "Failed while reading file" << endl;
        }
        cout << data_ << endl;
        std::stringstream ss;
        /* ss << "Readed " << file.gcount() << " bytes, total: "
           << file.tellg() << " bytes";
        std::cout << ss.str() << std::endl;*/



        boost::asio::async_write(socket_, boost::asio::buffer(data_, n_to_send),
                                 [this, size](std::error_code ec, std::size_t length) {
                                     if (!ec) {
                                         std::cout << "SUCCESS SENT BYTES : l[" << length << "] s[" << size << "]"
                                                   << endl;
                                         cout << " NEW SIZE " << size - length << endl;
                                         read_file_and_write(size - length);

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
    ifstream _file;

    void read_response() {
        string tmp;


        socket_.async_read_some(boost::asio::buffer(data_, LEN_BUFF),
                                [this](std::error_code ec, std::size_t length) {
                                    if (!ec) {
                                        std::cout << std::this_thread::get_id() << " READ_RESPONSE :" << data_
                                                  << std::endl;

                                    } else
                                        std::cout << std::this_thread::get_id() << " ERROR :" << ec.message()
                                                  << " CODE " << ec.value() << std::endl;
                                });
    }

    void login(const string name, const string pwd) {

        string tmp = "LOGIN " + name + " " + pwd+"\n" ;


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