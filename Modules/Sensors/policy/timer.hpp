
#pragma once
#include <cstdint>

// TODO

template<const size_t NumberTicks>
struct TimerPollPolicy {
public:
    size_t number_ticks = NumberTicks;

    inline bool shouldPoll () {
        return true;
    }
    
    inline void registerPollStart () {}
    inline void registerPollEnd () {}
};
