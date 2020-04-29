#ifndef __TYPES_H__
#define __TYPES_H__

#include <cstdint>
#include <chrono>

#include <openssl/sha.h>

typedef uint16_t C2SBaseType;
enum class C2S : C2SBaseType {
    SIGNUP,
    LOGIN,
    LOGOUT,
    ADD_FRIEND_REQ,
    ADD_FRIEND_REPLY,
    MSG,
    MSG_REQ,
    ONLINE_FRIENDS,
    ONLINE_GROUP_MEMBER,
    MSG_GROUP,
    MSG_GROUP_REQ,
    P2PCONN
};
enum class S2C : uint16_t {
    OK,
    FAIL,
    SOMEONE_LOGIN,
    SOMEONE_LOGOUT,
    ADD_FRIEND_REQ,
    ADD_FRIEND_REPLY,
    MSG,
    MSG_RPLY,
    ONLINE_FRIENDS,
    MSG_GROUP,
    MSG_GROUP_RPLY,
    ONLINE_GROUP_MEMBER,
    P2PCONN
};
enum class S2COK : uint16_t {
    SIGNUP,
    LOGIN,
    LOGOUT
};
enum class S2CFAIL : uint16_t {
    LOGIN_FIRST,
    ALREADY_LOGINED,
    ALREADY_LOGOUTED,
    MSG_TOO_LONG
};

//chat user id
typedef uint64_t userid_t;
//chat group id
typedef uint64_t groupid_t;
//message id
typedef uint64_t msgid_t;

const size_t MAX_CONTENT_LEN = 2000;
struct MsgSendHeader {
    userid_t to;
    msgid_t reply;
    size_t len; //length of content
};
struct MsgS2CReplyHeader {
    msgid_t msgid;
    std::chrono::milliseconds time;
};
struct MsgRecvHeader {
    msgid_t msgid;
    std::chrono::milliseconds time;
    userid_t from;
    msgid_t reply;
    size_t len; //length of content
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
