#pragma once

#include <filesystem>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <string>
#include <functional>
#include "node.h"
#include "hasher.h"

// Define available file changes
enum class FileStatus
{
    created,
    modified,
    erased
};

class FileWatcher
{
public:
    std::string path_to_watch;

    /* std::uint32_t checksum;
    std::string checksumString;

    md5 hash;
    md5::digest_type digest; */

    // Time interval at which we check the base folder for changes
    std::chrono::duration<int, std::milli> delay;

    // Keep a record of files from the base directory and their last modification time
    FileWatcher(std::string path_to_watch, std::chrono::duration<int, std::milli> delay) : path_to_watch{path_to_watch}, delay{delay}
    {
        for (auto &file : std::filesystem::recursive_directory_iterator(path_to_watch))
        {
            paths_[file.path().string()] = std::filesystem::last_write_time(file);
            /*checksum = crc32(file.path().string());
            checksumString = std::to_string(checksum);
            std::cout <<  file.path().string() << " " << checksum << " " << sizeof(checksum) << std::endl;*/
            
            

            //std::cout << "md5(" << s << ") = " << toString(digest) << '\n';
        }
    }

    // Monitor "path_to_watch" for changes and in case of a change execute the user supplied "action" function
    void start(const std::function<void(std::string, FileStatus)> &action)
    {
        std::cout << "START MONITORING" << std::endl;
        while (running_)
        {
            // Wait for "delay" milliseconds
            std::this_thread::sleep_for(delay);

            auto it = paths_.begin();
            while (it != paths_.end())
            {
                if (!std::filesystem::exists(it->first))
                {
                    action(it->first, FileStatus::erased);
                    it = paths_.erase(it);
                }
                else
                {
                    it++;
                }
            }

            // Check if a file was created or modified
            for (auto &file : std::filesystem::recursive_directory_iterator(path_to_watch))
            {
                std::filesystem::file_time_type current_file_last_write_time = std::filesystem::last_write_time(file);

                //std::cout << "  - " << file.path().string() << "  " << std::endl;
                // File creation
                if (!contains(file.path().string()))
                {
                    paths_[file.path().string()] = current_file_last_write_time;
                    action(file.path().string(), FileStatus::created);
                    // File modification
                }
                else
                {
                    if (paths_[file.path().string()] != current_file_last_write_time)
                    {
                        paths_[file.path().string()] = current_file_last_write_time;
                        action(file.path().string(), FileStatus::modified);
                    }
                }
            }
        }
    }

private:
    std::unordered_map<std::string, std::filesystem::file_time_type> paths_;
    bool running_ = true;

    // Check if "paths_" contains a given key
    // If your compiler supports C++20 use paths_.contains(key) instead of this function
    bool contains(const std::string &key)
    {
        auto el = paths_.find(key);
        return el != paths_.end();
    }
};