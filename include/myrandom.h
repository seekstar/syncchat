#include <random>

std::mt19937 e(std::random_device()());
template <typename T>
void genRandBin(T *dest) {
    static_assert(sizeof(T) >= sizeof(e::result_type), 
        "This function is designed for generating long random binary result, please use other function");
    
    std::random_device::result_type *i = dest, *j = dest + 1;
    for (; i < j; ++i) {
        *i = e();
    }
}