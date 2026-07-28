
#pragma once

struct AlwaysPollPolicy {
public:
    inline bool shouldPoll () {
        return true;
    }

    inline void registerPollStart () {}
    inline void registerPollEnd () {}
};
