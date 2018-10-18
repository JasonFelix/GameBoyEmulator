#pragma once
inline void NOP() {}

inline void LD_BC_D16() { //Load 16b value into the BC register.
	registers.bc.b = mmu->read(registers.pc++);
	registers.bc.c = mmu->read(registers.pc++);
}
inline void LD_BC_A() { //Load A's value into the BC register.
	registers.bc.w = registers.af.a;
}
inline void INC_BC() {
	registers.bc.w++;
}
