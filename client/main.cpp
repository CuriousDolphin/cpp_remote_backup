#include <iostream>
#include "file_watcher.h"
#include <boost/asio.hpp>
#include "client.h"
#include "../shared/duration_logger.h"
#include "../shared/shared_map.h"
#include "../shared/shared_box.h"

#include <boost/filesystem.hpp>

const std::string path_to_watch = "../my_sync_folder"; // c:/documenti/francesco/my_sync_folder
//const auto fw_delay = std::chrono::milliseconds(7000);
//const auto snapshot_delay = std::chrono::seconds(40);
mutex m; // for print
auto fw_delay = std::chrono::milliseconds(0);
auto snapshot_delay = std::chrono::seconds(0);

void print(const ostringstream &oss)
{
    unique_lock<mutex> lg(m);
    cout << oss.str() << endl;
}

int main(int argc, char *argv[])
{
    /*
     * parameters:
     *
     * [0] -> username (ivan)
     * [1] -> password (mimmo)
     * [2] -> host (localhost)
     * [3] -> port (5555)
     * [4] -> fw_delay
     * [5] -> snapshot_delay
     *
     */

    shared_map<Node> remote_snapshot;
    shared_map<bool> pending_operation;
    boost::asio::io_context io_context;
    boost::asio::ip::tcp::resolver resolver(io_context);

    std::string username, password, host, port;

    if (argc == 7)
    {
        username = argv[1];
        password = argv[2];
        host = argv[3];
        port = argv[4];
        fw_delay = std::chrono::milliseconds(std::atoi(argv[5]));
        std::cout << argv[5] << " " << argv[6] << std::endl;
        snapshot_delay = std::chrono::seconds(std::atoi(argv[6]));
    }
    else
    {
        username = "ivan";
        password = "mimmo";
        host = "localhost";
        port = "5555";
        fw_delay = std::chrono::milliseconds(7000);
        snapshot_delay = std::chrono::seconds(40);
    }

    //auto endpoints = resolver.resolve("localhost", "5555"); // host.docker.internal
    auto endpoints = resolver.resolve(host, port); // host.docker.internal
    Jobs<Request> jobs;
    //client client(io_context, endpoints, "ivan", "mimmo", &remote_snapshot, &pending_operation);
    client client(io_context, endpoints, username, password, &remote_snapshot, &pending_operation);

    std::thread io_thread([&io_context, &jobs, &client]() {
        ostringstream oss;
        oss << BLUE << "[IO_THREAD]: " << RESET << this_thread::get_id();

        print(oss);
        io_context.run();

        while (true)
        {
            optional<Request> req = jobs.get();
            if (!req.has_value())
            {
                break;
            }
            vector<string> params; // parsed header params
            vector<string> tmp;    // support
            std::string req_string;
            switch (req.value().method)
            {
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
                req_string = "Method::SNAPSHOT ";
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
        oss << BLUE << "[FW_THREAD]: " << RESET << this_thread::get_id() << endl;
        print(oss);
        // Create a FileWatcher instance that will check the current folder for changes every 5 seconds

        FileWatcher fw{&remote_snapshot, &pending_operation, path_to_watch, fw_delay};
        // Start monitoring a folder for changes and (in case of changes)
        // run a user provided lambda function
        fw.start([&jobs, &remote_snapshot, &pending_operation](Node node, FileStatus status) -> void {
            ostringstream oss;

            if (!node.is_dir() && node.getName() != "docker-compose.yml")
                switch (status)
                {
                case FileStatus::created:
                {
                    string op_key = node.getPath();
                    if (!pending_operation.exist(op_key))
                    { //se non c'e' gia' una richiesta uguale in coda
                        cout << GREEN << "[FW]" << BOLDYELLOW << "{CREATED} --> \n" << RESET << node.toString() << endl;
                        pending_operation.set(op_key, true);
                        jobs.put(Request(Method::PUT, node));
                    }
                    else
                    {
                        cout << GREEN << "[FW]" << BOLDYELLOW << "{THROTTLE CREATED}" << RESET << endl;
                    }
                }
                break;
                case FileStatus::modified:
                {
                    string op_key = node.getPath();
                    if (!pending_operation.exist(op_key))
                    {
                        std::cout << GREEN << "[FW]" << BOLDYELLOW << "{MODIFIED} --> " << RESET << node.toString() << '\n';
                        pending_operation.set(op_key, true);
                        jobs.put(Request(Method::PUT, node));
                    }
                    else
                    {
                        cout << GREEN << "[FW]" << BOLDYELLOW << "{THROTTLE MODIFIED}" << RESET << endl;
                    }
                }

                break;
                case FileStatus::erased:
                {

                    string op_key = node.getPath();
                    if (!pending_operation.exist(op_key))
                    { //se non c'e' gia' una richiesta uguale in coda
                        std::cout << GREEN << "[FW]" << BOLDYELLOW << "{ERASED} --> " << RESET << node.toString() << '\n';
                        pending_operation.set(op_key, true);
                        jobs.put(Request(Method::DELETE, node));
                    }
                    else
                    {
                        cout << GREEN << "[FW]" << BOLDYELLOW << "{THROTTLE ERASED}" << RESET << endl;
                    }
                }
                break;
                case FileStatus::missing:
                {
                    string op_key = node.getPath();
                    if (!pending_operation.exist(op_key))
                    {
                        std::cout << GREEN << "[FW]" << BOLDYELLOW << "{MISSING} --> " << RESET << node.toString() << '\n';
                        pending_operation.set(op_key, true);
                        jobs.put(Request(Method::GET, node));
                    }
                    else
                    {
                        cout << GREEN << "[FW]" << BOLDYELLOW << "{THROTTLE MISSING}" << RESET << endl;
                    }
                    break;
                }
                case FileStatus::untracked:
                {

                    string op_key = node.getPath();
                    if (!pending_operation.exist(op_key))
                    { //se non c'e' gia' una richiesta uguale in coda
                        std::cout << GREEN << "[FW]" << BOLDYELLOW << " {UNTRACKED} --> " << RESET << node.toString() << '\n';
                        pending_operation.set(op_key, true);
                        jobs.put(Request(Method::PUT, node));
                    }
                    else
                    {
                        cout << GREEN << "[FW]" << BOLDYELLOW << " {THROTTLE UNTRACKED}" << RESET << endl;
                    }
                }
                break;
                default:
                    std::cout << RED << "Error! Unknown file status.\n" << RESET << std::endl;
                }
        });
    });

    // reload snapshot every snapshot_delay
    while (true)
    {
        if (!pending_operation.exist("SNAPSHOT"))
        {
            Node n{"SNAPSHOT", false, "SNAAAAAAAAPSHOT"};

            jobs.put(Request(Method::SNAPSHOT, n));
            pending_operation.set(n.getPath(), true);
            this_thread::sleep_for(snapshot_delay);
        }
    }

    io_thread.join();
    fw_thread.join();
    return 0;
}
