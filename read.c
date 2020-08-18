#define MAP_OPCODE(x, func) case x: func; break;
#define MULTIMAP_BEGIN(x) case x: fread(&modrm, 1, 1, binary); reg[R_IP]++;
#define MULTIMAP_END(x) break;
#define EXTENSION_MAP(ext, func) if ((modrm | (ext << 3)) == modrm) func

int step(bool verbose) {
    uint8_t op;
    uint8_t modrm;
    fread(&op, 1, 1, binary);
    reg[R_IP]++;
    if (verbose) printf("Read opcode 0x%02x\n", op);
    switch (op) {
        MAP_OPCODE(0x00, std_op(add, 0));
        MAP_OPCODE(0x01, std_op(add, 1));
        MAP_OPCODE(0x02, std_op(add, 2));
        MAP_OPCODE(0x03, std_op(add, 3));
        MAP_OPCODE(0x04, std_op(add, 4));
        MAP_OPCODE(0x05, std_op(add, 5));
        MAP_OPCODE(0x08, std_op(or, 0));
        MAP_OPCODE(0x09, std_op(or, 1));
        MAP_OPCODE(0x0A, std_op(or, 2));
        MAP_OPCODE(0x0B, std_op(or, 3));
        MAP_OPCODE(0x0C, std_op(or, 4));
        MAP_OPCODE(0x0D, std_op(or, 5));
        MAP_OPCODE(0x20, std_op(and, 0));
        MAP_OPCODE(0x21, std_op(and, 1));
        MAP_OPCODE(0x22, std_op(and, 2));
        MAP_OPCODE(0x23, std_op(and, 3));
        MAP_OPCODE(0x24, std_op(and, 4));
        MAP_OPCODE(0x25, std_op(and, 5));
        MAP_OPCODE(0x28, std_op(sub, 0));
        MAP_OPCODE(0x29, std_op(sub, 1));
        MAP_OPCODE(0x2A, std_op(sub, 2));
        MAP_OPCODE(0x2B, std_op(sub, 3));
        MAP_OPCODE(0x2C, std_op(sub, 4));
        MAP_OPCODE(0x2D, std_op(sub, 5));
        MAP_OPCODE(0x30, std_op(xor, 0));
        MAP_OPCODE(0x31, std_op(xor, 1));
        MAP_OPCODE(0x32, std_op(xor, 2));
        MAP_OPCODE(0x33, std_op(xor, 3));
        MAP_OPCODE(0x34, std_op(xor, 4));
        MAP_OPCODE(0x35, std_op(xor, 5));
        MAP_OPCODE(0x3C, cmp(8));
        MAP_OPCODE(0x3D, cmp(8));
        MAP_OPCODE(0x50, push(0));
        MAP_OPCODE(0x51, push(1));
        MAP_OPCODE(0x52, push(2));
        MAP_OPCODE(0x53, push(3));
        MAP_OPCODE(0x54, push(4));
        MAP_OPCODE(0x55, push(5));
        MAP_OPCODE(0x56, push(6));
        MAP_OPCODE(0x57, push(7));
        MAP_OPCODE(0x58, pop(0));
        MAP_OPCODE(0x59, pop(1));
        MAP_OPCODE(0x5A, pop(2));
        MAP_OPCODE(0x5B, pop(3));
        MAP_OPCODE(0x5C, pop(4));
        MAP_OPCODE(0x5D, pop(5));
        MAP_OPCODE(0x5E, pop(6));
        MAP_OPCODE(0x5F, pop(7));
        MAP_OPCODE(0x74, jump((reg[R_FLAGS] & FL_ZF) == FL_ZF, 8, false));
        MAP_OPCODE(0x75, jump((reg[R_FLAGS] | ~FL_ZF) == ~FL_ZF, 8, false));
        MAP_OPCODE(0x78, jump((reg[R_FLAGS] & FL_SF) == FL_SF, 8, false));
        MAP_OPCODE(0x79, jump((reg[R_FLAGS] | ~FL_SF) == ~FL_SF, 8, false));
        MAP_OPCODE(0x7A, jump((reg[R_FLAGS] & FL_PF) == FL_PF, 8, false));
        MAP_OPCODE(0x7B, jump((reg[R_FLAGS] | ~FL_PF) == ~FL_PF, 8, false));
        MAP_OPCODE(0xE9, jump(true, 16, false));
        MAP_OPCODE(0xEB, jump(true, 8, false));
        MAP_OPCODE(0xEA, jump(true, 16, true));
    MULTIMAP_BEGIN(0xF6)
        EXTENSION_MAP(4, ax_op(mul, 8, modrm));
        EXTENSION_MAP(6, ax_op(udiv, 8, modrm));
    MULTIMAP_END(0xF6)
    MULTIMAP_BEGIN(0xF7)
        EXTENSION_MAP(4, ax_op(mul, 16, modrm));
        EXTENSION_MAP(6, ax_op(udiv, 16, modrm));
    MULTIMAP_END(0xF7)
    default:
        printf("Bad opcode 0x%02x at 0x%02x! Skipping...\n", op, reg[R_IP]);
        break;
    }
    return 0;
}
