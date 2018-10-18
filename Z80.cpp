#include "stdafx.h"
#include "Z80.h"
#include "registers.h"
#include "MMU.h"
#include <iostream>
#include <string>
#include <fstream>
#include <cstdio>
#include <chrono>
#include <thread>

using namespace memory;
using byte = unsigned char; //8 bit
using word = unsigned short; //16 bit

namespace cpu {


	int clock_cycles = 0; //Referred to as T.
	int machine_cycles = 0; //Referred to as M.
	memory::Registers* registers;
	memory::MMU* mmu;

	int CYCLES[] = {
		//  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f
		1, 3, 2, 2, 1, 1, 2, 1, 5, 2, 2, 2, 1, 1, 2, 1,  // 0
		1, 3, 2, 2, 1, 1, 2, 1, 3, 2, 2, 2, 1, 1, 2, 1,  // 1
		2, 3, 2, 2, 1, 1, 2, 1, 2, 2, 2, 2, 1, 1, 2, 1,  // 2
		2, 3, 2, 2, 3, 3, 3, 1, 2, 2, 2, 2, 1, 1, 2, 1,  // 3
		1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,  // 4
		1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,  // 5
		1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,  // 6
		2, 2, 2, 2, 2, 2, 1, 2, 1, 1, 1, 1, 1, 1, 2, 1,  // 7
		1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,  // 8
		1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,  // 9
		1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,  // a
		1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1,  // b
		2, 3, 3, 4, 3, 4, 2, 4, 2, 4, 3, 2, 3, 6, 2, 4,  // c
		2, 3, 3, 1, 3, 4, 2, 4, 2, 4, 3, 1, 3, 1, 2, 4,  // d
		3, 3, 2, 1, 1, 4, 2, 4, 4, 1, 4, 1, 1, 1, 2, 4,  // e
		3, 3, 2, 1, 1, 4, 2, 4, 3, 2, 4, 1, 0, 1, 2, 4   // f
	};

	int CB_CYCLES[] = {
		//  0  1  2  3  4  5  6  7  8  9  a  b  c  d  e  f   
		2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,  // 0
		2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,  // 1
		2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,  // 2
		2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,  // 3
		2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2,  // 4
		2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2,  // 5
		2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2,  // 6
		2, 2, 2, 2, 2, 2, 3, 2, 2, 2, 2, 2, 2, 2, 3, 2,  // 7
		2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,  // 8
		2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,  // 9
		2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,  // a
		2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,  // b
		2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,  // c
		2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,  // d
		2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2,  // e
		2, 2, 2, 2, 2, 2, 4, 2, 2, 2, 2, 2, 2, 2, 4, 2   // f
	};

	/* OPCODES */
	inline void nop() {}
	inline void stop() { mmu->next(); } // something about the gb going into a very low power mode, ex. when the device has been idle for a while.

	inline void inc(word &nn) { nn++; }
	inline void dec(word &nn) { nn--; }
	inline void inc(byte &n) {
		registers->off(registers->N_SUBTRACT);
		registers->set(registers->HALF_CARRY, (n & 0x0F) == 0x0F);
		n++;
		registers->set(registers->ZERO, !n);
	}
	inline void inc_a(word address) {
		registers->off(registers->N_SUBTRACT);
		registers->set(registers->HALF_CARRY, (mmu->read(address) & 0x0F) == 0x0F);
		char t = mmu->read(address);
		mmu->write(address, ++t);
		registers->set(registers->ZERO, !mmu->read(address));
	}
	inline void dec_a(word address) {
		registers->on(registers->N_SUBTRACT);
		char t = mmu->read(address);
		mmu->write(address, --t);
		registers->set(registers->HALF_CARRY, (t & 0x0F) == 0x0F);
		registers->set(registers->ZERO, !t);
	}
	inline void dec(byte &n) {
		registers->on(registers->N_SUBTRACT);
		n--;
		registers->set(registers->HALF_CARRY, (n & 0x0F) == 0x0F);
		registers->set(registers->ZERO, !n);
	}
	/*JUMPS*/
	inline void jr(Registers::FLAG_MASK flag, bool condition) { 
		registers->get_flag(flag) == condition ? registers->pc += (signed char) mmu->next() : registers->pc++;
	}
	inline void jr() {
		registers->pc += (signed char) mmu->next();
	}

	inline void jp() {
		word address = 0x0;
		address |= mmu->next();
		address |= (mmu->next() << 8);
		registers->pc = address;
	}

	inline void jp(Registers::FLAG_MASK flag, bool condition) { 
		registers->get_flag(flag) == condition ? jp() : registers->pc+=2;
	}

	inline void jp(word address) {
		registers->pc = address;
	}

	/*LOAD*/
	inline void ldh_a_nn(word nn) { registers->af.a = mmu->read(nn); }
	inline void ldh_n_adr(byte &n, word address) { n = mmu->read(address); }
	inline void ldh_adr_n(word address, byte &n) { mmu->write(address, n); }
	inline void ldh_nn_d8(word nn) { mmu->write(nn, mmu->next()); }
	inline void ld_nn_n(word &nn, byte &n) { nn = n; }
	inline void ld_nn_nn(word &nn, word n) { nn = n; }
	inline void ld_n_n(byte &n0, byte &n1) { n0 = n1; }
	inline void ld_n_d8(byte &n) { n = mmu->next(); }
	inline void ld_nn_d16(word &nn) { nn = (mmu->next() | (mmu->next() << 8)); }
	inline void ld_n_nn(byte &n, word &nn) { n = nn; }
	inline void ld_a_nn(word nn) { registers->af.a = nn; }
	inline void ld_a_n(word n) { registers->af.a = n; }
	inline void ld_hli_a() { mmu->write(registers->hl.w, registers->af.a); registers->hl.w++; }
	inline void ld_hld_a() { mmu->write(registers->hl.w, registers->af.a); registers->hl.w--; }
	inline void ldh_a_d8() { registers->af.a = mmu->read(0xFF00 + mmu->next()); }
	inline void ldh_d8_a() { mmu->write(mmu->next(), 0xFF00 + registers->af.a); }


	inline void add_nn_nn(word &nn0, word &nn1) {
		registers->off(registers->N_SUBTRACT);
		registers->set(registers->CARRY, (nn0 + nn1) > 0xFFFF);
		registers->set(registers->HALF_CARRY, ((nn0 & 0xFFF) + (nn1 & 0xFFF)) > 0xFFF);
		nn0 += nn1;
	}

	inline void adc_a_n(byte n) {
		char carry = registers->get_flag(registers->CARRY);
		registers->off(registers->N_SUBTRACT);
		registers->set(registers->CARRY, (n + carry + registers->af.a) > 0xFF);
		registers->set(registers->HALF_CARRY, ((n & 0xF) + (registers->af.a & 0xF) + carry) > 0xF);
		registers->af.a += (n + carry);
		registers->set(registers->ZERO, !registers->af.a);
	}

	inline void add_n_n(byte &n0, byte &n1) {
		registers->off(registers->N_SUBTRACT);
		registers->set(registers->CARRY, (n0 + n1) > 0xFF);
		registers->set(registers->HALF_CARRY, ((n0 & 0xF) + (n1 & 0xF)) > 0xF);
		n0 += n1;
		registers->set(registers->ZERO, !n0);
	}

