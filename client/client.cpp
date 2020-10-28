#include <boost/algorithm/string_regex.hpp>
#include "client.h"
#include "request.h"

client::client(boost::asio::io_context &io_context,
               const tcp::resolver::results_type &endpoints, const string name, const string pwd,shared_map<Node> *remote_snapshot)
    : io_context_(io_context),
    _socket(io_context, tcp::endpoint(tcp::v4(), 4444)),
    _remote_snapshot(remote_snapshot)
{
    connect(endpoints, name, pwd);
}

tcp::socket &client::socket()
{
    return _socket;
}

void client::do_write_str(std::string str)
{
    boost::asio::async_write(_socket, boost::asio::buffer(str, str.length()),
                             [this, str](std::error_code ec, std::size_t length) {
                                 if (!ec)
                                 {
                                     std::cout << "SUCCESS send " << str << std::endl;
                                 }
                                 else
                                 {
                                     std::cout << std::this_thread::get_id() << " ERROR :" << ec.message()
                                               << " CODE " << ec.value() << std::endl;
                                 }
                             });
}


std::string client::read_sync_n(int len)
{
    boost::system::error_code ec;
    _data.fill(0);
    int n = _socket.read_some(boost::asio::buffer(_data.data(), len), ec);
    cout << "~ [READED B]: " << n << " [EC]: " << ec.message() << endl;
    if (n <= 0)
    {
        return "";
    }

    return _data.data();
}

std::string client::read_sync()
{
    _socket.read_some(boost::asio::buffer(_data.data(), LEN_BUFF));
    return _data.data();
}


bool client::do_put_sync(Node n)
{
    _file.open(n.getPath(), ios::out | ios::app | ios::binary);
    if (_file.fail())
    {
        cout << "failed to open file: "<<n.getPath() << endl;
        return false;
    }
    _file.seekg(0, _file.beg);
    int size = n.getSize();
    int n_to_send;
    while (size > 0)
    {
        if (size > LEN_BUFF)
            n_to_send = LEN_BUFF;
        else
            n_to_send = size;
        _file.read(_data.data(), n_to_send);
        int r = _socket.write_some(boost::asio::buffer(_data, n_to_send));
        // cout <<this_thread::get_id()<< "SEND [" << n_to_send << "]" << "REC [" << r << "]"  << endl;
        size -= r;
    }
    _file.close();
    return true;
}
void client::read_and_send_file_async(int len)
{
    if(len>0){
        cout<<" READING FILE"<<endl;
        int n_to_send;
        if (len > LEN_BUFF)
            n_to_send = LEN_BUFF;
        else
            n_to_send = len;
        _file.read(_data.data(), n_to_send);


        int r = _socket.write_some(boost::asio::buffer(_data, n_to_send));

        boost::asio::async_write(_socket,boost::asio::buffer(_data, n_to_send),
                                 [this,len](std::error_code ec, std::size_t n) {
                                     if (!ec) {
                                         read_and_send_file_async(len-n);

                                     } else
                                         std::cout << std::this_thread::get_id() << " ERROR :" << ec.message()
                                                   << " CODE " << ec.value() << std::endl;


                                 });
    }else{
        cout<<"END READING FILE"<<endl;
        _file.close();
        read_sync(); // read response
        handle_response();
    }




    /* int size = n.getSize();
    int n_to_send;
    while (size > 0)
    {
        if (size > LEN_BUFF)
            n_to_send = LEN_BUFF;
        else
            n_to_send = size;
        _file.read(_data.data(), n_to_send);
        int r = socket_.write_some(boost::asio::buffer(_data, n_to_send));
        // cout <<this_thread::get_id()<< "SEND [" << n_to_send << "]" << "REC [" << r << "]"  << endl;
        size -= r;
    }
    _file.close(); */
}

void client::handle_response()
{
    vector<string> tmp1(4); // support
    boost::split(tmp1, _data, boost::is_any_of(REQUEST_DELIMITER));
    cout << "RESPONSE: " << tmp1[0] << endl;
}

size_t client::do_write_str_sync(std::string str) {

    size_t ris = _socket.write_some(boost::asio::buffer(str, str.length()));
    return ris;
}

void client::do_get_snapshot_sync()
{
    string str = "SNAPSHOT" + REQUEST_DELIMITER;
    size_t ris = _socket.write_some(boost::asio::buffer(str, str.length()));
}

void client::read_response()
{
    string tmp;
    boost::asio::async_read_until(_socket,
                                  boost::asio::dynamic_buffer(_input_buffer), REQUEST_DELIMITER,
                                  [this](std::error_code ec, std::size_t n){
                                      if (!ec)
                                      {
                                          std::cout << std::this_thread::get_id() << " READ_RESPONSE :" << _data.data()
                                                    << std::endl;
                                      }
                                      else
                                          std::cout << std::this_thread::get_id() << " ERROR :" << ec.message()
                                                    << " CODE " << ec.value() << std::endl;
    });
}

