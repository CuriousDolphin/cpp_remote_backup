#include <iostream>
#include "file_watcher.h"
#include <boost/asio.hpp>
#include "client.h"
#include "../shared/duration_logger.h"
#include "../shared/shared_map.h"
#include "../shared/shared_box.h"

#include <boost/filesystem.hpp>

const std::string path_to_watch = "../my_sync_folder";
const auto fw_delay = std::chrono::milliseconds(5000);
const auto snapshot_delay = std::chrono::seconds(120);
mutex m; // for print

void print(const ostringstream &oss) {
    unique_lock<mutex> lg(m);
    cout << oss.str() << endl;
}

int main() {
    shared_map<Node> remote_snapshot;
    shared_map<bool> pending_operation;
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve("localhost", "5555");
    Jobs<Request> jobs;
    client client(io_context, endpoints, "ivan", "mimmo", &remote_snapshot, &pending_operation);

    std::thread io_thread([&io_context, &jobs, &client]() {
        ostringstream oss;
        oss << "[IO_THREAD]: " << this_thread::get_id();

        print(oss);
        io_context.run();

        while (true) {
            optional<Request> req = jobs.get();
            if (!req.has_value()) {
                break;
            }
            vector<string> params; // parsed header params
            vector<string> tmp;    // support
            std::string req_string;
            switch (req.value().method) {
                case Method::GET:
                    req_string = "Method::GET " + req.value().node.toString();
                    break;
                case Method::DELETE:
                    req_string = "Method::DELETE " + req.value().node.toString();
                    break;
                case Method::PUT:
                    req_string = "Method::PUT " + req.value().node.toString();
                    break;
                case Method::PATCH:
                    req_string = "Method::PATCH " + req.value().node.toString();
                    break;
                case Method::SNAPSHOT:
                    req_string = "Method::SNAPSHOT " ;
                    break;
            }
            {
                DurationLogger dl(req_string);
                client.handle_request(req.value());
            }
        };
    });

    std::thread fw_thread([&jobs, &remote_snapshot, &pending_operation]() {
        ostringstream oss;
        oss << "[FW_THREAD]: " << this_thread::get_id() << endl;
        print(oss);
        // Create a FileWatcher instance that will check the current folder for changes every 5 seconds

        FileWatcher fw{&remote_snapshot, path_to_watch, fw_delay};
        // Start monitoring a folder for changes and (in case of changes)
        // run a user provided lambda function
        fw.start([&jobs, &remote_snapshot, &pending_operation](Node node, FileStatus status) -> void {
            ostringstream oss;
            // Process only regular files, all other file types are ignored
            //if(!std::filesystem::is_regular_file(std::filesystem::path(path_to_watch)) && status != FileStatus::erased) {
            //   return;
            //}


            if (!node.is_dir())
                switch (status) {
                    case FileStatus::created: {


                        string op_key = "PUT_" + node.toString();
                        if (!pending_operation.exist(op_key)) { //se non c'e' gia' una richiesta uguale in coda
                            cout << "================= FW { CREATED }: \n" << node.toString() << endl;
                            pending_operation.set(op_key, true);
                            jobs.put(Request(Method::PUT, node));
                        } else {
                            cout << "================= FW { THROTTLE CREATED }" << endl;
                        }

                    }
                        break;
                    case FileStatus::modified: {
                        string op_key = "PATCH_" + node.toString();
                        if (!pending_operation.exist(op_key)) {
                            std::cout << "================= FW { MODIFIED }: " << node.toString() << '\n';
                        } else {
                            cout << "================= FW { THROTTLE MODIFIED }" << endl;
                        }
                    }

                        break;
                    case FileStatus::erased: {

                        string op_key = "DELETE_" + node.toString() ;
                        if (!pending_operation.exist(op_key)) { //se non c'e' gia' una richiesta uguale in coda
                            std::cout << "================= FW { ERASED }: " << node.toString() << '\n';
                            pending_operation.set(op_key, true);
                            jobs.put(Request(Method::DELETE, node));
                        } else {
                            cout << "================= FW { THROTTLE ERASED }" << endl;
                        }
                    }
                        break;
                    case FileStatus::missing: {

                        string op_key = "DELETE_" + node.toString();
                        if (!pending_operation.exist(op_key)) { //se non c'e' gia' una richiesta uguale in coda
                            std::cout << "================= FW { MISSING }: " << node.toString() << '\n';
                            // TODO ADD GET HERE
                            pending_operation.set(op_key, true);
                            jobs.put(Request(Method::DELETE, node));
                        } else {
                            cout << "================= FW { THROTTLE MISSING}"<<endl;
                        }
                        break;
                    }
                    case FileStatus::untracked: {

                        string op_key = "PUT_" + node.toString();
                        if (!pending_operation.exist(op_key)) { //se non c'e' gia' una richiesta uguale in coda
                            std::cout << "================= FW { UNTRACKED} : " << node.toString() << '\n';
                            pending_operation.set(op_key, true);
                            jobs.put(Request(Method::PUT, node));
                        } else {
                            cout << "================= FW { THROTTLE UNTRACKED }"<<endl;
                        }
                    }
                        break;
                    default:
                        std::cout << "Error! Unknown file status.\n";
                }
        });
    });

    // reload snapshot every snapshot_delay
    while (true) {
        if (!pending_operation.exist("SNAPSHOT")) {
            Node n;
            jobs.put(Request(Method::SNAPSHOT, n));
            pending_operation.set("SNAPSHOT",true);
            this_thread::sleep_for(snapshot_delay);
        }


    }

    io_thread.join();
    fw_thread.join();
    return 0;
}
