#pragma once
#include <filesystem>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <string>
#include <functional>
#include "to_time_t.h"
class Node
{
    std::string path;
    bool isDir;
    std::vector<std::string> hash_histories;
    uintmax_t size;
    long int last_write_time;

public:
    Node() {}
    /*   Node(const Node &source) : path(source.path), isDir(source.isDir), size(source.size)
    {
        hash_histories.clear();
        for (auto const &value : source.hash_histories)
        {
            hash_histories.push_back(value);
        }
    } */

    Node(std::string path, bool isDir, std::string hash, uintmax_t size, long int last_time) : path(path), isDir(isDir), size(size), last_write_time(last_time)
    {
        hash_histories.push_back(hash);
    }

    std::string toString()

    {

        std::ostringstream buffer;
        buffer << path << "\t\n(head): " << getLastHash() << "\t\n(size): " << size << "\t\n(time): " << last_write_time << std::endl;
        return buffer.str();
    }

    std::string getLastHash()
    {
        return hash_histories.back();
    }
    void setLastHash(std::string hash)
    {
        hash_histories.push_back(hash);
    }
    void setLastWriteTime(long int time)
    {
        last_write_time = time;
    }
    long int getLastWriteTime()
    {
        return last_write_time;
    }
};
