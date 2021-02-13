#pragma once
#include <string>
#include <chrono>
#include "color.h"

class DurationLogger
{
    std::string name;
    std::chrono::high_resolution_clock::time_point start;
    std::chrono::high_resolution_clock::time_point end;

public:


    DurationLogger(const std::string &name) : name(name)
    {

        start = std::chrono::high_resolution_clock::now();
        std::cout << YELLOW << "[START " << name << "]" << RESET << std::endl;
    }

    ~DurationLogger()
    {
        end = std::chrono::high_resolution_clock::now();

        std::chrono::duration time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << YELLOW << "[END " << name << " : duration " << time.count() << " ms]" << RESET << std::endl;
    }
};
