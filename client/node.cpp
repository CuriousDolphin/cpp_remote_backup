//
// Created by Francesco Riba on 20/10/2020.
//
#include "node.h"

Node::Node(std::string path, bool isDir, std::string hash, uintmax_t size, long int last_time) : path(path), isDir(isDir), size(size), last_write_time(last_time)
{
    hash_histories.push_back(hash);
}
Node::Node(std::string path, bool isDir, uintmax_t size, long int last_time) : path(path), isDir(isDir), size(size), last_write_time(last_time)
{
}

Node::Node(std::string path, bool isDir, std::string hash) : path(path), isDir(isDir)
{
    hash_histories.push_back(hash);
}

std::string Node::toString()
{
    std::ostringstream buffer;
    buffer << getAbsolutePath() << "@" << getLastHash(); //<< "\t\n(size): " << size << "\t\n(time): " << last_write_time;
    return buffer.str();
}

/**
 * //todo
 * Unused?
 * @return
 */
std::string Node::getName()
{
    vector<string> tmp;
    boost::split(tmp, path, boost::is_any_of("/"));
    return tmp[tmp.size() - 1];
}

std::string Node::toPathSizeTimeHash()
{
    std::ostringstream buffer;
    //buffer << path << " " << size << " " << last_write_time;
    buffer << getAbsolutePath() << PARAM_DELIMITER << size << PARAM_DELIMITER << last_write_time << PARAM_DELIMITER << getLastHash() << endl;
    return buffer.str();
}

std::string Node::getHistory()
{
    std::ostringstream buffer;
    for (int i = 0; i < hash_histories.size(); i++)
        buffer << "-> " << hash_histories[i] << " ";
    return buffer.str();
}

/**
 * //todo
 * Unused?
 * @return
 */
std::string Node::getFirstHash()
{
    return hash_histories.front();
}

std::string Node::getLastHash()
{
    return hash_histories.back();
}

void Node::setLastHash(std::string hash)
{
    hash_histories.push_back(hash);
}

void Node::setLastWriteTime(long int time)
{
    last_write_time = time;
}

long int Node::getLastWriteTime()
{
    return last_write_time;
}

std::string Node::getAbsolutePath()
{
    if (path.rfind("../", 0) == 0)
    {
        // s starts with prefix
        return path.substr(2, path.length());
    }
    return path;
}
