#include "mycontent.h"

#include <sstream>

std::string content2str(CppContent content) {
    return std::string((const char*)content.data(), content.size());
}

std::string NameAndId(userid_t userid) {
    std::ostringstream disp;
    auto it = usernames.find(userid);
    if (usernames.end() != it) {
        disp << it->second;
    }
    disp << '(' << userid << ')';
    return disp.str();
}

std::string GenContentDisp(userid_t sender, msgtime_t time, CppContent content) {
    std::ostringstream disp;
    disp << NameAndId(sender) << "  " << format_ms(time) << std::endl << content2str(content);
    return disp.str();
}
//std::string GenDispHeader(userid_t sender, userid_t reply, msgtime_t time) {
//    std::ostringstream disp;
//    disp << NameAndId(sender) << " 回复 " << NameAndId(reply) << "  " << format_ms(time);
//    return disp.str();
//}
