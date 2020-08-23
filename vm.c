
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/stat.h>
#include <math.h>
#include <sys/types.h>
#include <assert.h>

// Hacky %b for printf from stackoverflow
//https://stackoverflow.com/questions/111928/is-there-a-printf-converter-to-print-in-binary-format
//https://stackoverflow.com/questions/111928/is-there-a-printf-converter-to-print-in-binary-format/25108449#25108449
/* --- PRINTF_BYTE_TO_BINARY macro's --- */
#define PRINTF_BINARY_SEPARATOR " "
#define PRINTF_BINARY_PATTERN_INT8 "%c%c%c%c%c%c%c%c"
#define PRINTF_BYTE_TO_BINARY_INT8(i)    \
    (((i) & 0x80ll) ? '1' : '0'), \
    (((i) & 0x40ll) ? '1' : '0'), \
    (((i) & 0x20ll) ? '1' : '0'), \
    (((i) & 0x10ll) ? '1' : '0'), \
    (((i) & 0x08ll) ? '1' : '0'), \
    (((i) & 0x04ll) ? '1' : '0'), \
    (((i) & 0x02ll) ? '1' : '0'), \
    (((i) & 0x01ll) ? '1' : '0')

#define PRINTF_BINARY_PATTERN_INT16 \
    PRINTF_BINARY_PATTERN_INT8               PRINTF_BINARY_SEPARATOR              PRINTF_BINARY_PATTERN_INT8
#define PRINTF_BYTE_TO_BINARY_INT16(i) \
    PRINTF_BYTE_TO_BINARY_INT8((i) >> 8),   PRINTF_BYTE_TO_BINARY_INT8(i)
#define PRINTF_BINARY_PATTERN_INT32 \
    PRINTF_BINARY_PATTERN_INT16              PRINTF_BINARY_SEPARATOR              PRINTF_BINARY_PATTERN_INT16
#define PRINTF_BYTE_TO_BINARY_INT32(i) \
    PRINTF_BYTE_TO_BINARY_INT16((i) >> 16), PRINTF_BYTE_TO_BINARY_INT16(i)
#define PRINTF_BINARY_PATTERN_INT64    \
    PRINTF_BINARY_PATTERN_INT32              PRINTF_BINARY_SEPARATOR              PRINTF_BINARY_PATTERN_INT32
#define PRINTF_BYTE_TO_BINARY_INT64(i) \
    PRINTF_BYTE_TO_BINARY_INT32((i) >> 32), PRINTF_BYTE_TO_BINARY_INT32(i)
/* --- end macros --- */

#define MAX_INPUT 100
#define MAX_DISPLAY 100

// Define registers
enum
{
    R_RAX = 0,
    R_RCX,
    R_RDX,
    R_RBX,
    R_RSP,
    R_RBP,
    R_RSI,
    R_RDI,
    R_RIP,
    R_R8,
    R_R9,
    R_R10,
    R_R11,
    R_R12,
    R_R13,
    R_R14,
    R_R15,
    R_FLAGS,
    R_COUNT
};

const char* reg_name[R_COUNT] = {"RAX", "RCX", "RDX", "RBX", "RSP", "RBP", "RSI", "RDI", "RIP", "R8", "R9", "R10", "R11", "R12", "R13", "R14", "R15", "FLAGS"};

// Enum for flags
// Decimal is simpler to write than 16-bit binary...
enum
{
    FL_CF = 0,
    FL_PF = 4,
    FL_AF = 16,
    FL_ZF = 64,
    FL_SF = 128,
    FL_TF = 256,
    FL_IF = 512,
    FL_DF = 1024,
    FL_OF = 2048
};

#define FL_COUNT 9
const char* flag_name[FL_COUNT] = {"CF", "PF", "AF", "ZF", "SF", "TF", "IF", "DF", "OF"};

// Staying 16 bit for now
uint64_t memory[UINT16_MAX+2];
uint64_t reg[R_COUNT];

FILE* binary;

// Simple file size detector via https://stackoverflow.com/questions/8236/how-do-you-determine-the-size-of-a-file-in-c
off_t fsize(const char *filename) {
    struct stat st; 
    if (stat(filename, &st) == 0)
        return st.st_size;
    return -1; 
}

// Update flags based on operation result
void update_flags(uint64_t r)
{
    if (r % 2 == 0) reg[R_FLAGS] |= FL_PF;
    else reg[R_FLAGS] &= ~FL_PF;
    if (r == 0) reg[R_FLAGS] |= FL_ZF;
    else reg[R_FLAGS] &= ~FL_ZF;
    if (r < 0) reg[R_FLAGS] |= FL_SF;
    else reg[R_FLAGS] &= ~FL_SF;
}

