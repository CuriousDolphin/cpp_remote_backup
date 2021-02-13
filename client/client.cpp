#include "client.h"
#include "../shared/duration_logger.h"

client::client(boost::asio::io_context &io_context,
               const tcp::resolver::results_type &endpoints,
               const string name,
               const string pwd,
               shared_map<Node> *remote_snapshot,
               shared_map<bool> *pending_operations)
        : name(name), pwd(pwd), endpoints(endpoints),
          io_context_(io_context),
          _socket(io_context, tcp::endpoint(tcp::v4(), 4444)),
          _remote_snapshot(remote_snapshot),
          _pending_operations(pending_operations) {

    connected = false;
    connect();
}


std::string client::read_sync_until_delimiter() {
    _input_buffer.clear();
    boost::system::error_code ec;
    int n = boost::asio::read_until(_socket, boost::asio::dynamic_buffer(_input_buffer), REQUEST_DELIMITER, ec);
    std::string res = _input_buffer.substr(0, n - REQUEST_DELIMITER.length());
    _input_buffer = _input_buffer.substr(n, _input_buffer.length() - REQUEST_DELIMITER.length());

    cout << " ~ [HEADER B]: " << n << "[DATA]" << res << " [EC]: " << ec.message() << endl;
    if (ec.value() != 0) {
        throw std::runtime_error(ec.message());
    }
    if (_input_buffer.length() > 1)
        cout << " ~ [BUFFER RESIDUO] " << _input_buffer.length() << endl;
    return res;
}

size_t client::do_write_str_sync(std::string str) {
    boost::system::error_code ec;

    size_t ris = _socket.write_some(boost::asio::buffer(str, str.length()),ec);
    if (ec.value() != 0) {
        throw std::runtime_error(ec.message());
    }
    return ris;
}


void client::login(const string name, const string pwd) {
    string tmp = "LOGIN" + PARAM_DELIMITER + name + PARAM_DELIMITER + pwd + REQUEST_DELIMITER;
    do_write_str_sync(tmp);
}

void client::connect() {
    boost::system::error_code ec;
    boost::asio::connect(_socket, endpoints, ec);

    if (ec.value() == 0) {
        connected = true;
        cout << BLUE << "[CONNECTED TO SERVER]: " << RESET << endl;
    }else{
        cout << RED << "[FAILED CONNECT TO SERVER]:  " << RESET << ec.value() << endl;
    }
    if (connected) {
        login(name, pwd);
        cout << BLUE << "[LOGIN RESPONSE]: " << RESET << endl;
        std::vector<string> login_response = read_header();
    }

}


void client::disconnect() {
    cout << "DISCONNECT FROM SERVER" << endl;
    try{
        _socket.shutdown(boost::asio::ip::tcp::socket::shutdown_send);
    }catch(...){
    }

    connected = false;
}

