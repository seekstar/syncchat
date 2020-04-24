#ifndef __SRVTYPES_H__
#define __SRVTYPES_H__

#include <openssl/sha.h>

struct pwinfo {
    uint8_t salt[SHA256_DIGEST_LENGTH];
    uint8_t pw_sha256_salt_sha256[SHA256_DIGEST_LENGTH];    //32
};

#endif  //__SRVTYPES_H__