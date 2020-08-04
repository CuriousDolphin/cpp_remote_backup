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
    std::vector<uint32_t> hash;

public:
    Node(std::string path, bool isDir, uint32_t hash) : path(path), isDir(isDir), hash(hash){};
};