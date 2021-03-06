//
// Created by frenk on 20/10/2020.
//

#include "session.h"

Session::Session(tcp::socket socket, Db *db)
        : _socket(std::move(socket)), _user(""), _db(db) {
    std::stringstream ss;
    ss << this_thread::get_id() <<" NEW SESSION";
    Session::log("Server", "info", ss.str());
}

void Session::start() {
    read_request();
}

Session::~Session() {
    _socket.cancel();
    std::stringstream ss;
    ss << "DELETED SESSION " ;
    Session::log(_user, "info", ss.str());
}

void Session::read_request() {
    auto self(shared_from_this());
    std::stringstream ss;
    ss <<"WAITING NEW REQUEST..."<<endl;
    Session::log("Server", "info", ss.str());
    //std::size_t read  = _socket.read_some(boost::asio::buffer(_data, LEN_BUFF));
    //cout<<"READ REQUEST:"<<read<<endl;
    _input_buffer.clear();
    boost::asio::async_read_until(_socket,boost::asio::dynamic_buffer(_input_buffer),REQUEST_DELIMITER, [this, self](std::error_code ec, std::size_t length) {
        if (!ec) {
            //  std::cout<<std::this_thread::get_id()<<" READ :"<<_data.data()<<"("<<length<<std::endl;
            std::stringstream ss;
            ss << "START REQUEST " ;
            Session::log(_user, "info", ss.str());
            handle_request(move(_input_buffer.substr(0, length - REQUEST_DELIMITER.length())));
        } else
            std::cout << std::this_thread::get_id() << " ERROR :" << ec.message()
                      << " CODE " << ec.value() << std::endl;
    });

   /* _socket.async_read_some(boost::asio::buffer(_data, LEN_BUFF),
                            [this, self](std::error_code ec, std::size_t length) {
                                if (!ec) {
                                    //  std::cout<<std::this_thread::get_id()<<" READ :"<<_data.data()<<"("<<length<<std::endl;
                                    cout << "===================[START REQ "<< _user<<"]==================" << std::endl;
                                    handle_request();
                                } else
                                    std::cout << std::this_thread::get_id() << " ERROR :" << ec.message()
                                              << " CODE " << ec.value() << std::endl;
                            });*/
}

void Session::read_and_save_file(std::string const & effectivePath,std::string const & relativePath, int len, std::string const & reqHash) {
    auto self(shared_from_this());
     //std::cout<<" [READ] FILE LEN:"<<len<<std::endl;

    if(len > 0)
    {
        _socket.async_read_some(boost::asio::buffer(_data, len),
                                [this, self,len,effectivePath,relativePath,reqHash](std::error_code ec, std::size_t length) {
                                    if (!ec) {
                                        //std::cout<<std::this_thread::get_id()<<" [READED] : ("<<length<<")"<<_data.data()<<std::endl;

                                        _outfile.write(_data.data(),length);
                                        read_and_save_file(effectivePath,relativePath,len-length, reqHash);

                                    } else
                                        std::cout << std::this_thread::get_id() << " ERROR :" << ec.message()
                                                  << " CODE " << ec.value() << std::endl;
                                });
    }else{
        std::stringstream ss;
        _outfile.close();
        if(_outfile.fail()){
            ss.clear();
            ss << "FAILED CREATING FILE";
            Session::log(_user, "error", ss.str());
            read_request();
            return;
        }
        //controllo hash de file, in caso di errore -> revert
        std::string hash = Hasher::getSHA(effectivePath);

        if (reqHash != hash){
            delete_file(effectivePath, relativePath);
            error_response_sync(ERROR_COD.at(Server_error::FILE_HASH_MISMATCH));
            //std::cout << effectivePath + ": " + reqHash + " VS " + hash << std::endl;
        }
        else {
            ss.clear();
            ss << " [SAVED FILE] at ~" << effectivePath;
            Session::log(_user, "info", ss.str());

            _db->save_user_file_hash(_user, relativePath, hash);
            /*   snapshot:ivan={
             *                  ../datadir/ivan/file1.txt:45cbf8aef38c570733b4594f345e710c
             *
             *              }
             *
             * */
            success_response_sync(hash);
        }

        read_request();
    }

}