void client::handle_request(Request req) {
    if (!connected) {
        cout << "Client is not connected...try to reconnect" << endl;
        connect();
        _pending_operations->remove(req.node.getPath());
    } else
        try {
            vector<string> params; // parsed header params
            vector<string> tmp;    // support
            std::string res;
            Node node = req.node;
            switch (req.method) {
                case Method::PUT: {
                    params.clear();
                    string str = "";
                    str += "PUT" + PARAM_DELIMITER + node.toPathSizeTimeHash() + REQUEST_DELIMITER;
                    do_write_str_sync(str); // send header request
                    params = read_header();
                    if (params.size() == 1 && params.at(0) == "OK") // se l'header di risposta e' ok mando il file
                    {
                        bool ris = send_file_chunked(node);
                        params.clear();
                        if (ris) { // file sended successfull
                            handle_response(move(req));
                        } else { // some kind of error --> remove pending operation
                            _pending_operations->remove(req.node.getPath());
                            disconnect();
                        }
                    }
                    if (params.size() == 2 &&
                        params.at(0) == "ERROR") { // se header risposta è error gestisco secondo ERROR CODE
                        handle_errors(std::stoi(params.at(1)), req, node);
                        _pending_operations->remove(req.node.getPath());
                    }

                    break;
                }
                case Method::DELETE: {
                    string str = "DELETE" + PARAM_DELIMITER + node.getAbsolutePath() + REQUEST_DELIMITER;
                    do_write_str_sync(str);
                    handle_response(move(req));
                    break;
                }

                case Method::GET: {
                    string str = "GET" + PARAM_DELIMITER + node.getAbsolutePath() + REQUEST_DELIMITER;
                    do_write_str_sync(str);
                    handle_response(move(req));
                    break;
                }
                case Method::PATCH:
                    break;
                case Method::SNAPSHOT: {
                    string str = "SNAPSHOT" + REQUEST_DELIMITER;
                    do_write_str_sync(str);
                    handle_response(move(req));
                }
                    break;

                default:
                    break;
            }
        } catch (std::exception &e) {

            _pending_operations->remove(req.node.getPath());
            std::stringstream ss;
            ss << e.what();
            cout <<RED<< "CATCHED: " << ss.str()<<RESET<<endl;
            disconnect();
        }
}

void client::handle_response(Request &&req) {
    if (!connected) {
        cout << "Client is not connected...try to reconnect" << endl;
        connect();
        _pending_operations->remove(req.node.getPath());
    } else
        try {
            vector<string> params; // parsed header params
            vector<string> tmp;    // support
            std::string res;
            Node node = req.node;

            params = read_header();
            switch (req.method) {
                case Method::PUT: // response  params must be OK HASH
                {
                    if (params.size() == 2 && params.at(0) == "OK") { // received and store correct on server
                        // aggiorno lo snapshot
                        string hash = params.at(1);
                        _remote_snapshot->set(req.node.getAbsolutePath(), move(node));
                    }

                    if (params.size() == 2 && params.at(0) == "ERROR") {
                        handle_errors(std::stoi(params.at(1)), req, node);
                    }
                    _pending_operations->remove(req.node.getPath());
                }
                    break;
                case Method::DELETE: {
                    if (params.size() == 2 && params.at(0) == "OK") { // received and store correct on server
                        // aggiorno lo snapshot
                        string hash = params.at(1);
                        _remote_snapshot->remove(req.node.getAbsolutePath());
                    }
                    if (params.size() == 2 && params.at(0) == "ERROR") {
                        std::cout << RED << "FILE DELETE ERROR" << RESET << std::endl;
                    }
                    _pending_operations->remove(req.node.getPath());
                }
                    break;

                case Method::GET: // response params must be OK-filesize or ERROR-error code
                {
                    if (params.size() == 2 && params.at(0) == "OK") { // file exists on server, start receiving
                        long int filesize = stol(params.at(1));
                        bool ris = read_and_save_file(node, filesize);
                        if (ris) {
                            std::string hash = Hasher::getSHA("../" + node.getPath());
                            node.setLastHash(hash);
                            std::cout << GREEN << "~ [FILE SAVED]: " << RESET << node.toString() << std::endl;
                            _remote_snapshot->set(node.getAbsolutePath(), move(node));
                        } else {
                            std::cout << RED << "ERROR ON FILE SAVING" << RESET << std::endl;
                        }
                    }
                    if (params.size() == 2 && params.at(0) == "ERROR") { //file does not exists on server
                        handle_errors(std::stoi(params.at(1)), req, node);
                    }
                    _pending_operations->remove(req.node.getPath());
                }
                    break;

                case Method::SNAPSHOT: {
                    if (params.at(0) == "OK" && !params.at(1).empty() && !params.at(2).empty()) {
                        int n_files = stoi(params.at(1));
                        int snapshot_size = stoi(params.at(2));
                        cout << " Number remote files: " << n_files << "\n dim payload: " << snapshot_size << endl;
                        read_chunked_snapshot_and_set(snapshot_size, n_files);
                    }
                    if (params.at(0) == "ERROR") {
                        std::cout << RED << "UNKNOWN ERROR DURING SNAPSHOT" << RESET << std::endl;
                    }
                    _pending_operations->remove("SNAPSHOT");
                }
                    break;
            }
        } catch (std::exception &e) {
            _pending_operations->remove(req.node.getPath());
            std::stringstream ss;
            ss << e.what();
            cout <<RED<< "CATCHED: " << ss.str()<<RESET<<endl;
            disconnect();
        }
}

