use modular_bitfield::prelude::*;

#[derive(Clone, Copy)]
enum Register {
    RAX, RCX, RDX, RBX, RSP, RBP, RSI, RDI,
    R8, R9, R10, R11, R12, R13, R14, R15,
    RIP, FLAGS, COUNT,
}

impl std::convert::From<Register> for usize {
    fn from(r: Register) -> usize {
        r as usize
    }
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

impl num::ToPrimitive for Register {
    fn to_i64(&self) -> Option<i64> {
	Some(*self as i64)
    }
    
    fn to_u64(&self) -> Option<u64> {
	Some(*self as u64)
    }
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

    fn get_reg<T: num::ToPrimitive>(&self, r: T) -> u64 {
	self.regs[r.to_usize().unwrap()]
    }

    fn set_reg(&mut self, r: Register, v: u64) {
	self.regs[r as usize] = v;
    }

    fn get_mem_ind(&self, r: Register) -> u8 {
	self.memory[self.regs[r as usize] as usize]
    }

    fn read<T: bytemuck::Pod>(&mut self) -> T {
	union Data<T> {
	    interpreted: std::mem::ManuallyDrop<T>,
	    dummy: (),
	}

	let mut bytes = Data { dummy: () };
	let mut base = ((&mut bytes) as *mut Data<_>).cast::<u8>();

	for _i in 0..std::mem::size_of::<T>()/8 {
	    unsafe {
		base.write(self.get_mem_ind(Register::RIP));
		base = base.add(1);
	    }
	    self.regs[Register::RIP as usize] += 1;
	}
	
	unsafe {
	    std::mem::ManuallyDrop::into_inner(bytes.interpreted)
	}	
    }

    fn parse_modrm(&mut self) -> ModRM {
	let modrm = self.read::<u8>();
	ModRM::new()
	    .with_mode((modrm & 0b11000000) >> 6)
	    .with_reg((modrm & 0b00111000) >> 3)
	    .with_rm(modrm & 0b00000111)
    }

    fn parse_sib(&mut self) -> SIB {
	let sib: u8 = self.read::<u8>();
	SIB::new()
	    .with_scale((sib & 0b11000000) >> 6)
	    .with_index((sib & 0b00111000) >> 3)
	    .with_base(sib & 0b00000111)
    }

    fn get_sib_addr(&mut self, modrm: ModRM, sib: SIB, prefix: Option<u8>) -> usize {
        let mut addr: usize;
        if modrm.mode() == 0b00 {
            if sib.base() == 0b101 && sib.index() == 0b100 {
                addr = self.read::<u32>() as usize;
            } else if sib.index() == 0b100 {
                addr = self.regs[sib.base() as usize] as usize;
            }
        }
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
	    addr = self.get_reg(Register::RIP) as usize;
	    addr += self.read::<u32>() as usize;
	} else {
	    addr = self.get_reg(true_rm) as usize;
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
    println!("{}", vm.get_reg(Register::RSI));
}
