#ifndef CPPBASE_H_
#define CPPBASE_H_

#include <iostream>

//TODO: Support std::endl
class _emptycout : std::ostream {
public:
	template <typename T>
    _emptycout& operator << (T val) {(void)val; return *this;}
};

extern _emptycout emptycout;

#ifndef dbgcout
#ifdef DEBUG
#define dbgcout std::cout
#else
#define dbgcout emptycout
#endif //DEBUG
#endif //dbgcout

#endif //CPPBASE_H_
