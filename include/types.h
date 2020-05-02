#ifndef __TYPES_H__
#define __TYPES_H__

#include <cstdint>
#include <openssl/sha.h>

#include "mychrono.h"

//谨防对齐错误

//All messages sent from client begin with a transaction id
//All reply from server begin with the transaction id it is replying.
typedef uint32_t transactionid_t;

typedef uint32_t C2SBaseType;
enum class C2S : C2SBaseType {
    //request
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
enum class S2C : uint32_t {
    //OK
    LOGIN_OK,
    
    //fail
    LOGIN_FIRST,
    ALREADY_LOGINED,
    ALREADY_LOGOUTED,
    USERNAME_TOO_LONG,
    PHONE_TOO_LONG,
    WRONG_PASSWORD,
    MSG_TOO_LONG,

    //response
    SIGNUP_RESP,
    MSG_RESP,
    ONLINE_FRIENDS,
    MSG_GROUP_REPONSE,
    ONLINE_GROUP_MEMBER,
    P2PCONN_RESP,  //The address of the peer

    //push
    SOMEONE_LOGIN,
    SOMEONE_LOGOUT,
    ADD_FRIEND_REQ,
    ADD_FRIEND_REPLY, //agree or not
    MSG,
    MSG_GROUP,
    P2PCONN
};

struct C2SHeader {
    transactionid_t tsid;   //4
    enum C2S type;          //4
};
struct S2CHeader {
    //0 means push
    transactionid_t tsid;   //4
    enum S2C type;          //4
};

//chat user id
typedef uint64_t userid_t;
//chat group id
typedef uint64_t groupid_t;
//message id
typedef uint64_t msgid_t;

#define MAX_USERNAME_LEN 100
#define MAX_PHONE_LEN 28
//uint8_t namelen;                            //1
//char phone[15];                             //15
struct SignupHeader {
    uint16_t namelen;                           //2
    uint16_t phonelen;                          //2
    uint32_t signaturelen;                      //4
    uint8_t pwsha256[SHA256_DIGEST_LENGTH];        //32
};
struct SignupReply {
    userid_t id;                                //8
    std::chrono::days::rep day;                 //4
    char __padding[sizeof(id) - sizeof(day)];   //4
};
struct LoginInfo {
    userid_t userid;                            //8
    uint8_t pwsha256[SHA256_DIGEST_LENGTH];     //32
};

constexpr size_t MAX_CONTENT_LEN = 2000;
struct MsgC2SHeader {
    userid_t to;                                //8
    uint64_t len; //length of content           //8
};
struct MsgS2CReply {
    msgid_t msgid;                          //8
    std::chrono::milliseconds::rep time;    //8
};
static_assert(sizeof(MsgS2CReply::time) == 8);
struct MsgS2CHeader : MsgS2CReply/*, MsgC2SHeader*/ {
    //Overlap with MsgC2SHeader
    userid_t from;                          //8
    uint64_t len; //length of content       //8
};

struct MsgGroupHeader {
    userid_t from;      //8
    groupid_t to;       //8
};

struct GroupInfoHeader {
    uint32_t namelen;               //4
    //Establishing time
    std::chrono::days::rep time;    //4
};

#endif	//__TYPES_H__