	inline void add_nn_n(word &nn, signed char &n) {
		registers->off(registers->N_SUBTRACT);
		registers->set(registers->CARRY, (nn + n) > 0xFF);
		registers->set(registers->HALF_CARRY, ((nn & 0xF) + (n & 0xF)) > 0xF);
		nn += n;
	}

	inline void sub(byte n) {
		registers->on(registers->N_SUBTRACT);
		registers->set(registers->CARRY, n > registers->af.a);
		registers->set(registers->HALF_CARRY, (registers->af.a & 0x0F) < (n & 0x0F)); //TODO: May need to fix
		//registers->set(registers->HALF_CARRY, ((registers->af.a & 0xf) - (n & 0xf)) & 0x8);
		registers->af.a -= n;
		registers->set(registers->ZERO, !registers->af.a);
	}

	inline void sbc(byte n) {
		char t = registers->get_flag(registers->CARRY);
		registers->on(registers->N_SUBTRACT);
		registers->set(registers->CARRY, (n + registers->get_flag(registers->CARRY)) > registers->af.a);
		registers->set(registers->HALF_CARRY, (registers->af.a & 0xF0) < (n & 0xF0));
		registers->af.a -= (n - t);
		registers->set(registers->ZERO, !registers->af.a);
	}

	inline void and(byte n) {
		registers->af.a &= n;
		registers->set(registers->ZERO, !registers->af.a);
		registers->off(registers->N_SUBTRACT);
		registers->on(registers->HALF_CARRY);
		registers->off(registers->CARRY);
	}

	inline void xor(byte n) {
		registers->af.a ^= n;
		registers->set(registers->ZERO, !registers->af.a);
		registers->off(registers->N_SUBTRACT);
		registers->off(registers->HALF_CARRY);
		registers->off(registers->CARRY);
	}

	inline void or(byte n) {
		registers->af.a |= n;
		registers->set(registers->ZERO, !registers->af.a);
		registers->off(registers->N_SUBTRACT);
		registers->off(registers->HALF_CARRY);
		registers->off(registers->CARRY);
	}

	inline void cmp(byte n) {
		registers->set(registers->ZERO, registers->af.a == n);
		registers->on(registers->N_SUBTRACT);
		registers->set(registers->CARRY, registers->af.a < n);
		registers->set(registers->HALF_CARRY, ((registers->af.a & 0x0F) < (n & 0x0F)));
	}

	inline void ld_a16_sp() {
		mmu->write(registers->pc++, registers->sp & 0x00FF);
		mmu->write(registers->pc++, registers->sp & 0xFF00);
	}

	inline void rlca() {
		registers->off(Registers::ZERO);
		registers->off(Registers::N_SUBTRACT);
		registers->off(Registers::HALF_CARRY);
		registers->set(registers->CARRY, registers->af.a & 0x80);
		registers->af.a = registers->af.a << 1;
		registers->af.a |= registers->get_flag(registers->CARRY);
	}
	inline void rrca() {
		registers->off(Registers::ZERO);
		registers->off(Registers::N_SUBTRACT);
		registers->off(Registers::HALF_CARRY);
		registers->set(registers->CARRY, registers->af.a & 0x1);
		registers->af.a = registers->af.a >> 1;
		registers->af.a |= (registers->get_flag(registers->CARRY) << 7);
	}

	inline void rla() {
		registers->off(Registers::ZERO);
		registers->off(Registers::N_SUBTRACT);
		registers->off(Registers::HALF_CARRY);
		char temp = registers->get_flag(registers->CARRY);
		registers->set(registers->CARRY, registers->af.a & 0x80);
		registers->af.a <<= 1;
		registers->af.a |= temp;
	}

	inline void rra() {
		registers->off(Registers::ZERO);
		registers->off(Registers::N_SUBTRACT);
		registers->off(Registers::HALF_CARRY);
		char temp = registers->get_flag(registers->CARRY);
		registers->set(registers->CARRY, registers->af.a & 0x1);
		registers->af.a >>= 1;
		registers->af.a |= (temp << 7);
	}
	inline void daa() {
		byte high = (registers->af.a & 0xF0) >> 4;
		byte low = registers->af.a & 0xF;
		byte t = registers->af.a;
		if(!registers->get_flag(registers->N_SUBTRACT)) {
			if(registers->get_flag(registers->HALF_CARRY) || low > 0x9)
				t += 0x6;
			if(registers->get_flag(registers->CARRY) || t > 0x9F)
				t += 0x60;
		} else {
			if(registers->get_flag(registers->HALF_CARRY))
				t = (t - 6) & 0xFF;
			if(registers->get_flag(registers->CARRY))
				t -= 0x60;
		}
		registers->off(registers->HALF_CARRY);
		registers->af.a = (t & 0xFF);		
		registers->set(registers->ZERO, !registers->af.a);
	}

	inline void cpl() {
		registers->af.a = ~registers->af.a;
		registers->on(registers->N_SUBTRACT);
		registers->on(registers->HALF_CARRY);
	}

	inline void ccf() {
		registers->toggle(registers->CARRY);
		registers->off(registers->N_SUBTRACT);
		registers->off(registers->HALF_CARRY);
	}

	inline void push(word &_register) {
		mmu->write(--registers->sp, _register >> 8);
		mmu->write(--registers->sp, _register);
	}
	inline void pop(word &_register) {
		_register = mmu->read(registers->sp++);
		_register |= mmu->read(registers->sp++) << 8;
	}
	inline void ret() {
		pop(registers->pc);
	}
	inline void ret(Registers::FLAG_MASK flag, bool condition) {
		if(registers->get_flag(flag) == condition) ret();
	}
	inline void rst(word address) {
		push(registers->pc);
		registers->pc = address;
	}
	inline void call() {
		word adr = registers->pc + 2;
		push(adr);		
		registers->pc = mmu->next() | (mmu->next() << 8);
	}

	inline void call(Registers::FLAG_MASK flag, bool condition) {
		if(registers->get_flag(flag) == condition) call();
		else registers->pc += 2;
	}

	inline void rlc(byte &_register) {
		registers->off(registers->N_SUBTRACT);
		registers->off(registers->HALF_CARRY);

		registers->set(registers->CARRY, _register > 0x7F);
		registers->set(registers->ZERO, ((_register <<= 1) | (registers->get_flag(registers->CARRY) << 7)) == 0);
	}

	inline void rl(byte &_register) {
		registers->off(registers->N_SUBTRACT);
		registers->off(registers->HALF_CARRY);

		char temp = registers->get_flag(registers->CARRY);
		registers->set(registers->CARRY, _register > 0x7F);
		registers->set(registers->ZERO, ((_register <<= 1) | (temp << 7)) == 0);
	}

	inline void rrc(byte &_register) {
		registers->off(registers->N_SUBTRACT);
		registers->off(registers->HALF_CARRY);

		registers->set(registers->CARRY, _register & 0x1);
		registers->set(registers->ZERO, ((_register >>= 1) | registers->get_flag(registers->CARRY)) == 0);
		_register |= (registers->get_flag(registers->CARRY) << 7);
	}

	inline void rr(byte &_register) {
		registers->off(registers->N_SUBTRACT);
		registers->off(registers->HALF_CARRY);

		char temp = registers->get_flag(registers->CARRY);
		registers->set(registers->CARRY, _register & 0x1);
		registers->set(registers->ZERO, (_register = ((_register >>= 1) |= temp)) == 0);
	}

