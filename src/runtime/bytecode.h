#ifndef TICK_BYTECODE_H
#define TICK_BYTECODE_H

#include <cstdint>

namespace Tick {

enum class OpCode : uint8_t {
    LOAD_CONST,
    LOAD_VAR,
    STORE_VAR,
    LOAD_GLOBAL,
    STORE_GLOBAL,
    
    ADD,
    SUB,
    MUL,
    DIV,
    MOD,
    CONCAT,
    
    EQ,
    NEQ,
    LT,
    GT,
    LTE,
    GTE,
    
    AND,
    OR,
    NOT,
    NEG,
    
    JUMP,
    JUMP_IF_FALSE,
    JUMP_IF_TRUE,
    
    CALL,
    RETURN,
    
    SIGNAL_EMIT,
    SIGNAL_RECV,
    EVENT_EXECUTE,
    
    POP,
    DUP,
    
    HALT
};

struct Instruction {
    OpCode opcode;
    int operand;
    
    Instruction() : opcode(OpCode::HALT), operand(0) {}
    Instruction(OpCode op, int opr = 0) : opcode(op), operand(opr) {}
};

}

#endif
