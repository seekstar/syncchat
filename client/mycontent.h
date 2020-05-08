#ifndef MYCONTENT_H
#define MYCONTENT_H

#include <string>
#include <sstream>

#include "types.h"
#include "clienttypes.h"
#include "myglobal.h"

template <typename T>
std::string format_ms(T time) {
    time_t tt = time / 1000;
    std::tm *now = std::localtime(&tt);
    std::ostringstream disp;
    disp << (now->tm_year + 1900) << '-' << now->tm_mon << '-' << now->tm_mday << ' '
         << now->tm_hour << ':' << now->tm_min << ':' << now->tm_sec;
    return disp.str();
}

std::string NameAndId(userid_t userid);

std::string content2str(CppContent content);
std::string GenContentDisp(userid_t sender, msgtime_t time, CppContent content);

#endif // MYCONTENT_H
