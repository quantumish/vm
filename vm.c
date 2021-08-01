#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>

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

typedef struct modrm {
	uint8_t mod;
	uint8_t reg;
	uint8_t rm;
} modrm_t;

typedef struct sib {
	uint8_t scale;
	uint8_t index;
	uint8_t base;
} sib_t;

typedef struct op_args {
	void* arg1;
	void* arg2;
	void* arg3;
	void* arg4;
	size_t len;
} op_args_t;

modrm_t parse_modrm()
{
	uint8_t modrm = *(uint8_t*)regs[R_IP];
	regs[R_IP]++;
	modrm_t parsed =  {
		.mod = (modrm & 0b11000000) >> 6,
		.reg = (modrm & 0b00111000) >> 3,
		.rm  = modrm & 0b00000111,
	};
	assert(parsed.mod <= 0b11);
	assert(parsed.reg <= MAX_REG);
	assert(parsed.rm <= MAX_REG);
	return parsed;
}

sib_t parse_sib()
{
	uint8_t sib = *(uint8_t*)regs[R_IP];
	regs[R_IP]++;
	sib_t parsed =  {
		.scale = (sib & 0b11000000) >> 6,
		.index = (sib & 0b00111000) >> 3,
		.base  = pow(2, sib & 0b00000111),
	};
	assert(parsed.scale == 1 || parsed.scale == 2 ||
		   parsed.scale == 4 || parsed.scale == 8);
	assert(parsed.index <= MAX_REG);
	assert(parsed.base <= MAX_REG);
	return parsed;
}

uint8_t read_imm8()
{
	uint8_t val = *(uint8_t*)regs[R_IP];
	regs[R_IP]++;
	return val;
}

uint8_t read_imm16()
{
	uint8_t val = *(uint16_t*)regs[R_IP];
	regs[R_IP]+=2;
	return val;
}

uint8_t read_imm32()
{
	uint8_t val = *(uint32_t*)regs[R_IP];
	regs[R_IP]+=4;
	return val;
}

uint8_t* get_sib_addr(modrm_t modrm, sib_t sib)
{
	uint8_t* addr = 0x0;
	if (modrm.mod == 0b00) {
		if (sib.base == 0b101 && sib.index == 0b100) {
			addr = (uint8_t*)(intptr_t)read_imm32();
		} else if (sib.index == 0b100) {
			addr = (uint8_t*)regs[sib.base];
		} else if (sib.base == 0b101) {
			addr = (uint8_t*)(regs[sib.index] * sib.scale) + read_imm32();
		} else {
			addr = (uint8_t*)(regs[sib.base] + (regs[sib.index] * sib.scale));
		}
	} else {
		// TODO Remove duplicate check if cleanly doable
		if (sib.index == 0b100) addr = (uint8_t*)regs[sib.base];
	    else addr = (uint8_t*)(regs[sib.base] + (regs[sib.index] * sib.scale));
		if (modrm.mod == 0b01) addr += read_imm8();
		else addr += read_imm32();
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
		addr += read_imm32();
		return addr;
	} else {
		addr = (uint8_t*)regs[modrm.rm];
	}
	if (modrm.mod == 0b01) addr += read_imm8();
	else addr += read_imm32();
	return addr;
}

// Read a ModR/M byte and get operands.
// Can read extra bytes in case of immediate or SIB byte.
// @returns struct containing arguments to opcode.
op_args_t get_args()
{
	op_args_t args;
	modrm_t modrm = parse_modrm();
	if (modrm.mod == 0b11) {
		args.arg1 = &regs[modrm.reg];
		args.arg2 = &regs[modrm.rm];
		return args;
	}
	else {
		args.arg1 = &regs[modrm.reg];
		args.arg2 = get_addr(modrm);
	}
	return args;
}

// TODO: Questionable
void add(op_args_t args)
{
	*(uint64_t*)args.arg1 += *(uint64_t*)args.arg2;
}

int main()
{
	regs[R_DX] = 2;
	regs[R_AX] = 3;
	memory[0] = 0x01;
	memory[1] = 0xd0;
	regs[R_IP] = (uint64_t)&memory[0];
	regs[R_IP]++;
	add(get_args());
	printf("%ld", regs[R_DX]);
	return 0;
}
