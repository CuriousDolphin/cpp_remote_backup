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
#ifndef SHARED_BOX_H
#define SHARED_BOX_H

#endif

using namespace std;
template <class T>
class shared_box
{
    mutex m;
    T _obj;

public:
    // setta un oggetto
    void set(T obj)
    {
        unique_lock<mutex> lg(m);
        _obj = obj;
    };
    // get dell'oggetto
    T get()
    {
        unique_lock<mutex> lg(m);
        return _obj;
    };
};
