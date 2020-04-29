#ifndef __SRVTYPES_H__
#define __SRVTYPES_H__

#include <random>
#include <openssl/sha.h>

//typedef uint8_t salttype[SHA256_DIGEST_LENGTH];
/*struct salttype {
    uint8_t byte[SHA256_DIGEST_LENGTH];
};*/
struct pwinfo {
    salttype salt;
    uint8_t pw_sha256_salt_sha256[SHA256_DIGEST_LENGTH];    //32
};
//extern std::independent_bits_engine<std::default_random_engine, sizeof(salttype), salttype> gensalt;

#endif  //__SRVTYPES_H__