vector<string> client::extract_params(string &&str) {
    vector<string> tmp; // parsed header params
    vector<string> params;
    boost::split_regex(tmp, str, boost::regex(PARAM_DELIMITER));
    ostringstream oss;
    oss << MAGENTA << "[params]:" << RESET << endl;
    for (int i = 0; i < tmp.size(); i++) {
        if (!tmp[i].empty()) {
            oss << "\t" << i << "\t" << tmp[i] << std::endl;
            params.push_back(tmp[i]);
        }
    }
    cout << oss.str();
    return params;
}

vector<string> client::read_header() {
    string header = read_sync_until_delimiter(); // read response HEADER containing ok or error
    return extract_params(move(header));
}

bool client::send_file_chunked(Node n) {
    cout << BLUE << "[SENDING FILE]" << RESET << endl;
    _file.open(n.getPath(), ios::out | ios::app | ios::binary);
    if (_file.fail()) {
        cout << RED << "failed to open file: " << RESET << n.getPath() << endl;
        return false;
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
        int r = _socket.write_some(boost::asio::buffer(_data, n_to_send));
        // cout <<this_thread::get_id()<< "SEND [" << n_to_send << "]" << "REC [" << r << "]"  << endl;
        size -= r;
    }
    _file.close();
    return true;
}

void client::read_chunked_snapshot_and_set(int len, int n_lines) {
    try {
        cout << BLUE << "[START READ SNAPSHOT CHUNKED]" << RESET << endl;
        int n_to_read;
        int size = len;

        string tmp = "";
        if (_input_buffer.size() > 0) { // ci sono ancora dati dalla lettura dell-header
            size -= _input_buffer.size();
            tmp += _input_buffer.data();
            cout << "   [ RESIDUO TROVATO ] " << endl;
        }
        // read chunked
        while (size > 0) {


            if (size > LEN_BUFF)
                n_to_read = LEN_BUFF - 1;
            else
                n_to_read = size;
            _data.fill(0);

            boost::system::error_code ec;


            //cout<<" ~ n to r:"<<n_to_read<<endl;

            ssize_t r = _socket.read_some(boost::asio::buffer(_data.data(), n_to_read));
            cout << " ~ [" << r << "]/" << n_to_read << " [EC]: " << ec.value() << endl;
            if (r > 0)
                tmp += _data.data();// _data.data();
            else
                break;
            size -= r;
        }

        std::string path;
        std::string hash;
        vector<string> lines; // support
        vector<string> arguments(3);
        boost::split_regex(lines, tmp, boost::regex(REQUEST_DELIMITER)); // split lines
        cout << CYAN << "[NUMBER LINES] " << RESET << lines.size() << endl;
        int i = 0;
        for (auto &line : lines) { // for each line split filepath and hash
            i++;
            if (!line.empty()) {
                boost::split_regex(arguments, line, boost::regex(PARAM_DELIMITER));

                if (arguments.size() == 2) {
                    path = arguments[0];
                    hash = arguments[1];
                    cout << "\t\t [" << i << "]  " << path << " @ " << hash << endl;
                    Node nod(path, false, hash);
                    _remote_snapshot->set(path, move(nod));
                }

            }
        }
        cout << BLUE << "[END READ SNAPSHOT]" << RESET << endl;
    } catch (...) {
        cout << RED << "[errore snapshot read chunked]" << RESET << endl;
    }
}

