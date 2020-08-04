#pragma once
#include <filesystem>
#include <chrono>
#include <thread>
#include <unordered_map>
#include <string>
#include <functional>

class Node
{
    std::string path;
    bool isDir;
    std::vector<std::string> hash_histories;
    uintmax_t size;

public:
    Node(std::string path, bool isDir, std::string hash, uintmax_t size) : path(path), isDir(isDir), size(size)
    {
        hash_histories.push_back(hash);

    }

    std::string toString()
    {
        std::ostringstream buffer;
        buffer << path << " HASH (head): " << getLastHash()
               << " size: (bytes) " << size << std::endl;
        return buffer.str();
    }

    std::string getLastHash()
    {

        return hash_histories.back();
    }
};