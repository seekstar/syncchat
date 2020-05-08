#include "sessionbase.h"

#include <vector>

void session::HandleMomentHeader(const boost::system::error_code& error) {
    HANDLE_ERROR;
    uint64_t len = *reinterpret_cast<uint64_t *>(buf_);
    boost::asio::async_read(socket_,
        boost::asio::buffer(buf_ + sizeof(uint64_t), len),
        boost::bind(&session::HandleMomentContent, this, boost::asio::placeholders::error));
}
void session::HandleMomentContent(const boost::system::error_code& error) {
    HANDLE_ERROR;
    SQLLEN len = *reinterpret_cast<uint64_t *>(buf_);
    uint8_t *content = reinterpret_cast<uint8_t *>(buf_ + sizeof(uint64_t));
    using namespace std::chrono;
    msgtime_t time = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_BINARY, MAX_CONTENT_LEN, 0, content, len, &len);
    if (odbc_exec(std::cerr, (
        "INSERT INTO moment(time, sender, content) VALUES(" + std::to_string(time) + ',' + 
        std::to_string(userid) + ',' + "?);"
    ).c_str())) {
        reset();
        return;
    }
    listen_request();
}

void session::HandleMomentsReq() {
    if (odbc_exec(std::cerr, (
        "SELECT id, time, sender, content FROM moment WHERE sender = " + std::to_string(userid) + ';'
    ).c_str())) {
        reset();
        return;
    }
    std::vector<CppMoment> moments;
    SQLLEN length, contentLen;
    momentid_t id;
    msgtime_t time;
    userid_t sender;
    SQLBindCol(hstmt, 1, SQL_C_UBIGINT, &id, sizeof(id), &length);
    SQLBindCol(hstmt, 2, SQL_C_SBIGINT, &time, sizeof(time), &length);
    SQLBindCol(hstmt, 3, SQL_C_UBIGINT, &sender, sizeof(sender), &length);
    SQLBindCol(hstmt, 4, SQL_C_BINARY, buf_, sizeof(buf_), &contentLen);
    while (SQL_NO_DATA != SQLFetch(hstmt)) {
        auto content = std::vector<uint8_t>(buf_, buf_ + contentLen);
        moments.emplace_back(id, time, sender, content);
    }

    if (odbc_exec(std::cerr, (
        "SELECT id, time, sender, content FROM moment JOIN friends ON sender = user1 "
        "WHERE user2 = " + std::to_string(userid) + ';'
    ).c_str())) {
        reset();
        return;
    }
    SQLBindCol(hstmt, 1, SQL_C_UBIGINT, &id, sizeof(id), &length);
    SQLBindCol(hstmt, 2, SQL_C_SBIGINT, &time, sizeof(time), &length);
    SQLBindCol(hstmt, 3, SQL_C_UBIGINT, &sender, sizeof(sender), &length);
    SQLBindCol(hstmt, 4, SQL_C_BINARY, buf_, MAX_CONTENT_LEN, &contentLen);
    while (SQL_NO_DATA != SQLFetch(hstmt)) {
        auto content = std::vector<uint8_t>(buf_, buf_ + contentLen);
        moments.emplace_back(id, time, sender, content);
    }

    S2CHeader *s2cHeader = reinterpret_cast<S2CHeader *>(buf_);
    uint64_t *len = reinterpret_cast<uint64_t *>(s2cHeader + 1);
    s2cHeader->tsid = 0;    //push
    s2cHeader->type = S2C::MOMENTS;
    *len = moments.size();
    SendLater(buf_, sizeof(S2CHeader) + sizeof(uint64_t));
    dbgcout << __PRETTY_FUNCTION__ << ": " << *len << " moments to send\n";

    for (auto& moment : moments) {
        MomentHeader *momentHeader = reinterpret_cast<MomentHeader *>(buf_);
        momentHeader->id = moment.id;
        momentHeader->time = moment.time;
        momentHeader->sender = moment.sender;
        momentHeader->len = moment.content.size();
        memcpy(momentHeader + 1, moment.content.data(), moment.content.size());
        SendLater(buf_, sizeof(MomentHeader) + moment.content.size());
    }
    listen_request();
}