bool Session::login(const string &user, const string &pwd) {
    string savedpwd = _db->get_user_pwd(user);
    if (savedpwd == Hasher::pswSHA(pwd)) {
        _user = user; // !!important!!
        return true;
    } else {
        return false;
    }
}
void Session::success_response_sync(){
    write_str_sync("OK"+REQUEST_DELIMITER);
    std::stringstream ss;
    ss<<"WRITED: OK, SEND ME THE FILE!";
    Session::log(_user, "info", ss.str());
}
void Session::success_response_sync(std::string param){
    write_str_sync("OK"+PARAM_DELIMITER+param+REQUEST_DELIMITER);
}

void Session::success_response_sync(std::string param1,std::string param2){
    int n=write_str_sync("OK"+PARAM_DELIMITER+param1+PARAM_DELIMITER+param2+REQUEST_DELIMITER);
    std::stringstream ss;
    ss<<" ("<<n<<") :"<<"OK "+param1+PARAM_DELIMITER+param2+REQUEST_DELIMITER;
    Session::log(_user, "header", ss.str());
}

void Session::error_response_sync(int cod_error){
    write_str_sync("ERROR"+PARAM_DELIMITER+to_string(cod_error)+REQUEST_DELIMITER);
}

vector<string> Session::extract_params(const string &str)
{
    vector<string> tmp; // parsed header params
    vector<string> params;
    boost::split_regex(tmp, str, boost::regex(PARAM_DELIMITER));
    ostringstream oss;
    oss << endl;
    //Session::log(_user, "params", oss.str());
    //oss.clear();
    for (int i = 0; i < tmp.size(); i++)
    {
        if (!tmp[i].empty())
        {
            oss << "\t" << i << "\t" << tmp[i] << std::endl;
            params.push_back(tmp[i]);
        }
    }
    Session::log(_user, "params", oss.str());
    return params;
}

