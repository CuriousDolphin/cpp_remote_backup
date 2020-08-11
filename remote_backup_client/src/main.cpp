#include <iostream>
#include "file_watcher.h"
#include <boost/asio.hpp>
#include "client.h"
int main()
{
    boost::asio::io_context io_context;

    boost::asio::ip::tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve("127.0.0.1", "4444");
    client c(io_context, endpoints);
    std::thread t([&io_context]() { io_context.run(); });

    std::cout << "STARTING CLIENT v4" << std::endl;
    // Create a FileWatcher instance that will check the current folder for changes every 5 seconds
    FileWatcher fw{"../my_sync_folder", std::chrono::milliseconds(5000)};

    // Start monitoring a folder for changes and (in case of changes)
    // run a user provided lambda function
    fw.start([](Node node, FileStatus status) -> void {
        // Process only regular files, all other file types are ignored
        /*if(!std::filesystem::is_regular_file(std::filesystem::path(path_to_watch)) && status != FileStatus::erased) {
            return;
        }*/
        std::cout << "-------------------------------------" << std::endl;
        switch (status)
        {
        case FileStatus::created:
            std::cout << "CREATED: \n"
                      << node.toString() << '\n';
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
    return 0;
}
