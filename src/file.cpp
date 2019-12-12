#include "./file.h"
#include "./type.h"
#include "./instruction.h"
#include "./constant.h"
#include "./function.h"
#include "./exception.h"
#include "./util/print.hpp"

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>

File::File(
    vm::u4 version, 
    std::vector<vm::Constant> constants, 
    std::vector<vm::Instruction> instructions, 
    std::vector<vm::Function> functions
) : version(version), constants(constants), start(instructions), functions(functions) {
    //
}


void File::output_text(std::ostream& out) {
    int i;
    
    i = 0;
    println(out, ".constants:");
    for (auto& constant : constants) {
        println(out, i++, constant);
    }

    i = 0;
    println(out, ".start:");
    for (auto& ins : start) {
        println(out, i++, ins);
    }

    std::vector<std::string> names;
    i = 0;
    println(out, ".functions:");
    for (auto& fun : functions) {
        std::string name = std::get<std::string>(constants.at(fun.nameIndex).value);
        names.push_back(name);
        println(out, i++, fun.nameIndex, fun.paramSize, fun.level, "#", name);
    }
    
    i = 0;
    for (auto& fun : functions) {
        printfmt(out, ".F{}: # {}", i, names.at(i)); println(out);
        int j = 0;
        for (auto& ins : fun.instructions) {
            println(out, j++, ins);
        }
        ++i;
    }
}

void File::output_binary(std::ofstream& out) {

    char bytes[8];
    auto writeNBytes = [&](void* addr, int count) {
        assert(0 <= count && count <= 8);
        char* p = reinterpret_cast<char*>(addr) + (count-1);
        for (int i = 0; i < count; ++i) {
            bytes[i] = *p--;
        }
        out.write(bytes, count);
    };

    // magic
    out.write("\x43\x30\x3A\x29", 4);
    // version
    out.write("\x00\x00\x00\x01", 4);
    // constants_count
    vm::u2 constants_count = constants.size();
    writeNBytes(&constants_count, 2);
    // constants
    for (auto& constant : constants) {
        switch (constant.type)
        {
        case vm::Constant::Type::STRING: {
            out.write("\x00", 1);
            std::string v = std::get<vm::str_t>(constant.value);
            vm::u2 len = v.length();
            writeNBytes(&len, 2);
            out.write(v.c_str(), len);
        } break;
        case vm::Constant::Type::INT: {
            out.write("\x01", 1);
            int v = std::get<vm::int_t>(constant.value);
            writeNBytes(&v, 4);
        } break;
        case vm::Constant::Type::DOUBLE: {
            out.write("\x02", 1);
            double v = std::get<vm::double_t>(constant.value);
            writeNBytes(&v, 8);
        } break;
        default: assert(("unexpected error", false)); break;
        }
    }

    auto to_binary = [&](const std::vector<vm::Instruction>& v) {
        vm::u2 instructions_count = v.size();
        writeNBytes(&instructions_count, 2);
        for (auto& ins : v) {
            vm::u1 op = static_cast<vm::u1>(ins.op); 
            writeNBytes(&op, 1);
            if (auto it = vm::paramSizeOfOpCode.find(ins.op); it != vm::paramSizeOfOpCode.end()) {
                auto paramSizes = it->second;
                vm::u4 x = ins.x; 
                switch (paramSizes[0]) {
                #define CASE(n) case n: { vm::u##n x = ins.x; writeNBytes(&x, n); }
                CASE(1); break; 
                CASE(2); break;
                CASE(4); break;
                #undef CASE
                default: assert(("unexpected error", false));
                }
                if (paramSizes.size() == 2) {
                    vm::u4 y = ins.y; 
                    writeNBytes(&y, 4);
                }
            }
        }
    };

    // start
    to_binary(start);
    // functions_count
    vm::u2 functions_count = functions.size();
    writeNBytes(&functions_count, 2);
    // functions
    for (auto& fun : functions) {
        vm::u2 v;
        v = fun.nameIndex; writeNBytes(&v, 2);
        v = fun.paramSize; writeNBytes(&v, 2);
        v = fun.level;     writeNBytes(&v, 2);
        to_binary(fun.instructions);
    }
}