void session::HandleCommentHeader(const boost::system::error_code& error) {
    dbgcout << __PRETTY_FUNCTION__ << '\n';
    HANDLE_ERROR;
    C2SCommentHeader *c2sCommentHeader = reinterpret_cast<C2SCommentHeader *>(buf_);
    boost::asio::async_read(socket_,
        boost::asio::buffer(buf_ + sizeof(C2SCommentHeader), c2sCommentHeader->len),
        boost::bind(&session::HandleCommentContent, this, boost::asio::placeholders::error));
}
void session::HandleCommentContent(const boost::system::error_code& error) {
    dbgcout << __PRETTY_FUNCTION__ << '\n';
    HANDLE_ERROR;
    C2SCommentHeader *c2sCommentHeader = reinterpret_cast<C2SCommentHeader *>(buf_);
    uint8_t *content = reinterpret_cast<uint8_t *>(c2sCommentHeader + 1);

    using namespace std::chrono;
    SQLLEN length = c2sCommentHeader->len;
    msgtime_t time = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    SQLBindParameter(hstmt, 1, SQL_PARAM_INPUT, SQL_C_BINARY, SQL_BINARY, MAX_CONTENT_LEN, 0, 
        content, c2sCommentHeader->len, &length);
    if (c2sCommentHeader->reply) {
        if (odbc_exec(std::cerr, (
            "INSERT INTO comment(time, sender, tommt, reply, content) VALUES(" + std::to_string(time) + ',' + 
            std::to_string(userid) + ',' + std::to_string(c2sCommentHeader->to) + ',' +
            std::to_string(c2sCommentHeader->reply) + ",?);"
        ).c_str())) {
            reset();
            return;
        }
    } else { //reply is null
        if (odbc_exec(std::cerr, (
            "INSERT INTO comment(time, sender, tommt, content) VALUES(" + std::to_string(time) + ',' + 
            std::to_string(userid) + ',' + std::to_string(c2sCommentHeader->to) + ",?);"
        ).c_str())) {
            reset();
            return;
        }
    }
    listen_request();
}

void session::HandleCommentsReq(const boost::system::error_code& error) {
    dbgcout << __PRETTY_FUNCTION__ << '\n';
    HANDLE_ERROR;
    momentid_t momentid = *reinterpret_cast<momentid_t *>(buf_);
    //qq style
    if (odbc_exec(std::cerr, ("SELECT id, time, sender, reply, content FROM comment WHERE tommt = " +
        std::to_string(momentid) + ';'
    ).c_str())) {
        reset();
        return;
    }
    std::vector<CppComment> comments;
    commentid_t id;
    msgtime_t time;
    userid_t sender;
    commentid_t reply;
    SQLLEN length, replyLen, contentLen;
    SQLBindCol(hstmt, 1, SQL_C_UBIGINT, &id, sizeof(id), &length);
    SQLBindCol(hstmt, 2, SQL_C_SBIGINT, &time, sizeof(time), &length);
    SQLBindCol(hstmt, 3, SQL_C_UBIGINT, &sender, sizeof(sender), &length);
    SQLBindCol(hstmt, 4, SQL_C_UBIGINT, &reply, sizeof(reply), &replyLen);
    SQLBindCol(hstmt, 5, SQL_C_BINARY, buf_, MAX_CONTENT_LEN, &contentLen);
    while (SQL_NO_DATA != SQLFetch(hstmt)) {
        comments.emplace_back(id, time, sender, (SQL_NULL_DATA == replyLen ? 0 : reply),
                              CppContent(buf_, buf_ + contentLen));
    }

    S2CHeader *s2cHeader = reinterpret_cast<S2CHeader *>(buf_);
    CommentArrayHeader *arrayHeader = reinterpret_cast<CommentArrayHeader *>(s2cHeader + 1);
    s2cHeader->tsid = 0;    //push
    s2cHeader->type = S2C::COMMENTS;
    arrayHeader->num = comments.size();
    arrayHeader->to = momentid;
    SendLater(buf_, sizeof(S2CHeader) + sizeof(CommentArrayHeader));
    dbgcout << __PRETTY_FUNCTION__ << ": " << arrayHeader->num << " comments to send\n";

    for (auto& comment : comments) {
        CommentHeader *commentHeader = reinterpret_cast<CommentHeader *>(buf_);
        commentHeader->id = comment.id;
        commentHeader->time = comment.time;
        commentHeader->sender = comment.sender;
        commentHeader->reply = comment.reply;
        commentHeader->len = comment.content.size();
        memcpy(commentHeader + 1, comment.content.data(), comment.content.size());
        dbgcout << "The length of this comment is " << commentHeader->len << ' ' << comment.content.size() << '\n';
        SendLater(buf_, sizeof(CommentHeader) + comment.content.size());
    }
    listen_request();
}
