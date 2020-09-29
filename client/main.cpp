#include <iostream>
#include "file_watcher.h"
#include <boost/asio.hpp>
#include "client.h"
#include "../shared/job.h"
#include "../shared/duration_logger.h"
int main() {
    boost::asio::io_context io_context;

    boost::asio::ip::tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve("localhost", "5555");

    client client(io_context, endpoints, "ivan", "mimmo");

    Node n("mingw.exe", false, "9670c3701f0b546ca63a3e6d7749e59e", 960504, 1600783199);
    Jobs<std::tuple<FileStatus, Node>> jobs;

    // Create a FileWatcher instance that will check the current folder for changes every 5 seconds
    FileWatcher fw{"../my_sync_folder", std::chrono::milliseconds(5000)};
    std::thread t([&io_context, &jobs, &client]() {
        cout << "IO_THREAD: " << this_thread::get_id() << endl;
        io_context.run();
        FileStatus status;
        Node n;
        while (true) {
            // get username
            optional<tuple<FileStatus, Node>> action = jobs.get();
            if (!action.has_value()) {
                break;
            }
            status = get<0>(action.value());
            n = get<1>(action.value());
            std::cout << "-------------------------------------" << std::endl;
            switch (status) {
                case FileStatus::created:
                    std::cout << "CREATED: \n"
                              << n.toString() << '\n';
                    {
                        DurationLogger dl("put file");
                        client.do_put_sync(n);
                    }


                    break;
                case FileStatus::modified:
                    std::cout << "MODIFIED: " << n.toString() << '\n';
                    break;
                case FileStatus::erased:
                    std::cout << "ERASED: " << n.toString() << '\n';
                    break;
                default:
                    std::cout << "Error! Unknown file status.\n";
            }
            std::cout << "-------------------------------------" << std::endl;


        }
    });

    // Start monitoring a folder for changes and (in case of changes)
    // run a user provided lambda function
    fw.start([&jobs](Node node, FileStatus status) -> void {
        // Process only regular files, all other file types are ignored
        //if(!std::filesystem::is_regular_file(std::filesystem::path(path_to_watch)) && status != FileStatus::erased) {
        //     return;
        //  }
        jobs.put(std::make_tuple(status, node));

    });

    t.join();
    return 0;
}
