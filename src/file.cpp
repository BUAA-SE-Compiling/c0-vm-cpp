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
    const auto writeNBytes = [&](void* addr, int count) {
        assert(0 < count && count <= 8);
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
    writeNBytes(&constants_count, sizeof constants_count);
    // constants
    for (auto& constant : constants) {
        switch (constant.type)
        {
        case vm::Constant::Type::STRING: {
            out.write("\x00", 1);
            std::string v = std::get<vm::str_t>(constant.value);
            vm::u2 len = v.length();
            writeNBytes(&len, sizeof len);
            out.write(v.c_str(), len);
        } break;
        case vm::Constant::Type::INT: {
            out.write("\x01", 1);
            vm::int_t v = std::get<vm::int_t>(constant.value);
            writeNBytes(&v, sizeof v);
        } break;
        case vm::Constant::Type::DOUBLE: {
            out.write("\x02", 1);
            vm::double_t v = std::get<vm::double_t>(constant.value);
            writeNBytes(&v, sizeof v);
        } break;
        default: assert(("unexpected error", false)); break;
        }
    }

    auto to_binary = [&](const std::vector<vm::Instruction>& v) {
        vm::u2 instructions_count = v.size();
        writeNBytes(&instructions_count, sizeof instructions_count);
        for (auto& ins : v) {
            vm::u1 op = static_cast<vm::u1>(ins.op); 
            writeNBytes(&op, sizeof op);
            if (auto it = vm::paramSizeOfOpCode.find(ins.op); it != vm::paramSizeOfOpCode.end()) {
                auto paramSizes = it->second;
                switch (paramSizes[0]) {
                #define CASE(n) case n: { vm::u##n x = ins.x; writeNBytes(&x, n); }
                CASE(1); break; 
                CASE(2); break;
                CASE(4); break;
                #undef CASE
                default: assert(("unexpected error", false));
                }
                if (paramSizes.size() == 2) {
                    switch (paramSizes[1]) {
                    #define CASE(n) case n: { vm::u##n y = ins.y; writeNBytes(&y, n); }
                    CASE(1); break; 
                    CASE(2); break;
                    CASE(4); break;
                    #undef CASE
                    default: assert(("unexpected error", false));
                    }
                }
            }
        }
    };

    // start
    to_binary(start);
    // functions_count
    vm::u2 functions_count = functions.size();
    writeNBytes(&functions_count, sizeof functions_count);
    // functions
    for (auto& fun : functions) {
        vm::u2 v;
        v = fun.nameIndex; writeNBytes(&v, sizeof v);
        v = fun.paramSize; writeNBytes(&v, sizeof v);
        v = fun.level;     writeNBytes(&v, sizeof v);
        to_binary(fun.instructions);
    }
}

