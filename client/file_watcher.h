#pragma once
#ifndef FILEWATCHER_H
#define FILEWATCHER_H
#include <filesystem>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <string>
#include <functional>
#include "node.h"
#include "../shared/hasher.h"
#include "../shared/to_time_t.h"
#include "../shared/shared_map.h"
#include "../shared/const.h"
#include <boost/filesystem/fstream.hpp>


using namespace  std;
class FileWatcher {
public:
    std::string path_to_watch;

    shared_map<Node> * remote_snapshot;


    // Time interval at which we check the base folder for changes
    std::chrono::duration<int, std::milli> delay;

    // Keep a record of files from the base directory and their last modification time
    FileWatcher(shared_map<Node> * remote_snapshot,std::string path_to_watch, std::chrono::duration<int, std::milli> delay);

    // Monitor "path_to_watch" for changes and in case of a change execute the user supplied "action" function
    void start(const std::function<void(Node, FileStatus)> &action);

private:
    std::unordered_map<std::string, Node> _remote_snapshot;
    std::unordered_map<std::string, Node> paths_;
    bool running_ = true;

    Node createNode(const std::filesystem::directory_entry &file);

    // Check if "paths_" contains a given key
    // If your compiler supports C++20 use paths_.contains(key) instead of this function
    //bool contains(const std::string &key);

    bool local_snapshot_contains(const string &key);

    bool remote_snapshot_contains(const string &key);

    bool is_file_being_copied(const boost::filesystem::path &filePath);

    bool areTsEqual(long ts1, long ts2);
};

#endif