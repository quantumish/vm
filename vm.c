
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/stat.h>
#include <sys/types.h>

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
    R_AX = 0,
    R_CX,
    R_DX,
    R_BX,
    R_SP,
    R_BP,
    R_SI,
    R_DI,
    R_IP,
    R_FLAGS,
    R_COUNT
};

const char* reg_name[R_COUNT] = {"AX", "CX", "DX", "BX", "SP", "BP", "SI", "DI", "IP", "FLAGS"};

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
    if (r % 2 == 0) reg[R_FLAGS] |= FL_PF;
    else reg[R_FLAGS] &= ~FL_PF;
    if (r == 0) reg[R_FLAGS] |= FL_ZF;
    else reg[R_FLAGS] &= ~FL_ZF;
    if (r < 0) reg[R_FLAGS] |= FL_SF;
    else reg[R_FLAGS] &= ~FL_SF;
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

void mul(uint16_t* a, uint16_t b)
{
    printf("%d * %d\n", *a, b);
    *a *= b;
}

void udiv(uint16_t* a, uint16_t b)
{
    *a /= b;
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

void cmp(int immsize)
{
    if (immsize == 8) {
        int8_t imm8;
        fread(&imm8, 1, 1, binary);
        reg[R_IP]++;
        //printf("%d - %d = %d\n", (reg[R_AX] << 8 >> 8), sign_extend(imm8, 8), (reg[R_AX] << 8 >> 8) - sign_extend(imm8, 8));
        update_flags((reg[R_AX] << 8 >> 8) - sign_extend(imm8, 8));
    }
    else if (immsize == 16) {
        int16_t imm16;
        fread(&imm16, 2, 1, binary);
        reg[R_IP]+=2;
        update_flags(reg[R_AX] - imm16);
    }
}

int get_addr(uint8_t modrm)
{
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
        addr = reg[disp16];
        reg[R_IP] += 2;
    }
    return addr;
}

void ax_op(void(*op)(uint16_t*, uint16_t), int regsize, uint8_t modrm)
{
    printf("Hiya\n");
    if ((modrm & 0b11000000) == 0b11000000) {
        // TODO: Check that 8 bit carries dont happen
        if (regsize == 8) (*op)(&reg[0], reg[modrm & 0b00111000] << 8 >> 8);
        else (*op)(&reg[0], reg[modrm & 0b00111000]);
    }
    else {
        int addr = get_addr(modrm);
        if (regsize == 8) (*op)(&reg[0], memory[addr] << 8 >> 8);
        else (*op)(&reg[0], memory[addr]);
    }
    update_flags(reg[0]);
}

void std_op(void(*op)(uint16_t*, uint16_t), int variant)
{
    if (variant == 5) {
        uint16_t imm16;
        fread(&imm16, 2, 1, binary);
        (*op)(&reg[0], imm16);
        reg[R_IP]+=2;
        update_flags(reg[0]);
    }
    else if (variant == 4) {
        uint8_t imm8;
        fread(&imm8, 1, 1, binary);
        (*op)(&reg[0], sign_extend(imm8, 8));
        reg[R_IP]++;
        update_flags(reg[0]);
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
            if (variant == 0 || variant == 1) update_flags(reg[modrm & 0b00000111]);
            else update_flags(reg[modrm & 0b00111000]);
        }
        else {
            int addr = get_addr(modrm);
            if (variant == 0) (*op)(&memory[addr], reg[modrm & 0b00111000] << 8 >> 8);
            else if (variant == 1) (*op)(&memory[addr], reg[modrm & 0b00111000]); 
            else if (variant == 2) (*op)(&reg[modrm & 0b00111000], memory[addr] << 8 >> 8);
            else if (variant == 3) (*op)(&reg[modrm & 0b00111000], memory[addr]);
            if (variant == 0 || variant == 1) update_flags(reg[modrm & 0b00111000]);
            else update_flags(memory[addr]);
        }
    }
}