void Session::handle_request(const std::string& request) {
    vector<string> params  = extract_params(request) ; // parsed values
    vector<string> tmp1; // support
    std::stringstream ss;
    //boost::split_regex(tmp1, request, boost::regex(REQUEST_DELIMITER)); // take one line

    //boost::split_regex(params, tmp1[0], boost::regex(PARAM_DELIMITER)); // split by __

    //cout<<this_thread::get_id() << " REQUEST "<<"FROM "<<_user <<": "<< std::endl;

    //for (int i = 0; i < params.size(); i++) {
      //  cout <<i<< "\t" << params[i] << std::endl;
    //}
    int action = -1;
    try {
        action = commands.at(params[0]);
    }
    catch (const std::exception &) {
        ss.clear();
        ss <<  "UNKNOWN COMMAND" ;
        Session::log(_user, "error", ss.str());
        error_response_sync(ERROR_COD.at(Server_error::UNKNOWN_COMMAND));
        return;
    }

    switch (action) {
        case 1: { // LOGIN
            if (params.size() < 3) {
                error_response_sync(ERROR_COD.at(Server_error::WRONG_N_ARGS));
                return;
            }
            string user = params.at(1);
            string pwd = params.at(2);
            std::stringstream ss;
            bool res = login(user, pwd);
            if (res) {
                ss << "RESPONSE: OK";
                Session::log(_user, "info", ss.str());
                success_response_sync("");
                read_request();
            } else {
                ss<<"RESPONSE: WRONG CREDENTIALS";
                Session::log(_user, "error", ss.str());
                error_response_sync(ERROR_COD.at(Server_error::WRONG_CREDENTIALS));
            }
        }
            break;
        case 2:  { //GET

            if(!is_logged()){
                error_response_sync(ERROR_COD.at(Server_error::USER_NOT_LOGGED));
                return;
            }


            if (params.size() != 2) {
                ss.clear();
                ss <<  "WRONG N ARGS" ;
                Session::log(_user, "error", ss.str());
                error_response_sync(ERROR_COD.at(Server_error::WRONG_N_ARGS));
                return;
            }
            string path = params.at(1);
            string full_path = DATA_DIR + _user + path;
            if (_db->get_user_file_hash(_user, path) != ""){ //does requested file exists?
                if (boost::filesystem::exists(full_path)) {
                    _infile.open(full_path);
                    if (_infile.fail()) {
                        ss.clear();
                        ss << "FILE OPEN ERROR!"; //TODO manage error on client
                        Session::log(_user, "error", ss.str());
                        error_response_sync(ERROR_COD.at(Server_error::FILE_OPEN_ERROR));
                        return;
                    }
                    int len = boost::filesystem::file_size(full_path); //get file size
                    success_response_sync(std::to_string(len)); //send ok - filesize
                    send_file_chunked(full_path, path, len);
                    return;
                }
                else{
                    ss.clear();
                    ss <<  "FILE NOT EXIST ON FILESYSTEM" ;
                    Session::log(_user, "error", ss.str());
                    _db->delete_file_from_snapshot(_user,path);
                    error_response_sync(ERROR_COD.at(Server_error::FILE_NOT_FOUND));
                    read_request();
                    return;
                }
            }
            else{
                ss.clear();
                ss <<  "FILE NOT EXIST IN DB" ;
                Session::log(_user, "error", ss.str());
                error_response_sync(ERROR_COD.at(Server_error::FILE_NOT_FOUND));
                read_request();
            }
            break;
        }

            break;
        case 3: // PUT
        {
            if(!is_logged()){
                error_response_sync(ERROR_COD.at(Server_error::USER_NOT_LOGGED));
                return;
            }
            if (params.size() < 4) {
                ss.clear();
                ss <<  "WRONG N ARGS" ;
                Session::log(_user, "error", ss.str());
                error_response_sync(ERROR_COD.at(Server_error::WRONG_N_ARGS));
                return;
            }
            string path = params.at(1);
            string full_path = DATA_DIR + _user + path;
            long int len = std::stol(params.at(2));
            int time = std::stoi(params.at(3));
            string hash = params.at(4);

            if (!boost::filesystem::exists(full_path)) { //file already exists?

                create_dirs(full_path); // create dirs if not exists

                _outfile.open(full_path);
                if (_outfile.fail()) {
                    ss.clear();
                    ss << "FILE OPEN ERROR";
                    Session::log(_user, "error", ss.str());
                    error_response_sync(ERROR_COD.at(Server_error::FILE_CREATE_ERROR));
                    return;
                }
                success_response_sync(); // send ok
                read_and_save_file(full_path, path, len, hash);
            }
            else{

                if (hash == Hasher::getSHA(full_path)){ //TODO check on db's hash, don't calculate it!
                    ss.clear();
                    ss <<  "FILE ALREADY EXISTS" ;
                    Session::log(_user, "error", ss.str());
                    error_response_sync(ERROR_COD.at(Server_error::FILE_ALREADY_EXISTS));
                }
                else{ //file already existent on server with different hash --> PATCH
                    bool check = delete_file(full_path, path);
                    if (check) { // successful delete
                        create_dirs(full_path); // create dirs if not exists
                        _outfile.open(full_path);
                        if (_outfile.fail()) {
                            ss.clear();
                            ss << "FILE OPEN ERROR";
                            Session::log(_user, "error", ss.str());
                            error_response_sync(ERROR_COD.at(Server_error::FILE_CREATE_ERROR));
                            read_request();
                            return;
                        }
                        success_response_sync();
                        read_and_save_file(full_path, path, len, hash); //revert complete
                        return;
                    }
                    else{ //delete error
                        ss.clear();
                        ss <<  "FILE DELETE ERROR" ;
                        Session::log(_user, "error", ss.str());
                        error_response_sync(ERROR_COD.at(Server_error::FILE_DELETE_ERROR));
                    }
                }
                read_request();
            }
        }
        break;
        case 4: // PATCH
        {
            if(!is_logged()){
                error_response_sync(ERROR_COD.at(Server_error::USER_NOT_LOGGED));
                return;
            }
            read_request();
            break;
        }
        case 5: // SNAPSHOT
        {
            if(!is_logged()){
                error_response_sync(ERROR_COD.at(Server_error::USER_NOT_LOGGED));
                return;
            }
            try {
                std::map<string, string> files = _db->get_user_snapshot(_user);
                std::vector<std::string> lines_to_send;
                auto it = files.begin();
                int tot_len =0;
                std::string line ;
                // SCAN SNAPSHOT SAVED IN REDIS
                while(it!= files.end()){
                    line="";
                    line+=it->first;
                    line+=PARAM_DELIMITER;
                    line+=it->second;
                    //line+=PARAM_DELIMITER;
                    line+=REQUEST_DELIMITER;
                    tot_len+=line.length();
                    lines_to_send.push_back(line);
                    it++;
                }
                success_response_sync(to_string(files.size()),to_string(tot_len)); // send HEADER: num_files dim_payload
                std::ostringstream oss;
                for(auto& line:lines_to_send){ // send payload list of pats and hash
                    if(line.length()>1){
                        cout<<"\t\t"<<line;
                        oss<<line;
                    }
                }
                int n = write_str_sync(oss.str()); // send body
                std::stringstream ss;
                ss<<"[SENT] ("<<n <<") ";
                Session::log(_user, "info", ss.str());
            }catch(exception e){
                error_response_sync(ERROR_COD.at(Server_error::UNKNOWN_ERROR));
            }

            read_request();
            break;
        }
        case 6: // DELETE
        {
            if(!is_logged()){
                error_response_sync(ERROR_COD.at(Server_error::USER_NOT_LOGGED));
                return;
            }
            string path = params.at(1);
            string full_path = DATA_DIR + _user + path;
            bool check = delete_file(full_path,path);
            if (check) { // succesfull delete
                success_response_sync(std::to_string(check));
            }
            else{ //some kind of error during delete
                ss.clear();
                ss <<  "FILE DELETE ERROR" ;
                Session::log(_user, "error", ss.str());
                error_response_sync(ERROR_COD.at(Server_error::FILE_DELETE_ERROR));
            }
            read_request();
            break;
        }
    }
    //cout << "===================[END REQ "<< _user<<"]==================" << std::endl;
}


