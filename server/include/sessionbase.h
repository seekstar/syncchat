#ifndef SESSIONBASE_H_
#define SESSIONBASE_H_

#include "session.h"

#include <unordered_map>

#include "odbcbase.h"
#include "cppbase.h"

#define HANDLE_ERROR    \
    if (error) {        \
        dbgcout << __PRETTY_FUNCTION__ << '\n';\
        reset();        \
        return;         \
    }

extern std::unordered_map<userid_t, session*> user_session;

#endif //SESSIONBASE_H_