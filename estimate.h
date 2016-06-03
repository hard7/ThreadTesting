#ifndef THREADTESTING_ESTIMATE_H
#define THREADTESTING_ESTIMATE_H

#include <iostream>
#include <chrono>

class estimate {
public:
    estimate(std::string const& description_) : description(description_), begin(clock_::now()) {}
    ~estimate() {
        std::cout << description << " " <<  std::chrono::duration_cast<second_> (clock_::now() - begin).count() << std::endl;
    }

private:
    typedef std::chrono::high_resolution_clock clock_;
    typedef std::chrono::duration<double, std::ratio<1> > second_;
    std::chrono::time_point<clock_> begin;
    std::string description;
};


#endif //THREADTESTING_ESTIMATE_H
