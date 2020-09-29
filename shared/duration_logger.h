
#include <string>
#include <chrono>

class DurationLogger
{
    std::string name;
    std::chrono::high_resolution_clock::time_point start;
    std::chrono::high_resolution_clock::time_point end;

public:


    DurationLogger(const std::string &name) : name(name)
    {

        start = std::chrono::high_resolution_clock::now();
        std::cout << " >>>>>>>>>>>>>> start [" << name << "]" << std::endl;
    }

    ~DurationLogger()
    {
        end = std::chrono::high_resolution_clock::now();

        std::chrono::duration time = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        std::cout << "<<<<<<<<<<<<<<< end [" << name << " : duration " << time.count() << " ms] " << std::endl;
    }
};
