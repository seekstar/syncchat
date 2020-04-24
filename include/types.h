#ifndef __TYPES_H__
#define __TYPES_H__

#include <cstdint>
#include <openssl/sha.h>

enum class C2S : uint16_t {
    MSG,
    MSG_GROUP,
    P2PCONN,
    SIGNUP,
    LOGOUT,
    ONLINE_USER,
    MSG_REQ,
    MSG_GROUP_REQ
};
enum class S2C : uint16_t {
    MSG = static_cast<uint16_t>(C2S::MSG),
    MSG_GROUP = static_cast<uint16_t>(C2S::MSG_GROUP),
    P2PCONN = static_cast<uint16_t>(C2S::P2PCONN),
    FAIL,
    SIGNUP_OK,
    LOGIN_OK,
    ALREADY_LOGINED,
    SOMEONE_LOGIN,
    SOMEONE_LOGOUT,
    ONLINE_FRIENDS,
    LOGOUT_OK,
    ALREADY_LOGOUTED,
    MSG_RPLY,
    MSG_GROUP_RPLY
};

//chat user id
typedef uint64_t userid_t;
//chat group id
typedef uint64_t groupid_t;
#define CHATLEN (BODYLEN - 2 * sizeof(chatid_t) - sizeof(uint32_t))
struct MsgHeader {
    userid_t from;
    userid_t to;
};
struct MsgGroupHeader {
    userid_t from;
    groupid_t to;
};

#define NAMELEN 64
//basic information without userid
struct userbasic {
    char name[NAMELEN];
};
struct signupinfo {
    struct userbasic basic;
    char pwsha256[SHA256_DIGEST_LENGTH];
};
struct logininfo {
    userid_t userid;
    char pwsha256[SHA256_DIGEST_LENGTH];
};

struct groupinfo {
    char name[NAMELEN];
};

#endif	//__TYPES_H__
