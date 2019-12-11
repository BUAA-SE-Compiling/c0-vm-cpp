#ifndef VM_H_INCLUDED
#define VM_H_INCLUDED

#include "./type.h"
#include "./instruction.h"
#include "./constant.h"
#include "./function.h"
#include "./file.h"

#include <memory>
#include <cstdint>
#include <string>
#include <vector>
#include <variant>

namespace vm {

class VM {
private:
    static const addr_t MIN_STACK_ADDR;
    static const addr_t MAX_STACK_ADDR;
    static const addr_t MAX_STACK_SIZE;
    static const addr_t MIN_HEAP_ADDR;
    static const addr_t MAX_HEAP_ADDR;
    static const addr_t MAX_HEAP_SIZE;

private:
    bool prepared;
    File _file;
    //std::vector<std::shared_ptr<Stack>> stacks;
    std::unique_ptr<slot_t[]> _stack;
    std::unique_ptr<slot_t[]> _heap;
    std::vector<std::pair<addr_t, addr_t>> _heapRecord;
    addr_t _sp;
    addr_t _bp;
    addr_t _ip;
    int _counterInstruction;
    // int _counterMicroIns;
    
    struct Context {
        addr_t prevPC;
        addr_t prevSP;
        addr_t prevBP;
        addr_t BP;
        int staticLink; // index in contexts
        int functionIndex;
        std::string functionName;
        vm::u2 functionLevel;
    };
    std::vector<Context> _contexts;
    std::vector<Instruction> _currentInstructions;
    std::unordered_map<vm::u2, addr_t> _stringLiteralPool;
    
public:
    VM(File) noexcept;
    VM(const VM&) = delete;
    VM(VM&&) = delete;
    VM& operator=(VM) = delete;

public:
    static std::unique_ptr<VM> make_vm(File file);
    void start();

private: 
    void init() noexcept;
    void buildStringLiteralPool();
    void run();
    void ensureStackRest(addr_t count);
    void ensureStackUsed(addr_t count);
    slot_t* checkAddr(addr_t addr, addr_t count);
    slot_t* toHeapPtr(addr_t);
    slot_t* toStackPtr(addr_t);
    void printStackTrace(std::ostream&);

    void    DEC_SP(addr_t count);
    void    INC_SP(addr_t count);
    addr_t  NEW(addr_t count);
    void    DUP();
    void    DUP2();
    template<typename T>
    T       POP();
    template<typename T>
    void    PUSH(T val);
    template<typename T>
    T       READ(addr_t addr);
    template<typename T>
    void    WRITE(addr_t addr, T value);

    void    JUMP(u2 offset);
    void    CALL(u2 index);
    void    RET();

private:
    void executeInstruction(const Instruction&);

    void ipush(int_t value);
    void popn(addr_t count);
    void dup(); void dup2();
    void loadc(u2 index);
    void loada(u2 level_diff, addr_t offset);
    
    void _new();
    void snew(addr_t count);
    
    template<typename T>
    void Tload();
    template<typename T>
    void Taload();
    template<typename T>
    void Tstore();
    template<typename T>
    void Tastore();

    template <typename T>
    void Tadd();
    template <typename T>
    void Tsub();
    template <typename T>
    void Tmul();
    template <typename T>
    void Tdiv();
    template <typename T>
    void Tneg();
    template <typename T>
    void Tcmp();

    template <typename T1, typename T2>
    void T2T();

    void jmp(u2 offset);
    void je(u2 offset); void jne(u2 offset); 
    void jl(u2 offset); void jge(u2 offset); 
    void jg(u2 offset); void jle(u2 offset);

    void call(u2 index);
    template <typename T>
    void Tret();
    
    template <typename T> 
    void Tprint();
    void sprint(); 
    void printl();
    template <typename T>
    void Tscan();
};

}


#endif