bool client::read_and_save_file(Node n, int filesize) {
    cout << GREEN << "~ [RECEIVING FILE]" << RESET << endl;
    // TO DO HANDLE ../
    create_dirs("../" + n.getPath());
    _ofile.open("../" + n.getPath());
    if (_ofile.fail()) {
        cout << RED << "failed to open file: " << RESET << n.getPath() << endl;
        return false;
    }

    if (_input_buffer.size() > 0) { // ci sono ancora dati dalla lettura dell-header
        filesize -= _input_buffer.size();
        _ofile.write(_input_buffer.data(), _input_buffer.size());
        cout << "   [ RESIDUO TROVATO ] " << endl;
    }
    //_ofile.seekg(0, _ofile.beg);
    int n_to_receive;
    while (filesize > 0) {
        if (filesize > LEN_BUFF)
            n_to_receive = LEN_BUFF;
        else
            n_to_receive = filesize;
        boost::system::error_code ec;

        int r = _socket.read_some(boost::asio::buffer(_data, n_to_receive), ec);
        // int r = _socket.read_some(boost::asio::buffer(_data, n_to_receive),&ec);
        //cout<<"READED: "<<r<<"[ec]: "<<ec.value()<<endl;
        _ofile.write(_data.data(), r);
        // cout <<this_thread::get_id()<< "SEND [" << n_to_send << "]" << "REC [" << r << "]"  << endl;
        filesize -= r;
    }
    _ofile.close();
    if (_ofile.fail()) {
        cout << RED << "FAILED RECEIVING FILE" << RESET << endl;
        return false;
    }
    return true;
}

void client::handle_errors(int error_code, Request req, Node node) {
    switch (get_error_by_code(error_code)) {
        case Server_error::WRONG_CREDENTIALS : {
            std::cout << RED << "WRONG CREDENTIALS!" << RESET << std::endl;
            break;
        }
        case Server_error::UNKNOWN_COMMAND : {
            std::cout << RED << "UNKNOWN COMMAND!" << RESET << std::endl;
            break;
        }
        case Server_error::WRONG_N_ARGS : {
            std::cout << RED << "WRONG NUMBER OF ARGUMENTS!" << RESET << std::endl;
            break;
        }
        case Server_error::FILE_EXIST : {
            std::cout << RED << "FILE EXIST!" << RESET << std::endl;
            break;
        }
        case Server_error::UNKNOWN_ERROR : {
            std::cout << RED << "UNKNOWN ERROR!" << RESET << std::endl;
            break;
        }
        case Server_error::FILE_CREATE_ERROR : {
            std::cout << RED << "FILE CREATE ERROR!" << RESET << std::endl;
            break;
        }
        case Server_error::FILE_HASH_MISMATCH : {
            std::cout << RED << "FILE HASH MISMATCH!" << RESET << std::endl;
            break;
        }
        case Server_error::FILE_ALREADY_EXISTS : {
            _remote_snapshot->set(req.node.getAbsolutePath(), move(node));
            std::cout << RED << "FILE ALREADY EXISTS" << RESET << std::endl;
            break;
        }
        case Server_error::FILE_DELETE_ERROR : {
            std::cout << RED << "FILE DELETE ERROR" << RESET << std::endl;
            break;
        }
        case Server_error::FILE_NOT_FOUND : {
            _remote_snapshot->remove(req.node.getAbsolutePath());
            std::cout << RED << "FILE NOT EXISTS ON SERVER!" << RESET << std::endl;
            break;
        }
        case Server_error::FILE_OPEN_ERROR : {
            std::cout << RED << "FILE OPEN ERROR!" << RESET << std::endl;
            break;
        }

        default:
            std::cout << RED << "UNKNOWN ERROR!" << RESET << std::endl;
            break;
    }

}

// create directories if doesnt  exist
void client::create_dirs(string path) {
    try {
        boost::filesystem::path dirPath(path);
        boost::filesystem::create_directories(dirPath.parent_path());
    }
    catch (const boost::filesystem::filesystem_error &err) {
        std::cerr << err.what() << std::endl;
    }
}

