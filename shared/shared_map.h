//
// Created by isnob on 15/06/2020.
//
#pragma once
#include <iostream>

#include <mutex>
#include <map>
#include <vector>
#include <future>
#include <fstream>
#include <queue>
#include <optional>
#include <condition_variable>
#ifndef SHARED__map_H
#define SHARED__map_H

#endif

using namespace std;
template <class T>
class shared_map
{
    mutex m;
    std::unordered_map<string, T> _map;

public:
    void set(string key, T &&value)
    {

        try
        {
            unique_lock<mutex> lg(m);

            _map.insert({key, move(value)});
            //cout << "[SHARED MAP SET] " << key << endl;
        }
        catch (std::exception &e)
        {
            std::cerr << "Exception: " << e.what() << "\n";
        }
    }

    T get(string key)
    {
        auto it = _map.find(key);

        if (it == _map.end())
            return NULL;
        else
        {
            return it->second;
        }
    }

    bool exist(string key)
    {
        auto it = _map.find(key);
        if (it == _map.end())
            return false;
        return true;
    }
    // setta la _mappa
    void set_map(std::unordered_map<string, T> new_map)
    {
        unique_lock<mutex> lg(m);

        try
        {
            _map = new_map;
        }
        catch (std::exception &e)
        {
            std::cerr << "Exception: " << e.what() << "\n";
        }
    };
    // get di tutta la _mappa
    std::unordered_map<string, T> get_map()
    {
        unique_lock<mutex> lg(m);
        return _map;
    };
};
