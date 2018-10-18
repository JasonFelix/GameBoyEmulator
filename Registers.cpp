#include "stdafx.h"
#include "Registers.h"
#include "MMU.h"
#include "GameBoy.h"
#include <thread>

using byte = unsigned char;//8 bit
using word = unsigned short;// 16 bit

namespace memory {
	MMU* mmu;

	void handle_timers();

	Registers::Registers(memory::MMU &_mmu) {
		//set up regiters.
		mmu = &_mmu;
		mmu->write(Registers::JOYPAD, 0xFF);
		mmu->write(Registers::LCDC, 0x91);
		mmu->write(Registers::BGP, 0xFC);
		mmu->write(Registers::OBP0, 0xFF);
		mmu->write(Registers::OBP1, 0xFF);
		mmu->write(Registers::DMA, 0xFF);
		mmu->write(Registers::IF, 0x1);
		mmu->write(Registers::SC, 0x7E);
	}
	Registers::Registers() {}
	Registers::~Registers() {
	}

	byte Registers::get_register(word _register) {
		return mmu->read(_register);
	}	
	bool Registers::get_register(word _register, byte bit) {
		return mmu->read(_register) & (1 << bit);
	}
	void Registers::set_register(word _register, byte bit, bool set) {
		if(set)
			mmu->write(_register, mmu->read(_register) | (0x1 << bit));
		else
			mmu->write(_register, mmu->read(_register) & ~(0x1 << bit));
	}

	/*FF07 - TAC - Timer Control (R/W)
	Bit 2 - Timer Stop (0=Stop, 1=Start)
	Bits 1-0 - Input Clock Select
	00: 4096 Hz (~4194 Hz SGB) x 1
	01: 262144 Hz (~268400 Hz SGB) x 64
	10: 65536 Hz (~67110 Hz SGB) x 16
	11: 16384 Hz (~16780 Hz SGB) x 4
	*/
	void Registers::handle_timers() {
		long i = 0;
		while(true) {
			std::thread timer([](){std::this_thread::sleep_for(std::chrono::microseconds(244)); }); 
			i++;
			if(!(i % 4))
				mmu->write(Registers::DIV, mmu->read(Registers::DIV) + 1);
			if(mmu->read(Registers::TAC) & (1 << 2)) {
				switch(mmu->read(Registers::TAC) & 3) {
					case 0:
						mmu->write(Registers::TIMA, mmu->read(Registers::TIMA) + 1);
						break;
					case 1:
						if(i % 64 == 0)
							mmu->write(Registers::TIMA, mmu->read(Registers::TIMA) + 1);
						break;
					case 2:
						if(i % 16 == 0)
							mmu->write(Registers::TIMA, mmu->read(Registers::TIMA) + 1);
						break;
					case 3:
						if(i % 4 == 0)
							mmu->write(Registers::TIMA, mmu->read(Registers::TIMA) + 1);
						break;
				}
				if(mmu->read(Registers::TIMA) >= 0xFF) {
					mmu->write(Registers::TIMA, mmu->read(Registers::TMA));
					set_register(Registers::IF, 2, true);
				}
			}
			timer.join();
		}
	}

}