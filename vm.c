

#include <stdio.h>
#include <stdint.h>

// Define registers
enum
{
    R_R0 = 0,
    R_R1,
    R_R2,
    R_R3,
    R_R4,
    R_R5,
    R_R6,
    R_R7,
    R_PC,
    R_COND,
    R_COUNT
};

// Define opcodes
enum
{
    OP_ADD = 0x00,
    OP_LDR = 0x10, // Imaginary instruction that is basically just MOV r/m16 r16
    OP_STR = 0x11, // Imaginary instruction that is basically just MOV r/m16 r16
    OP_AND = 0x20, // TODO: Figure out right opcode for AND
    OP_NOT = 0x13, // TODO: Address two-byte opcode problem 
    OP_JMP = 0xe9
};

// Define condition flag enums
enum
{
    FL_POS = 1 << 0,
    FL_ZRO = 1 << 1,
    FL_NEG = 1 << 2
};
