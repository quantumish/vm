

#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

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

// Staying 16 bit for now
uint16_t memory[UINT16_MAX+1];
uint16_t reg[R_COUNT];

// Simple file size detector via https://stackoverflow.com/questions/8236/how-do-you-determine-the-size-of-a-file-in-c
off_t fsize(const char *filename) {
    struct stat st; 
    if (stat(filename, &st) == 0)
        return st.st_size;
    return -1; 
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("Invalid command: need binary file input!\n");
        return 1;
    }
    FILE* binary = fopen(argv[1], "r");
    reg[R_PC] = 0x0000; // init program counter
    for (int i = 0; i < fsize(argv[1]); i++) {
        reg[R_PC]++;
        uint16_t op;
        fread(&op, 2, 1, binary);
        switch (op) {
        case OP_ADD:
            break;
        case OP_LDR:
            break;
        case OP_STR:
            break;
        case OP_AND:
            break;
        case OP_NOT:
            break;
        case OP_JMP:
            break;
        default:
            printf("Bad opcode at %x! Skipping...", i);
            break;
        }
    }
}
