#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include <iostream>
#include <iomanip>
#include <cstdint>
#include <algorithm>
#include <string>
#include <sstream>

inline bool is_hex_digit(unsigned char ch) {
    return '0' <= ch && ch <= '9'
        || 'a' <= ch && ch <= 'f'
        || 'A' <= ch && ch <= 'F'
    ;
}

inline int hex_digit_to_int(unsigned char ch) {
    if ('0' <= ch && ch <= '9') {
        return ch - '0';
    }
    if ('a' <= ch && ch <= 'f') {
        return ch - 'a' + 10;
    }
    if ('A' <= ch && ch <= 'F') {
        return ch - 'A' + 10;
    }
    assert(("unknown error", false));
    return -1;
}

inline std::string to_hex_string(double d) {
    std::stringstream ss;
    ss << "0x";
    union {
        double d;
        unsigned char s[8];
    } v;
    v.d = d;
    for (int i = 0; i < 8; ++i) {
        ss << std::hex 
            << std::setw(2) 
            << std::setfill('0') 
            << std::uppercase 
            << (unsigned)v.s[7-i];
    }
    return ss.str();
}

inline std::string trim(std::string s) {
    auto bg = 0;
    auto ed = s.size();
    for (;bg < ed; ++bg) {
        if (!isspace(static_cast<unsigned char>(s[bg]))) {
            break;
        }
    }
    for (; ed > bg; --ed) {
        if (!isspace(static_cast<unsigned char>(s[ed]))) {
            break;
        }
    }
    return s.substr(bg, ed-bg);
}

inline std::int32_t try_to_int(std::string s) {
    s = trim(s);
    if (s.length() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        return static_cast<std::int32_t>(std::stoull(s, 0, 0));
    }
    else {
        return std::stoi(s);
    }
}

inline double try_to_double(std::string s) {
    if (s.length() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
        union {
            double d;
            std::uint64_t ll;
        } v;
        v.ll = static_cast<std::uint64_t>(std::stoull(s, 0, 16));
        return v.d;
    }
    else {
        return std::stod(s, 0);
    }
}

inline std::vector<std::string> split(std::string s, char delimiter) {
    std::vector<std::string> rtv;
    std::string temp = "";
    for (auto it = s.begin(); true; ++it) {
        if (it == s.end()) {
            rtv.push_back(std::move(temp));
            return rtv;
        }
        char ch = *it;
        if (ch == delimiter) {
            rtv.push_back(std::move(temp));
            temp = "";
        }
        else {
            temp += ch;
        }
    }
}

inline std::string to_lower(std::string s) {
    std::transform(s.begin(), s.end(), s.begin(),
        [](unsigned char ch){ return std::tolower(ch); }
    );
    return s;
}

#endif
