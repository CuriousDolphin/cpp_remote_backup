//
// Created by frenk on 20/10/2020.
//

#include "session.h"

Session::Session(tcp::socket socket, Db *db)
        : _socket(std::move(socket)), _user(""), _db(db) {
    cout << " new session " << this_thread::get_id() << endl;
}

void Session::start() {
    read_request();
}

Session::~Session() {
    _socket.cancel();
    cout << "DELETED SESSION " << _user << endl;
}

void Session::read_request() {
    auto self(shared_from_this());
    cout<<"========================[WAITING REQ]"<<endl;
    //std::size_t read  = _socket.read_some(boost::asio::buffer(_data, LEN_BUFF));
    //cout<<"READ REQUEST:"<<read<<endl;
    _socket.async_read_some(boost::asio::buffer(_data, LEN_BUFF),
                            [this, self](std::error_code ec, std::size_t length) {
                                if (!ec) {
                                    //  std::cout<<std::this_thread::get_id()<<" READ :"<<_data.data()<<"("<<length<<std::endl;
                                    cout << "===================[START REQ "<< _user<<"]==================" << std::endl;
                                    handle_request();
                                } else
                                    std::cout << std::this_thread::get_id() << " ERROR :" << ec.message()
                                              << " CODE " << ec.value() << std::endl;
                            });
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

        _outfile.close();
        if(_outfile.fail()){
            cout<<"FAILED CREATING FILE"<<endl;
            read_request();
            return;
        }
        //controllo hash de file, in caso di errore -> revert
        std::string hash = Hasher::getSHA(effectivePath);

        if (reqHash != hash){
            delete_file(effectivePath, relativePath);
            error_response_sync(ERROR_COD.at(Server_error::FILE_HASH_MISMATCH));
            std::cout << effectivePath + ": " + hash + " VS " + reqHash << std::endl;
        }
        else {
            std::cout << " [SAVED FILE] at ~" << effectivePath << std::endl;

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
    cout<<"WRITED: OK"<<endl;
}
void Session::success_response_sync(std::string param){
    write_str_sync("OK"+PARAM_DELIMITER+param+REQUEST_DELIMITER);
}

void Session::success_response_sync(std::string param1,std::string param2){
    int n=write_str_sync("OK"+PARAM_DELIMITER+param1+PARAM_DELIMITER+param2+REQUEST_DELIMITER);
    cout<<"[write header]: ("<<n<<") :"<<"OK "+param1+PARAM_DELIMITER+param2+REQUEST_DELIMITER<<endl;
}

void Session::error_response_sync(int cod_error){
    write_str_sync("ERROR"+PARAM_DELIMITER+to_string(cod_error)+REQUEST_DELIMITER);
}

void Session::handle_request() {
    vector<string> params; // parsed values
    vector<string> tmp1(4); // support
    boost::split(tmp1, _data, boost::is_any_of(REQUEST_DELIMITER)); // take one line
    boost::split_regex(params, tmp1[0], boost::regex(PARAM_DELIMITER)); // split by __

    //cout<<this_thread::get_id() << " REQUEST "<<"FROM "<<_user <<": "<< std::endl;

    for (int i = 0; i < params.size(); i++) {
        cout <<i<< "\t" << params[i] << std::endl;
    }
    cout << "======================================================" << std::endl;
    int action = -1;
    try {
        action = commands.at(params[0]);
    }
    catch (const std::exception &) {
        cout << "UNKNOWN COMMAND" << std::endl;
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
            bool res = login(user, pwd);
            if (res) {
                cout << "RESPONSE: OK" << std::endl;
                success_response_sync("");
                cout << "-------------------------------------" << std::endl;
                read_request();
            } else {
                cout<<"RESPONSE: WRONG CREDENTIALS"<<endl;
                cout << "-------------------------------------" << std::endl;
                error_response_sync(ERROR_COD.at(Server_error::WRONG_CREDENTIALS));
            }
        }
            break;
        case 2:  { //GET
            if (params.size() != 2) {
                error_response_sync(ERROR_COD.at(Server_error::WRONG_N_ARGS));
                return;
            }
            string path = params.at(1);
            string full_path = DATA_DIR + _user + path;
            if (_db->get_user_file_hash(_user, path) != ""){ //does requested file exists?
                if (boost::filesystem::exists(full_path)) {
                    _infile.open(full_path);
                    if (_infile.fail()) {
                        cout << "FILE OPEN ERROR!" << endl; //TODO manage error on client
                        error_response_sync(ERROR_COD.at(Server_error::FILE_OPEN_ERROR));
                        return;
                    }
                    int len = boost::filesystem::file_size(full_path); //get file size
                    success_response_sync(std::to_string(len)); //send ok - filesize
                    send_file_chunked(full_path, path, len);
                    return;
                }
                else{
                    cout << "FILE DOES NOT EXIST IN FILESYSTEM!" << endl; //TODO manage error on client
                    error_response_sync(ERROR_COD.at(Server_error::FILE_NOT_FOUND));
                    read_request();
                    return;
                }
            }
            else{
                //TODO handle errors
                std::cout << "FILE DOES NOT EXISTS IND DB!" << std::endl;

                error_response_sync(ERROR_COD.at(Server_error::FILE_NOT_FOUND));
                read_request();
            }
            break;
        }

            break;
        case 3: // PUT
        {
            if (params.size() < 4) {
                error_response_sync(ERROR_COD.at(Server_error::WRONG_N_ARGS));
                return;
            }
            string path = params.at(1);
            string full_path = DATA_DIR + _user + path;
            int len = std::stoi(params.at(2));
            int time = std::stoi(params.at(3));
            string hash = params.at(4);

            if (!boost::filesystem::exists(full_path)) { //file already exists?

                create_dirs(full_path); // create dirs if not exists

                _outfile.open(full_path);
                if (_outfile.fail()) {
                    cout << "FILE OPEN ERROR" << endl;

                    error_response_sync(ERROR_COD.at(Server_error::FILE_CREATE_ERROR));
                    return;
                }
                success_response_sync(); // send ok
                read_and_save_file(full_path, path, len, hash);
            }
            else{

                if (hash == Hasher::getSHA(full_path)){ //TODO check on db's hash, don't calculate it!
                    cout << "FILE ALREADY EXISTS" << endl;
                    error_response_sync(ERROR_COD.at(Server_error::FILE_ALREADY_EXISTS));
                }
                else{ //file already existent on server with different hash --> PATCH
                    bool check = delete_file(full_path, path);
                    if (check) { // succesfull delete
                        create_dirs(full_path); // create dirs if not exists
                        _outfile.open(full_path);
                        if (_outfile.fail()) {
                            cout << "FILE OPEN ERROR" << endl;
                            error_response_sync(ERROR_COD.at(Server_error::FILE_CREATE_ERROR));
                            read_request();
                            return;
                        }
                        success_response_sync();
                        read_and_save_file(full_path, path, len, hash); //revert complete
                        return;
                    }
                    else{ //delete error
                        std::cout << "FILE DELETE ERROR!" << std::endl;
                        error_response_sync(ERROR_COD.at(Server_error::FILE_DELETE_ERROR));
                    }
                }
                read_request();
            }
        }
        break;
        case 4: // PATCH
        {
            read_request();
            break;
        }
        case 5: // SNAPSHOT
        {
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
                    line+=PARAM_DELIMITER;
                    line+=REQUEST_DELIMITER;
                    tot_len+=line.length();
                    lines_to_send.push_back(line);
                    it++;
                }
                success_response_sync(
                        to_string(files.size()),
                        to_string(tot_len)
                                      ); // send HEADER: num_files dim_payload

                std::ostringstream oss;
                for(auto& line:lines_to_send){ // send payload list of pats and hash
                    if(line.length()>1){
                        cout<<"\t\t"<<line;
                        oss<<line;
                    }
                }
                int n = write_str_sync(oss.str()); // send body
                cout<<"[SENT] ("<<n <<") "<<endl;
            }catch(exception e){
                error_response_sync(ERROR_COD.at(Server_error::UNKNOWN_ERROR));
            }

            read_request();
            break;
        }
        case 6: // DELETE
        {
            string path = params.at(1);
            string full_path = DATA_DIR + _user + path;
            bool check = delete_file(full_path,path);
            if (check) { // succesfull delete
                success_response_sync(std::to_string(check));
            }
            else{ //some kind of error during delete
                std::cout << "FILE DELETE ERROR!" << std::endl;
                error_response_sync(ERROR_COD.at(Server_error::FILE_DELETE_ERROR));
            }
            read_request();
            break;
        }
    }
    //cout << "===================[END REQ "<< _user<<"]==================" << std::endl;
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
    int byte_sent=_socket.write_some(boost::asio::buffer(str, str.size()));
    return byte_sent;
}

void Session::send_file_chunked(std::string const & effectivePath,std::string const & relativePath, int len) {
    auto self(shared_from_this());
    //std::cout <<" [WRITE] FILE LEN:" << len << std::endl;

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
            cout<<"FAILED SENDING FILE"<<endl;
            read_request();
            return;
        }
        else {
            std::cout << " [SENT FILE] ~" << effectivePath << std::endl;

            read_request();
        }
    }

}