// Extend an n-bit number to a 64 bit number
uint64_t sign_extend(uint64_t x, int bit_count)
{
    if ((x >> (bit_count - 1)) & 1) {
        x |= ((int)pow(2, 64) << bit_count);
    }
    return x;
}

void add(uint64_t* a, uint64_t b, size_t portion)
{
    uint64_t a_portion = *a & (uint64_t)pow(2, portion)-1;
    a_portion += (b & (uint64_t)pow(2, portion)-1);
    update_flags(a_portion);
    if (portion != 64) *a = *a & ((uint64_t)pow(2, portion)-1) << portion;
    else *a &= 0;
    *a |= a_portion;
}

void sub(uint64_t* a, uint64_t b, size_t portion)
{
    uint64_t a_portion = *a & (uint64_t)pow(2, portion)-1;
    a_portion += b & (uint64_t)pow(2, portion)-1;
    update_flags(a_portion);
    if (portion != 64) *a = *a & ((uint64_t)pow(2, portion)-1) << portion;
    else *a &= 0;
    *a |= a_portion;
}

void mul(uint64_t* a, uint64_t b, size_t portion)
{
    uint64_t a_portion = *a & (uint64_t)pow(2, portion)-1;
    a_portion *= b & (uint64_t)pow(2, portion)-1;
    update_flags(a_portion);
    if (portion != 64) *a = *a & ((uint64_t)pow(2, portion)-1) << portion;
    else *a &= 0;
    *a |= a_portion;
}

void udiv(uint64_t* a, uint64_t b, size_t portion)
{
    uint64_t a_portion = *a & (uint64_t)pow(2, portion)-1;
    a_portion /= b & (uint64_t)pow(2, portion)-1;
    update_flags(a_portion);
    if (portion != 64) *a = *a & ((uint64_t)pow(2, portion)-1) << portion;
    else *a &= 0;
    *a |= a_portion;
}

void and(uint64_t* a, uint64_t b, size_t portion)
{
    uint64_t a_portion = *a & (uint64_t)pow(2, portion)-1;
    a_portion &= b & (uint64_t)pow(2, portion)-1;
    update_flags(a_portion);
    if (portion != 64) *a = *a & ((uint64_t)pow(2, portion)-1) << portion;
    else *a &= 0;
    *a |= a_portion;
} 
 
void or(uint64_t* a, uint64_t b, size_t portion)
{
    uint64_t a_portion = *a & (uint64_t)pow(2, portion)-1;
    a_portion |= b;
    update_flags(a_portion);
    if (portion != 64) *a = *a & ((uint64_t)pow(2, portion)-1) << portion;
    else *a &= 0;
    *a |= a_portion;
}

void xor(uint64_t* a, uint64_t b, size_t portion)
{
    uint64_t a_portion = *a & (uint64_t)pow(2, portion)-1;
    a_portion ^= b & (uint64_t)pow(2, portion)-1;
    update_flags(a_portion);
    if (portion != 64) *a = *a & ((uint64_t)pow(2, portion)-1) << portion;
    else *a &= 0;
    *a |= a_portion;
}

void cmp(uint64_t* a, uint64_t b, size_t portion)
{
    uint64_t a_portion = *a & (uint64_t)pow(2, portion)-1;
    a_portion -= b & (uint64_t)pow(2, portion)-1;
    update_flags(a_portion);
}

// ONLY WORKS FOR 16 BIT
int get_addr_16bit(uint8_t modrm)
{
    int addr;
    if ((modrm & 0b00000111) == 0b00000111) addr = reg[R_RBX] & 0xFFFF;
    else if ((modrm & 0b00000111) == 0b000000101) addr = reg[R_RDI] & 0xFFFF;
    else if ((modrm & 0b00000111) == 0b000000100) addr = reg[R_RSI] & 0xFFFF;
    else if ((modrm & 0b00000111) == 0b000000011) addr = (reg[R_RBP] + reg[R_RDI]) & 0xFFFF;
    else if ((modrm & 0b00000111) == 0b000000010) addr = (reg[R_RBP] + reg[R_RSI]) & 0xFFFF;
    else if ((modrm & 0b00000111) == 0b000000001) addr = (reg[R_RBX] + reg[R_RDI]) & 0xFFFF;
    else if ((modrm & 0b00000111) == 0b000000000) addr = (reg[R_RBX] + reg[R_RSI]) & 0xFFFF;
    if ((modrm & 0b01000000) == 0b01000000) {
        uint8_t disp8;
        fread(&disp8, 1, 1, binary);
        addr += disp8;
        reg[R_RIP]++;
    }
    else if ((modrm & 0b10000000) == 0b10000000) {
        uint16_t disp16;
        fread(&disp16, 2, 1, binary);
        addr += disp16;
        reg[R_RIP]+=2;
    }
    // Exception to pattern in ModR/M table has to be addressed separately
    else if ((modrm | 0b00111111) == 0b00111111 && (modrm & 0b00000111) == 0b000000110) {
        uint16_t disp16;
        fread(&disp16, 2, 1, binary);
        addr = reg[disp16];
        reg[R_RIP] += 2;
    }
    return addr;
}

