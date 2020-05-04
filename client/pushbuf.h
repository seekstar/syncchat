#ifndef PUSHBUF_H
#define PUSHBUF_H

#include <stdint.h>
#include <vector>

void PushBuf(std::vector<uint8_t>& buf, const void *x, std::size_t n);

#endif // PUSHBUF_H
