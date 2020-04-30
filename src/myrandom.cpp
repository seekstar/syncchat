#include <assert.h>
#include <random>

std::random_device rd;
typedef std::mt19937 RandEngine;
//RandEngine e(std::random_device()());
RandEngine e(rd());
//Generate long random binary result
void genrand(void *dest, size_t n) {
    assert(n >= sizeof(RandEngine::result_type) &&
        "This function is designed for generating long random binary result, please use other function");
    assert(n % sizeof(RandEngine::result_type) == 0);
    
    RandEngine::result_type *i = (RandEngine::result_type *)dest;
    RandEngine::result_type *j = (RandEngine::result_type *)((uint8_t*)dest + n);
    for (; i < j; ++i) {
        *i = e();
    }
}
