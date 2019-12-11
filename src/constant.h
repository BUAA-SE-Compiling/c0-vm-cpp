#ifndef CONSTANT_H_INCLUDED
#define CONSTANT_H_INCLUDED

#include "./type.h"
#include "./util/print.hpp"
#include "./util/util.hpp"

#include <cstdint>
#include <string>
#include <variant>
#include <utility>

namespace vm {

struct Constant {
    enum class Type : u1 {
        STRING = 0, INT = 1, DOUBLE = 2
    } type;

    std::variant<vm::str_t, vm::int_t, vm::double_t> value;
};

}

template <>
inline void print(std::ostream& out, const vm::Constant& t) {
    switch (t.type)
    {
    case vm::Constant::Type::STRING: 
        printfmt(out, "S \"{}\"", std::get<vm::str_t>(t.value));
        break;
    case vm::Constant::Type::INT: {
        auto v = std::get<vm::int_t>(t.value);
        // printidx(out, "I 0x{0}{1}{2} # {2}", std::hex, std::uppercase, v, v);
        out << "I" 
               " 0x" << std::hex << std::uppercase << v
            << " # " << std::dec << v; 
    } break;
        
    case vm::Constant::Type::DOUBLE: {
        auto v = std::get<vm::double_t>(t.value);
        // printfmt(out, "D {}{} # {}", std::scientific, v, to_hex_string(v));
        out << "D " << to_hex_string(v)
            << " # " << std::scientific << v;
    } break;
    default:
        print(out, "?", "????????");
        break;
    }
}

#endif

