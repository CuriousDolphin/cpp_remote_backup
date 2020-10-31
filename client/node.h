#pragma once
#ifndef NODE_H
#define NODE_H
#include <filesystem>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <string>
#include <functional>
#include <boost/algorithm/string.hpp>
#include <future>
#include <vector>
#include "../shared/const.h"

using namespace std;
class Node
{
    std::string path;
    bool isDir;
    std::vector<std::string> hash_histories;
    uintmax_t size;
    long int last_write_time;

public:
    Node(){};
    Node(std::string path, bool isDir, std::string hash);
    Node(std::string path, bool isDir, std::string hash, uintmax_t size, long int last_time);
    uintmax_t getSize() { return size; }
    void setSize(uintmax_t s) { size = s; }
    bool is_dir() { return isDir; }
    std::string toString();
    std::string getPath() { return path; }
    std::string getAbsolutePath() ;
    std::string getName();
    std::string toPathSizeTimeHash();
    std::string getHistory();
    std::string getFirstHash();
    std::string getLastHash();
    void setLastHash(std::string hash);
    void setLastWriteTime(long int time);
    long int getLastWriteTime();
};

#endif