	inline void sla(byte &_register) {
		registers->off(registers->N_SUBTRACT);
		registers->off(registers->HALF_CARRY);

		registers->set(registers->CARRY, _register > 0x7F);
		registers->set(registers->ZERO, (_register <<= 1) == 0);
	}

	inline void sra(byte &_register) {
		registers->off(registers->N_SUBTRACT);
		registers->off(registers->HALF_CARRY);
		registers->off(registers->CARRY);

		char temp = _register & 0x80;
		registers->set(registers->CARRY, _register & 0x1);
		registers->set(registers->ZERO, (_register = ((_register >>= 1) |= temp)) == 0);
	}

	inline void srl(byte& _register) {
		registers->off(registers->N_SUBTRACT);
		registers->off(registers->HALF_CARRY);

		registers->set(registers->CARRY, _register & 0x1);
		registers->set(registers->ZERO, (_register = (_register >>= 1)) == 0);
	}

	inline void swap(byte &_register) {
		registers->off(registers->N_SUBTRACT);
		registers->off(registers->HALF_CARRY);
		registers->off(registers->CARRY);

		byte high = (_register & 0xF0) >> 4, low = (_register & 0x0F) << 4;
		registers->set(registers->ZERO, (_register = (high | low)) == 0);
	}

	inline void set(byte bit, byte &_register) {
		_register |= (0x1 << bit);
	}

	inline void res(byte bit, byte &_register) {
		_register &= ~(0x1 << bit);
	}

	inline void bit(byte bit, byte &_register) {
		registers->off(registers->N_SUBTRACT);
		registers->on(registers->HALF_CARRY);
		registers->set(registers->ZERO, (_register & (0x1 << bit)) == 0);
	}

