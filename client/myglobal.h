#ifndef MYGLOBAL_H
#define MYGLOBAL_H

#include "types.h"

#include <unordered_map>
#include <string>

extern std::unordered_map<userid_t, std::string> usernames;
extern std::unordered_map<grpid_t, std::string> grpnames;

#endif // MYGLOBAL_H
