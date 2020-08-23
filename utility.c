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

uint64_t read_immediate(int bits)
{
    assert(bits == 8 || bits == 16 || bits == 32);
    if (bits == 8) {
        uint8_t imm8;
        fread(&imm8, bits/8, 1, binary);
        reg[R_RIP]+=bits/8;
        return sign_extend(imm8, 8);
    }
    else if (bits == 16) {
        uint16_t imm16;
        fread(&imm16, bits/8, 1, binary);
        reg[R_RIP]+=bits/8;
        return sign_extend(imm16, 16);
    }
    else if (bits == 32) {
        uint32_t imm32;
        fread(&imm32, bits/8, 1, binary);
        reg[R_RIP]+=bits/8;
        return sign_extend(imm32, 32);
    }
}