bool Session::is_logged(){
    if(_user.empty() ||  _user.length()==0){
        std::stringstream ss;
        ss <<  "{USER NOT LOGGED}" ;
        Session::log(_user, "error", ss.str());
        return false;
    }else{
        return true;
    }

}

// delete file from fs and redis
bool Session::delete_file(std::string const & effectivePath,std::string const & relativePath){
    bool fs_deleted = false;
    if(boost::filesystem::exists(effectivePath)){
        fs_deleted = boost::filesystem::remove(effectivePath);
    }
    _db->delete_file_from_snapshot(_user,relativePath);
    return fs_deleted;
}

// create directories if doesnt  exist
void Session::create_dirs(string path){
    try {
        boost::filesystem::path dirPath(path);
        boost::filesystem::create_directories(dirPath.parent_path());
    }
    catch(const boost::filesystem::filesystem_error& err) {
        std::cerr << err.what() << std::endl;
    }
}

void Session::read_user_snapshot(){
    std::map<string,string> tmp=_db->get_user_snapshot(_user);
    cout<<"USER_SNAPSHOT:\n["<<endl;
    for(auto& val:tmp){
        cout<<val.first<<":"<<val.second<<endl;
    }
    cout<<']'<<endl;
}

void Session::do_write(std::size_t length) {
    auto self(shared_from_this());
    boost::asio::async_write(_socket, boost::asio::buffer(_data, length),
                             [this, self](std::error_code ec, std::size_t /*length*/) {
                                 if (!ec) {
                                     //do_read();
                                 }
                             });
}

