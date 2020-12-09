#include "client.h"
#include "../shared/duration_logger.h"

client::client(boost::asio::io_context &io_context,
               const tcp::resolver::results_type &endpoints,
               const string name,
               const string pwd,
               shared_map<Node> *remote_snapshot,
               shared_map<bool> *pending_operations)
    : io_context_(io_context),
      _socket(io_context, tcp::endpoint(tcp::v4(), 4444)),
      _remote_snapshot(remote_snapshot),
      _pending_operations(pending_operations)
{
    connect(endpoints, name, pwd);
}

tcp::socket &client::socket()
{
    return _socket;
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

std::string client::read_sync_until_delimiter()
{
    _input_buffer.clear();
    boost::system::error_code ec;
    int n = boost::asio::read_until(_socket, boost::asio::dynamic_buffer(_input_buffer), REQUEST_DELIMITER, ec);
    cout << "~ [READED B]: " << n << " [EC]: " << ec.message() << endl;
    return _input_buffer.substr(0, n - REQUEST_DELIMITER.length());
}

size_t client::do_write_str_sync(std::string str)
{

    size_t ris = _socket.write_some(boost::asio::buffer(str, str.length()));
    return ris;
}

void client::read_response_login()
{
    string tmp;
    boost::asio::async_read_until(_socket,
                                  boost::asio::dynamic_buffer(_input_buffer), REQUEST_DELIMITER,
                                  [this](std::error_code ec, std::size_t n) {
                                      if (!ec)
                                      {
                                          std::cout << std::this_thread::get_id() << " READ_RESPONSE :" << _input_buffer.data()
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
                                     cout << "[SEND  LOGIN]" << endl;
                                     read_response_login();
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
                               [this, name, pwd](boost::system::error_code ec, const tcp::endpoint &) {
                                   if (!ec)
                                   {
                                       std::cout << this_thread::get_id() << "[CLIENT] CONNECTED TO SERVER" << std::endl;
                                       login(name, pwd);
                                   }
                                   else
                                   {
                                       std::cout << "FAILED CONNECTION" << ec << std::endl;
                                   }
                               });
}

void client::handle_request(Request req)
{
    vector<string> params; // parsed header params
    vector<string> tmp;    // support
    std::string res;
    Node node = req.node;
    switch (req.method)
    {
    case Method::PUT:
    {
        params.clear();
        string str = "";
        str += "PUT" + PARAM_DELIMITER + node.toPathSizeTimeHash() + REQUEST_DELIMITER;
        do_write_str_sync(str); // send header request
        params = read_header();
        if (params.size() == 1 && params.at(0) == "OK") // se l'header di risposta e' ok mando il file
        {
            bool ris = send_file_chunked(node);
            params.clear();
            if (ris)
            { // file sended successfull
                handle_response(move(req));
            }
            else
            { // some kind of error --> remove pending operation
                _pending_operations->remove( req.node.toString());
            }
        }
        if (params.size() == 2 && params.at(0) == "ERROR")
        { // se header risposta è error gestisco secondo ERROR CODE
            handle_errors(std::stoi(params.at(1)), req, node);
            _pending_operations->remove( req.node.toString());
        }

        break;
    }
    case Method::DELETE:
    {
        string str = "DELETE" + PARAM_DELIMITER + node.getAbsolutePath() + REQUEST_DELIMITER;
        do_write_str_sync(str);
        handle_response(move(req));
        break;
    }

    case Method::GET:
    {
        string str = "GET" + PARAM_DELIMITER + node.getAbsolutePath() + REQUEST_DELIMITER;
        do_write_str_sync(str);
        handle_response(move(req));
        break;
    }
    case Method::PATCH:
        break;
    case Method::SNAPSHOT:
    {
        string str = "SNAPSHOT" + REQUEST_DELIMITER;
        do_write_str_sync(str);
        handle_response(move(req));
    }
    break;

    default:
        break;
    }
}

void client::handle_response(Request &&req)
{
    vector<string> params; // parsed header params
    vector<string> tmp;    // support
    std::string res;
    Node node = req.node;

    params = read_header();
    switch (req.method)
    {
    case Method::PUT: // response  params must be OK HASH
    {
        if (params.size() == 2 && params.at(0) == "OK")
        { // received and store correct on server
            // aggiorno lo snapshot
            string hash = params.at(1);
            _remote_snapshot->set(req.node.getAbsolutePath(), move(node));
        }

        if (params.size() == 2 && params.at(0) == "ERROR")
        {
            handle_errors(std::stoi(params.at(1)), req, node);
        }
        _pending_operations->remove( req.node.toString());
    }
    break;
    case Method::DELETE:
    {
        if (params.size() == 2 && params.at(0) == "OK")
        { // received and store correct on server
            // aggiorno lo snapshot
            string hash = params.at(1);
            _remote_snapshot->remove(req.node.getAbsolutePath());
        }
        if (params.size() == 2 && params.at(0) == "ERROR")
        {
            std::cout << "FILE DELETE ERROR" << std::endl;
        }
        _pending_operations->remove(req.node.toString());
    }
    break;

    case Method::GET: // response params must be OK-filesize or ERROR-error code
    {
        if (params.size() == 2 && params.at(0) == "OK")
        { // file exists on server, start receiving
            long int filesize = stol(params.at(1));
            bool ris = read_and_save_file(node, filesize);
            if (ris)
            {
                // TO DO BETTER HANDLE ../
                std::string hash = Hasher::getSHA("../" + node.getPath());
                node.setLastHash(hash);
                std::cout << GREEN << "~ [FILE SAVED]: " << RESET << node.toString()<< std::endl;
                _remote_snapshot->set(node.getAbsolutePath(), move(node));
            }
            else
            {
                std::cout << RED << "ERROR ON FILE SAVING" << RESET <<std::endl;
            }
        }
        if (params.size() == 2 && params.at(0) == "ERROR")
        { //file does not exists on server
            handle_errors(std::stoi(params.at(1)), req, node);
        }
        _pending_operations->remove( req.node.toString());
    }
    break;

    case Method::SNAPSHOT:
    {
        if (params.at(0) == "OK" && !params.at(1).empty() && !params.at(2).empty())
        {
            int n_files = stoi(params.at(1));
            int snapshot_size = stoi(params.at(2));
            cout << " Number remote files: " << n_files << "\n dim payload: " << snapshot_size << endl;
            read_chunked_snapshot_and_set(snapshot_size);
        }
        if (params.at(0) == "ERROR")
        {
            std::cout << RED << "UNKNOWN ERROR DURING SNAPSHOT" << RESET << std::endl;
        }
        _pending_operations->remove("SNAPSHOT");
    }
    break;
    }
}

vector<string> client::extract_params(string &&str)
{
    vector<string> tmp; // parsed header params
    vector<string> params;
    boost::split_regex(tmp, str, boost::regex(PARAM_DELIMITER));
    ostringstream oss;
    oss << "~ [params received]:" << endl;
    for (int i = 0; i < tmp.size(); i++)
    {
        if (tmp[i] != "")
        {
            oss << "\t" << i << "\t" << tmp[i] << std::endl;
            params.push_back(tmp[i]);
        }
    }
    cout << oss.str();
    return params;
}

vector<string> client::read_header()
{
    string header = read_sync_until_delimiter(); // read response HEADER containing ok or error
    return extract_params(move(header));
}

bool client::send_file_chunked(Node n)
{
    cout << GREEN << "~ [SENDING FILE]" << RESET << endl;
    _file.open(n.getPath(), ios::out | ios::app | ios::binary);
    if (_file.fail())
    {
        cout << "failed to open file: " << n.getPath() << endl;
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

void client::read_chunked_snapshot_and_set(int len)
{
    int n_to_read;
    int size = len;

    // read chunked
    string tmp = "";
    while (size > 0)
    {
        if (size > LEN_BUFF)
            n_to_read = LEN_BUFF - 1;
        else
            n_to_read = size;
        _data.fill(0);
        int r = _socket.read_some(boost::asio::buffer(_data.data(), n_to_read));
        if (r > 0)
            tmp += _data.data();
        size -= r;
    }

    std::string path;
    std::string hash;
    vector<string> lines; // support
    vector<string> arguments(2);
    boost::split_regex(lines, tmp, boost::regex(REQUEST_DELIMITER)); // split lines
    cout << "~ [NUMBER LINES] " << lines.size() << endl;
    int i = 0;
    for (auto &line : lines)
    { // for each line split filepath and hash
        i++;
        if (!line.empty())
        {
            boost::split_regex(arguments, line, boost::regex(PARAM_DELIMITER));
            path = arguments[0];
            hash = arguments[1];
            cout << "\t\t [" << i << "]  " << path << " @ " << hash << endl;
            Node nod(path, false, hash);
            _remote_snapshot->set(path, move(nod));
        }
    }
}

bool client::read_and_save_file(Node n, int filesize)
{
    cout << GREEN << "~ [RECEIVING FILE]" << RESET << endl;
    _ofile.open("../" + n.getPath());
    if (_ofile.fail())
    {
        cout << "failed to open file: " << n.getPath() << endl;
        return false;
    }
    //_ofile.seekg(0, _ofile.beg);
    int n_to_receive;
    while (filesize > 0)
    {
        if (filesize > LEN_BUFF)
            n_to_receive = LEN_BUFF;
        else
            n_to_receive = filesize;
        int r = _socket.read_some(boost::asio::buffer(_data, n_to_receive));
        _ofile.write(_data.data(), n_to_receive);
        // cout <<this_thread::get_id()<< "SEND [" << n_to_send << "]" << "REC [" << r << "]"  << endl;
        filesize -= r;
    }
    _ofile.close();
    if (_ofile.fail())
    {
        cout << RED << "FAILED RECEIVING FILE" << RESET << endl;
        return false;
    }
    return true;
}

void client::handle_errors(int error_code, Request req, Node node){
    switch (get_error_by_code(error_code)){
        case Server_error::WRONG_CREDENTIALS :{
            std::cout << RED << "WRONG CREDENTIALS!" << RESET << std::endl;
            break;
        }
        case Server_error::UNKNOWN_COMMAND :{
            std::cout << RED << "UNKNOWN COMMAND!" << RESET <<std::endl;
            break;
        }
        case Server_error::WRONG_N_ARGS :{
            std::cout << RED << "WRONG NUMBER OF ARGUMENTS!" << RESET << std::endl;
            break;
        }
        case Server_error::FILE_EXIST :{
            std::cout << RED << "FILE EXIST!" << RESET << std::endl;
            break;
        }
        case Server_error::UNKNOWN_ERROR :{
            std::cout << RED << "UNKNOWN ERROR!" << RESET << std::endl;
            break;
        }
        case Server_error::FILE_CREATE_ERROR :{
            std::cout << RED << "FILE CREATE ERROR!" << RESET << std::endl;
            break;
        }
        case Server_error::FILE_HASH_MISMATCH :{
            std::cout << RED << "FILE HASH MISMATCH!" << RESET << std::endl;
            break;
        }
        case Server_error::FILE_ALREADY_EXISTS :{
            _remote_snapshot->set(req.node.getAbsolutePath(), move(node));
            std::cout << RED << "FILE ALREADY EXISTS" << RESET << std::endl;
            break;
        }
        case Server_error::FILE_DELETE_ERROR :{
            std::cout << RED << "FILE DELETE ERROR" << RESET << std::endl;
            break;
        }
        case Server_error::FILE_NOT_FOUND :{
            _remote_snapshot->remove(req.node.getAbsolutePath());
            std::cout << RED << "FILE NOT EXISTS ON SERVER!" << RESET << std::endl;
            break;
        }
        case Server_error::FILE_OPEN_ERROR :{
            std::cout << RED << "FILE OPEN ERROR!" << RESET << std::endl;
            break;
        }

        default:
            std::cout << RED << "UNKNOWN ERROR!" << RESET << std::endl;
            break;
    }

}
