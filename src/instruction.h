#ifndef INSTRUCTION_H_INCLUDED
#define INSTRUCTION_H_INCLUDED

#include "./opcode.h"
#include "./util/print.hpp"

#include <cstdint>

namespace vm {

struct Instruction {
    OpCode op;
    u4 x;
    u4 y;
};

}

template <>
inline void print(std::ostream& out, const vm::Instruction& t) {
    const char* name = vm::nameOfOpCode.at(t.op); 
    if (auto it = vm::paramSizeOfOpCode.find(t.op); it != vm::paramSizeOfOpCode.end()) {
        switch (it->second.size()) {
        case 0: print(out, name); break;
        case 1: print(out, name, t.x); break;
        case 2: printfmt(out, "{} {},{}", name, t.x, t.y); break;
        default: print(out, "????"); break;
        }
    }
    else {
        print(out, name);
    }
}

#endif