#include <iostream>
#include "file_watcher.h"
#include <boost/asio.hpp>
#include "client.h"
int main()
{
    boost::asio::io_context io_context;

    boost::asio::ip::tcp::resolver resolver(io_context);
    auto endpoints = resolver.resolve("localhost", "5555");

    client c(io_context, endpoints,"ivan","mimmo");

     Node n("mingw.exe",false,"9670c3701f0b546ca63a3e6d7749e59e",960504,1600783199);


    // Create a FileWatcher instance that will check the current folder for changes every 5 seconds
    FileWatcher fw{"../my_sync_folder", std::chrono::milliseconds(5000)};
    std::thread t([&io_context]() { io_context.run(); });

    // Start monitoring a folder for changes and (in case of changes)
    // run a user provided lambda function
    fw.start([&c](Node node, FileStatus status) -> void {
        // Process only regular files, all other file types are ignored
        //if(!std::filesystem::is_regular_file(std::filesystem::path(path_to_watch)) && status != FileStatus::erased) {
       //     return;
      //  }
        std::cout << "-------------------------------------" << std::endl;
        switch (status)
        {
            case FileStatus::created:
                std::cout << "CREATED: \n"
                          << node.toString() << '\n';
                c.do_put_sync(node );

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

    t.join();
    return 0;
}
