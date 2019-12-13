#ifndef PRINT_H_INCLUDED
#define PRINT_H_INCLUDED

#include "./tuple_visit.hpp"

#include <iostream>
#include <sstream>
#include <tuple>

inline void print(std::ostream& out) {
	;
}

template <typename T>
inline void print(std::ostream& out, const T& t) {
    out << t;
}

template <typename T, typename ...Args>
// ("A", 1, 'B') -> ("A 1 B")
inline void print(std::ostream& out, const T& t, const Args&... args) {
    print(out, t);
	print(out, " ");
    print(out, args...);
}

inline void println(std::ostream& out) {
	out << std::endl;
}

template <typename ...Args>
// ("A", 1, 'B') -> ("A 1 B") and new-line
inline void println(std::ostream& out, const Args&... args) {
    print(out, args...);
	println(out);
}

template <typename ...Args>
// ("A", 1, 'B') -> ("log: A 1 B")
inline void log(const Args&... args) {
#ifdef DEBUG
    println(std::cout, "log:", args...);
#endif
}

inline void printfmt(std::ostream& out, const char *format) {
    print(out, format);
}

template <typename T, typename... Args>
// ("{} is {}, not {}", "A", 1, 'B') -> ("A is 1, not B")
inline void printfmt(std::ostream& out, const char *format, const T& arg, const Args&... args) {
    char* s = const_cast<char*>(format);
    for (unsigned char ch; (ch = *s) != '\0'; out << *s++) {
        if (ch == '{' && *(s+1) == '}') {
            print(out, arg);
            printfmt(out, s+2, args...);
            break;
        }
    }
}

template <typename... Args>
// ("{} is {}, not {}", "A", 1, 'B') -> ("A is 1, not B")
inline std::string strfmt(const char *format, const Args&... args) {
    std::stringstream ss;
    printfmt(ss, format, args...);
    return ss.str();
}

inline bool is_digit(int ch) {
    return '0' <= ch && ch <= '9';
}

inline void printidx(std::ostream& out, const char *format) {
    print(out, format);
}

template <typename... Args>
// ("{0} is {1}, not {0}", "A", 1, 'B') -> ("A is 1, not A")
inline void printidx(std::ostream& out, const char *format, const Args&... args) {
    auto tp = std::make_tuple(args...);
    size_t N = std::tuple_size<std::decay_t<decltype(tp)>>::value;
    char* s = const_cast<char*>(format);
    for (unsigned char ch; (ch = *s) != '\0'; ) {
        if (ch == '{' && is_digit(*(s+1))) {
            size_t index = 0;
            ch = *++s;
            do {
                index = index*10 + (ch - '0');
            } while(is_digit(ch = *++s));
            if (ch == '}' && index < N) {
                visit_at(tp, index, [&out](const auto& v){ print(out, v); });
                ++s;
                continue;
            }
            else {
                out << '{' << index;
            }
        }
        out << *s++;
    }
}

#endif
