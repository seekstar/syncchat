#ifndef BASE_H_
#define BASE_H_

#ifndef DBG
#ifdef DEBUG
#define DBG(fmt, ...) printf("DEBUG: " fmt "\n", ##__VA_ARGS__)
#else
#define DBG(fmt, ...)
#endif //DEBUG
#endif //DBG

#ifndef unlikely
#define unlikely(x)    (__builtin_expect(!!(x), 0))
#endif //unlikely
#ifndef likely
#define likely(x)    (__builtin_expect(!!(x), 1))
#endif //likely

#endif //BASE_H_