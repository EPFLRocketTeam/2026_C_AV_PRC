
#include <meta>
#include <type_traits>

#include <iostream>
#include "./header.hpp"
#include "./value.hpp"
#include "./annotations.hpp"

struct CsvChannel {};

enum E { A, B };
struct SampleType {
    int a;
    
    CSV_RENAME("state")
    E b;

    CSV_IGNORE
    int c;

    std::array<bool, 4> e;
};

int main (void) {
    std::cout << csv::header<SampleType>{} << std::endl;
    std::cout << csv::value<SampleType>{ {42, (E) 3, 3, { 0, 1, 0, 0 }} } << std::endl;
}