	/* END OF OPCODES*/
	int counter = 0;
	inline void cb();
	vector<word> breakpoints = vector<word>();
	bool run = false;
	void process() {
		for(auto breakpoint : breakpoints) {
			if(registers->pc == breakpoint) {
				run = false;
			}
		}
		byte opcode = mmu->next();
		int input = 0xBEEF;
		while(input != 2 && !run) {
			cout << "\n\n Current Address: 0x" << hex << 0x0 + registers->pc - 1;
			cout << "\n Current Opcode: 0x" << hex << 0x0 + opcode;
			cout << "\n 1: Run till breakpoint";
			cout << "\n 2: Step";
			cout << "\n 3: Read";
			cout << "\n 4: Write";
			cout << "\n 5: Show Registers";
			cout << "\n 6: Add Breakpoint";
			cout << "\n 7: Remove Breakpoint";
			cout << "\n 8: View Breakpoints\n";
			cin >> input;
			switch(input) {
				case 1:
					run = true;
					break;
				case 2:
					break;
				case 3:
					cout << "\n Input an address to read:\n0x";
					cin >> hex >> input;
					cout << "0x" << input << " - " << 0x0 + mmu->read(input);
					break;
				case 4:
				{
						  int adrr = 0;
						  cout << "\n Input an address to write to:\n0x";
						  cin >> adrr;
						  cout << "\n Input the value:\n0x";
						  cin >> input;
						  mmu->write(adrr, input);
						  cout << hex << "\n0x" << hex << adrr << " - " << 0x0 + mmu->read(adrr);
						  break;
				}
				case 5:
					cout << "\n Registers:";
					cout << "\n	AF - A:" << hex << 0x0 + registers->af.a << " | Z:" << hex << 0x0 + registers->get_flag(registers->ZERO) << hex << 0x0 + " N:" << registers->get_flag(registers->N_SUBTRACT) << " H:" << hex << 0x0 + registers->get_flag(registers->HALF_CARRY) << " C:" << hex << 0x0 + registers->get_flag(registers->CARRY);
					cout << "\n	BC - B:" << hex << 0x0 + registers->bc.b << " | C:" << hex << 0x0 + registers->bc.c;
					cout << "\n	DE - D:" << hex << 0x0 + registers->de.d << " | E:" << hex << 0x0 + registers->de.e;
					cout << "\n	HL - H:" << hex << 0x0 + registers->hl.h << " | L:" << hex << 0x0 + registers->hl.l;
					cout << "\n	SP - " << hex << 0x0 + registers->sp;
					cout << "\n	ROM Bank - " << hex << 0x0 + mmu->get_rom_bank();
					cout << "\n	RAM Bank - " << hex << 0x0 + mmu->get_ram_bank();
					break;
				case 6:
					cout << "\n Breakpoint Address:0x";
					cin >> hex >> input;
					breakpoints.push_back(input);
					cout << "\nBreakpoint set at 0x" << hex << input << "\n";
					break;
				case 7:
				{
						  cout << "\n Delete breakpoint at address:0x";
						  cin >> hex >> input;
						  bool found = false;
						  for(auto i = breakpoints.begin(); i != breakpoints.end(); i++) {
							  if(*i == input) {
								  breakpoints.erase(i);
								  cout << "\nBreakpoint deleted at 0x" << hex << input << "\n";
								  found = true;
							  }
							  if(found)
								  break;
						  }
						  break;
				}
				case 8:
					cout << "\nBreakpoints:\n";
					for(auto i = breakpoints.begin(); i != breakpoints.end(); i++) {
						cout << "0x" << *i << "\n";
					}
					break;
			}
		}
		clock_cycles += (CYCLES[opcode] * 4);
		switch(opcode) {
			case 0x0: nop(); break;
			case 0x1: ld_nn_d16(registers->bc.w); break;
			case 0x2: ldh_adr_n(registers->bc.w, registers->af.a); break; //Hmm was ld_nn_n
			case 0x3: inc(registers->bc.w); break;
			case 0x4: inc(registers->bc.b); break;
			case 0x5: dec(registers->bc.b); break;
			case 0x6: ld_n_d8(registers->bc.b); break;
			case 0x7: rlca(); break;
			case 0x8: ld_a16_sp(); break;
			case 0x9: add_nn_nn(registers->hl.w, registers->bc.w); break;
			case 0xa: { byte t = mmu->read(registers->bc.w); ld_a_n(t); break; }
			case 0xb: dec(registers->bc.w); break;
			case 0xc: inc(registers->bc.c); break;
			case 0xd: dec(registers->bc.c); break;
			case 0xe: ld_n_d8(registers->bc.c); break;
			case 0xf: rrca(); break;
			case 0x10: stop(); break;
			case 0x11: ld_nn_d16(registers->de.w); break;
			case 0x12: ldh_adr_n(registers->de.w, registers->af.a); break; //Hmm was ld_nn_n
			case 0x13: inc(registers->de.w); break;
			case 0x14: inc(registers->de.d); break;
			case 0x15: dec(registers->de.d); break;
			case 0x16: ld_n_d8(registers->de.d); break;
			case 0x17: rla(); break;
			case 0x18: jr(); break;
			case 0x19: add_nn_nn(registers->hl.w, registers->de.w); break;
			case 0x1a: { byte t = mmu->read(registers->de.w); ld_a_n(t); break; }
			case 0x1b: dec(registers->de.w); break;
			case 0x1c: inc(registers->de.e); break;
			case 0x1d: dec(registers->de.e); break;
			case 0x1e: ld_n_d8(registers->de.e); break;
			case 0x1f: rra(); break;
			case 0x20: jr(Registers::FLAG_MASK::ZERO, false); break;
			case 0x21: ld_nn_d16(registers->hl.w); break;
			case 0x22: ld_hli_a(); break;
			case 0x23: inc(registers->hl.w); break;
			case 0x24: inc(registers->hl.h); break;
			case 0x25: dec(registers->hl.h); break;
			case 0x26: ld_n_d8(registers->hl.h); break;
			case 0x27: daa(); break;
			case 0x28: jr(Registers::FLAG_MASK::ZERO, true); break;
			case 0x29: add_nn_nn(registers->hl.w, registers->hl.w); break;
			case 0x2a: { byte t = mmu->read(registers->hl.w); ld_a_n(t); registers->hl.w++; break; }
			case 0x2b: dec(registers->hl.w); break;
			case 0x2c: inc(registers->hl.l); break;
			case 0x2d: dec(registers->hl.l); break;
			case 0x2e: ld_n_d8(registers->hl.l); break;
			case 0x2f: cpl(); break;
			case 0x30: jr(Registers::FLAG_MASK::CARRY, false); break;
			case 0x31: ld_nn_d16(registers->sp); break;
			case 0x32: ld_hld_a(); break;
			case 0x33: inc(registers->sp); break;
			case 0x34: inc_a(registers->hl.w); break;
			case 0x35: dec_a(registers->hl.w); break;
			case 0x36: ldh_nn_d8(registers->hl.w); break;
			case 0x37: registers->on(registers->CARRY); registers->off(registers->N_SUBTRACT); registers->off(registers->HALF_CARRY); break;
			case 0x38: jr(Registers::FLAG_MASK::CARRY, true); break;
			case 0x39: add_nn_nn(registers->hl.w, registers->sp); break;
			case 0x3a: { byte t = mmu->read(registers->hl.w); ld_a_n(t); registers->hl.w--; break; }
			case 0x3b: dec(registers->sp); break;
			case 0x3c: inc(registers->af.a); break;
			case 0x3d: dec(registers->af.a); break;
			case 0x3e: ld_n_d8(registers->af.a); break;
			case 0x3f: ccf(); break;
			case 0x40: ld_n_n(registers->bc.b, registers->bc.b); break;
			case 0x41: ld_n_n(registers->bc.b, registers->bc.c); break;
			case 0x42: ld_n_n(registers->bc.b, registers->de.d); break;
			case 0x43: ld_n_n(registers->bc.b, registers->de.e); break;
			case 0x44: ld_n_n(registers->bc.b, registers->hl.h); break;
			case 0x45: ld_n_n(registers->bc.b, registers->hl.l); break;
			case 0x46: ldh_n_adr(registers->bc.b, registers->hl.w); break;
			case 0x47: ld_n_n(registers->bc.b, registers->af.a); break;
			case 0x48: ld_n_n(registers->bc.c, registers->bc.b); break;
			case 0x49: ld_n_n(registers->bc.c, registers->bc.c); break;
			case 0x4a: ld_n_n(registers->bc.c, registers->de.d); break;
			case 0x4b: ld_n_n(registers->bc.c, registers->de.e); break;
			case 0x4c: ld_n_n(registers->bc.c, registers->hl.h); break;
			case 0x4d: ld_n_n(registers->bc.c, registers->hl.l); break;
			case 0x4e: ldh_n_adr(registers->bc.c, registers->hl.w); break;
			case 0x4f: ld_n_n(registers->bc.c, registers->af.a); break;
			case 0x50: ld_n_n(registers->de.d, registers->bc.b); break;
			case 0x51: ld_n_n(registers->de.d, registers->bc.c); break;
			case 0x52: ld_n_n(registers->de.d, registers->de.d); break;
			case 0x53: ld_n_n(registers->de.d, registers->de.e); break;
			case 0x54: ld_n_n(registers->de.d, registers->hl.h); break;
			case 0x55: ld_n_n(registers->de.d, registers->hl.l); break;
			case 0x56: ldh_n_adr(registers->de.d, registers->hl.w); break;
			case 0x57: ld_n_n(registers->de.d, registers->af.a); break;
			case 0x58: ld_n_n(registers->de.e, registers->bc.b); break;
			case 0x59: ld_n_n(registers->de.e, registers->bc.c); break;
			case 0x5a: ld_n_n(registers->de.e, registers->de.d); break;
			case 0x5b: ld_n_n(registers->de.e, registers->de.e); break;
			case 0x5c: ld_n_n(registers->de.e, registers->hl.h); break;
			case 0x5d: ld_n_n(registers->bc.c, registers->hl.l); break;
			case 0x5e: ldh_n_adr(registers->de.e, registers->hl.w); break;
			case 0x5f: ld_n_n(registers->de.e, registers->af.a); break;
			case 0x60: ld_n_n(registers->hl.h, registers->bc.b); break;
			case 0x61: ld_n_n(registers->hl.h, registers->bc.c); break;
			case 0x62: ld_n_n(registers->hl.h, registers->de.d); break;
			case 0x63: ld_n_n(registers->hl.h, registers->de.e); break;
			case 0x64: ld_n_n(registers->hl.h, registers->hl.h); break;
			case 0x65: ld_n_n(registers->hl.h, registers->hl.l); break;
			case 0x66: ldh_n_adr(registers->hl.h, registers->hl.w); break;
			case 0x67: ld_n_n(registers->hl.h, registers->af.a); break;
			case 0x68: ld_n_n(registers->hl.l, registers->bc.b); break;
			case 0x69: ld_n_n(registers->hl.l, registers->bc.c); break;
			case 0x6a: ld_n_n(registers->hl.l, registers->de.d); break;
			case 0x6b: ld_n_n(registers->hl.l, registers->de.e); break;
			case 0x6c: ld_n_n(registers->hl.l, registers->hl.h); break;
			case 0x6d: ld_n_n(registers->hl.l, registers->hl.l); break;
			case 0x6e: ldh_n_adr(registers->hl.l, registers->hl.w); break;
			case 0x6f: ld_n_n(registers->hl.l, registers->af.a); break;
			case 0x70: ldh_adr_n(registers->hl.w, registers->bc.b); break;
			case 0x71: ldh_adr_n(registers->hl.w, registers->bc.c); break;
			case 0x72: ldh_adr_n(registers->hl.w, registers->de.d); break;
			case 0x73: ldh_adr_n(registers->hl.w, registers->de.e); break;
			case 0x74: ldh_adr_n(registers->hl.w, registers->hl.h); break;
			case 0x75: ldh_adr_n(registers->hl.w, registers->hl.l); break;
			case 0x76: registers->pc--; break;
			case 0x77: ldh_adr_n(registers->hl.w, registers->af.a); break;
			case 0x78: ld_n_n(registers->af.a, registers->bc.b); break;
			case 0x79: ld_n_n(registers->af.a, registers->bc.c); break;
			case 0x7a: ld_n_n(registers->af.a, registers->de.d); break;
			case 0x7b: ld_n_n(registers->af.a, registers->de.e); break;
			case 0x7c: ld_n_n(registers->af.a, registers->hl.h); break;
			case 0x7d: ld_n_n(registers->af.a, registers->hl.l); break;
			case 0x7e: ldh_n_adr(registers->af.a, registers->hl.w); break;
			case 0x7f: ld_n_n(registers->af.a, registers->af.a); break;
			case 0x80: add_n_n(registers->af.a, registers->bc.b); break;
			case 0x81: add_n_n(registers->af.a, registers->bc.c); break;
			case 0x82: add_n_n(registers->af.a, registers->de.d); break;
			case 0x83: add_n_n(registers->af.a, registers->de.e); break;
			case 0x84: add_n_n(registers->af.a, registers->hl.h); break;
			case 0x85: add_n_n(registers->af.a, registers->hl.l); break;
			case 0x86: {byte t = mmu->read(registers->hl.w);  add_n_n(registers->af.a, t); break; }
			case 0x87: add_n_n(registers->af.a, registers->af.a); break;
			case 0x88: adc_a_n(registers->bc.b); break;
			case 0x89: adc_a_n(registers->bc.c); break;
			case 0x8a: adc_a_n(registers->de.d); break;
			case 0x8b: adc_a_n(registers->de.e); break;
			case 0x8c: adc_a_n(registers->hl.h); break;
			case 0x8d: adc_a_n(registers->hl.l); break;
			case 0x8e: {byte t = mmu->read(registers->hl.w); adc_a_n(t); break; }
			case 0x8f: adc_a_n(registers->af.a); break;
			case 0x90: sub(registers->bc.b); break;
			case 0x91: sub(registers->bc.c); break;
			case 0x92: sub(registers->de.d); break;
			case 0x93: sub(registers->de.e); break;
			case 0x94: sub(registers->hl.h); break;
			case 0x95: sub(registers->hl.l); break;
			case 0x96: {byte t = mmu->read(registers->hl.w); sub(t); break; }
			case 0x97: sub(registers->af.a); break;
			case 0x98: sbc(registers->bc.b); break;
			case 0x99: sbc(registers->bc.c); break;
			case 0x9a: sbc(registers->de.d); break;
			case 0x9b: sbc(registers->de.e); break;
			case 0x9c: sbc(registers->hl.h); break;
			case 0x9d: sbc(registers->hl.l); break;
			case 0x9e: {byte t = mmu->read(registers->hl.w); sbc(t); break; }
			case 0x9f: sbc(registers->af.a); break;
			case 0xa0: and(registers->bc.b); break;
			case 0xa1: and(registers->bc.c); break;
			case 0xa2: and(registers->de.d); break;
			case 0xa3: and(registers->de.e); break;
			case 0xa4: and(registers->hl.h); break;
			case 0xa5: and(registers->hl.l); break;
			case 0xa6: {byte t = mmu->read(registers->hl.w); and(t); break; }
			case 0xa7: and(registers->af.a); break;
			case 0xa8: and(registers->af.a); break;
			case 0xa9: xor(registers->bc.c); break;
			case 0xaa: xor(registers->de.d); break;
			case 0xab: xor(registers->de.e); break;
			case 0xac: xor(registers->hl.h); break;
			case 0xad: xor(registers->hl.l); break;
			case 0xae: {byte t = mmu->read(registers->hl.w); xor(t); }
			case 0xaf: xor(registers->af.a); break;
			case 0xb0: or(registers->bc.b); break;
			case 0xb1: or(registers->bc.c); break;
			case 0xb2: or(registers->de.d); break;
			case 0xb3: or(registers->de.e); break;
			case 0xb4: or(registers->hl.h); break;
			case 0xb5: or(registers->hl.l); break;
			case 0xb6: {byte t = mmu->read(registers->hl.w); or(t); break; }
			case 0xb7: or(registers->af.a); break;
			case 0xb8: cmp(registers->bc.b); break;
			case 0xb9: cmp(registers->bc.c); break;
			case 0xba: cmp(registers->de.d); break;
			case 0xbb: cmp(registers->de.e); break;
			case 0xbc: cmp(registers->hl.h); break;
			case 0xbd: cmp(registers->hl.l); break;
			case 0xbe: {byte t = mmu->read(registers->hl.w); cmp(t); break; }
			case 0xbf: cmp(registers->af.a); break;
			case 0xc0: ret(Registers::FLAG_MASK::ZERO, false); break;
			case 0xc1: pop(registers->bc.w); break;
			case 0xc2: jp(Registers::FLAG_MASK::ZERO, false); break;
			case 0xc3: jp(); break;
			case 0xc4: call(Registers::FLAG_MASK::ZERO, false); break;
			case 0xc5: push(registers->bc.w); break;
			case 0xc6: {byte t = mmu->next();  add_n_n(registers->af.a, t); break; }
			case 0xc7: rst(0x0); break;
			case 0xc8: ret(Registers::FLAG_MASK::ZERO, true); break;
			case 0xc9: ret(); break;
			case 0xca: jp(Registers::FLAG_MASK::ZERO, true); break;
			case 0xcb: cb(); break;
			case 0xcc: call(Registers::FLAG_MASK::ZERO, true); break;
			case 0xcd: call(); break;
			case 0xce: {byte t = mmu->next(); adc_a_n(t); break; }
			case 0xcf: rst(0x8); break;
			case 0xd0: ret(Registers::FLAG_MASK::CARRY, false); break;
			case 0xd1: pop(registers->de.w); break;
			case 0xd2: jp(Registers::FLAG_MASK::CARRY, false); break;
			case 0xd3: break; //not used.
			case 0xd4: call(Registers::FLAG_MASK::CARRY, false); break;
			case 0xd5: push(registers->de.w); break;
			case 0xd6: { byte t = mmu->next(); sub(t); } break;
			case 0xd7: rst(0x10); break;
			case 0xd8: ret(Registers::FLAG_MASK::CARRY, true); break;
			case 0xd9: pop(registers->pc); registers->interrupt_master_enable = true; break;
			case 0xda: jp(Registers::FLAG_MASK::CARRY, true); break;
			case 0xdb: break; // not used.
			case 0xdc: call(Registers::FLAG_MASK::CARRY, true); break;
			case 0xdd: break; // not used.
			case 0xde: { byte t = mmu->next(); sbc(t); break; }
			case 0xdf: rst(0x18); break;
			case 0xe0: { word address = 0xFF00 + mmu->next(); ldh_adr_n(address, registers->af.a); break; }
			case 0xe1: pop(registers->hl.w); break;
			case 0xe2: { word t = 0xFF00 + registers->bc.c;  ldh_adr_n(t, registers->af.a);  break; }
			case 0xe3: break; // not used.
			case 0xe4: break; // not used.
			case 0xe5: push(registers->hl.w); break;
			case 0xe6: { byte t = mmu->next(); and(t); break; }
			case 0xe7: rst(0x20); break;
			case 0xe8: {signed char t = mmu->next(); registers->off(registers->ZERO); add_nn_n(registers->sp, t); break; }
			case 0xe9: jp(registers->hl.w);  break;
			case 0xea: { word t = mmu->next() | (mmu->next() << 8); ldh_adr_n(t, registers->af.a); break; }
			case 0xeb: break; // not used.
			case 0xec: break; // not used.		
			case 0xed: break; // not used.
			case 0xee: { byte t = mmu->next(); xor(t); break; }
			case 0xef: rst(0x28); break;
			case 0xf0: { word address = 0xFF00 + mmu->next(); ldh_n_adr(registers->af.a, address); break; }
			case 0xf1: pop(registers->af.w); registers->af.w &= 0xFFF0; break;
			case 0xf2: { word t = 0xFF00 + registers->bc.c;  ldh_n_adr(registers->af.a, t); break; }
			case 0xf3: registers->interrupt_master_enable = 0; break;
			case 0xf4: break; // not used.		
			case 0xf5: push(registers->af.w); break;
			case 0xf6: { byte t = mmu->next(); or(t); break; }
			case 0xf7: rst(0x30); break;
			case 0xf8:
			{
						 signed char t = mmu->next();
						 registers->set(registers->CARRY, (registers->sp + t) > 0xFFFF);
						 registers->set(registers->HALF_CARRY, ((registers->sp & 0x0FFF) + t) > 0x0FFF);
						 registers->off(registers->N_SUBTRACT);
						 registers->off(registers->ZERO);
						 ld_nn_nn(registers->hl.w, registers->sp + t); break;
			}
			case 0xf9: ld_nn_nn(registers->sp, registers->hl.w); break;
			case 0xfa: {word t = mmu->read(mmu->next() | (mmu->next() << 8)); ld_a_n(t); break; }
			case 0xfb: registers->interrupt_master_enable = true; break;
			case 0xfc: break; // not used.		
			case 0xfd: break; // not used.
			case 0xfe: { byte t = mmu->next(); cmp(t); break; }
			case 0xff: rst(0x38); break;
			default: cout << "unknown opcode:" << "0x" << hex << 0x0 + opcode << "\n"; break;
		}
	}