void Session::write_str(std::string str) {
    auto self(shared_from_this());
    boost::asio::async_write(_socket, boost::asio::buffer(str, str.size()),
                             [this, self](std::error_code ec, std::size_t /*length*/) {
                                 if (!ec) {
                                     // do_read();
                                 } else
                                     std::cout << std::this_thread::get_id() << " ERROR :" << ec.message()
                                               << " CODE " << ec.value() << std::endl;
                             });
}

int Session::write_str_sync(std::string&& str) {
    int byte_sent=boost::asio::write(_socket,boost::asio::buffer(str, str.size()), boost::asio::transfer_all());
    return byte_sent;
}

void Session::send_file_chunked(std::string const & effectivePath,std::string const & relativePath, int len) {
    auto self(shared_from_this());
    //std::cout <<" [WRITE] FILE LEN:" << len << std::endl;
    std::stringstream ss;
    if(len > 0)
    {
        int length = 0;
        if (len > LEN_BUFF)
            length = LEN_BUFF;
        else
            length = len;
        _infile.read(_data.data(), length);
        async_write(_socket, boost::asio::buffer(_data, len),
                                 [this, self,len,effectivePath,relativePath](std::error_code ec, std::size_t length){
                                    if (!ec) {
                                       // std::cout<<std::this_thread::get_id()<<" [READED] : ("<<length<<")"<<_data.data()<<std::endl;

                                        send_file_chunked(effectivePath,relativePath,len-length);

                                    } else
                                        std::cout << std::this_thread::get_id() << " ERROR :" << ec.message()
                                                  << " CODE " << ec.value() << std::endl;
                                });

    }else{

        _infile.close();
        if(_infile.fail()){
            ss.clear();
            ss<<"FAILED SENDING FILE";
            Session::log(_user, "error", ss.str());
            read_request();
            return;
        }
        else {
            ss.clear();
            ss << " SENT FILE ~ " << effectivePath;
            Session::log(_user, "info", ss.str());
            read_request();
        }
    }

}

void Session::log(const std::string &arg1, const std::string &arg2,const std::string &message){ //move(message)
    std::ofstream log_file;
    /*** console logging ***/
    if (arg2 == "redis") {
        std::cout << GREEN << "[" << arg1 << "] " << RESET << YELLOW << "[" << arg2 << "] " << RESET << message << std::endl;
    }
    if (arg2 == "error") {
        std::cout << GREEN << "[" << arg1 << "] " << RESET << RED << "[" << arg2 << "] " << RESET << message << std::endl;
    }
    if (arg2 == "info") {
        std::cout << GREEN << "[" << arg1 << "] " << RESET << BLUE << "[" << arg2 << "] " << RESET << message << std::endl;
    }
    if (arg2 == "params") {
        std::cout << GREEN << "[" << arg1 << "] " << RESET << MAGENTA << "[" << arg2 << "] " << RESET << message;
    }
    if (arg2 == "header") {
        std::cout << GREEN << "[" << arg1 << "] " << RESET << CYAN << "[" << arg2 << "] " << RESET << message;
    }
    /*if (arg2 == "server") {
        std::cout << BLUE << "[" << arg2 << "]: " << RESET << message << std::endl;
    }*/
    /*** file logging ***/
    log_file.open("../data/log.txt", ios::out | ios::app);
    if (log_file.is_open()) {
        /*if (arg2 == "server") {
            log_file << "[" << arg2 << "]: " << message << "\n";
            log_file.close();
        }
        else{*/
            log_file << "[" << arg1 << "] " << "[" << arg2 << "] " << message << "\n";
            log_file.close();
    }
    else
        std::cout << GREEN << "[" << arg1 << "] " << RESET
        << RED << "[" << arg2 << "] " << RESET << "Error during logging on file!" << std::endl;
}


