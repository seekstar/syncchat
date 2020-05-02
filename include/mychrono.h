#ifndef MYCHRONO_H_
#define MYCHRONO_H_

#include <chrono>

namespace std {
namespace chrono {
typedef duration<int32_t, ratio<3600*24, 1> > days;
}
}

#endif //MYCHRONO_H_