	inline void cb() {
		byte opcode = mmu->next();
		clock_cycles += (CYCLES[opcode] * 4);
		switch(opcode) {
			case 0x0: rlc(registers->bc.b); break;
			case 0x1: rlc(registers->bc.c); break;
			case 0x2: rlc(registers->de.d); break;
			case 0x3: rlc(registers->de.e); break;
			case 0x4: rlc(registers->hl.h); break;
			case 0x5: rlc(registers->hl.l); break;
			case 0x6: { byte t = mmu->read(registers->hl.w); rlc(t); mmu->write(registers->hl.w, t); break; }
			case 0x7: rlc(registers->af.a); break;
			case 0x8: rrc(registers->bc.b); break;
			case 0x9: rrc(registers->bc.c); break;
			case 0xa: rrc(registers->de.d); break;
			case 0xb: rrc(registers->de.e); break;
			case 0xc: rrc(registers->hl.h); break;
			case 0xd: rrc(registers->hl.l); break;
			case 0xe: { byte t = mmu->read(registers->hl.w); rrc(t); mmu->write(registers->hl.w, t); break; }
			case 0xf: rrc(registers->af.a); break;
			case 0x10: rl(registers->bc.b); break;
			case 0x11: rl(registers->bc.c); break;
			case 0x12: rl(registers->de.d); break;
			case 0x13: rl(registers->de.e); break;
			case 0x14: rl(registers->hl.h); break;
			case 0x15: rl(registers->hl.l); break;
			case 0x16: { byte t = mmu->read(registers->hl.w); rl(t); mmu->write(registers->hl.w, t); break; }
			case 0x17: rl(registers->af.a); break;
			case 0x18: rr(registers->bc.b); break;
			case 0x19: rr(registers->bc.c); break;
			case 0x1a: rr(registers->de.d); break;
			case 0x1b: rr(registers->de.e); break;
			case 0x1c: rr(registers->hl.h); break;
			case 0x1d: rr(registers->hl.l); break;
			case 0x1e: { byte t = mmu->read(registers->hl.w); rr(t); mmu->write(registers->hl.w, t); break; }
			case 0x1f: rr(registers->af.a); break;
			case 0x20: sla(registers->bc.b); break;
			case 0x21: sla(registers->bc.c); break;
			case 0x22: sla(registers->de.d); break;
			case 0x23: sla(registers->de.e); break;
			case 0x24: sla(registers->hl.h); break;
			case 0x25: sla(registers->hl.l); break;
			case 0x26: { byte t = mmu->read(registers->hl.w); sla(t); mmu->write(registers->hl.w, t); break; }
			case 0x27: sla(registers->af.a); break;
			case 0x28: sra(registers->bc.b); break;
			case 0x29: sra(registers->bc.c); break;
			case 0x2a: sra(registers->de.d); break;
			case 0x2b: sra(registers->de.e); break;
			case 0x2c: sra(registers->hl.h); break;
			case 0x2d: sra(registers->hl.l); break;
			case 0x2e: { byte t = mmu->read(registers->hl.w); sra(t); mmu->write(registers->hl.w, t); break; }
			case 0x2f: sra(registers->af.a); break;
			case 0x30: swap(registers->bc.b); break;
			case 0x31: swap(registers->bc.c); break;
			case 0x32: swap(registers->de.d); break;
			case 0x33: swap(registers->de.e); break;
			case 0x34: swap(registers->hl.h); break;
			case 0x35: swap(registers->hl.l); break;
			case 0x36: { byte t = mmu->read(registers->hl.w); swap(t); mmu->write(registers->hl.w, t); break; }
			case 0x37: swap(registers->af.a); break;
			case 0x38: srl(registers->bc.b); break;
			case 0x39: srl(registers->bc.c); break;
			case 0x3a: srl(registers->de.d); break;
			case 0x3b: srl(registers->de.e); break;
			case 0x3c: srl(registers->hl.h); break;
			case 0x3d: srl(registers->hl.l); break;
			case 0x3e: { byte t = mmu->read(registers->hl.w); srl(t); mmu->write(registers->hl.w, t); break; }
			case 0x3f: srl(registers->af.a); break;
			case 0x40: bit(0, registers->bc.b); break;
			case 0x41: bit(0, registers->bc.c); break;
			case 0x42: bit(0, registers->de.d); break;
			case 0x43: bit(0, registers->de.e); break;
			case 0x44: bit(0, registers->hl.h); break;
			case 0x45: bit(0, registers->hl.l); break;
			case 0x46: { byte t = mmu->read(registers->hl.w); bit(0, t); mmu->write(registers->hl.w, t); break; }
			case 0x47: bit(0, registers->af.a); break;
			case 0x48: bit(1, registers->bc.b); break;
			case 0x49: bit(1, registers->bc.c); break;
			case 0x4a: bit(1, registers->de.d); break;
			case 0x4b: bit(1, registers->de.e); break;
			case 0x4c: bit(1, registers->hl.h); break;
			case 0x4d: bit(1, registers->hl.l); break;
			case 0x4e: { byte t = mmu->read(registers->hl.w); bit(1, t); mmu->write(registers->hl.w, t); break; }
			case 0x4f: bit(1, registers->af.a); break;
			case 0x50: bit(2, registers->bc.b); break;
			case 0x51: bit(2, registers->bc.c); break;
			case 0x52: bit(2, registers->de.d); break;
			case 0x53: bit(2, registers->de.e); break;
			case 0x54: bit(2, registers->hl.h); break;
			case 0x55: bit(2, registers->hl.l); break;
			case 0x56: { byte t = mmu->read(registers->hl.w); bit(2, t); mmu->write(registers->hl.w, t); break; }
			case 0x57: bit(2, registers->af.a); break;
			case 0x58: bit(3, registers->bc.b); break;
			case 0x59: bit(3, registers->bc.c); break;
			case 0x5a: bit(3, registers->de.d); break;
			case 0x5b: bit(3, registers->de.e); break;
			case 0x5c: bit(3, registers->hl.h); break;
			case 0x5d: bit(3, registers->hl.l); break;
			case 0x5e: { byte t = mmu->read(registers->hl.w); bit(3, t); mmu->write(registers->hl.w, t); break; }
			case 0x5f: bit(3, registers->af.a); break;
			case 0x60: bit(4, registers->bc.b); break;
			case 0x61: bit(4, registers->bc.c); break;
			case 0x62: bit(4, registers->de.d); break;
			case 0x63: bit(4, registers->de.e); break;
			case 0x64: bit(4, registers->hl.h); break;
			case 0x65: bit(4, registers->hl.l); break;
			case 0x66: { byte t = mmu->read(registers->hl.w); bit(4, t); mmu->write(registers->hl.w, t); break; }
			case 0x67: bit(4, registers->af.a); break;
			case 0x68: bit(5, registers->bc.b); break;
			case 0x69: bit(5, registers->bc.c); break;
			case 0x6a: bit(5, registers->de.d); break;
			case 0x6b: bit(5, registers->de.e); break;
			case 0x6c: bit(5, registers->hl.h); break;
			case 0x6d: bit(5, registers->hl.l); break;
			case 0x6e: { byte t = mmu->read(registers->hl.w); bit(5, t); mmu->write(registers->hl.w, t); break; }
			case 0x6f: bit(5, registers->af.a); break;
			case 0x70: bit(6, registers->bc.b); break;
			case 0x71: bit(6, registers->bc.c); break;
			case 0x72: bit(6, registers->de.d); break;
			case 0x73: bit(6, registers->de.e); break;
			case 0x74: bit(6, registers->hl.h); break;
			case 0x75: bit(6, registers->hl.l); break;
			case 0x76: { byte t = mmu->read(registers->hl.w); bit(6, t); mmu->write(registers->hl.w, t); break; }
			case 0x77: bit(6, registers->af.a); break;
			case 0x78: bit(7, registers->bc.b); break;
			case 0x79: bit(7, registers->bc.c); break;
			case 0x7a: bit(7, registers->de.d); break;
			case 0x7b: bit(7, registers->de.e); break;
			case 0x7c: bit(7, registers->hl.h); break;
			case 0x7d: bit(7, registers->hl.l); break;
			case 0x7e: { byte t = mmu->read(registers->hl.w); bit(7, t); mmu->write(registers->hl.w, t); break; }
			case 0x7f: bit(7, registers->af.a); break;
			case 0x80: res(0, registers->bc.b); break;
			case 0x81: res(0, registers->bc.c); break;
			case 0x82: res(0, registers->de.d); break;
			case 0x83: res(0, registers->de.e); break;
			case 0x84: res(0, registers->hl.h); break;
			case 0x85: res(0, registers->hl.l); break;
			case 0x86: { byte t = mmu->read(registers->hl.w); res(0, t); mmu->write(registers->hl.w, t); break; }
			case 0x87: res(0, registers->af.a); break;
			case 0x88: res(1, registers->bc.b); break;
			case 0x89: res(1, registers->bc.c); break;
			case 0x8a: res(1, registers->de.d); break;
			case 0x8b: res(1, registers->de.e); break;
			case 0x8c: res(1, registers->hl.h); break;
			case 0x8d: res(1, registers->hl.l); break;
			case 0x8e: { byte t = mmu->read(registers->hl.w); res(1, t); mmu->write(registers->hl.w, t); break; }
			case 0x8f: res(1, registers->af.a); break;
			case 0x90: res(2, registers->bc.b); break;
			case 0x91: res(2, registers->bc.c); break;
			case 0x92: res(2, registers->de.d); break;
			case 0x93: res(2, registers->de.e); break;
			case 0x94: res(2, registers->hl.h); break;
			case 0x95: res(2, registers->hl.l); break;
			case 0x96: { byte t = mmu->read(registers->hl.w); res(2, t); mmu->write(registers->hl.w, t); break; }
			case 0x97: res(2, registers->af.a); break;
			case 0x98: res(3, registers->bc.b); break;
			case 0x99: res(3, registers->bc.c); break;
			case 0x9a: res(3, registers->de.d); break;
			case 0x9b: res(3, registers->de.e); break;
			case 0x9c: res(3, registers->hl.h); break;
			case 0x9d: res(3, registers->hl.l); break;
			case 0x9e: { byte t = mmu->read(registers->hl.w); res(3, t); mmu->write(registers->hl.w, t); break; }
			case 0x9f: res(3, registers->af.a); break;
			case 0xa0: res(4, registers->bc.b); break;
			case 0xa1: res(4, registers->bc.c); break;
			case 0xa2: res(4, registers->de.d); break;
			case 0xa3: res(4, registers->de.e); break;
			case 0xa4: res(4, registers->hl.h); break;
			case 0xa5: res(4, registers->hl.l); break;
			case 0xa6: { byte t = mmu->read(registers->hl.w); res(4, t); mmu->write(registers->hl.w, t); break; }
			case 0xa7: res(4, registers->af.a); break;
			case 0xa8: res(5, registers->bc.b); break;
			case 0xa9: res(5, registers->bc.c); break;
			case 0xaa: res(5, registers->de.d); break;
			case 0xab: res(5, registers->de.e); break;
			case 0xac: res(5, registers->hl.h); break;
			case 0xad: res(5, registers->hl.l); break;
			case 0xae: { byte t = mmu->read(registers->hl.w); res(5, t); mmu->write(registers->hl.w, t); break; }
			case 0xaf: res(5, registers->af.a); break;
			case 0xb0: res(6, registers->bc.b); break;
			case 0xb1: res(6, registers->bc.c); break;
			case 0xb2: res(6, registers->de.d); break;
			case 0xb3: res(6, registers->de.e); break;
			case 0xb4: res(6, registers->hl.h); break;
			case 0xb5: res(6, registers->hl.l); break;
			case 0xb6: { byte t = mmu->read(registers->hl.w); res(6, t); mmu->write(registers->hl.w, t); break; }
			case 0xb7: res(6, registers->af.a); break;
			case 0xb8: res(7, registers->bc.b); break;
			case 0xb9: res(7, registers->bc.c); break;
			case 0xba: res(7, registers->de.d); break;
			case 0xbb: res(7, registers->de.e); break;
			case 0xbc: res(7, registers->hl.h); break;
			case 0xbd: res(7, registers->hl.l); break;
			case 0xbe: { byte t = mmu->read(registers->hl.w); res(7, t); mmu->write(registers->hl.w, t); break; }
			case 0xbf: res(7, registers->af.a); break;
			case 0xc0: set(0, registers->bc.b); break;
			case 0xc1: set(0, registers->bc.c); break;
			case 0xc2: set(0, registers->de.d); break;
			case 0xc3: set(0, registers->de.e); break;
			case 0xc4: set(0, registers->hl.h); break;
			case 0xc5: set(0, registers->hl.l); break;
			case 0xc6: { byte t = mmu->read(registers->hl.w); set(0, t); mmu->write(registers->hl.w, t); break; }
			case 0xc7: set(0, registers->af.a); break;
			case 0xc8: set(1, registers->bc.b); break;
			case 0xc9: set(1, registers->bc.c); break;
			case 0xca: set(1, registers->de.d); break;
			case 0xcb: set(1, registers->de.e); break;
			case 0xcc: set(1, registers->hl.h); break;
			case 0xcd: set(1, registers->hl.l); break;
			case 0xce: { byte t = mmu->read(registers->hl.w); set(1, t); mmu->write(registers->hl.w, t); break; }
			case 0xcf: set(1, registers->af.a); break;
			case 0xd0: set(2, registers->bc.b); break;
			case 0xd1: set(2, registers->bc.c); break;
			case 0xd2: set(2, registers->de.d); break;
			case 0xd3: set(2, registers->de.e); break;
			case 0xd4: set(2, registers->hl.h); break;
			case 0xd5: set(2, registers->hl.l); break;
			case 0xd6: { byte t = mmu->read(registers->hl.w); set(2, t); mmu->write(registers->hl.w, t); break; }
			case 0xd7: set(2, registers->af.a); break;
			case 0xd8: set(3, registers->bc.b); break;
			case 0xd9: set(3, registers->bc.c); break;
			case 0xda: set(3, registers->de.d); break;
			case 0xdb: set(3, registers->de.e); break;
			case 0xdc: set(3, registers->hl.h); break;
			case 0xdd: set(3, registers->hl.l); break;
			case 0xde: { byte t = mmu->read(registers->hl.w); set(3, t); mmu->write(registers->hl.w, t); break; }
			case 0xdf: set(3, registers->af.a); break;
			case 0xe0: set(4, registers->bc.b); break;
			case 0xe1: set(4, registers->bc.c); break;
			case 0xe2: set(4, registers->de.d); break;
			case 0xe3: set(4, registers->de.e); break;
			case 0xe4: set(4, registers->hl.h); break;
			case 0xe5: set(4, registers->hl.l); break;
			case 0xe6: { byte t = mmu->read(registers->hl.w); set(4, t); mmu->write(registers->hl.w, t); break; }
			case 0xe7: set(4, registers->af.a); break;
			case 0xe8: set(5, registers->bc.b); break;
			case 0xe9: set(5, registers->bc.c); break;
			case 0xea: set(5, registers->de.d); break;
			case 0xeb: set(5, registers->de.e); break;
			case 0xec: set(5, registers->hl.h); break;
			case 0xed: set(5, registers->hl.l); break;
			case 0xee: { byte t = mmu->read(registers->hl.w); set(5, t); mmu->write(registers->hl.w, t); break; }
			case 0xef: set(5, registers->af.a); break;
			case 0xf0: set(6, registers->bc.b); break;
			case 0xf1: set(6, registers->bc.c); break;
			case 0xf2: set(6, registers->de.d); break;
			case 0xf3: set(6, registers->de.e); break;
			case 0xf4: set(6, registers->hl.h); break;
			case 0xf5: set(6, registers->hl.l); break;
			case 0xf6: { byte t = mmu->read(registers->hl.w); set(6, t); mmu->write(registers->hl.w, t); break; }
			case 0xf7: set(6, registers->af.a); break;
			case 0xf8: set(7, registers->bc.b); break;
			case 0xf9: set(7, registers->bc.c); break;
			case 0xfa: set(7, registers->de.d); break;
			case 0xfb: set(7, registers->de.e); break;
			case 0xfc: set(7, registers->hl.h); break;
			case 0xfd: set(7, registers->hl.l); break;
			case 0xfe: { byte t = mmu->read(registers->hl.w); set(7, t); mmu->write(registers->hl.w, t); break; }
			case 0xff: set(7, registers->af.a); break;
			default: cout << "unknown opcode:" << "0xCB 0x" << hex << opcode << "\n"; break;
		}
	}