File File::parse_file_binary(std::ifstream& in) {
    // read raw
    const std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(in), {});
    size_t pos = 0;
    const auto bufferSize = buffer.size();
    auto readByte = [&]() {
        if (pos + 1 > bufferSize) {
            throw InvalidFile("incomplete binary file");
        }
        vm::u1 rtv = static_cast<vm::u1>(buffer[pos]);
        pos += 1;
        return rtv;
    };
    auto read2bytes = [&] {
        if (pos + 2 > bufferSize) {
            throw InvalidFile("incomplete binary file");
        }
        vm::u2 rtv = static_cast<vm::u2>(
            (buffer[pos] << 8) | buffer[pos+1]
        );
        pos += 2;
        return rtv;
    };
    auto read4bytes = [&]() {
        if (pos + 4 > bufferSize) {
            throw InvalidFile("incomplete binary file");
        }
        vm::u4 rtv = static_cast<vm::u4>(
            (buffer[pos] << 24) | (buffer[pos+1] << 16) | (buffer[pos+2] << 8) | buffer[pos+3]
        );
        pos += 4;
        return rtv;
    };
    auto readDouble = [&]() {
        if (pos + 8 > bufferSize) {
            throw InvalidFile("invalid binary file: incomplete double constant");
        }
        union {
            vm::double_t d;
            unsigned char b[8];
        } rtv;
        for (int i = 7; i >= 0; --i) {
            rtv.b[i] = buffer[pos++];
        }
        return rtv.d;
    };
    auto readString = [&](vm::u2 length) {
        if (pos + length > bufferSize) {
            throw InvalidFile("invalid binary file: incomplete string constant");
        }
        vm::str_t rtv;
        while (length-- ) {
            rtv += buffer[pos++];
        }
        return rtv;
    };
    auto parseInstruction = [&]() {
        vm::Instruction ins;
        ins.op = static_cast<vm::OpCode>(readByte());
        if (vm::nameOfOpCode.count(ins.op) == 0) {
            throw InvalidFile("invalid binary file: invalid opcode");
        }
        if (auto it = vm::paramSizeOfOpCode.find(ins.op); it != vm::paramSizeOfOpCode.end()) {
            auto paramSizes = it->second;
            switch (paramSizes[0]) {
            case 1: ins.x = readByte(); break;
            case 2: ins.x = read2bytes(); break;
            case 4: ins.x = read4bytes(); break;
            default: assert(("unexpected error", false));
            }
            if (paramSizes.size() == 2) {
                ins.y = read4bytes();
            }
        }
        return ins;
    };

    // parse magic
    auto magic = read4bytes(); 
    if (magic != magic_v) {
        throw InvalidFile("invalid binary file: invalid magic");
    }

    // parse version
    auto version = read4bytes(); 

    // parse constants
    auto constantsCount = read2bytes();
    std::vector<vm::Constant> constants;
    for (int j = 0; j < constantsCount; ++j) {
        vm::Constant constant;
        constant.type = static_cast<vm::Constant::Type>(readByte());
        switch (constant.type)
        {
        case vm::Constant::Type::STRING: {
            auto length = read2bytes();
            constant.value = std::move(readString(length));
        } break;
        case vm::Constant::Type::INT: {
            constant.value = static_cast<vm::int_t>(read4bytes());
        } break;
        case vm::Constant::Type::DOUBLE: {
            constant.value = readDouble();
        } break;
        default:
            throw InvalidFile("invalid binary file: invalid constant type");
        }
        constants.push_back(std::move(constant));
    }

    // parse start
    auto instructionsCount = read2bytes();
    std::vector<vm::Instruction> start;
    for (int j = 0; j < instructionsCount; ++j) {
        start.push_back(std::move(parseInstruction()));
    }

    // parse functions
    auto functionsCount = read2bytes();
    std::vector<vm::Function> functions;
    bool mainFound = false;
    for (int j = 0; j < functionsCount; ++j) {
        vm::Function fun;
        fun.nameIndex = read2bytes();
        if (fun.nameIndex >= constants.size()) {
            throw InvalidFile("invalid binary file: function name not found");
        }
        if (constants[fun.nameIndex].type != vm::Constant::Type::STRING) {
            throw InvalidFile("invalid binary file: function name not found");
        }
        if (std::get<vm::str_t>(constants[fun.nameIndex].value) == "main") {
            mainFound = true;
        }
        fun.paramSize = read2bytes();
        fun.level = read2bytes();
        auto instructionsCount = read2bytes();
        for (int k = 0; k < instructionsCount; ++k) {
            fun.instructions.push_back(std::move(parseInstruction()));
        }
        functions.push_back(std::move(fun));
    }

    if (!mainFound) {
        throw InvalidFile("invalid binary file: main() not found");
    }

    if (pos != buffer.size()) {
        throw InvalidFile("invalid binary file: unused content");
    }

    return File{version, std::move(constants), std::move(start), std::move(functions)};
}

