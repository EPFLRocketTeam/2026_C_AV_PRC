
extern "C" {
    #include "Application/main.h"
	#include <stdio.h>
};


#include <iostream>
#include <cstdint>

struct CallRecord {
    const char* message;
    const char* file;
    int line;
};

extern const CallRecord __start_calls;
extern const CallRecord __stop_calls;

#define _InternalStringToIndex(data) ([]{ \
        static const char msg[] = data; \
        static const CallRecord record \
        __attribute__((section("calls"), used, aligned(1))) \
        = { msg, __FILE__, __LINE__ }; \
        return (int) ((&record) - (&__start_calls)); \
    })()

#define StringToIndex(data) ([] { \
        static uint64_t val = _InternalStringToIndex(data); \
        printf("String \"%s\" has index %lld\n", data, val); \
        return val; \
    })();

volatile int i;
void UnreachableFunction () {
    int i1 = StringToIndex("Never, World !");
    i = i1;
}
void ShowAllCalls() {
	printf("\n");
    printf("--- Statically Found All Calls ---\n");
    int index = 0;
    for (const CallRecord* it = &__start_calls; it < &__stop_calls; ++it) {
    	printf("INDEX %d: %s at %s:%d\n", index, it->message, it->file, it->line);

        index ++;
    }
    printf("\n");
}

void _main () {
    // Some stuff that did happen
    int i1 = StringToIndex("Hello, World !");
    i = i1;

    ShowAllCalls();

    // Some stuff that happens after
    int i2 = StringToIndex("Bye, World !");
    i = i2;
}

void mainLoop () {
    _main();
}