File File::parse_file_binary(std::ifstream& in) {
    // read raw
    const std::vector<unsigned char> buffer(std::istreambuf_iterator<char>(in), {});
    size_t pos = 0;
    const auto bufferSize = buffer.size();
    const auto readByte = [&]() {
        if (pos + 1 > bufferSize) {
            throw InvalidFile("incomplete binary file");
        }
        vm::u1 rtv = static_cast<vm::u1>(buffer[pos]);
        pos += 1;
        return rtv;
    };
    const auto read2bytes = [&] {
        if (pos + 2 > bufferSize) {
            throw InvalidFile("incomplete binary file");
        }
        vm::u2 rtv = static_cast<vm::u2>(
            (buffer[pos] << 8) | buffer[pos+1]
        );
        pos += 2;
        return rtv;
    };
    const auto read4bytes = [&]() {
        if (pos + 4 > bufferSize) {
            throw InvalidFile("incomplete binary file");
        }
        vm::u4 rtv = static_cast<vm::u4>(
            (buffer[pos] << 24) | (buffer[pos+1] << 16) | (buffer[pos+2] << 8) | buffer[pos+3]
        );
        pos += 4;
        return rtv;
    };
    const auto readDouble = [&]() {
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
    const auto readString = [&](vm::u2 length) {
        if (pos + length > bufferSize) {
            throw InvalidFile("invalid binary file: incomplete string constant");
        }
        vm::str_t rtv;
        while (length-- ) {
            rtv += buffer[pos++];
        }
        return rtv;
    };
    const auto readInstruction = [&]() {
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
        start.push_back(std::move(readInstruction()));
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
            fun.instructions.push_back(std::move(readInstruction()));
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
    const auto readLine = [&]() {
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
            // remove leading and trailing whitespaces
            line = trim(std::move(line));
            // not a blank line     
            if (line.size() != 0) {
                ss.str(line);
                ss.clear();
                return;
            }
        }
        // eof
        line = "";
        ss.str("");
    };
    const auto reuseLine = [&]() {
        ss.str(line);
        ss.clear();
    };
    const auto errorLineInfo = [&]() {
        println(std::cerr, "line", line_count, ":\n   ", line);
    };
    const auto errorInvalidFile = [&](std::string msg) {
        errorLineInfo();
        throw InvalidFile(msg);
    };
    #define errorIf(cond, msg)    do { if ((cond)) { errorInvalidFile((msg)); } } while(false)
    #define errorIfNot(cond, msg) errorIf(!(cond), (msg))
    #define errorIfAssignFailed(lhs, rhs, msg) do { try { lhs = (rhs); } catch (const std::exception&) { errorIf(true, (msg)); } } while(false)
    auto ensureNoMoreInput = [&]() {
        if (char ch; ss >> std::skipws >> ch) {
            errorIf(true, "invalid line");
        }
    };

    readLine();

    // parse constants
    std::vector<vm::Constant> constants;
    ss >> str;
    if (str == ".constants:") {
        ensureNoMoreInput();
        while (true) {
            // {index} {type} {value}
            readLine();
            vm::Constant constant;
            std::string temp;
            int index;
            std::string type;
            std::string value;
            // eof
            if (!(ss >> temp)) { 
                reuseLine(); break; 
            }
            // parse index
            try {
                index = try_to_int(temp);
            }
            catch (const std::exception&) {
                reuseLine(); break;
            }
            errorIf(index != constants.size(), "unordered index");
            errorIfNot(ss >> type, "constant type expected");
            if (type == "S") {
                constant.type = vm::Constant::Type::STRING;
                char ch;
                errorIfNot(ss >> std::skipws >> ch, "string constant expected");
                errorIf(ch != '\"', "no leading qoute for string constant");
                // parse the content of string
                while (true) {
                    errorIfNot(ss.get(ch), "no trailing quote for string constant");
                    if (ch == '\"') { 
                        break; 
                    }
                    if (ch == '\\') {
                        errorIfNot(ss.get(ch), "incomplete escape seq");
                        switch (ch) {
                        case '\\': value += '\\'; break;
                        case '\'': value += '\''; break;
                        case '\"': value += '\"'; break;
                        case 'n':  value += '\n'; break;
                        case 'r':  value += '\r'; break;
                        case 't':  value += '\t'; break;
                        case 'x': {
                            errorIfNot(ss.get(ch), "incomplete hex escape seq");
                            errorIfNot(is_hex_digit(ch), "invalid hex escape seq");
                            char v = (0xff & hex_digit_to_int(ch)) << 4;
                            errorIfNot(ss.get(ch), "incomplete hex escape seq");
                            errorIfNot(is_hex_digit(ch), "invalid hex escape seq");
                            v |= (0xff & hex_digit_to_int(ch));
                            value += v;
                        }; break;
                        default: errorIf(true, strfmt("unknown escape seq \"\\{}\"", ch));
                        }
                    }
                    else {
                        value += ch;
                    }
                }
                errorIf(value.length() > UINT16_MAX, "too long the string constant");
                constant.value = std::move(value);
            }
            else if (type == "I") {
                constant.type = vm::Constant::Type::INT;
                errorIfNot(ss >> std::skipws >> value, "invalid format");
                errorIfAssignFailed(constant.value, try_to_int(value), "out of range or invalid format");
            }
            else if (type == "D") {
                constant.type = vm::Constant::Type::DOUBLE;
                errorIfNot(ss >> std::skipws >> value, "invalid format");
                errorIfAssignFailed(constant.value, try_to_double(value), "out of range or invalid format");
            }
            else {
                errorIf(true, "invalid constant type");
            }
            constants.push_back(std::move(constant));
            ensureNoMoreInput();
        }
    }
    else {
        errorIf(true, ".constants expected");
    }
    errorIf(constants.size() > U2_MAX, "too many constants");

    // parse instructions
    auto parseInstructions = [&]() {
        std::vector<vm::Instruction> rtv;
        while(true) {
            // {index} {opcode} {param1} {param2}
            readLine();
            std::string temp;
            int index;
            std::string opName;
            // eof
            if (!(ss >> temp)) { 
                reuseLine(); break; 
            }
            // parse index
            try {
                index = try_to_int(temp);
            }
            catch (const std::exception&) {
                reuseLine(); break;
            }
            errorIf(index != rtv.size(), "unordered index");
            errorIfNot(ss >> opName, "opcode expected");
            opName = to_lower(opName);
            vm::Instruction ins;
            if (auto it = vm::opCodeOfName.find(opName); true) {
                errorIf(it == vm::opCodeOfName.end(), "no such opcode");
                ins.op = it->second;
            }
            if (auto it = vm::paramSizeOfOpCode.find(ins.op); it != vm::paramSizeOfOpCode.end()) {
                int paramCount = it->second.size();
                std::string restLine;
                errorIfNot(std::getline(ss, restLine), "parameters expected");
                auto params = split(restLine, ',');
                errorIf(params.size() != paramCount, 
                    strfmt("{} parameters expected, {} got", paramCount, params.size())
                );
                errorIfAssignFailed(ins.x, try_to_int(params[0]), 
                    strfmt("invalid first parameter: {}", params[0])
                );
                if (paramCount == 2) {
                    errorIfAssignFailed(ins.y, try_to_int(params[1]), 
                        strfmt("invalid second parameter: {}", params[1])
                    );
                }
            }
            ensureNoMoreInput();
            rtv.push_back(ins);
        }
        errorIf(rtv.size() > U2_MAX, "too many instructions");
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
        errorIf(true, ".start expected");
    }

    // parse functions
    std::vector<vm::Function> functions;
    bool mainFound = false;
    ss >> str;
    if (str == ".functions:") {
        ensureNoMoreInput();
        while (true) {
            // {index} {nameIndex} {paramSize} {level}
            readLine();
            vm::Function function;
            std::string temp;
            int index;
            // no more function
            if (!(ss >> temp)) { 
                reuseLine(); break; 
            }
            // parse index
            try {
                index = try_to_int(temp);
            }
            catch (const std::exception&) {
                reuseLine(); break;
            }
            errorIf(index != functions.size(), "unordered index");
            errorIfNot(ss >> temp, "name_index expected");
            errorIfAssignFailed(function.nameIndex, try_to_int(temp), "invalid name_index");
            errorIf(function.nameIndex >= constants.size(), "name not found");
            errorIf(constants[function.nameIndex].type != vm::Constant::Type::STRING, "name not found");
            if (std::get<vm::str_t>(constants[function.nameIndex].value) == "main") {
                mainFound = true;
            }
            errorIfNot(ss >> temp, "param_size expected");
            errorIfAssignFailed(function.paramSize, try_to_int(temp), "invalid param_size");
            errorIf(function.paramSize > U2_MAX, "too many parameters");
            errorIfNot(ss >> temp, "level expected");
            errorIfAssignFailed(function.level, try_to_int(temp), "invalid level");
            errorIf(function.level > U2_MAX, "too high the level");
            functions.push_back(std::move(function));
            ensureNoMoreInput();
        }
    }
    else {
        errorIf(true, ".functions expected");
    }
    errorIfNot(mainFound, "main() not found");

    int functions_count = functions.size();
    errorIf(functions_count > U2_MAX, "too many functions");
    for (int i = 0; i < functions_count; ++i) {
        errorIfNot(ss >> str, strfmt("\".F{}:\" expected", i));
        errorIf((str.length() < 2 || str.back() != ':'), strfmt("\".F{}:\" expected", i));
        str.pop_back();

        int index = -1;
        if (str.front() == '.') {
            str.erase(str.begin());
            ss.str(str);
            ss.clear();
            char ch;
            ss >> std::skipws >> ch;
            if ((ch == 'F' || ch == 'f')) {
                // .Fx
                std::string temp;
                errorIfNot(ss >> temp, strfmt("\".F{}:\" expected", i));
                errorIfAssignFailed(index, try_to_int(temp), "invalid function index");
                errorIf(index != i, strfmt("\".F{}:\" expected", i));
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
        errorIf(index < 0, "no such function");
        functions.at(index).instructions = std::move(parseInstructions());
    }

    errorIf(in >> str, "unused content");

    return File{0x00000001, std::move(constants), std::move(start), std::move(functions)};
}
