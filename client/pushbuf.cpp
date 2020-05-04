#include "pushbuf.h"

#include <string.h>

void PushBuf(std::vector<uint8_t>& buf, const void *x, size_t n) {
    size_t oldsz = buf.size();
    buf.resize(buf.size() + n);
    memcpy(buf.data() + oldsz, x, n);
}
