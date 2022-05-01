use modular_bitfield::prelude::*;

enum Register {
	RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI,
	R8, R9, R10, R11, R12, R13, R14, R15,
	RIP, FLAGS, COUNT,
}

#[derive(Clone, Copy)]
#[bitfield]
struct ModRM {
	mode: B2,
	reg: B3,
	rm: B3,
}

#[bitfield]
struct SIB {
	scale: B2,
	index: B3,
	base: B3,
}

#[bitfield]
struct REX {
	op_ext: B1,
	reg_ext: B1,
	sib_ext: B1,
	rm_ext: B1,
	pad: B4, // modular_bitfield requires multiple of 8
}

struct VM {
	regs: [u64; Register::COUNT as usize],
	memory: [u8; u16::MAX as usize]
}

impl VM {
	fn new() -> Self {
		VM {
			regs: [0; Register::COUNT as usize],
			memory: [0; u16::MAX as usize]
		}
	}

	fn read<T: std::convert::TryFrom<u64>>(&mut self) -> T {
		let val = T::try_from(self.regs[Register::RIP as usize]);
		let val = match val {
			Ok(v) => v,
			Err(_) => panic!() // :flag_es:
		};
		self.regs[Register::RIP as usize] += std::mem::size_of::<T>() as u64;
		val
	}

	fn parse_modrm(&mut self) -> ModRM {
		let modrm = self.memory[self.regs[Register::RIP as usize] as usize];
		self.regs[Register::RIP as usize] += 1;
		ModRM::new()
			.with_mode((modrm & 0b11000000) >> 6)
			.with_reg((modrm & 0b00111000) >> 3)
			.with_rm(modrm & 0b00000111)
	}

	fn parse_sib(&mut self) -> SIB {
		let sib = self.memory[self.regs[Register::RIP as usize] as usize];
		self.regs[Register::RIP as usize] += 1;
		SIB::new()
			.with_scale((sib & 0b11000000) >> 6)
			.with_index((sib & 0b00111000) >> 3)
			.with_base(sib & 0b00000111)
	}

	fn get_sib_addr(&mut self, modrm: ModRM, sib: SIB, prefix: Option<u8>) -> usize {
		todo!()
	}

	fn get_addr(&mut self, modrm: ModRM, rex: Option<REX>) -> usize {
		let mut addr: usize;
		let true_rm = if let Some(r) = rex {
			modrm.rm() + (0b1000 * r.reg_ext())
		} else { modrm.rm() };
		if modrm.rm() == 0b100 {
			let sib = self.parse_sib();
			addr = self.get_sib_addr(modrm, sib, None);
		} else if modrm.rm() == 0b101 && modrm.mode() == 0b00 {
			addr = self.regs[Register::RIP as usize] as usize;
			addr += self.read::<u32>() as usize;
		} else {
			addr = self.regs[true_rm as usize] as usize;
		}
		if modrm.mode() == 0b01 { addr += self.read::<u8>() as usize }
		else if modrm.mode() == 0b10 { addr += self.read::<u32>() as usize }
		addr
	}

	fn get_args(&mut self) -> (&u64, &u64) {
		let modrm = self.parse_modrm();
		if modrm.mode() == 0b11 {
			(&self.regs[modrm.reg() as usize], &self.regs[modrm.rm() as usize])
		} else {
			let mem = self.memory[self.get_addr(modrm, None)];
			// (&self.regs[modrm.reg() as usize], &mem)
			todo!()
		}
	}
}

fn main() {
	let vm = VM::new();
	let rm = ModRM::new().with_mode(1);
	println!("{}", vm.regs[Register::RSI as usize]);
}
