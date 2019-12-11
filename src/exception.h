#ifndef EXCEPTION_H_INCLUDED
#define EXCEPTION_H_INCLUDED

#include "./type.h"

#include <exception>
#include <string>

class InvalidFile : public std::exception {
public:
    InvalidFile(std::string msg) : msg(std::move(msg)) {}
    virtual ~InvalidFile() {}
    virtual const char* what() const noexcept {
        return msg.c_str();
    }
private:
    std::string msg;
};

class InCompleteFile : public InvalidFile {
public:
    InCompleteFile() : InvalidFile("incomplete input file") {}
    virtual ~InCompleteFile() {}
};

namespace vm {
    
class InvalidMemoryAccess : public std::exception {
public:
    InvalidMemoryAccess(std::string msg) : msg(msg) {}
    virtual ~InvalidMemoryAccess() {}
    virtual const char* what() const noexcept {
        return msg.c_str();
    }
private:
    std::string msg;
};

class StackOverflow : public std::exception {
public:
    StackOverflow() {}
    virtual ~StackOverflow() {}
    virtual const char* what() const noexcept {
        return "stack overflow";
    }
};

class HeapOverflow : public std::exception {
public:
    HeapOverflow() {}
    virtual ~HeapOverflow() {}
    virtual const char* what() const noexcept {
        return "heap overflow";
    }
};

class InvalidInstruction : public std::exception {
public:
    InvalidInstruction() {}
    virtual ~InvalidInstruction() {}
    virtual const char* what() const noexcept {
        return "invalid instruction";
    }
};

class DivideByZero : public std::exception {
public:
    DivideByZero() {}
    virtual ~DivideByZero() {}
    virtual const char* what() const noexcept {
        return "divide integer by zero";
    }
};

class InvalidControlTransfer : public std::exception {
public:
    InvalidControlTransfer() {}
    virtual ~InvalidControlTransfer() {}
    virtual const char* what() const noexcept {
        return "invalid control transfer";
    }
};

class IOError : public std::exception {
public:
    IOError() {}
    virtual ~IOError() {}
    virtual const char* what() const noexcept {
        return "I/O error";
    }
};

}

#endif