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



    //std::size_t read  = _socket.read_some(boost::asio::buffer(_data, LEN_BUFF));
    //cout<<"READ REQUEST:"<<read<<endl;
    _socket.async_read_some(boost::asio::buffer(_data, LEN_BUFF),
                            [this, self](std::error_code ec, std::size_t length) {
                                if (!ec) {
                                    //  std::cout<<std::this_thread::get_id()<<" READ :"<<_data.data()<<"("<<length<<std::endl;
                                    handle_request();
                                } else
                                    std::cout << std::this_thread::get_id() << " ERROR :" << ec.message()
                                              << " CODE " << ec.value() << std::endl;
                            });
}

void Session::read_and_save_file(std::string const & effectivePath,std::string const & relativePath, int len) {
    auto self(shared_from_this());
    // std::cout<<std::this_thread::get_id()<<" READ FILE LEN:"<<len<<std::endl;

    if(len > 0)
    {
        _socket.async_read_some(boost::asio::buffer(_data, len),
                                [this, self,len,effectivePath,relativePath](std::error_code ec, std::size_t length) {
                                    if (!ec) {
                                        //std::cout<<std::this_thread::get_id()<<" READ :"<<_data.data()<<std::endl;

                                        _outfile.write(_data.data(),length);
                                        read_and_save_file(effectivePath,relativePath,len-length);

                                    } else
                                        std::cout << std::this_thread::get_id() << " ERROR :" << ec.message()
                                                  << " CODE " << ec.value() << std::endl;
                                });
    }else{

        _outfile.close();
        if(_outfile.fail()){
            cout<<"FAILED CREATING FILE"<<endl;
            read_request();
        }
        std::cout<<std::this_thread::get_id()<<" SAVED FILE at ~"<<effectivePath<<std::endl;
        //_db->set(path,getMD5(path));

        _db->save_user_file_hash(_user,relativePath,Hasher::getSHA(effectivePath));

        /*   snapshot:ivan={
         *                  ../datadir/ivan/file1.txt:45cbf8aef38c570733b4594f345e710c
         *
         *              }
         *
         * */

        string tmp = _db->get(effectivePath);
        cout<<tmp<<endl;
        success_response_sync("");

        read_user_snapshot();

        read_request();
    }

}

// TODO ADD HASHING
bool Session::login(const string &user, const string &pwd) {
    string savedpwd = _db->get_user_pwd(user);
    if (savedpwd == pwd) {
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
    cout<<"[success response]: ("<<n<<") \n\t"<<"OK"+PARAM_DELIMITER+param1+PARAM_DELIMITER+param2+REQUEST_DELIMITER<<endl;
}

void Session::error_response_sync(){
    write_str_sync("ERROR"+REQUEST_DELIMITER);
}

void Session::handle_request() {
    vector<string> params; // parsed values
    vector<string> tmp1(4); // support
    boost::split(tmp1, _data, boost::is_any_of(REQUEST_DELIMITER)); // take one line
    boost::split_regex(params, tmp1[0], boost::regex(PARAM_DELIMITER)); // split by __

    cout << "-------------------------------------" << std::endl;
    cout<<this_thread::get_id() << " REQUEST "<<"FROM "<<_user <<": "<< std::endl;
    cout << "-------------------------------------" << std::endl;

    for (int i = 0; i < params.size(); i++) {
        cout <<i<< "\t" << params[i] << std::endl;
    }
    cout << "-------------------------------------" << std::endl;
    int action = -1;
    try {
        action = commands.at(params[0]);
    }
    catch (const std::exception &) {
        cout << "UNKNOWN COMMAND" << std::endl;
        write_str("Unknown command...Bye!");
        return;
    }

    switch (action) {
        case 1: { // LOGIN
            if (params.size() < 3) {
                write_str_sync("Wrong number arguments...Bye!");
                return;
            }
            string user = params.at(1);
            string pwd = params.at(2);
            bool res = login(user, pwd);
            if (res) {
                cout << "RESPONSE: OK" << std::endl;
                success_response_sync("");
                //  write_str_sync("OK\n");
                cout << "-------------------------------------" << std::endl;
                read_request();
            } else {
                cout<<"RESPONSE: WRONG CREDENTIALS"<<endl;
                cout << "-------------------------------------" << std::endl;
                write_str_sync("Wrong credentials...Bye!");
            }

        }
            break;
        case 2:  // GET
        {

            read_request();
        }

            break;
        case 3: // PUT
        {
            if (params.size() < 4) {
                write_str_sync("Wrong number arguments...Bye!");
                return;
            }
            string path = params.at(1);
            string full_path=DATA_DIR+_user+path;


            create_dirs(full_path); // create dirs if not exists

            int len = std::stoi(params.at(2));
            int time = std::stoi(params.at(3));
            string hash = params.at(4);
            // TODO MANAGE HASH AND CHECK IF FILE SAVED HAS SAME HASH
            // TODO check here if in db and fs exist this file
            // TODO handle bad response
            success_response_sync();

            _outfile.open(full_path);
            if(_outfile.fail()){
                cout<<"FILE OPEN ERROR"<<endl;
            }


            read_and_save_file(full_path,path,len);
            // read_request();


            // read_request();
        }
            break;
        case 4: // PATCH
        {
            read_request();
        }
        case 5: // SNAPSHOT
        {
            try {
                std::map<string, string> files = _db->get_user_snapshot(_user);

                std::vector<std::string> lines_to_send;
                auto it = files.begin();
                int tot_len =0;
                std::string line ;
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
                success_response_sync(to_string(files.size()),to_string(tot_len)); // send HEADER: num_files dim_payload

                std::ostringstream oss;
                for(auto& line:lines_to_send){ // send payload list of pats and hash
                    if(line.length()>1){
                        cout<<line;
                        oss<<line;
                    }
                }
                int n=write_str_sync(oss.str());
                cout<<"\t[SENT] ("<<n <<") "<<endl;
            }catch(exception e){
                // TODO HANDLE THIS
            }


            read_request();
        }
            break;

    }


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

int Session::write_str_sync(std::string str) {
    int byte_sent=_socket.write_some(boost::asio::buffer(str, str.size()));
    return byte_sent;
}


