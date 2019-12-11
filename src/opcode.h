#ifndef OPCODE_H_INCLUDED
#define OPCODE_H_INCLUDED

#include "./type.h"

#include <vector>
#include <unordered_map>

namespace vm {

enum class OpCode : u1 {
    // do nothing
    nop = 0x00,
    
    // bipush value(1)
    // ...
    // ..., value
    bipush = 0x01,

    // ipush value(4)
    // ...
    // ..., value
    ipush  = 0x02,

    // pop / popn count(4)
    // ..., value(n)
    // ...
    pop = 0x04, pop2 = 0x05, popn = 0x06,
    // dup
    // ..., value
    // ..., value, value
    dup = 0x07, dup2 = 0x08,
    
    // loadc index(2)
    // ...
    // ..., value
    loadc = 0x09,
    
    // loada level_diff(2), offser(4)
    // ...
    // ..., addr
    loada = 0x0a,

    // new
    // ..., count
    // ..., addr
    _new = 0x0b,
    // snew count(4)
    // ...
    // ..., value
    snew = 0x0c,
    
    // Tload
    // ..., addr
    // ..., value
    iload = 0x10,   dload = 0x11,   aload = 0x12,
    // Taload
    // ..., array, index
    // ..., value
    iaload = 0x18,  daload = 0x19,  aaload = 0x1a,
    // Tstore
    // ..., addr, value
    // ...
    istore = 0x20,  dstore = 0x21,  astore = 0x22,
    // Tastore
    // ..., array, index, value
    // ...
    iastore = 0x28, dastore = 0x29, aastore = 0x2a,
    
    // Tadd
    // ..., lhs, rhs
    // ..., result
    iadd = 0x30, dadd = 0x31,
    // Tsub
    // ..., lhs, rhs
    // ..., result
    isub = 0x34, dsub = 0x35,
    // Tmul
    // ..., lhs, rhs
    // ..., result
    imul = 0x38, dmul = 0x39,
    // Tdiv
    // ..., lhs, rhs
    // ..., result
    idiv = 0x3c, ddiv = 0x3d,
    // Tneg
    // ..., value
    // ..., result
    ineg = 0x40, dneg = 0x41,
    // Tcmp
    // ..., lhs, rhs
    // ..., result
    icmp = 0x44, dcmp = 0x45,

    // T2T
    // ..., value
    // ..., result
    i2d = 0x60, d2i = 0x61, i2c = 0x62,

    // jmp offset(2)
    jmp = 0x70,
    
    // jCOND offset(2)
    // ..., value
    // ...
    je = 0x71, jne = 0x72, jl = 0x73, jge = 0x74, jg = 0x75, jle = 0x76,

    // call index(2)
    // ..., params
    // ...
    call = 0x80,
    
    // ret
    ret = 0x88,
    // Tret
    iret = 0x89, dret = 0x8a, aret = 0x8b,
    
    // Tprint
    // ..., value
    // ...
    iprint = 0xa0, dprint = 0xa1, cprint = 0xa2, sprint = 0xa3,
    
    // printl
    printl = 0xaf,
    
    // Tscan
    // ...
    // ..., value
    iscan = 0xb0, dscan = 0xb1, cscan = 0xb2,
};

#define NAME(op) { OpCode::op, #op }
const std::unordered_map<OpCode, const char*> nameOfOpCode = {
    NAME(nop),

    NAME(bipush), NAME(ipush),
    NAME(pop),    NAME(pop2), NAME(popn),
    NAME(dup),    NAME(dup2),
    NAME(loadc),  NAME(loada),
    {OpCode::_new, "new"},
    NAME(snew),
        
    NAME(iload),   NAME(dload),   NAME(aload),
    NAME(iaload),  NAME(daload),  NAME(aaload),
    NAME(istore),  NAME(dstore),  NAME(astore),
    NAME(iastore), NAME(dastore), NAME(aastore),
        
    NAME(iadd), NAME(dadd),
    NAME(isub), NAME(dsub),
    NAME(imul), NAME(dmul),
    NAME(idiv), NAME(ddiv),
    NAME(ineg), NAME(dneg),
    NAME(icmp), NAME(dcmp),
        
    NAME(i2d), NAME(d2i), NAME(i2c),
        
    NAME(jmp),
    NAME(je), NAME(jne), NAME(jl), NAME(jge), NAME(jg), NAME(jle),

    NAME(call),
    NAME(ret),
    NAME(iret), NAME(dret), NAME(aret),

    NAME(iprint), NAME(dprint), NAME(cprint), NAME(sprint),
    NAME(printl),
    NAME(iscan),  NAME(dscan),  NAME(cscan),
};
#undef NAME

const std::unordered_map<OpCode, std::vector<int>> paramSizeOfOpCode = {
    { OpCode::bipush, {1} },    { OpCode::ipush, {4} },    
    { OpCode::popn, {4} },
    { OpCode::loadc, {2} },    { OpCode::loada, {2, 4} },
    { OpCode::snew, {4} },
    
    { OpCode::jmp, {2} },
    { OpCode::je, {2} }, { OpCode::jne, {2} }, { OpCode::jl, {2} }, { OpCode::jge, {2} }, { OpCode::jg, {2} }, { OpCode::jle, {2} },

    { OpCode::call, {2} },
};

#define NAME(op) { #op, OpCode::op }
const std::unordered_map<std::string, OpCode> opCodeOfName = {
    NAME(nop),

    NAME(bipush), NAME(ipush),
    NAME(pop),    NAME(pop2), NAME(popn),
    NAME(dup),    NAME(dup2),
    NAME(loadc),  NAME(loada),
    {"new", OpCode::_new},
    NAME(snew),
        
    NAME(iload),   NAME(dload),   NAME(aload),
    NAME(iaload),  NAME(daload),  NAME(aaload),
    NAME(istore),  NAME(dstore),  NAME(astore),
    NAME(iastore), NAME(dastore), NAME(aastore),
        
    NAME(iadd), NAME(dadd),
    NAME(isub), NAME(dsub),
    NAME(imul), NAME(dmul),
    NAME(idiv), NAME(ddiv),
    NAME(ineg), NAME(dneg),
    NAME(icmp), NAME(dcmp),
        
    NAME(i2d), NAME(d2i), NAME(i2c),
        
    NAME(jmp),
    NAME(je), NAME(jne), NAME(jl), NAME(jge), NAME(jg), NAME(jle),

    NAME(call),
    NAME(ret),
    NAME(iret), NAME(dret), NAME(aret),

    NAME(iprint), NAME(dprint), NAME(cprint), NAME(sprint),
    NAME(printl),
    NAME(iscan),  NAME(dscan),  NAME(cscan),
};
#undef NAME

}

#endif
