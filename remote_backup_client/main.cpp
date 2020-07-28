#include <iostream>
#include "file_watcher.h"
int main()
{
    // Create a FileWatcher instance that will check the current folder for changes every 5 seconds
    FileWatcher fw{"./", std::chrono::milliseconds(5000)};

    // Start monitoring a folder for changes and (in case of changes)
    // run a user provided lambda function
    fw.start([](std::string path_to_watch, FileStatus status) -> void {
        // Process only regular files, all other file types are ignored
        /*if(!std::filesystem::is_regular_file(std::filesystem::path(path_to_watch)) && status != FileStatus::erased) {
            return;
        }*/

        switch (status)
        {
        case FileStatus::created:
            std::cout << "File created: " << path_to_watch << '\n';
            break;
        case FileStatus::modified:
            std::cout << "File modified: " << path_to_watch << '\n';
            break;
        case FileStatus::erased:
            std::cout << "File erased: " << path_to_watch << '\n';
            break;
        default:
            std::cout << "Error! Unknown file status.\n";
        }
    });
    return 0;
}
