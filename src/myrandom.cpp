#include <assert.h>
#include <random>

std::random_device rd;
typedef std::mt19937 RandEngine;
//RandEngine e(std::random_device()());
RandEngine e(rd());
//Generate long random binary result
void genrand(void *dest, size_t n) {
    typedef uint32_t res_t;
    assert(n >= sizeof(res_t) &&
        "This function is designed for generating long random binary result, please use other function");
    assert(n % sizeof(res_t) == 0);
    
    res_t *i = reinterpret_cast<res_t *>(dest);
    res_t *j = reinterpret_cast<res_t *>((uint8_t*)dest + n);
    for (; i < j; ++i) {
        *i = e();
    }
}
