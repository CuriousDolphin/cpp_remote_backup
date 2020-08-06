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

    // Time interval at which we check the base folder for changes
    std::chrono::duration<int, std::milli> delay;

    // Keep a record of files from the base directory and their last modification time
    FileWatcher(std::string path_to_watch, std::chrono::duration<int, std::milli> delay) : path_to_watch{path_to_watch}, delay{delay}
    {
        for (auto &file : std::filesystem::recursive_directory_iterator(path_to_watch))
        {
            //TODO CHECK SE E' UNA DIR
            uintmax_t size = std::filesystem::file_size(file);
            std::string hash = getMD5(file.path().string());
            auto time = std::filesystem::last_write_time(file);
            Node tmp(file.path().string(), false, hash, size, to_timestamp(time));
            std::cout << tmp.toString() << std::endl;
            paths_.insert({file.path().string(), tmp});
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
                    action(it->first, FileStatus::erased); // it->first chiave
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
                std::filesystem::file_time_type time = std::filesystem::last_write_time(file);
                long int current_file_last_write_time = to_timestamp(time);
                // CREATION
                if (!contains(file.path().string()))
                {

                    uintmax_t size = std::filesystem::file_size(file);
                    std::string hash = getMD5(file.path().string());
                    auto time = std::filesystem::last_write_time(file);
                    Node tmp(file.path().string(), false, hash, size, current_file_last_write_time);
                    std::cout << tmp.toString() << std::endl;
                    paths_.insert({file.path().string(), tmp});

                    // paths_[file.path().string()] = current_file_last_write_time;
                    action(file.path().string(), FileStatus::created);
                }

                // File modification
                else
                {
                    if (paths_[file.path().string()].getLastWriteTime() != current_file_last_write_time)
                    {
                        std::string old_hash = paths_[file.path().string()].getLastHash();
                        std::string new_hash = getMD5(file.path().string());

                        if (old_hash != new_hash)
                        { // se cambia l'hash la modifica e' veritiera
                            paths_[file.path().string()].setLastWriteTime(current_file_last_write_time);
                            paths_[file.path().string()].setLastHash(new_hash);
                            std::cout << "UPDATE FROM: " << old_hash << " -> " << new_hash << std::endl;
                            action(file.path().string(), FileStatus::modified);
                        }
                    }
                }
            }
        }
    }

private:
    std::unordered_map<std::string, Node> paths_;
    bool running_ = true;

    // Check if "paths_" contains a given key
    // If your compiler supports C++20 use paths_.contains(key) instead of this function
    bool contains(const std::string &key)
    {
        auto el = paths_.find(key);
        return el != paths_.end();
    }
};