#include "escape.h"

bool NeedEscape(char c) {
    switch (c) {
    case '\\':
    case '\"':
    case '\'':
        return true;
    }
    return false;
}

std::string escape(std::string s) {
    std::string out;
    for (char c : s) {
        if (NeedEscape(c)) {
            out += '\\';
        }
        out += c;
    }
    return out;
}

std::string escape(const char *s, size_t len) {
    std::string out;
    for (size_t i = 0; i < len; ++i) {
        if (NeedEscape(s[i])) {
            out += '\\';
        }
        out += s[i];
    }
    return out;
}
