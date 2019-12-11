#ifndef FILE_H_INCLUDED
#define FILE_H_INCLUDED

#include "./type.h"
#include "./instruction.h"
#include "./constant.h"
#include "./function.h"

#include <iostream>
#include <fstream>
#include <vector>

struct File
{
    static const vm::u4 magic_v = 0x43303A29;
    vm::u4 version;
    std::vector<vm::Constant> constants;
    std::vector<vm::Instruction> start;
    std::vector<vm::Function> functions;

    File(vm::u4, std::vector<vm::Constant>, std::vector<vm::Instruction>, std::vector<vm::Function>);

    static File parse_file_text(std::ifstream& in);
    static File parse_file_binary(std::ifstream& in);
    void output_text(std::ostream& out);
    void output_binary(std::ofstream& out);
};

#endif