void client::login(const string name, const string pwd)
{

    string tmp = "LOGIN" + PARAM_DELIMITER + name + PARAM_DELIMITER + pwd + REQUEST_DELIMITER;

    boost::asio::async_write(_socket, boost::asio::buffer(tmp, tmp.length()),
                             [this, tmp](std::error_code ec, std::size_t length) {
                                 if (!ec)
                                 {
                                     read_response();
                                 }
                                 else
                                 {
                                     std::cout << std::this_thread::get_id() << " ERROR :" << ec.message()
                                               << " CODE " << ec.value() << std::endl;
                                 }
                             });
}

void client::connect(const tcp::resolver::results_type &endpoints, const string name, const string pwd)
{
    boost::asio::async_connect(_socket, endpoints,
                               [this, name, pwd](boost::system::error_code ec, tcp::endpoint) {
                                   if (!ec)
                                   {
                                       std::cout << this_thread::get_id() << "[CLIENT] CONNECTED TO SERVER" << std::endl;
                                       login(name, pwd);
                                       //   do_read_header();
                                   }
                                   else
                                   {
                                       std::cout << "FAILED CONNECTION" << ec << std::endl;
                                   }
                               });
}


void client::handle_request(Request req) {
    vector<string> params; // parsed header params
    vector<string> tmp; // support
    std::string res;
    Node n = req.node;
    switch (req.method) {
        case Method::PUT: {
            string str="";
            str += "PUT" + PARAM_DELIMITER + n.toPathSizeTimeHash() + REQUEST_DELIMITER;
            do_write_str_sync(str);
            res="";
            res=read_sync_n(30); // read response HEADER containing ok or error

            cout<<"HEADER RECEIVED: "+res;
            params.clear();
            tmp.clear();
            boost::split(tmp, res, boost::is_any_of(REQUEST_DELIMITER)); // take one line
            boost::split_regex(params, tmp[0], boost::regex(PARAM_DELIMITER)); // split by __
            ostringstream oss;
            oss<<"[params]:"<<endl;
            for (int i = 0; i < params.size(); i++) {
                oss <<"\t"<<i<< "\t" << params[i] << std::endl;
            }
            cout<<oss.str();

            if(params.at(0)=="OK"){ // se l'header di risposta e' ok mando il file

                /*_file.open(n.getPath(), ios::out | ios::app | ios::binary);
                if (_file.fail())
                {
                    cout << "failed to open file" << endl;
                    break;
                }
                _file.seekg(0, _file.beg);
                 read_and_send_file_async(n.getSize()); // send file
                 */
                //
                bool riis = do_put_sync(n);
                if(riis) {
                    // handle response
                    read_sync(); // read response
                    handle_response(); // handle response
                }
            }



        }
            break;
        case Method::DELETE:
        {
            string str="";
            str += "DELETE" + PARAM_DELIMITER + n.getAbsolutePath() + REQUEST_DELIMITER;
            do_write_str_sync(str);
            read_sync(); // read response
            handle_response(); // handle response

        }

            break;
        case Method::GET:
            break;
        case Method::PATCH:
            break;
        case Method::SNAPSHOT: {
            do_get_snapshot_sync();
            res="";
            res=read_sync_n(30); // read response HEADER containing number of elements of snapshot

            // handle response HEADER N_FILES AND DIM PAYLOAD

            params.clear();
            tmp.clear();
            boost::split(tmp, res, boost::is_any_of(REQUEST_DELIMITER)); // take one line
            boost::split_regex(params, tmp[0], boost::regex(PARAM_DELIMITER)); // split by __
            ostringstream oss;
            oss<<"[params]:"<<endl;
            for (int i = 0; i < params.size(); i++) {
                oss <<"\t"<<i<< "\t" << params[i] << std::endl;
            }
            cout<<oss.str();
            if(params.at(0)=="OK" && !params.at(1).empty() && !params.at(2).empty()){
                int n_files= stoi(params.at(1));
                int snapshot_size =  stoi(params.at(2));
                cout<<" Number remote files: "<<n_files<<"\n dim payload: "<<snapshot_size<<endl;
                std::map<string, Node> my_map;
                std::string path;
                std::string hash;
                vector<string> lines; // support
                // TODO MANAGE SNAPSHOT LEN > BUFFER
                if(snapshot_size<LEN_BUFF){
                    std::string tmp2=read_sync_n(snapshot_size);

                    // cout<<"[payload]:\n"<<tmp2<<"\n[end payload]"<<endl;


                    boost::split_regex(lines, tmp2, boost::regex(REQUEST_DELIMITER)); // split lines
                    vector<string> arguments(2);
                    std::unordered_map<std::string, Node> remote_paths;
                    for(auto&line:lines){ // for each line split filepath and hash
                        if(!line.empty()){
                            //cout<<"[" <<line<<"]"<<endl;
                            boost::split_regex(arguments, line, boost::regex(PARAM_DELIMITER));
                            path=arguments[0];
                            hash=arguments[1];
                            cout<<"\t\t "<<path<<" @ "<<hash<<endl;
                            Node nod(path,false,hash);
                            _remote_snapshot->set(path,move(nod));
                        }
                    }

                }



            }
        }


            break;

        default:
            break;
    }

}