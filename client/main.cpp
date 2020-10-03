#include <iostream>
#include "file_watcher.h"
#include <boost/asio.hpp>
#include "client.h"
#include "../shared/job.h"
#include "../shared/duration_logger.h"
#include "../shared/shared_box.h"

const std::string path_to_watch = "../my_sync_folder";
const auto delay = std::chrono::milliseconds(5000);
mutex m; // for print

// Define available file changes
enum class Request {
    SNAPSHOT,
    GET,
    PUT,
    PATCH,
    DELETE,
};

void print(const ostringstream&  oss){
    unique_lock<mutex> lg(m);
    cout<<oss.str()<<endl;
}

int main() {


    std::map<string, string> tmp = {};
    shared_box<std::map<string, string>> remote_snapshot;
    remote_snapshot.set(tmp);
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve("localhost", "5555");
    client client(io_context, endpoints, "ivan", "mimmo");
    Jobs<std::tuple<Request, Node>> jobs;


    std::thread io_thread([&io_context, &jobs, &client]() {
        ostringstream  oss;
        oss << "[IO_THREAD]: " << this_thread::get_id() ;

        print(oss);
        io_context.run();
        Request req;
        Node n;
        while (true) {
            // get username
            optional<tuple<Request, Node>> action = jobs.get();
            if (!action.has_value()) {
                break;
            }
            req = get<0>(action.value());
            n = get<1>(action.value());
            switch (req) {
                case Request::PUT: {
                    DurationLogger dl("put file");
                    bool ris = client.do_put_sync(n); // send file ( first header, next content)

                    if (ris) {
                        client.read_sync(); // read response
                        client.handle_response(); // handle response TODO
                    }
                }
                    break;
                case Request::GET:
                    break;
                case Request::PATCH:
                    break;
                case Request::SNAPSHOT:

                    break;

                default:
                    break;
            }


        }
    });

    std::thread fw_thread([&jobs]() {
        ostringstream  oss;
        oss << "[FW_THREAD]: " << this_thread::get_id() << endl;
        print(oss);
        // Create a FileWatcher instance that will check the current folder for changes every 5 seconds

        FileWatcher fw{path_to_watch, delay};
        // Start monitoring a folder for changes and (in case of changes)
        // run a user provided lambda function
        fw.start([&jobs](Node node, FileStatus status) -> void {
            ostringstream  oss;
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
                            jobs.put(std::make_tuple(Request::PUT, node));
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


    io_thread.join();
    fw_thread.join();
    return 0;
}


