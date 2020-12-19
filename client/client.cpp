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


std::string client::read_sync_until_delimiter()
{
    _input_buffer.clear();
    boost::system::error_code ec;
    int n = boost::asio::read_until(_socket, boost::asio::dynamic_buffer(_input_buffer), REQUEST_DELIMITER, ec);
    std::string res=_input_buffer.substr(0, n - REQUEST_DELIMITER.length());
    _input_buffer=_input_buffer.substr(n ,_input_buffer.length() - REQUEST_DELIMITER.length());

    cout << " ~ [HEADER B]: " << n <<"[DATA]"<<res<< " [EC]: " << ec.message() << endl;
    if(_input_buffer.length() > 1)
        cout<<" ~ [BUFFER RESIDUO] "<<_input_buffer.length() <<endl;
    return res;
}

size_t client::do_write_str_sync(std::string str)
{

    size_t ris = _socket.write_some(boost::asio::buffer(str, str.length()));
    return ris;
}



void client::login(const string name, const string pwd)
{
    string tmp = "LOGIN" + PARAM_DELIMITER + name + PARAM_DELIMITER + pwd + REQUEST_DELIMITER;
   do_write_str_sync(tmp);
}

void client::connect(const tcp::resolver::results_type &endpoints, const string name, const string pwd)
{
    boost::system::error_code ec;
    boost::asio::connect(_socket, endpoints,ec);
    cout<<"[CLIENT_CONNECT_EC]:  "<<ec.value()<<endl;
    login(name,pwd);
    cout<<" {LOGIN RESPONSE} "<<endl;
    std::vector<string> login_response = read_header();
    cout<<"-------------------------------------------"<<endl;

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
                _pending_operations->remove( req.node.getPath());
            }
        }
        if (params.size() == 2 && params.at(0) == "ERROR")
        { // se header risposta è error gestisco secondo ERROR CODE
            switch (std::stoi(params.at(1)))
            { //cast code error in int
            case 3:
            { //WRONG_N_ARGS
                std::cout << "WRONG NUMBER OF ARGUMENTS!" << std::endl;
                break;
            }
            case 6:
            { // FILE_CREATE_ERROR
                std::cout << "FILE CREATE ERROR!" << std::endl;
                break;
            }
            case 8:
            { // FILE_ALREADY_EXISTS
                //TODO update remote snapshot
                _remote_snapshot->set(req.node.getAbsolutePath(), move(node));

                std::cout << "FILE ALREADY EXISTS" << std::endl;
                break;
            }
            default:
                break;
            }
            _pending_operations->remove( req.node.getPath());
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
            switch (std::stoi(params.at(1)))
            { //cast code error in int
            case 7:
            { // FILE_HASH_MISMATCH
                std::cout << "FILE HASH MISMATCH!" << std::endl;
                break;
            }
            default:
                break;
            }
        }
        _pending_operations->remove( req.node.getPath());
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
        _pending_operations->remove(req.node.getPath());
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
                std::string hash = Hasher::getSHA("../" + node.getPath());
                node.setLastHash(hash);
                std::cout << "~ [FILE SAVED]: " << node.toString()<< std::endl;
                _remote_snapshot->set(node.getAbsolutePath(), move(node));
            }
            else
            {
                std::cout << "ERROR ON FILE SAVING" << std::endl;
            }
        }
        if (params.size() == 2 && params.at(0) == "ERROR")
        { //file does not exists on server

            if(get_error_by_code(std::stoi(params.at(1))) == Server_error::FILE_NOT_FOUND) {
                _remote_snapshot->remove(req.node.getAbsolutePath());
                std::cout << "FILE NOT EXISTS ON SERVER!" << std::endl;
            }
        }
        _pending_operations->remove( req.node.getPath());
    }
    break;

    case Method::SNAPSHOT:
    {
        if (params.at(0) == "OK" && !params.at(1).empty() && !params.at(2).empty())
        {
            int n_files = stoi(params.at(1));
            int snapshot_size = stoi(params.at(2));
            cout << " Number remote files: " << n_files << "\n dim payload: " << snapshot_size << endl;
            read_chunked_snapshot_and_set(snapshot_size,n_files);
        }
        if (params.at(0) == "ERROR")
        {
            std::cout << "UNKNOWN ERROR DURING SNAPSHOT" << std::endl;
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
    oss << " ~ [params received]:" <<endl;
    for (int i = 0; i < tmp.size(); i++)
    {
        if (!tmp[i].empty())
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
    cout << "~ [SENDING FILE]" << endl;
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

void client::read_chunked_snapshot_and_set(int len,int n_lines)
{
    try {
        cout<<" ~ [START READ SNAPSHOT CHUNKED]"<<endl;
        int n_to_read;
        int size = len;

        string tmp = "";
        if(_input_buffer.size()>0){ // ci sono ancora dati dalla lettura dell-header
            size-=_input_buffer.size();
            tmp+=_input_buffer.data();
            cout<<"   [ RESIDUO TROVATO ] "<<endl;
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
            cout<<" ~ ["<<r<<"]/"<<n_to_read<<" [EC]: "<<ec.value()<<endl;
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
        cout << "~ [NUMBER LINES] " << lines.size() << endl;
        int i = 0;
        for (auto &line : lines) { // for each line split filepath and hash
            i++;
            if (!line.empty()) {
                boost::split_regex(arguments, line, boost::regex(PARAM_DELIMITER));

                if(arguments.size()==2){
                    path = arguments[0];
                    hash = arguments[1];
                    cout << "\t\t [" << i << "]  " << path << " @ " << hash << endl;
                    Node nod(path, false, hash);
                    _remote_snapshot->set(path, move(nod));
                }

            }
        }
        cout << "[END]" << endl;
    } catch (...) {
        cout << "[errore snapshot read chunked]" << endl;
    }
}

bool client::read_and_save_file(Node n, int filesize)
{
    cout << "~ [RECEIVING FILE]" << endl;
    // TO DO HANDLE ../
    create_dirs("../" + n.getPath());
    _ofile.open("../" + n.getPath());
    if (_ofile.fail())
    {
        cout << "failed to open file: " << n.getPath() << endl;
        return false;
    }

    if(_input_buffer.size()>0){ // ci sono ancora dati dalla lettura dell-header
        filesize-=_input_buffer.size();
        _ofile.write(_input_buffer.data(), _input_buffer.size());
        cout<<"   [ RESIDUO TROVATO ] "<<endl;
    }
    //_ofile.seekg(0, _ofile.beg);
    int n_to_receive;
    while (filesize > 0)
    {
        if (filesize > LEN_BUFF)
            n_to_receive = LEN_BUFF;
        else
            n_to_receive = filesize;
        boost::system::error_code ec;

        int r = _socket.read_some(boost::asio::buffer(_data, n_to_receive),ec);
       // int r = _socket.read_some(boost::asio::buffer(_data, n_to_receive),&ec);
        //cout<<"READED: "<<r<<"[ec]: "<<ec.value()<<endl;
        _ofile.write(_data.data(), r);
        // cout <<this_thread::get_id()<< "SEND [" << n_to_send << "]" << "REC [" << r << "]"  << endl;
        filesize -= r;
    }
    _ofile.close();
    if (_ofile.fail())
    {
        cout << "FAILED RECEIVING FILE" << endl;
        return false;
    }
    return true;
}

// create directories if doesnt  exist
void client::create_dirs(string path){
    try {
        boost::filesystem::path dirPath(path);
        boost::filesystem::create_directories(dirPath.parent_path());
    }
    catch(const boost::filesystem::filesystem_error& err) {
        std::cerr << err.what() << std::endl;
    }
}

