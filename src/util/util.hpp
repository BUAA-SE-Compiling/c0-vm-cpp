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

#endif
