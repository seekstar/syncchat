#include "cppbase.h"

_emptycout emptycout;

void DbgPrintHex(void *data, size_t len) {
#if DEBUG
    uint8_t *d = (uint8_t *)data;
    for (size_t i = 0; i < len; ++i) {
        printf("%02x", d[i]);
    }
#else
    (void)data;
    (void)len;
#endif
}
