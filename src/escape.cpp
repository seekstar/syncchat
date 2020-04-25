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