int get_addr(uint8_t modrm)
{
    int addr;
    uint8_t rm = modrm & 0b00000111;
    uint8_t mod = modrm & 0b11000000;
    if ((prefix & 0b01000001) != 0b01000000 &&) rm = (rm << 1) + 1;
    // See table at wiki.osdev.org/X86-64_Instruction_Encoding to understand why this madness exists
    if (rm < 4 || (rm > 9 && rm < 12) || rm > 13) addr = reg[rm];
    else if (rm == 8 || rm == 12) {
        uint8_t sib;
        fread(&sib, 1, 1, binary);
        reg[R_RIP]++;
        uint8_t scale = sib & 0b11000000;
        uint8_t index = sib & 0b00111000;
        uint8_t base = sib & 0b00000111;
        if ((prefix & 0b01000010) != 0b01000000 &&) index = (index << 1) + 1;
        if ((prefix & 0b01000001) != 0b01000000 &&) base = (base << 1) + 1;
        if (mod == 0) {
            if (index == 2) {
                if (base == 5 || base == 13);
                else
            }
        }
        else addr = reg[base] + (reg[index] * scale);
            
        if (index < 4) addr = reg[base] + (reg[index] * scale);
    }
    else if ((rm == 13 || rm == 9) && mod = 0) addr = reg[rm]
    
}

void std_op(uint8_t prefix, void(*op)(uint64_t*, uint64_t, size_t), int variant)
{
    if (variant == 5) {
        if (prefix == 0x66) {
            uint16_t imm16;
            fread(&imm16, 2, 1, binary);
            (*op)(&reg[0], sign_extend(imm16, 16), 16);
            reg[R_RIP]+=2;
        }
        else {
            uint32_t imm32;
            fread(&imm32, 4, 1, binary);
            (*op)(&reg[0], sign_extend(imm32, 32), 32);
            reg[R_RIP]+=4;
        }
        update_flags(reg[0]);
    }
    else if (variant == 4) {
        uint8_t imm8;
        fread(&imm8, 1, 1, binary);
        (*op)(&reg[0], sign_extend(imm8, 8), 8);
        reg[R_RIP]++;
        update_flags(reg[0]);
    }
    else if (variant < 4) {
        uint8_t modrm;
        fread(&modrm, 1, 1, binary);
        reg[R_RIP]++;
        size_t portion;
        if ((prefix & 0b01001000) == prefix) portion = 64;
        else if (prefix == 0x66) portion = 16;
        else if (variant == 0 || variant == 2) portion = 8;
        else portion = 32;
        if ((modrm & 0b11000000) == 0b11000000) {
            // TODO: Check that 8 bit carries dont happen
            int op1 = modrm & 0b00111000, op2 = modrm & 0b00000111;
            if ((prefix & 0b01000100) != 0b01000000) op1 = (op1 << 1) + 1;
            if ((prefix & 0b01000100) != 0b01000000) op2 = (op2 << 1) + 1;
            if (variant < 2) (*op)(&reg[op2], reg[op1], portion);
            else (*op)(&reg[op1], reg[op2], portion);
        }
        else {
            // TODO: Actually implement 64 bit addressing
            int addr = get_addr_16bit(modrm);
            if (variant == 0) (*op)(&memory[addr], reg[modrm & 0b00111000] & 0xFF, portion);
            else if (variant == 1) (*op)(&memory[addr], reg[modrm & 0b00111000], portion); 
            else if (variant == 2) (*op)(&reg[modrm & 0b00111000], memory[addr] & 0xFF, portion);
            else if (variant == 3) (*op)(&reg[modrm & 0b00111000], memory[addr], portion);

        }
    }
}

void jump(bool condition, int immsize, bool far)
{
    if (!condition) {
        reg[R_RIP]+=immsize/8;
        fseek(binary, immsize/8, SEEK_CUR);
        return;
    }
    int loc = SEEK_CUR;
    if (far) loc = SEEK_SET;
    if (immsize == 8) {
        int8_t imm8;
        fread(&imm8, 1, 1, binary);
        reg[R_RIP]++;
        fseek(binary, imm8, loc);
        reg[R_RIP] += imm8;
    }
    else if (immsize == 16) {
        int16_t imm16;
        fread(&imm16, 2, 1, binary);
        reg[R_RIP]+=2;
        fseek(binary, imm16, loc);
        reg[R_RIP] += imm16;
    }
    // TODO: Add register JMP instructions
}

