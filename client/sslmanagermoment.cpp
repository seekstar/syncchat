#include "sslmanagerbase.h"

void SslManager::SendMoment(CppContent content) {
    auto buf = C2SHeaderBuf(C2S::MOMENT);
    uint64_t len = content.size();
    PushBuf(buf, &len, sizeof(len));
    PushBuf(buf, content.data(), content.size());
    SendLater(buf);
}

void SslManager::MomentsReq() {
    SendLater(C2SHeaderBuf_noreply(C2S::MOMENTS_REQ));
}

void SslManager::HandleMomentsMainHeader(const boost::system::error_code& error) {
    qDebug() << __PRETTY_FUNCTION__;
    HANDLE_ERROR;
    uint64_t num = *reinterpret_cast<uint64_t *>(recvbuf_);
    qDebug() << __PRETTY_FUNCTION__ << ":" << num << "moments to receive";
    HandleMomentArray(num);
}
void SslManager::HandleMomentArray(uint64_t num) {
    //qDebug() << __PRETTY_FUNCTION__ << ":" << num;
    if (0 == num) {
        emit Moments(moments_);
        moments_.clear();
        ListenToServer();
        return;
    }
    boost::asio::async_read(*socket_,
        boost::asio::buffer(recvbuf_, sizeof(MomentHeader)),
        boost::bind(&SslManager::HandleMomentArray2, this, num, boost::asio::placeholders::error));
}
void SslManager::HandleMomentArray2(uint64_t num, const boost::system::error_code& error) {
    HANDLE_ERROR;
    MomentHeader *momentHeader = reinterpret_cast<MomentHeader *>(recvbuf_);
    boost::asio::async_read(*socket_,
        boost::asio::buffer(momentHeader + 1, momentHeader->len),
        boost::bind(&SslManager::HandleMomentArray3, this, num, boost::asio::placeholders::error));
}
void SslManager::HandleMomentArray3(uint64_t num, const boost::system::error_code& error) {
    HANDLE_ERROR;
    MomentHeader *momentHeader = reinterpret_cast<MomentHeader *>(recvbuf_);
    uint8_t *content = reinterpret_cast<uint8_t*>(momentHeader + 1);
    moments_.emplace_back(momentHeader->id, momentHeader->time, momentHeader->sender,
                                    CppContent(content, content + momentHeader->len));
    HandleMomentArray(num - 1);
}

void SslManager::SendComment(momentid_t to, commentid_t reply, CppContent content) {
    auto buf = C2SHeaderBuf_noreply(C2S::COMMENT);
    C2SCommentHeader c2sCommentHeader{to, reply, content.size()};
    PushBuf(buf, &c2sCommentHeader, sizeof(c2sCommentHeader));
    PushBuf(buf, content.data(), content.size());
    SendLater(buf);
}

void SslManager::CommentsReq(momentid_t to) {
    auto buf = C2SHeaderBuf_noreply(C2S::COMMENTS_REQ);
    PushBuf(buf, &to, sizeof(to));
    SendLater(buf);
}

void SslManager::HandleCommentArrayHeader(const boost::system::error_code &error) {
    HANDLE_ERROR;
    auto commentArrayHeader = reinterpret_cast<CommentArrayHeader *>(recvbuf_);
    qDebug() << commentArrayHeader->num << "comments to receive.";
    HandleCommentArray(commentArrayHeader->to, commentArrayHeader->num);
}

void SslManager::HandleCommentArray(momentid_t to, uint64_t num) {
    qDebug() << __PRETTY_FUNCTION__ << ":" << num;
    if (0 == num) {
        emit Comments(to, comments_);
        comments_.clear();
        ListenToServer();
        return;
    }
    boost::asio::async_read(*socket_,
        boost::asio::buffer(recvbuf_, sizeof(CommentHeader)),
        boost::bind(&SslManager::HandleCommentArrayElemHeader, this, to, num, boost::asio::placeholders::error));
}
void SslManager::HandleCommentArrayElemHeader(momentid_t to, uint64_t num, const boost::system::error_code &error) {
    HANDLE_ERROR;
    CommentHeader *commentHeader = reinterpret_cast<CommentHeader *>(recvbuf_);
    qDebug() << "The length of this comment is" << commentHeader->len;
    boost::asio::async_read(*socket_,
        boost::asio::buffer(commentHeader + 1, commentHeader->len),
        boost::bind(&SslManager::HandleCommentArrayElemContent, this, to, num, boost::asio::placeholders::error));
}
void SslManager::HandleCommentArrayElemContent(momentid_t to, uint64_t num, const boost::system::error_code &error) {
    HANDLE_ERROR;
    CommentHeader *commentHeader = reinterpret_cast<CommentHeader *>(recvbuf_);
    uint8_t *content = reinterpret_cast<uint8_t *>(commentHeader + 1);
    comments_.emplace_back(commentHeader->id, commentHeader->time, commentHeader->sender, commentHeader->reply,
                           CppContent(content, content + commentHeader->len));
    HandleCommentArray(to, num - 1);
}
