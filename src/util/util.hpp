#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include <iostream>
#include <iomanip>
#include <cstdint>
#include <algorithm>
#include <string>
#include <sstream>

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

inline std::int32_t try_to_int(std::string s) {
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
