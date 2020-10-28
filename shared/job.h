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
#include <deque>
#include <optional>
#include <condition_variable>
#ifndef LAB5_JOB_H
#define LAB5_JOB_H

#endif

using namespace std;
int MAX_QUEUE_LEN = 50;
template <class T>
class Jobs
{
    mutex m;
    condition_variable cv; // conditional get

    deque<T> coda;
    std::atomic<bool> completed = false;

public:
    // inserisce un job in coda in attesa di essere processato, può
    // essere bloccante se la coda dei job è piena
    void put(T &&job)
    {
        cout << "[JOB PUT]" << endl;
        unique_lock<mutex> lg(m);
        coda.push_back(move(job));
        cv.notify_all(); // notifica i consumers
    };

    // legge un job dalla coda e lo rimuove
    // si blocca se non ci sono valori in coda
    optional<T> get()
    {
        unique_lock<mutex> lg(m);
        cv.wait(lg, [this]() { return !coda.empty() || completed.load(); }); // true sblocca la wait
        if (completed.load() && coda.empty())
        {
            return nullopt;
        }
        T data = move(coda.front());
        coda.pop_front();
        cout << "[JOBS GET]" << endl;
        return move(data);
    };
    bool has(T &&obj)
    {
        if (find(coda.begin(), coda.end(), obj) != coda.end())
            return true;
        return false;
    }
    bool is_completed()
    {
        return completed.load();
    }

    bool is_empty()
    {
        return coda.empty();
    }

    // per notificare la chiusura
    void complete()
    {
        completed.store(true);
        cv.notify_all();
        // std::cout << this_thread::get_id() << "COMPLETED " << std::endl;
    }
};
