#include "sessionbase.h"

void session::HandleAddFriendReq(const boost::system::error_code& error) {
    dbgcout << __PRETTY_FUNCTION__ << '\n';
    HANDLE_ERROR;
    C2SAddFriendReq *c2sAddFriendReq = reinterpret_cast<C2SAddFriendReq *>(buf_ + sizeof(C2SHeader));
    //Check whether they are already friends
    if (odbc_exec(std::cerr, ("SELECT user2 FROM friends WHERE user1 = " + std::to_string(userid) +
        " and user2 = " + std::to_string(c2sAddFriendReq->to) + ';').c_str())
    ) {
        reset();
        return;
    }
    if (SQL_NO_DATA != SQLFetch(hstmt)) {
        if (odbc_close_cursor(std::cerr)) {
            reset();
            return;
        }
        dbgcout << "ALREADY_FRIENDS\n";
        SendType(S2C::ALREADY_FRIENDS);
        listen_request();
        return;
    }
    //TODO: write to db
    auto it = user_session.find(c2sAddFriendReq->to);
    if (user_session.end() == it) {
        dbgcout << __PRETTY_FUNCTION__ << ": user " << c2sAddFriendReq->to << " offline\n";
        listen_request();
        return;
    }
    SendType(S2C::ADD_FRIEND_SENT); //In fact send it later
    struct S2CHeader *s2cHeader = reinterpret_cast<struct S2CHeader *>(buf_);
    struct S2CAddFriendReqHeader *s2cAddFriendReq = reinterpret_cast<struct S2CAddFriendReqHeader *>(s2cHeader + 1);
    char *username = reinterpret_cast<char *>(s2cAddFriendReq + 1);
    s2cHeader->tsid = 0;    //push
    s2cHeader->type = S2C::ADD_FRIEND_REQ;
    s2cAddFriendReq->from = userid;
    if (odbc_exec(std::cerr, ("SELECT username FROM user WHERE userid = " + std::to_string(userid) + ';').c_str())) {
        reset();
        return;
    }
    SQLLEN length;
    SQLBindCol(hstmt, 1, SQL_C_BINARY, username, SHA256_DIGEST_LENGTH, &length);
    if (SQL_NO_DATA == SQLFetch(hstmt)) {
        std::cerr << "Unexpected no such a user\n";
        reset();
        return;
    }
    assert(SQL_NULL_DATA != length);
    if (odbc_close_cursor(std::cerr)) {
        reset();
        return;
    }
    //dbgcout << "username of the target user is " << std::string(username, length) << '\n';
    //dbgcout << "Length of username of the target user is " << length << '\n';
    s2cAddFriendReq->nameLen = length;
    it->second->SendLater(buf_, sizeof(S2CHeader) + sizeof(S2CAddFriendReqHeader) + length);
    listen_request();
}
void session::HandleAddFriendReply(const boost::system::error_code& error) {
    dbgcout << __PRETTY_FUNCTION__ << '\n';
    //TODO: read from db to make sure that there is such an add friend request
    HANDLE_ERROR;
    S2CHeader *s2cHeader = reinterpret_cast<S2CHeader *>(buf_);
    C2SAddFriendReply *c2sAddFriendReply = reinterpret_cast<C2SAddFriendReply *>(s2cHeader + 1);
    s2cHeader->tsid = 0;    //push
    s2cHeader->type = S2C::ADD_FRIEND_REPLY;
    userid_t to = c2sAddFriendReply->to;
    S2CAddFriendReply *s2cAddFriendReply = reinterpret_cast<S2CAddFriendReply *>(c2sAddFriendReply);
    s2cAddFriendReply->from = userid;
    auto it = user_session.find(to);
    if (user_session.end() == it) {
        dbgcout << __PRETTY_FUNCTION__ << ": user " << c2sAddFriendReply->to << " offline\n";
        listen_request();
        return;
    }
    it->second->SendLater(buf_, sizeof(S2CHeader) + sizeof(S2CAddFriendReply));
    if (s2cAddFriendReply->reply) {
        //They becomes friends.
        if (odbc_exec(std::cerr, ("INSERT INTO friends VALUES(" +
            std::to_string(userid) + ',' + std::to_string(to) + "),(" +
            std::to_string(to) + ',' + std::to_string(userid) + ");").c_str())
        ) {
            reset();
            return;
        }
    }
    listen_request();
}

void session::HandleDeleteFriend(const boost::system::error_code& error) {
    HANDLE_ERROR;
    //userid_t friendId = *reinterpret_cast<userid_t *>(s2cHeader + 1);
    userid_t friendId = *reinterpret_cast<userid_t *>(buf_);
    if (odbc_exec(std::cerr, ("DELETE FROM friends WHERE user1 = " + std::to_string(userid) +
        " and user2 = " + std::to_string(friendId) + ';'
    ).c_str())) {
        std::cerr << "Error in " << __PRETTY_FUNCTION__ << ": odbc_exec failed\n";
        reset();
        return;
    }
    if (odbc_exec(std::cerr, ("DELETE FROM friends WHERE user1 = " + std::to_string(friendId) +
        " and user2 = " + std::to_string(userid) + ';'
    ).c_str())) {
        std::cerr << "Error in " << __PRETTY_FUNCTION__ << ": odbc_exec failed\n";
        reset();
        return;
    }

    S2CHeader *s2cHeader = reinterpret_cast<S2CHeader *>(buf_);
    userid_t *peerId = reinterpret_cast<userid_t *>(s2cHeader + 1);
    s2cHeader->tsid = 0;    //push
    s2cHeader->type = S2C::DELETE_FRIEND;
    *peerId = userid;
    //TODO: Handle the offline case
    auto it = user_session.find(friendId);
    if (it != user_session.end()) {
        it->second->SendLater(buf_, sizeof(S2CHeader) + sizeof(userid_t));
    }
    listen_request();
}

void session::HandleAllFriendsReq() {
    dbgcout << __PRETTY_FUNCTION__ << '\n';
    struct S2CHeader *s2cHeader = reinterpret_cast<struct S2CHeader *>(buf_);
    uint64_t *num = reinterpret_cast<uint64_t *>(s2cHeader + 1);
    userid_t *friendIdStart = reinterpret_cast<userid_t *>(num + 1);
    s2cHeader->tsid = 0;    //push
    s2cHeader->type = S2C::FRIENDS;
    if (odbc_exec(std::cerr, ("SELECT user2 FROM friends WHERE user1 = " + std::to_string(userid) + ';').c_str())) {
        std::cerr << "Error in " << __PRETTY_FUNCTION__ << ": odbc_exec fail\n";
        reset();
        return;
    }
    *num = 0;
    userid_t *friendId = friendIdStart;
    while (1) {
        if (reinterpret_cast<uint8_t*>(friendId + 1) > buf_ + BUFSIZE) {
            SendLater(buf_, reinterpret_cast<uint8_t*>(friendId) - buf_);
            friendId = friendIdStart;
            *num = 0;
        }
        SQLLEN length;
        SQLBindCol(hstmt, 1, SQL_C_UBIGINT, friendId, sizeof(userid_t), &length);
        if (SQL_NO_DATA != SQLFetch(hstmt)) {
            ++*num;
            ++friendId;
        } else {
            if (*num) {
                SendLater(buf_, reinterpret_cast<uint8_t*>(friendId) - buf_);
            }
            break;
        }
    }
    listen_request();
}
