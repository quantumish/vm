

#include <stdio.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>

// Hacky %b for printf from stackoverflow
//https://stackoverflow.com/questions/111928/is-there-a-printf-converter-to-print-in-binary-format
#define BYTE_TO_BINARY_PATTERN "%c%c%c%c%c%c%c%c"
#define BYTE_TO_BINARY(byte)  \
  (byte & 0x80 ? '1' : '0'), \
  (byte & 0x40 ? '1' : '0'), \
  (byte & 0x20 ? '1' : '0'), \
  (byte & 0x10 ? '1' : '0'), \
  (byte & 0x08 ? '1' : '0'), \
  (byte & 0x04 ? '1' : '0'), \
  (byte & 0x02 ? '1' : '0'), \
  (byte & 0x01 ? '1' : '0') 

// Define registers
enum
{
    R_AX = 0,
    R_CX,
    R_DX,
    R_BX,
    R_SP,
    R_BP,
    R_SI,
    R_DI,
    R_PC,
    R_COND,
    R_COUNT
};

// Define opcodes
enum
{
    OP_ADD = 0x01,
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

void update_flags(uint16_t r)
{
    if (reg[r] == 0) {
        reg[R_COND] = FL_ZRO;
    }
    else if (reg[r] >> 15) { // TODO: Look into how negative numbers work for x86
        reg[R_COND] = FL_NEG;
    }
    else {
        reg[R_COND] = FL_POS;
    }
}

uint16_t sign_extend(uint16_t x, int bit_count)
{
    if ((x >> (bit_count - 1)) & 1) {
        x |= (0xFFFF << bit_count);
    }
    return x;
}

void add(FILE* binary, int variant)
{
    if (variant < 2) {
        uint8_t modrm;
        fread(&modrm, 1, 1, binary);
        if ((modrm & 0b11000000) == 0b11000000) {
            if (variant == 1) reg[modrm & 0b00111000] += reg[modrm & 0b00000111];
            if (variant == 0) {
                // TODO: Check that 8 bit carries dont happen
                reg[modrm & 0b00111000] += reg[modrm & 0b00000111] << 8 >> 8;
            }
        }
        else printf("Indirect adressing not supported yet. Skipping operation.\n");
    }
    else if (variant == 4) {
        uint16_t imm16;
        fread(&imm16, 1, 1, binary);
        reg[0] += imm16;
    }
    else if (variant == 5) {
        uint8_t imm8;
        fread(&imm8, 1, 1, binary);
        reg[0] += sign_extend(imm8, 8);
    }
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("Invalid command: need binary file input!\n");
        return 1;
    }
    FILE* binary = fopen(argv[1], "r");
    reg[R_PC] = 0x0000; // init program counter
    //uint8_t signature; // Avoid file signature shifting everything over by one.
    //fread(&signature, 1, 1, binary);
    reg[R_AX] = 1;
    reg[R_CX] = 1;
    for (int i = 0; i < fsize(argv[1]); i++) {
        reg[R_PC]++;
        uint8_t op;
        fread(&op, 1, 1, binary);
        printf("Read opcode 0x%02x\n", op);
        switch (op) {
        case 0x00:
            add(binary,0);
            i++;
            break;
        case 0x01:
            add(binary,1);
            i++;
            break;
        case 0x04:
            add(binary,4);
            i+=2;
            break;
        case 0x05:
            add(binary,5);
            i++;
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
            printf("Bad opcode 0x%02x at 0x%02x! Skipping...\n", op, i);
            break;
        }
        printf("AX contains the decimal value %d\n", reg[R_AX]);
    }
}
