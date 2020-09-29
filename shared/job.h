//
// Created by isnob on 15/06/2020.
//
#include <iostream>

#include <mutex>
#include <map>
#include <vector>
#include <future>
#include <fstream>
#include <queue>
#include <optional>
#include <condition_variable>
#ifndef LAB5_JOB_H
#define LAB5_JOB_H

#endif

using namespace std;
int MAX_QUEUE_LEN = 20;
template <class T>
class Jobs
{
    mutex m;
    condition_variable cv; // conditional get

    queue<T> coda;
    std::atomic<bool> completed = false;

public:
    // inserisce un job in coda in attesa di essere processato, può
    // essere bloccante se la coda dei job è piena
    void put(T &&job)
    {
        unique_lock<mutex> lg(m);
        coda.push(move(job));
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
        coda.pop();
        return move(data);
    };

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
