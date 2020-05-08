#ifndef __TYPES_H__
#define __TYPES_H__

#include <cstdint>
#include <vector>
#include <openssl/sha.h>

#include "mychrono.h"

//谨防对齐错误

//All messages sent from client begin with a transaction id
//All reply from server begin with the transaction id it is replying.
typedef uint32_t transactionid_t;

typedef uint32_t C2SBaseType;
enum class C2S : C2SBaseType {
    IMALIVE,
    //request
    SIGNUP,
    LOGIN,
    LOGOUT,
    USER_PUBLIC_INFO_REQ,   //username
    USER_PRIVATE_INFO_REQ,  //username, phone
    STATISTICS,
    FIND_BY_USERNAME,
    ADD_FRIEND_REQ,
    ADD_FRIEND_REPLY,
    DELETE_FRIEND,
    ALL_FRIENDS,
    ONLINE_FRIENDS,
    MSG,
    MSG_REQ,
    CREATE_GROUP,
    JOIN_GROUP,
    LEAVE_GROUP,
    CHANGE_GROUP_OWNER,
    ALL_GROUPS,
    ALL_GROUP_MEMBER,
    ONLINE_GROUP_MEMBER,
    GRP_INFO,
    GRPMSG,
    GRPMSG_REQ,
    MOMENT,
    MOMENTS_REQ,
    COMMENT,
    COMMENTS_REQ,
    P2PCONN
};
typedef uint32_t S2CBaseType;
enum class S2C : S2CBaseType {
    //OK
    LOGIN_OK,
    ADD_FRIEND_SENT,
    //DELETE_FRIEND_OK,

    //fail
    LOGIN_FIRST,
    ALREADY_LOGINED,
    ALREADY_LOGOUTED,
    USERNAME_TOO_LONG,
    PHONE_TOO_LONG,
    NO_SUCH_USER,
    WRONG_PASSWORD,
    ALREADY_FRIENDS,
    MSG_TOO_LONG,
    GROUPNAME_TOO_LONG, //TODO
    ALREADY_GROUP_MEMBER, //TODO

    //response
    SIGNUP_RESP,
    USER_PRIVATE_INFO,
    USER_PUBLIC_INFO,
    FIND_BY_USERNAME_REPLY,
    //ONLINE_FRIENDS,
    MSG_RESP,
    CREATE_GROUP_RESP,
    GRP_INFO,
    //ONLINE_GROUP_MEMBER,
    GRPMSG_RESP,
    P2PCONN_RESP,  //The address of the peer

    //push
    INFO,
    SOMEONE_LOGIN,
    SOMEONE_LOGOUT,
    ADD_FRIEND_REQ,
    ADD_FRIEND_REPLY, //agree or not
    DELETE_FRIEND,
    FRIENDS,    //There may be many friends, so send them in batches.
    MSG,
    JOIN_GROUP_OK,
    GRPMSG,
    MOMENTS,
    COMMENTS,
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
typedef uint64_t grpid_t;
//message id
typedef uint64_t msgid_t;
//group message id
typedef uint64_t grpmsgid_t;
typedef uint64_t momentid_t;
typedef uint64_t commentid_t;

typedef std::chrono::days::rep daystamp_t;          //4
typedef std::chrono::milliseconds::rep msgtime_t;   //8

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
    daystamp_t day;                 //4
    char __padding[sizeof(id) - sizeof(day)];   //4
};
struct LoginInfo {
    userid_t userid;                            //8
    uint8_t pwsha256[SHA256_DIGEST_LENGTH];     //32
};

struct UserPrivateInfoHeader {
    uint32_t nameLen;
    uint32_t phoneLen;
};
struct UserPublicInfoHeader {
    uint32_t nameLen;
    char __padding[4];
};
struct C2SAddFriendReq {
    userid_t to;
};
struct S2CAddFriendReqHeader {
    userid_t from;
    uint64_t nameLen;
    //uint32_t remarkLen;
};
struct C2SAddFriendReply {
    userid_t to;                        //8
    bool reply; //0 is no, 1 is yes     //1
    char __padding[8 - sizeof(reply)];  //7
};
struct S2CAddFriendReply {
    //Overlap with C2SAddFriendReply
    userid_t from;                      //8
    bool reply;
    char __padding[8 - sizeof(reply)];  //7
};

constexpr size_t MAX_CONTENT_LEN = 2000;
struct MsgC2SHeader {
    userid_t to;                                //8
    uint64_t len; //length of content           //8
};
struct MsgS2CReply {
    msgid_t msgid;                          //8
    msgtime_t time;    //8
};
static_assert(sizeof(MsgS2CReply::time) == 8);
struct MsgS2CHeader : MsgS2CReply/*, MsgC2SHeader*/ {
    //Overlap with MsgC2SHeader
    userid_t from;                          //8
    uint64_t len; //length of content       //8
};
// struct FriendInfoHeader {
//     userid_t userid;
//     uint32_t nameLen;
//     char __padding[4];
// };

#define MAX_GROUPNAME_LEN 100
struct CreateGroupReply {
    grpid_t grpid;      //8
    //Establishing time
    daystamp_t time;        //4
    char __padding[4];      //4
};
struct GroupInfoHeader {
    uint32_t namelen;               //4
    //Establishing time
    daystamp_t time;    //4
};
struct C2SGrpMsgHeader {
    grpid_t to;         //8
    uint64_t len;       //8
};
struct S2CGrpMsgReply {
    grpmsgid_t grpmsgid;        //8
    msgtime_t time;     //8
};
struct S2CMsgGrpHeader : S2CGrpMsgReply {
    userid_t from;      //8
    //overlap with C2SGrpMsgHeader
    grpid_t to;         //8
    uint64_t len;       //8
};

typedef std::vector<uint8_t> CppContent;
struct CppMoment {
    momentid_t id;
    msgtime_t time;
    userid_t sender;
    CppContent content;
    CppMoment(momentid_t _id, msgtime_t _time, userid_t _sender, CppContent _content)
        : id(_id), time(_time), sender(_sender), content(_content)
    {
    }
    bool operator < (const CppMoment& rhs) const {
        return time < rhs.time;
    }
};

struct MomentHeader {
    momentid_t id;
    msgtime_t time;
    userid_t sender;
    uint64_t len;
};

struct C2SCommentHeader {
    momentid_t to;
    commentid_t reply;
    uint64_t len;
};
struct CppComment {
    commentid_t id;
    msgtime_t time;
    userid_t sender;
    //momentid_t to;
    commentid_t reply;
    CppContent content;
    CppComment(commentid_t _id, msgtime_t _time, userid_t _sender, commentid_t _reply, CppContent _content)
        : id(_id), time(_time), sender(_sender), reply(_reply), content(_content)
    {}
    bool operator < (const CppComment& rhs) const {
        return time < rhs.time;
    }
};

struct CommentArrayHeader {
    uint64_t num;
    momentid_t to;
};

struct CommentHeader {
    commentid_t id;
    msgtime_t time;
    userid_t sender;
    uint64_t reply;
    uint64_t len;
};

#endif	//__TYPES_H__
