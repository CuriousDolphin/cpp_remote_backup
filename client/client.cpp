#include "client.h"

client::client(boost::asio::io_context &io_context,
       const tcp::resolver::results_type &endpoints, const string name, const string pwd)
        : io_context_(io_context),
          socket_(io_context, tcp::endpoint(tcp::v4(), 4444)) {
    connect(endpoints, name, pwd);
}

tcp::socket &client::socket() {
    return socket_;
}

void client::do_write_str(std::string str) {
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

std::string client::read_sync(){
    socket_.read_some(boost::asio::buffer(_data.data(), LEN_BUFF));
    return _data.data();
}

std::string client::read_sync_until_delimiter(){
    boost::asio::streambuf buff;
    auto size=boost::asio::read_until(socket_,buff,REQUEST_DELIMITER);
    boost::asio::streambuf::const_buffers_type bufs = buff.data();
    std::string str(boost::asio::buffers_begin(bufs),
                    boost::asio::buffers_begin(bufs) + size);
    cout<<"PD:"<<size<<str<<endl;
    return str;
}

std::string client::read_sync_n(int len){
    socket_.read_some(boost::asio::buffer(_data.data(), len));
    return _data.data();
}

bool client::do_put_sync(Node n) {
    string str = "PUT"+PARAM_DELIMITER + n.toPathSizeTimeHash()+REQUEST_DELIMITER;
    _file.open(n.getPath(), ios::out | ios::app | ios::binary);
    if (_file.fail()) {
        cout << "failed to open file" << endl;
        return false;
    }
    size_t ris = socket_.write_some(boost::asio::buffer(str, str.length()));
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
    return true;
}

void client::handle_response(){
    vector<string> tmp1(4); // support
    boost::split(tmp1, _data, boost::is_any_of(REQUEST_DELIMITER));
    cout<<"RESPONSE: "<<tmp1[0]<<endl;
}

void client::do_get_snapshot_sync(){
    string str = "SNAPSHOT"+REQUEST_DELIMITER;
    size_t ris = socket_.write_some(boost::asio::buffer(str, str.length()));

}

void client::read_response() {
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

void client::login(const string name, const string pwd) {

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

void client::connect(const tcp::resolver::results_type &endpoints, const string name, const string pwd) {
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