File File::parse_file_text(std::ifstream& in) {
    int line_count = 0;
    std::string line = "";
    std::string str = "";
    std::stringstream ss;
    auto readLine = [&]() {
        while(std::getline(in, line)) {
            ++line_count;
            int bg = 0;
            int ed = line.length();
            // remove comment
            for (auto i = bg; i < ed; ++i) {
                if (line[i] == '#') {
                    ed = i;
                    break;
                }
            }
            // remove leading whitespaces
            for (;bg < ed; ++bg) {
                if (!isspace(static_cast<unsigned char>(line[bg]))) {
                    break;
                }
            }   
            // not a blank line     
            if (bg != ed) {
                line = line.substr(bg, ed-bg);
                ss.str(line);
                ss.clear();
                return;
            }
        }
        // eof
        line = "";
        ss.str("");
    };
    auto reuseLine = [&]() {
        ss.str(line);
        ss.clear();
    };
    auto errorLineInfo = [&]() {
        println(std::cerr, "line", line_count, ":\n   ", line);
    };
    auto throwInvalidFileException = [&](const char* msg) {
        errorLineInfo();
        throw InvalidFile(msg);
    };
    auto ensureNoMoreInput = [&]() {
        char ch;
        if (ss >> std::skipws >> ch) {
            throwInvalidFileException("invalid line");
        }
    };
    readLine();

    // parse constants
    std::vector<vm::Constant> constants;
    ss >> str;
    if (str == ".constants:") {
        ensureNoMoreInput();
        while (true) {
            readLine();
            vm::Constant constant;
            unsigned int index;
            std::string type;
            std::string value;
            // no more constants
            if (!(ss >> index >> type)) {
                reuseLine();
                break;
            }
            if (index != constants.size()) {
                throwInvalidFileException("unordered index");
            }
            if (type == "S") {
                constant.type = vm::Constant::Type::STRING;
                char ch;
                if (!(ss >> std::skipws >> ch)) {
                    throwInvalidFileException("string constant expected");
                }
                if (ch != '\"') {
                    throwInvalidFileException("no qoute for string constant");
                }
                while (true) {
                    if (!ss.get(ch)) {
                        throwInvalidFileException("endless string constant");
                    }
                    if (ch == '\"') {
                        break;
                    }
                    value += ch;
                }
            
                if (value.length() > UINT16_MAX) {
                    throwInvalidFileException("too long the string constant");
                }
                constant.value = std::move(value);
            }
            else if (type == "I") {
                constant.type = vm::Constant::Type::INT;
                if (!(ss >> std::skipws >> value)) {
                    throwInvalidFileException("invalid format");
                }
                try {
                    constant.value = try_to_int(value);
                }
                catch (const std::exception&) {
                    throwInvalidFileException("out of range or invalid format");
                }
            }
            else if (type == "D") {
                constant.type = vm::Constant::Type::DOUBLE;
                if (!(ss >> std::skipws >> value)) {
                    throwInvalidFileException("invalid format");
                }
                try {
                    constant.value = try_to_double(value);
                }
                catch (const std::exception&) {
                    throwInvalidFileException("out of range or invalid format");
                }
            }
            else {
                throwInvalidFileException("invalid constant type");
            }
            constants.push_back(std::move(constant));
            ensureNoMoreInput();
        }
    }
    else {
        throwInvalidFileException(".constants expected");
    }
    if (constants.size() > U2_MAX) {
        throw InvalidFile("too many constants");
    }

    // parse instructions
    auto parseInstructions = [&]() {
        std::vector<vm::Instruction> rtv;
        while(true) {
            readLine();
            unsigned int index;
            std::string opName;
            // no more instructions
            if (!(ss >> index >> opName)) {
                reuseLine();
                break;
            }
            if (index != rtv.size()) {
                throwInvalidFileException("unordered index");
            }
            for (auto& ch : opName) {
                ch = tolower(static_cast<unsigned char>(ch));
            }
            vm::Instruction ins;
            if (auto it = vm::opCodeOfName.find(opName); it != vm::opCodeOfName.end()) {
                ins.op = it->second;
            }
            else {
                throwInvalidFileException("no such opcode");
            }
            if (auto it = vm::paramSizeOfOpCode.find(ins.op); it != vm::paramSizeOfOpCode.end()) {
                int paramCount = it->second.size();
                if (!(ss >> ins.x)) {
                    throwInvalidFileException("parameter expected");
                }
                if(paramCount == 2) {
                    char ch;
                    if (!(ss >> std::skipws >> ch)) {
                        throwInvalidFileException("another parameter expected");
                    }
                    if (ch != ',') {
                        throwInvalidFileException("parameters should be seperated by \",\"");
                    }
                    if (!(ss >> ins.y)) {
                        throwInvalidFileException("another parameter expected");
                    }
                }
            }
            rtv.push_back(ins);
            ensureNoMoreInput();
        }
        if (rtv.size() > U2_MAX) {
            throw InvalidFile("too many instructions");
        }
        return rtv;
    };

    // parse start
    std::vector<vm::Instruction> start;
    ss >> str;
    if (str == ".start:") {
        ensureNoMoreInput();
        start = std::move(parseInstructions());
    }
    else {
        throwInvalidFileException(".start expected");
    }

    // parse functions
    std::vector<vm::Function> functions;
    bool mainFound = false;
    ss >> str;
    if (str == ".functions:") {
        ensureNoMoreInput();
        while (true) {
            readLine();
            vm::Function function;
            unsigned int index;
            unsigned int nameIndex;
            unsigned int paramSize;
            unsigned int level;
            // no more function
            if (!(ss >> std::skipws >> index >> nameIndex >> paramSize >> level)) {
                reuseLine();
                break;
            }
            if (index != functions.size()) {
                throwInvalidFileException("unordered index");
            }
            if (nameIndex >= constants.size()) {
                throwInvalidFileException("name not found");
            }
            if (constants[nameIndex].type != vm::Constant::Type::STRING) {
                throwInvalidFileException("name not found");
            }
            if (std::get<vm::str_t>(constants[nameIndex].value) == "main") {
                mainFound = true;
            }
            if (paramSize > U2_MAX) {
                throwInvalidFileException("too many parameters");
            }
            if (level > U2_MAX) {
                throwInvalidFileException("too high the level");
            }
            function.nameIndex = nameIndex;
            function.paramSize = paramSize;
            function.level = level;
            functions.push_back(std::move(function));
            ensureNoMoreInput();
        }
    }
    else {
        throwInvalidFileException(".functions expected");
    }
    if (!mainFound) {
        throw InvalidFile("main() not found");
    }

    int functions_count = functions.size();
    if (functions_count > U2_MAX) {
        throw InvalidFile("too many functions");
    }
    for (int i = 0; i < functions_count; ++i) {
        if (!(ss >> str)) {
            throwInvalidFileException("\".F{index}:\" expected");
        }
        if (str.length() < 2 || str.back() != ':') {
            throwInvalidFileException("\".F{index}:\" expected");
        }
        str.pop_back();

        int index = -1;
        if (str.front() == '.') {
            str.erase(str.begin());
            ss.str(str);
            ss.clear();
            char ch;
            ss >> std::skipws >> ch;
            if ((ch == 'F' || ch == 'f') && ss >> index) {
                // .Fx
                if (index < 0 || index >= functions_count) {
                    throwInvalidFileException("index out of range");
                }
            }
        }
        else {
            // str is name
            index = -1;
            for (auto j = 0; j < functions_count; ++j) {
                if (std::get<std::string>(constants.at(functions.at(j).nameIndex).value) == str) {
                    index = j;
                    break;
                }
            } 
        }
        ensureNoMoreInput();
        if (index < 0) {
            throwInvalidFileException("no such function");
        }
        functions.at(index).instructions = std::move(parseInstructions());
    }

    if (in >> str) {
        throwInvalidFileException("unused content");
    }

    return File{0x00000001, std::move(constants), std::move(start), std::move(functions)};
}