void push(int reg_num)
{
    reg[R_RSP] -= 1;
    memory[reg[R_RSP]] = reg[reg_num];
}

void pop(int reg_num)
{
    reg[reg_num] = memory[reg[R_RSP]];
    reg[R_RSP] += 1;
}

#include "read.c"

int run(char* filename, long offset)
{
    // Initialize stack
    reg[R_RBP] = rand() % (UINT16_MAX + 1);
    reg[R_RSP] = reg[R_RBP];
    binary = fopen(filename, "r");
    fseek(binary, offset, SEEK_SET);
    for (reg[R_RIP] = offset; reg[R_RIP] < fsize(filename);) {
        if (step(true, true) == 1) return 1;
    }
    return 0;
}

int main(int argc, char** argv)
{
    bool imode = false;
    if (argc < 3) {
        printf("Invalid command: need binary file input + entry offset!\n");
        return 1;
    }
    for (int i = 0; i < argc; i++) if (strcmp(argv[i], "-i") == 0) imode = 1;
    if (imode) {
        bool init = false;
        rl_bind_key('\t', rl_insert);
        printf("\nVM 0.001 interactive interface\n-\n");
        while (1) {
            char* msg = "default";
            msg = readline ("\033[1;36m(vm)\033[0m ");
            if (strlen(msg) > 0) add_history(msg);
            if (strcmp(msg, "run") == 0) {
                run(argv[1], strtol(argv[2], NULL, 10));
                printf("Execution complete!\n");
            }
            else if (strcmp(msg, "specs") == 0) {
                printf("CPU type: 16 bit\n");
                printf("Memory: %d KB\n", (int)round((float)(2*UINT16_MAX)/1024));
                printf("# of supported opcodes: 82\n"); // TODO: Fix specs code
            }
            else if (strcmp(msg, "registers") == 0) {
                printf("       Hex              │ base10               │ Binary\n");
                for (int i = 0; i < R_COUNT; i++) printf("%5s: %16llx │ %20llu │ "PRINTF_BINARY_PATTERN_INT64"\n", reg_name[i], reg[i], reg[i], PRINTF_BYTE_TO_BINARY_INT64(reg[i]));
            }
            else if (strcmp(msg, "flags") == 0) {
                printf("Full FLAG register: "PRINTF_BINARY_PATTERN_INT64"\n", PRINTF_BYTE_TO_BINARY_INT64(reg[R_FLAGS]));
                printf("PF: %d\n", (reg[R_FLAGS] & FL_PF) == FL_PF);
                printf("ZF: %d\n", (reg[R_FLAGS] & FL_ZF) == FL_ZF);
                printf("SF: %d\n", (reg[R_FLAGS] & FL_SF) == FL_SF);
            }
            else if (strcmp(msg, "stack") == 0) {
                for (int i = reg[R_RSP]; i < reg[R_RBP]; i+=1) {
                    printf("        --------\n");
                    printf("0x%04x:  0x%04llx ", i, memory[i]);
                    for (int j = 0; j < R_COUNT; j++) if (reg[j] == i) printf(" <- %s", reg_name[j]);
                    printf("\n");
                }
                printf("        --------\n");
                printf("0x%016llx:  0x%016llx ", reg[R_RBP], memory[reg[R_RBP]]);
                printf(" <- BP");
                printf("\n        --------\n");
            }
            else if (strcmp(msg, "step") == 0) {
                if (!init) {
                    reg[R_RBP] = rand() % (UINT16_MAX + 1);
                    reg[R_RSP] = reg[R_RBP];
                    binary = fopen(argv[1], "r");
                    long offset = strtol(argv[2], NULL, 10);
                    fseek(binary, offset, SEEK_SET);
                    reg[R_RIP] = offset;
                    printf("Initialized program.\n");
                    init = true;
                }
                else {
                    if (reg[R_RIP] < fsize(argv[1])) {
                        if(step(true, true) == 1) {
                            printf("Program return.\n");
                            init = false;
                        }
                    }
                    else {
                        printf("Program end.\n");
                        init = false;
                    }
                }
            }
            else if (strcmp(msg, "quit") == 0 || strcmp(msg, "exit") == 0 || strcmp(msg, "q") == 0) exit(1);
        }
    }
    else run(argv[1], strtol(argv[2], NULL, 10));
}
