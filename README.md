# vm
## About
A work-in-progress 64-bit virtual machine for the x86 architecture. It currently supports (with bugs!) basic arithmetic/logical instructions, simple comparisons and jumps, pushing/popping/accessing the stack, MOV, and 8/16/32/64 bit operations.

This project isn't very functional just yet, and mainly serves as a way of learning more about the details of assembly and x86. The current goal as of now is to get the C program `primes.c` to execute correctly.

## Usage
```
./vm /path/to/executable offset [-i]
```
- `offset` is the location of the executable entry point (in bytes) in the file.
- Pass in the `-i` flag to run in interactive mode.

For reference, the starting offset in the provided binary `primes` is 3920.

### Interactive Mode
- `step` either initializes the program or executes a single opcode (including prefixes)
- `run` runs your code as if you were not in interactive mode but does not exit out
- `registers` lists the contents of the registers in hexadecimal, decimal, and binary
- `flags` lists the contents of the FLAGS register
- `stack` lists the contents of the stack and which registers point to where in memory
