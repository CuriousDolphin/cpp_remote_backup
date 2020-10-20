#include <iostream>
#include "file_watcher.h"
#include <boost/asio.hpp>
#include "client.h"
#include "../shared/job.h"
#include "../shared/duration_logger.h"
#include "../shared/shared_box.h"
#include <boost/algorithm/string_regex.hpp>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>

const std::string path_to_watch = "../my_sync_folder";
const auto fw_delay = std::chrono::milliseconds(5000);
const auto snapshot_delay = std::chrono::seconds(20);
mutex m; // for print

// Define available file changes
enum class Method {
    SNAPSHOT,
    GET,
    PUT,
    PATCH,
    DELETE,
};

void print(const ostringstream &oss) {
    unique_lock<mutex> lg(m);
    cout << oss.str() << endl;
}

int main() {
    shared_box<std::unordered_map<string, Node>> remote_snapshot;
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve("localhost", "5555");
    client client(io_context, endpoints, "ivan", "mimmo");
    Jobs<std::tuple<Method, Node>> jobs;


    std::thread io_thread([&io_context, &jobs, &client,&remote_snapshot]() {
        ostringstream oss;
        oss << "[IO_THREAD]: " << this_thread::get_id();

        print(oss);
        io_context.run();
        Method req;
        Node n;
        while (true) {
            // get username
            optional<tuple<Method, Node>> action = jobs.get();
            if (!action.has_value()) {
                break;
            }
            req = get<0>(action.value());
            n = get<1>(action.value());
            switch (req) {
                case Method::PUT: {
                    DurationLogger dl("put file");
                    bool ris = client.do_put_sync(n); // send file ( first header, next content)

                    if (ris) {
                        client.read_sync(); // read response
                        client.handle_response(); // handle response
                    }
                }
                    break;
                case Method::GET:
                    break;
                case Method::PATCH:
                    break;
                case Method::SNAPSHOT: {
                    DurationLogger dl("snapshot");
                    client.do_get_snapshot_sync();
                    std::string res=client.read_sync_n(30); // read response HEADER containing number of elements of snapshot

                    // handle response HEADER N_FILES AND DIM PAYLOAD
                    vector<string> params; // parsed values
                    vector<string> tmp; // support
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

                        if(snapshot_size<LEN_BUFF){
                            std::string tmp2=client.read_sync_n(snapshot_size);

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
                                    std::cout<<"\t\tpath: "<<path<<endl;
                                    std::cout<<"\t\thash: "<<hash<<endl;
                                    Node nod(path,false,hash);
                                    remote_paths.insert({path, nod});
                                }
                            }
                            remote_snapshot.set(remote_paths);
                        }



                    }
                }


                    break;

                default:
                    break;
            }


        }
    });

    std::thread fw_thread([&jobs,&remote_snapshot]() {
        ostringstream oss;
        oss << "[FW_THREAD]: " << this_thread::get_id() << endl;
        print(oss);
        // Create a FileWatcher instance that will check the current folder for changes every 5 seconds

        FileWatcher fw{path_to_watch, fw_delay};
        // Start monitoring a folder for changes and (in case of changes)
        // run a user provided lambda function
        fw.start([&jobs,&remote_snapshot](Node node, FileStatus status) -> void {
            ostringstream oss;
            // Process only regular files, all other file types are ignored
            //if(!std::filesystem::is_regular_file(std::filesystem::path(path_to_watch)) && status != FileStatus::erased) {
            //   return;
            //}
            cout << "------------- [FILE WATCHER]------------" << std::endl;

            switch (status) {
                case FileStatus::created:
                    cout << "CREATED: \n"
                         << node.toString() << '\n';
                    {
                        if (!node.is_dir()) {
                            jobs.put(std::make_tuple(Method::PUT, node));
                        }

                    }
                    break;
                case FileStatus::modified:
                    std::cout << "MODIFIED: " << node.toString() << '\n';
                    break;
                case FileStatus::erased:
                    std::cout << "ERASED: " << node.toString() << '\n';
                    break;
                default:
                    std::cout << "Error! Unknown file status.\n";
            }
            std::cout << "-------------------------------------" << std::endl;

        });

    });


    while (true) {
        Node n;
        jobs.put(std::make_tuple(Method::SNAPSHOT, n));
        this_thread::sleep_for(snapshot_delay);
    }


    io_thread.join();
    fw_thread.join();
    return 0;
}


