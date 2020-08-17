
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
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
    R_IP,
    R_COND,
    R_COUNT
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

FILE* binary;

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

void add(uint16_t* a, uint16_t b)
{
    *a += b;
}

void sub(uint16_t* a, uint16_t b)
{
    *a += b;
}

void and(uint16_t* a, uint16_t b)
{
    *a &= b;
} 
 
void or(uint16_t* a, uint16_t b)
{
    *a |= b;
}

void xor(uint16_t* a, uint16_t b)
{
    *a ^= b;
}

void std_op(void(*op)(uint16_t*, uint16_t), int variant)
{
    if (variant == 5) {
        uint16_t imm16;
        fread(&imm16, 2, 1, binary);
        (*op)(&reg[0], imm16);
        reg[R_IP]+=2;
    }
    else if (variant == 4) {
        uint8_t imm8;
        fread(&imm8, 1, 1, binary);
        (*op)(&reg[0], sign_extend(imm8, 8));
        reg[R_IP]++;
    }
    else if (variant < 4) {
        uint8_t modrm;
        fread(&modrm, 1, 1, binary);
        reg[R_IP]++;
        if ((modrm & 0b11000000) == 0b11000000) {
            // TODO: Check that 8 bit carries dont happen
            if (variant == 0) (*op)(&reg[modrm & 0b00000111], reg[modrm & 0b00111000] << 8 >> 8);
            else if (variant == 1) (*op)(&reg[modrm & 0b00000111], reg[modrm & 0b00111000]);
            else if (variant == 2) (*op)(&reg[modrm & 0b00111000], reg[modrm & 0b00000111] << 8 >> 8);
            else if (variant == 3) (*op)(&reg[modrm & 0b00111000], reg[modrm & 0b00000111]);
        }
        else {
            int addr;
            if ((modrm & 0b00000111) == 0b00000111) addr = reg[R_BX];
            else if ((modrm & 0b00000111) == 0b000000101) addr = reg[R_DI];
            else if ((modrm & 0b00000111) == 0b000000100) addr = reg[R_SI];
            else if ((modrm & 0b00000111) == 0b000000011) addr = reg[R_BP] + reg[R_DI];
            else if ((modrm & 0b00000111) == 0b000000010) addr = reg[R_BP] + reg[R_SI];
            else if ((modrm & 0b00000111) == 0b000000001) addr = reg[R_BX] + reg[R_DI];
            else if ((modrm & 0b00000111) == 0b000000000) addr = reg[R_BX] + reg[R_SI];
            if ((modrm & 0b01000000) == 0b01000000) {
                uint8_t disp8;
                fread(&disp8, 1, 1, binary);
                addr += disp8;
                reg[R_IP]++;
            }
            else if ((modrm & 0b10000000) == 0b10000000) {
                uint16_t disp16;
                fread(&disp16, 2, 1, binary);
                addr += disp16;
                reg[R_IP]+=2;
            }
            // Exception to pattern in ModR/M table has to be addressed separately
            else if ((modrm | 0b00111111) == 0b00111111 && (modrm & 0b00000111) == 0b000000110) {
                uint16_t disp16;
                fread(&disp16, 2, 1, binary);
                reg[modrm & 0b00111000] += memory[reg[disp16]];
                reg[R_IP] += 2;
            }
            if (variant == 0) (*op)(&memory[addr], reg[modrm & 0b00111000] << 8 >> 8);
            else if (variant == 1) (*op)(&memory[addr], reg[modrm & 0b00111000]); 
            else if (variant == 2) (*op)(&reg[modrm & 0b00111000], memory[addr] << 8 >> 8);
            else if (variant == 3) (*op)(&reg[modrm & 0b00111000], memory[addr]);
        }
    }
}

void jump(int immsize)
{
    if (immsize == 8) {
        int8_t imm8;
        fread(&imm8, 1, 1, binary);
        fseek(binary, imm8, SEEK_CUR);
        reg[R_IP] -= imm8;
    }
    else if (immsize == 16) {
        int8_t imm16;
        fread(&imm16, 1, 1, binary);
        fseek(binary, imm16, SEEK_CUR);
        reg[R_IP] -= imm16;
    }
}

void push(int reg_num)
{
    reg[R_SP] -= 2;
    memory[reg[R_SP]] = reg[reg_num];
}

void pop(int reg_num)
{
    reg[reg_num] = memory[reg[R_SP]];
    reg[R_SP] += 2;
}

int main(int argc, char** argv)
{
    if (argc < 2) {
        printf("Invalid command: need binary file input!\n");
        return 1;
    }
    // Initialize stack
    reg[R_BP] = rand() % (UINT16_MAX + 1);
    reg[R_SP] = reg[R_BP];
    
    binary = fopen(argv[1], "r");
    //uint8_t signature; // Avoid file signature shifting everything over by one.
    //fread(&signature, 1, 1, binary);
    reg[R_AX] = 1;
    reg[R_CX] = 1;
    for (reg[R_IP] = 0x0000; reg[R_IP] < fsize(argv[1]); reg[R_IP]++) {
        uint8_t op;
        fread(&op, 1, 1, binary);
        printf("Read opcode 0x%02x\n", op);
        switch (op) {
        case 0x00:
            std_op(add, 0);
            break;
        case 0x01:
            std_op(add, 1);
            break;
        case 0x02:
            std_op(add, 2);
            break;
        case 0x03:
            std_op(add, 3);
            break;
        case 0x04:
            std_op(add, 4);
            break;
        case 0x05:
            std_op(add, 5);
            break;
        case 0x08:
            std_op(or, 0);
            break;
        case 0x09:
            std_op(or, 1);
            break;
        case 0x0A:
            std_op(or, 2);
            break;
        case 0x0B:
            std_op(or, 3);
            break;
        case 0x0C:
            std_op(or, 4);
            break;
        case 0x0D:
            std_op(or, 5);
            break;
        case 0x20:
            std_op(and, 0);
            break;
        case 0x21:
            std_op(and, 1);
            break;
        case 0x22:
            std_op(and, 2);
            break;
        case 0x23:
            std_op(and, 3);
            break;
        case 0x24:
            std_op(and, 4);
            break;
        case 0x25:
            std_op(and, 5);
            break;
        case 0xEB:
            jump(8);
            break;
        case 0xE9:
            jump(16);
            break;
        case 0x50:
            push(0);
            break;
        case 0x51:
            push(1);
            break;
        case 0x52:
            push(2);
            break;
        case 0x53:
            push(3);
            break;
        case 0x54:
            push(4);
            break;
        case 0x55:
            push(5);
            break;
        case 0x56:
            push(6);
            break;
        case 0x57:
            push(7);
            break;
        case 0x58:
            pop(0);
            break;
        case 0x59:
            pop(1);
            break;
        case 0x60:
            pop(2);
            break;
        case 0x61:
            pop(3);
            break;
        case 0x62:
            pop(4);
            break;
        case 0x63:
            pop(5);
            break;
        case 0x64:
            pop(6);
            break;
        case 0x65:
            pop(7);
            break; 
        default:
            printf("Bad opcode 0x%02x at 0x%02x! Skipping...\n", op, reg[R_IP]);
            break;
        }
        printf("AX contains the decimal value %d\n", reg[R_AX]);
    }
}
