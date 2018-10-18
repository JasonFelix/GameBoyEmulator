#pragma once
#include "Cartridge.h"
#include "Registers.h"
#include "Joypad.h"
#include "stdafx.h"

using byte = unsigned char;//8 bit
using word = unsigned short;// 16 bit

namespace memory {
	class MMU {
	public:
		MMU();
		~MMU();

		void load_cartridge(Cartridge &cartridge);
		void set_registers(Registers &registers);
		void attach_joypad(input::Joypad &joypad);
		unsigned char read(word address); //Read the byte located at the address.
		unsigned char next(); //Read the next byte.
		void write(word address, byte value);
		unsigned short get_rom_bank();
		unsigned short get_ram_bank();
	};

}