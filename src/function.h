#ifndef FUNCTION_H_INCLUDED
#define FUNCTION_H_INCLUDED

#include "./type.h"
#include "./util/print.hpp"
#include "./instruction.h"

#include <cstdint>
#include <vector>

namespace vm {

struct Function {
    u2 nameIndex;
    u2 paramSize;
    u2 level;
    std::vector<vm::Instruction> instructions;
};

}



#endif