	void Z80::run() {
		auto sync = std::chrono::steady_clock::now();
		while(true) {
			if(clock_cycles >= 4194304) {
				if(std::chrono::duration_cast<std::chrono::duration<double>>(std::chrono::steady_clock::now() - sync).count() > 1) {
					sync = std::chrono::steady_clock::now();
					clock_cycles = 0;
				}
			} else {
				handle_interrupts();
				process();
				dma_transfer();
			}
		}
	}

	void Z80::handle_interrupts() {
		if(registers->interrupt_master_enable == 1 && registers->get_register(Registers::IE) && registers->get_register(Registers::IF)) {
			if((registers->get_register(Registers::IE) & (1 << 0)) && (registers->get_register(Registers::IF) & (1 << 0))) { //VBlank
				if(mmu->read(registers->pc) == 0x76) {
					registers->pc++;
				} else {
					registers->set_register(Registers::IF, 0, false);
					registers->interrupt_master_enable = false;
					push(registers->pc);
					registers->pc = 0x40;
				}
			} else if((registers->get_register(Registers::IE) & (1 << 1)) && (registers->get_register(Registers::IF) & (1 << 1))) { //LCD STAT
				if(mmu->read(registers->pc) == 0x76) {
					registers->pc++;
				} else {
					registers->set_register(Registers::IF, 1, false);
					registers->interrupt_master_enable = false;
					push(registers->pc);
					registers->pc = 0x48;
				}
			} else if((registers->get_register(Registers::IE) & (1 << 2)) && (registers->get_register(Registers::IF) & (1 << 2))) { //Timer
				if(mmu->read(registers->pc) == 0x76) {
					registers->pc++;
				} else {
					registers->set_register(Registers::IF, 2, false);
					registers->interrupt_master_enable = false;
					push(registers->pc);
					registers->pc = 0x50;
				}
			} else if((registers->get_register(Registers::IE) & (1 << 3)) && (registers->get_register(Registers::IF) & (1 << 3))) { //Serial Port I/O
				if(mmu->read(registers->pc) == 0x76) {
					registers->pc++;
				} else {
					registers->set_register(Registers::IF, 3, false);
					registers->interrupt_master_enable = false;
					push(registers->pc);
					registers->pc = 0x58;
				}
			} else if((registers->get_register(Registers::IE) & (1 << 4)) && (registers->get_register(Registers::IF) & (1 << 4))) { //Joypad
				cout << "button pressed" << endl;

				if(mmu->read(registers->pc) == 0x76) {
					registers->pc++;
				} else {
					registers->set_register(Registers::IF, 4, false);
					registers->interrupt_master_enable = false;
					push(registers->pc);
					registers->pc = 0x60;
				}
			}
		}
	}

	inline void Z80::dma_transfer() { // Maybe move this into the V-Blank interrupt.
		if(registers->get_register(Registers::DMA)) {// && registers->get_register(Registers::IF) & (1 << 0)) {
			word source = registers->get_register(Registers::DMA) << 8;
			mmu->write(Registers::DMA, 0);
			for(int i = 0; i < 159; i++) { // 160 or 159..?
				mmu->write(0xFE00 + i, mmu->read(source + i));
			}
		}
	}

	Z80::Z80(memory::MMU &_mmu, memory::Registers &_registers) {
		mmu = &_mmu;
		registers = &_registers;
	}
	Z80::Z80() {}
	Z80::~Z80() { mmu = nullptr; }
}