void jump(bool condition, int immsize, bool far)
{
    printf("condition %d (t %d f %d)\n", condition, true, false);
    if (!condition) {
        reg[R_IP]+=immsize/8;
        fseek(binary, immsize/8, SEEK_CUR);
        return;
    }
    int loc = SEEK_CUR;
    if (far) loc = SEEK_SET;
    if (immsize == 8) {
        int8_t imm8;
        fread(&imm8, 1, 1, binary);
        reg[R_IP]++;
        fseek(binary, imm8, loc);
        reg[R_IP] += imm8;
    }
    else if (immsize == 16) {
        int16_t imm16;
        fread(&imm16, 2, 1, binary);
        reg[R_IP]+=2;
        fseek(binary, imm16, loc);
        reg[R_IP] += imm16;
    }
    // TODO: Add register JMP instructions
}

void push(int reg_num)
{
    reg[R_SP] -= 1;
    memory[reg[R_SP]] = reg[reg_num];
}

void pop(int reg_num)
{
    reg[reg_num] = memory[reg[R_SP]];
    reg[R_SP] += 1;
}

#include "read.c"

int run(char* filename)
{
    // Initialize stack
    reg[R_BP] = rand() % (UINT16_MAX + 1);
    reg[R_SP] = reg[R_BP];
    reg[R_CX] = 2;
    binary = fopen(filename, "r");
    //uint8_t signature; // Avoid file signature shifting everything over by one.
    //fread(&signature, 1, 1, binary);
    for (reg[R_IP] = 0x0000; reg[R_IP] < fsize(filename);) {
        step(false);
    }
    return 0;
}

int main(int argc, char** argv)
{
    bool imode = false;
    if (argc < 2) {
        printf("Invalid command: need binary file input!\n");
        return 1;
    }
    for (int i = 0; i < argc; i++) if (strcmp(argv[i], "-i") == 0) imode = 1;
    if (imode) {
        char msg[MAX_INPUT] = "";
        bool init = false;
        printf("VM 0.001 interactive interface\n-\n");
        while (1) {
            printf("(vm) ");
            fgets(msg, MAX_INPUT, stdin);
            char* command = strtok(msg," ");
            if (strcmp(command, "run\n") == 0) run(argv[1]);
            else if (strcmp(command, "registers\n") == 0) {
                printf("       Hex    │ base10 │ Binary\n");
                for (int i = 0; i < R_COUNT; i++) printf("%5s: 0x%04x │ %05d  │ "PRINTF_BINARY_PATTERN_INT16"\n", reg_name[i], reg[i], reg[i], PRINTF_BYTE_TO_BINARY_INT16(reg[i]));
            }
            else if (strcmp(command, "flags\n") == 0) {
                printf("PF: %d\n", (reg[R_FLAGS] & FL_PF) == FL_PF);
                printf("ZF: %d\n", (reg[R_FLAGS] & FL_ZF) == FL_ZF);
                printf("SF: %d\n", (reg[R_FLAGS] & FL_SF) == FL_SF);
            }
            else if (strcmp(command, "stack\n") == 0) {
                for (int i = reg[R_SP]; i < reg[R_BP]; i+=1) {
                    printf("        --------\n");
                    printf("0x%04x:  0x%04x ", i, memory[i]);
                    for (int j = 0; j < R_COUNT; j++) if (reg[j] == i) printf(" <- %s", reg_name[j]);
                    printf("\n");
                }
                printf("        --------\n");
                printf("0x%04x:  0x%04x ", reg[R_BP], memory[reg[R_BP]]);
                printf(" <- BP");
                printf("\n        --------\n");
            }
            else if (strcmp(command, "step\n") == 0) {
                if (!init) {
                    reg[R_BP] = rand() % (UINT16_MAX + 1);
                    reg[R_SP] = reg[R_BP];
                    binary = fopen(argv[1], "r");
                    reg[R_IP] = 0;
                    printf("Initialized program.\n");
                    init = true;
                }
                else {
                    if (reg[R_IP] < fsize(argv[1])) step(true);
                    else {
                        printf("Program end.\n");
                        init = false;
                    }
                }
            }
            else if (strcmp(msg, "quit\n") == 0 || strcmp(msg, "exit\n") == 0 || strcmp(msg, "q\n") == 0) exit(1);
        }
    }
    else run(argv[1]);
}
