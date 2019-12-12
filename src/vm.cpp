#include "./vm.h"
#include "./type.h"
#include "./instruction.h"
#include "./exception.h"

#include <iostream>
#include <iomanip>
#include <cmath>

namespace vm {

const addr_t VM::MIN_STACK_ADDR = 0;
const addr_t VM::MAX_STACK_ADDR = 0x00ffffff;
const addr_t VM::MAX_STACK_SIZE = 0x01000000;

const addr_t VM::MIN_HEAP_ADDR  = 0x01000000;
const addr_t VM::MAX_HEAP_ADDR  = 0x01ffffff;
const addr_t VM::MAX_HEAP_SIZE  = 0x01000000;

VM::VM(File file) noexcept : _file(std::move(file)){
    init();
}

std::unique_ptr<VM> VM::make_vm(File file) {
    // found main function
    vm::u4 mainIndex = 0;
    bool mainFound = false;
    for (auto& fun : file.functions) {
        if (0 > fun.nameIndex || fun.nameIndex >= file.constants.size()) {
            throw InvalidFile("function name index out of range");
        }
        if (auto& constant = file.constants.at(fun.nameIndex); constant.type == vm::Constant::Type::STRING) {
            if (std::get<vm::str_t>(constant.value) == "main") {
                file.start.push_back(Instruction{OpCode::snew, fun.paramSize});
                file.start.push_back(Instruction{OpCode::call, mainIndex});
                break;
            }
        }
        else {
            throw InvalidFile("function name not found");
        }
        ++mainIndex;
    }
    if (mainIndex == file.functions.size()) {
        throw InvalidFile("main not found");
    }
    auto vm = std::make_unique<VM>(std::move(file));
    vm->_stack = std::make_unique<slot_t[]>(MAX_STACK_ADDR-MIN_STACK_ADDR);
    vm->_heap  = std::make_unique<slot_t[]>(MAX_HEAP_ADDR-MIN_HEAP_ADDR);
    return std::move(vm);
}

void VM::init() noexcept {
    prepared = false;
    _sp = 0;
    _bp = 0;
    _ip = 0;
    _counterInstruction = 0;
    _contexts.clear();
    _heapRecord.clear();
    _stringLiteralPool.clear();
}

void VM::buildStringLiteralPool() {
    u2 i = 0;
    for (auto it = _file.constants.begin(), ed = _file.constants.end(); it != ed; ++it) {
        auto& c = *it;
        if (c.type == vm::Constant::Type::STRING) {
            str_t str = std::get<str_t>(c.value);
            addr_t addr = NEW(str.length()+1);
            _stringLiteralPool[i] = addr;
            slot_t* dst =  toHeapPtr(addr);
            for (auto ch : str) {
                *dst++ = ch & 0xff;
            }
            *dst = '\0';
        }
        ++i;
    }
}

void VM::start() {
    init();
    buildStringLiteralPool();
    Context globalContext;
    globalContext.prevPC = 0;
    globalContext.prevSP = 0;
    globalContext.prevBP = 0;
    globalContext.BP = 0;
    globalContext.staticLink = 0;
    globalContext.functionIndex = -1;
    globalContext.functionName = "__START__";
    globalContext.functionLevel = 0;
    _currentInstructions = _file.start;
    _contexts.push_back(globalContext);
    prepared = true;
    run();
}

void VM::run() {
    try {
        while (_ip < _currentInstructions.size()) {
            executeInstruction(_currentInstructions.at(_ip));
            ++_ip;
            ++_counterInstruction;
        }
        if (_contexts.size() != 1) {
            // no ret at the end of funtion
            throw InvalidControlTransfer();
        }
    }
    catch (const std::exception& e) {
        println(std::cerr, "runtime error", e.what(), " occurred at:");
        printStackTrace(std::cerr);
    }
}

void VM::printStackTrace(std::ostream& out) {
    auto red = _contexts.rend();
    auto rit = _contexts.rbegin();
    if (red == rit) {
        return;
    }
    auto pc = this->_ip;
    if (pc >= _currentInstructions.size()) {
        println(out, "          function", rit->functionName, "at instruction", pc, ":", _currentInstructions.at(pc));
    }
    else {
        println(out, "          control reaches the end of function", rit->functionName, "without return");
    }
    while (true) {
        pc = rit->prevPC;
        ++rit;
        if (rit == red) {
            return;
        }
        if (rit->functionIndex == -1) {
            println(out, "called by .start at instruction", pc, ":", _file.start.at(pc));
            return;
        }
        println(out, "called by function", rit->functionName, "at instruction", pc, ":", _file.functions.at(rit->functionIndex).instructions.at(pc));
    }
}

void VM::ensureStackRest(addr_t count) {
    if (_sp + count > MAX_STACK_ADDR) {
        throw StackOverflow();
    }
}

void VM::ensureStackUsed(addr_t count) {
    if (_bp + count > _sp) {
        throw InvalidMemoryAccess("tried to modify important stack info");
    }
}

slot_t* VM::toStackPtr(addr_t addr) {
    return _stack.get() + addr;
}
slot_t* VM::toHeapPtr(addr_t addr) {
    return _heap.get() + (addr-MIN_HEAP_ADDR);
}

slot_t* VM::checkAddr(addr_t addr, addr_t count) {
    addr_t end = addr + count;
    if (MIN_STACK_ADDR <= addr && addr < this->_sp) {
        if (end > this->_sp) {
            throw InvalidMemoryAccess("tried to access unused stack memory");
        }
        return toStackPtr(addr);
    }
    if (MIN_HEAP_ADDR <= addr && addr < MAX_HEAP_ADDR) {
        for (auto& p : _heapRecord) {
            if (p.first <= addr && end <= p.first+p.second) {
                return toHeapPtr(addr);
            }
        }
        throw InvalidMemoryAccess("tried to access unused or constant heap memory");
    }
    throw InvalidMemoryAccess("tried to access unexistent memory");
}


void VM::DEC_SP(addr_t count) {
    ensureStackUsed(count);
    _sp -= count;
}

void VM::INC_SP(addr_t count) {
    ensureStackRest(count);
    _sp += count;
}

addr_t VM::NEW(addr_t count) {
    addr_t st = MIN_HEAP_ADDR;
    if (!_heapRecord.empty()) {
        auto& last = _heapRecord.back();
        st = last.first + last.second;
    }
    if (st + count >= MAX_HEAP_ADDR) {
        throw HeapOverflow();
    }
    _heapRecord.emplace_back(st, count);
    return st;
}

void VM::DUP() {
    ensureStackUsed(1);
    ensureStackRest(1);
    _stack[_sp] = _stack[_sp-1];
    ++_sp;
}

void VM::DUP2() {
    ensureStackUsed(2);
    ensureStackRest(2);
    _stack[_sp] = _stack[_sp-2];
    _stack[_sp+1] = _stack[_sp-1];
    _sp += 2;
}

template<>
char_t VM::POP<char_t>() {
    ensureStackUsed(1);
    return static_cast<char_t>(_stack[--_sp]);
}

template<>
int_t VM::POP<int_t>() {
    ensureStackUsed(1);
    return static_cast<int_t>(_stack[--_sp]);
}

template<>
double_t VM::POP<double_t>() {
    ensureStackUsed(2);
    _sp -= 2;
    double_t* p = reinterpret_cast<double_t*>(toStackPtr(_sp));
    return *p;
}

template<>
void VM::PUSH<char_t>(char_t value) {
    ensureStackRest(1);
    _stack[_sp++] = 0x000000ff & value;
}

template<>
void VM::PUSH<int_t>(int_t value) {
    ensureStackRest(1);
    _stack[_sp++] = value;
}

template<>
void VM::PUSH<double_t>(double_t val) {
    ensureStackRest(2);
    double_t* p = reinterpret_cast<double_t*>(_stack.get() + _sp);
    *p = val;
    _sp += 2;
}

template<>
int_t VM::READ<int_t>(addr_t addr) {
    return *reinterpret_cast<int_t*>(checkAddr(addr, 1));
}

template<>
char_t VM::READ<char_t>(addr_t addr) {
    return 0xff & (*reinterpret_cast<int_t*>(checkAddr(addr, 1)));
}

template<>
double_t VM::READ<double_t>(addr_t addr) {
    return *reinterpret_cast<double_t*>(checkAddr(addr, 2));
}

template<>
void VM::WRITE<int_t>(addr_t addr, int_t value) {
    *reinterpret_cast<int_t*>(checkAddr(addr, 1)) = value;
}

template<>
void VM::WRITE<char_t>(addr_t addr, char_t value) {
    *reinterpret_cast<int_t*>(checkAddr(addr, 1)) = 0x000000ff & value;
}


template<>
void VM::WRITE<double_t>(addr_t addr, double_t value) {
    *reinterpret_cast<double_t*>(checkAddr(addr, 2)) = value;
}

void VM::JUMP(u2 offset) {
    if (0 > offset || offset >= _currentInstructions.size()) {
        throw InvalidControlTransfer();
    }
    this->_ip = offset - 1;
}

void VM::CALL(u2 index) {
    if (0 > index || index >= this->_file.functions.size()) {
        throw InvalidControlTransfer();
    }
    Function& calledFunction = this->_file.functions.at(index);
    Context newContext;
    newContext.functionIndex = index;
    newContext.functionName = std::get<str_t>(this->_file.constants.at(calledFunction.nameIndex).value);

    newContext.functionLevel = calledFunction.level;
    int newLv = newContext.functionLevel;
    int curLv = _contexts.back().functionLevel;
    if (newLv == curLv + 1) {
        newContext.staticLink = _contexts.size()-1;
    }
    else if (newLv <= curLv) {
        int staticLink = _contexts.back().staticLink;
        for (; curLv > newLv; --curLv) {
            staticLink = _contexts.at(staticLink).staticLink;
        }
        newContext.staticLink = staticLink;
    }
    else {
        throw InvalidControlTransfer();
    }
    newContext.prevBP = this->_bp;
    newContext.prevPC = this->_ip;
    ensureStackUsed(calledFunction.paramSize);
    this->_bp = this->_sp - calledFunction.paramSize;
    newContext.prevSP = this->_bp;
    newContext.BP = this->_bp;
    _contexts.push_back(newContext);
    this->_ip = -1;
    this->_currentInstructions = calledFunction.instructions;
}

void VM::RET() {
    if (_contexts.size() <= 1) {
        throw InvalidControlTransfer();
    }
    Context curContext = _contexts.back();
    this->_sp = curContext.prevSP;
    this->_bp = curContext.prevBP;
    this->_ip = curContext.prevPC;
    _contexts.pop_back();
    if (_contexts.size() != 1) {
        this->_currentInstructions = _file.functions.at(_contexts.back().functionIndex).instructions;
    }
    else {
        this->_currentInstructions = _file.start;
    }
}

void VM::ipush(int_t value) {
    PUSH(value);
}

void VM::popn(addr_t count) {
    DEC_SP(count);
}

void VM::dup() {
    DUP();
}

void VM::dup2() {
    DUP2();
}

void VM::loadc(u2 index) {
    if (index < 0 || index >= _file.constants.size()) {
        throw;
    }
    auto& constant = _file.constants.at(index);
    switch (constant.type)
    {
    case Constant::Type::STRING: PUSH(_stringLiteralPool.at(index)); break;
    case Constant::Type::INT:    PUSH(std::get<int_t>(constant.value));    break;
    case Constant::Type::DOUBLE: PUSH(std::get<double_t>(constant.value)); break;
    default: throw; break;
    }
}

void VM::loada(u2 level_diff, addr_t offset) {
    int staticLink = _contexts.size()-1;
    for (int ld = level_diff; ld > 0; --ld) {
        staticLink = _contexts.at(staticLink).staticLink;
    }
    addr_t bp = _contexts.at(staticLink).BP;
    PUSH<addr_t>(bp+offset);
}

void VM::_new() {
    PUSH(NEW(POP<int_t>()));
}

void VM::snew(addr_t count) {
    INC_SP(count);
}

template <typename T>
void VM::Tload() {
    PUSH(READ<T>(POP<addr_t>()));
}

template <typename T>
void VM::Taload() {
    addr_t addr = slots_count<T> * POP<addr_t>();
    addr += POP<addr_t>();
    PUSH(READ<T>(addr));
}

template <typename T>
void VM::Tstore() {
    auto value = POP<T>();
    auto addr = POP<addr_t>();
    WRITE(addr, value);
}

template <typename T>
void VM::Tastore() {
    auto value = POP<T>();
    addr_t addr = slots_count<T> * POP<addr_t>();
    addr += POP<addr_t>();
    WRITE(addr, value);
}

template <typename T>
void VM::Tadd() {
    static_assert(std::is_arithmetic_v<T>);
    auto rhs = POP<T>();
    auto lhs = POP<T>();
    PUSH(lhs+rhs);
}

template <typename T>
void VM::Tsub() {
    static_assert(std::is_arithmetic_v<T>);
    auto rhs = POP<T>();
    auto lhs = POP<T>();
    PUSH(lhs-rhs);
}

template <typename T>
void VM::Tmul() {
    static_assert(std::is_arithmetic_v<T>);
    auto rhs = POP<T>();
    auto lhs = POP<T>();
    PUSH(lhs*rhs);
}

template <typename T>
void VM::Tdiv() {
    static_assert(std::is_arithmetic_v<T>);
    auto rhs = POP<T>();
    auto lhs = POP<T>();
    if constexpr (std::is_integral_v<T>) {
        if (rhs == 0) {
            throw DivideByZero();
        }
    }
    PUSH(lhs/rhs);
}

template <typename T>
void VM::Tneg() {
    static_assert(std::is_arithmetic_v<T>);
    PUSH(-POP<T>());
}

template <typename T>
void VM::Tcmp() {
    static_assert(std::is_arithmetic_v<T>);
    auto rhs = POP<T>();
    auto lhs = POP<T>();
    if constexpr (std::is_floating_point_v<T>) {
        if (std::isnan(lhs) || std::isnan(rhs)) {
            PUSH(0);
            return;
        }
        else if (std::isinf(lhs) && std::isinf(rhs) && lhs * rhs > 0) {
            PUSH(0);
            return;
        }
    }
    if (lhs > rhs) {
        PUSH(1);
    }
    else if (lhs < rhs) {
        PUSH(-1);
    }
    else {
        PUSH(0);
    }
}

template <typename T1, typename T2>
void VM::T2T() {
    // static_assert(std::is_arithmetic_v<T1> && std::is_arithmetic_v<T2>);
    static_assert(!std::is_same_v<T1, T2>);
    PUSH(static_cast<T2>(POP<T1>()));
}

void VM::jmp(u2 offset) {
    JUMP(offset);
}

void VM::je(u2 offset) {
    auto cond = POP<int_t>();
    if (cond == 0) {
        JUMP(offset);
    }
}

void VM::jne(u2 offset) {
    auto cond = POP<int_t>();
    if (cond != 0) {
        JUMP(offset);
    }
}

void VM::jl(u2 offset) {
    auto cond = POP<int_t>();
    if (cond < 0) {
        JUMP(offset);
    }
}

void VM::jge(u2 offset) {
    auto cond = POP<int_t>();
    if (cond >= 0) {
        JUMP(offset);
    }
}

void VM::jg(u2 offset) {
    auto cond = POP<int_t>();
    if (cond > 0) {
        JUMP(offset);
    }
}

void VM::jle(u2 offset) {
    auto cond = POP<int_t>();
    if (cond <= 0) {
        JUMP(offset);
    }
}

void VM::call(u2 index) {
    CALL(index);
}

template <typename T>
void VM::Tret() {
    auto rtv = POP<T>();
    RET();
    PUSH(rtv);
}

template <>
void VM::Tret<void>() {
    RET();
}

template <typename T>
void VM::Tprint() {
    auto value = POP<T>();
    if constexpr (std::is_floating_point_v<T>) {
        std::cout << std::fixed << std::setprecision(6) << value;
    }
    else if constexpr (std::is_integral_v<T>) {
        std::cout << value;
    }
}

void VM::sprint() {
    auto str = POP<addr_t>();
    // std::cout << reinterpret_cast<const char*>(str);
    char_t ch;
    while ((ch = READ<char_t>(str++)) != '\0') {
        std::cout << ch;
    }
}

void VM::printl() {
    std::cout << std::endl;
}

template <typename T>
void VM::Tscan() {
    if (T value; std::cin >> value) {
        PUSH(value);
    }
    else {
        throw IOError();
    }
}

void VM::executeInstruction(const Instruction& ins) {
    //println(std::cout, "execute", ins);
    switch (ins.op)
    {
    case OpCode::nop: break;
    case OpCode::bipush:
    case OpCode::ipush:   ipush(ins.x); break;
    case OpCode::pop:     popn(1);      break;
    case OpCode::pop2:    popn(2);      break;
    case OpCode::popn:    popn(ins.x);  break;
    case OpCode::dup:     dup();        break;
    case OpCode::dup2:    dup2();       break;
    case OpCode::loadc:   loadc(ins.x); break;
    case OpCode::loada:   loada(ins.x, ins.y);break;
    case OpCode::_new:    _new();       break;
    case OpCode::snew:    snew(ins.x);  break;
    
    case OpCode::iload:   Tload<int_t>();      break;
    case OpCode::dload:   Tload<double_t>();   break;
    case OpCode::aload:   Tload<addr_t>();     break;
    case OpCode::iaload:  Taload<int_t>();     break;
    case OpCode::daload:  Taload<double_t>();  break;
    case OpCode::aaload:  Taload<addr_t>();    break;
    
    case OpCode::istore:  Tstore<int_t>();     break;
    case OpCode::dstore:  Tstore<double_t>();  break;
    case OpCode::astore:  Tstore<addr_t>();    break;
    case OpCode::iastore: Tastore<int_t>();    break;
    case OpCode::dastore: Tastore<double_t>(); break;
    case OpCode::aastore: Tastore<addr_t>();   break;
    
    case OpCode::iadd:    Tadd<int_t>();       break;
    case OpCode::dadd:    Tadd<double_t>();    break;
    case OpCode::isub:    Tsub<int_t>();       break;
    case OpCode::dsub:    Tsub<double_t>();    break;
    case OpCode::imul:    Tmul<int_t>();       break;
    case OpCode::dmul:    Tmul<double_t>();    break;
    case OpCode::idiv:    Tdiv<int_t>();       break;
    case OpCode::ddiv:    Tdiv<double_t>();    break;
    case OpCode::ineg:    Tneg<int_t>();       break;
    case OpCode::dneg:    Tneg<double_t>();    break;

    case OpCode::icmp:    Tcmp<int_t>();       break;
    case OpCode::dcmp:    Tcmp<double_t>();    break;
    
    case OpCode::i2d:     T2T<int_t, double_t>(); break;
    case OpCode::d2i:     T2T<double_t, int_t>(); break;
    case OpCode::i2c:     T2T<int_t, char_t>();   break;
    
    case OpCode::jmp:     jmp(ins.x);   break;
    case OpCode::je:      je(ins.x);    break;
    case OpCode::jne:     jne(ins.x);   break;
    case OpCode::jl:      jl(ins.x);    break;
    case OpCode::jge:     jge(ins.x);   break;
    case OpCode::jg:      jg(ins.x);    break;
    case OpCode::jle:     jle(ins.x);   break;

    case OpCode::call:    call(ins.x);      break;
    case OpCode::ret:     Tret<void>();     break;
    case OpCode::iret:    Tret<int_t>();    break;
    case OpCode::dret:    Tret<double_t>(); break;
    case OpCode::aret:    Tret<addr_t>();   break;

    case OpCode::iprint:  Tprint<int_t>();    break;
    case OpCode::dprint:  Tprint<double_t>(); break;
    case OpCode::cprint:  Tprint<char_t>();   break;
    case OpCode::sprint:  sprint();           break;
    case OpCode::printl:  printl();           break;
    case OpCode::iscan:   Tscan<int_t>();     break;
    case OpCode::dscan:   Tscan<double_t>();  break;
    case OpCode::cscan:   Tscan<char_t>();    break;
    default:
        break;
    }
}

}