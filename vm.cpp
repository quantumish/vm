#include <cstdint>
#include <cmath>
#include <cassert>
#include <iostream>
#include <variant>

#ifndef __GNUC__
#error "GCC required for binary literals."
#endif

enum {
    R_AX = 0b000,
    R_CX = 0b001,
    R_DX = 0b010,
    R_BX = 0b011,
    R_SP = 0b100,
    R_BP = 0b101,
    R_SI = 0b110,
    R_DI = 0b111,
    R_IP,
    R_FLAGS,
    R_COUNT
};

const uint8_t MAX_REG = R_DI;
uint64_t regs[R_COUNT];
uint8_t memory[UINT16_MAX+1];

struct modrm_t {
    uint8_t mod;
    uint8_t reg;
    uint8_t rm;
};

struct sib_t {
    uint8_t scale;
    uint8_t index;
    uint8_t base;
};

template<typename A, typename B>
struct op_args_t {
    A& arg1;
    B& arg2;    
};

modrm_t parse_modrm() {
    uint8_t modrm = *(uint8_t*)regs[R_IP];
    regs[R_IP]++;
    modrm_t parsed =  {
	.mod = static_cast<uint8_t>((modrm & 0b11000000) >> 6),
	.reg = static_cast<uint8_t>((modrm & 0b00111000) >> 3),
	.rm  = static_cast<uint8_t>(modrm & 0b00000111),
    };
    assert(parsed.mod <= 0b11);
    assert(parsed.reg <= MAX_REG);
    assert(parsed.rm <= MAX_REG);
    return parsed;
}

sib_t parse_sib() {
    uint8_t sib = *(uint8_t*)regs[R_IP];
    regs[R_IP]++;
    sib_t parsed =  {
	.scale = static_cast<uint8_t>((sib & 0b11000000) >> 6),
	.index = static_cast<uint8_t>((sib & 0b00111000) >> 3),
	.base  = static_cast<uint8_t>(pow(2, sib & 0b00000111)),
    };
    assert(parsed.scale % 2 == 0 && parsed.scale < 8);
    assert(parsed.index <= MAX_REG);
    assert(parsed.base <= MAX_REG);
    return parsed;
}

template<typename T>
T read() {
    T val = *reinterpret_cast<T*>(regs[R_IP]);
    regs[R_IP] += sizeof(T);
    return val;
}

uint8_t* get_sib_addr(modrm_t modrm, sib_t sib) {
    uint8_t* addr = 0x0;
    if (modrm.mod == 0b00) {
	if (sib.base == 0b101 && sib.index == 0b100) {
	    addr = reinterpret_cast<uint8_t*>(read<uint32_t>());
	} else if (sib.index == 0b100) {
	    addr = (uint8_t*)regs[sib.base];
	} else if (sib.base == 0b101) {
	    addr = (uint8_t*)(regs[sib.index] * sib.scale) + read<uint32_t>();
	} else {
	    addr = (uint8_t*)(regs[sib.base] + (regs[sib.index] * sib.scale));
	}
    } else {
	// TODO Remove duplicate check if cleanly doable
	if (sib.index == 0b100) addr = (uint8_t*)regs[sib.base];
	else addr = (uint8_t*)(regs[sib.base] + (regs[sib.index] * sib.scale));
	if (modrm.mod == 0b01) addr += read<uint8_t>();
	else addr += read<uint32_t>();
    }
    return addr;
}

uint8_t* get_addr(modrm_t modrm)
{
    uint8_t* addr = 0x0;
    if (modrm.rm == 0b100) {
	addr = get_sib_addr(modrm, parse_sib());
    } else if (modrm.rm == 0b101 && modrm.mod == 0b00) {
	// TODO Look into exact behavior of RIP-based indirect addressing
	addr = (uint8_t*)regs[R_IP];
	addr += read<uint32_t>();
	return addr;
    } else {
	addr = (uint8_t*)regs[modrm.rm];
    }
    if (modrm.mod == 0b01) addr += read<uint8_t>();
    else addr += read<uint32_t>();
    return addr;
}

// Read a ModR/M byte and get operands.
// Can read extra bytes in case of immediate or SIB byte.
// @returns struct containing arguments to opcode.
op_args_t get_args_prefix(uint8_t prefix) {    
    modrm_t modrm = parse_modrm();
    if (modrm.mod == 0b11) {
        return {
	    .arg1 = regs[modrm.reg],
	    .arg2 = regs[modrm.rm],
	};
	return args;
    }
    // else {
    // 	args.arg1 = &regs[modrm.reg];
    // 	args.arg2 = get_addr(modrm);
    // 	return args;
    // }
}
#define get_args() get_args_prefix(0x00)

// // TODO: Questionable
// template <typename T>
// void add(op_args_t args) {
//     *reinterpret_cast<T*>(args.arg1) += *reinterpret_cast<T*>(args.arg2);
// }

int main() {
    regs[R_DX] = 2;
    regs[R_AX] = 3;
    memory[0] = 0x01;
    memory[1] = 0xd0;
    regs[R_IP] = (uint64_t)&memory[0];
    regs[R_IP]++;   
    // add<uint64_t>(get_args());
    std::cout << regs[R_DX] << "\n";
    return 0;
}
