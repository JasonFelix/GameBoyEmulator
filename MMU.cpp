#include "stdafx.h"
#include "MMU.h"
#include <vector>
#include <fstream>
#include <cstdio>
#include <iostream>

using byte = unsigned char;//8 bit
using word = unsigned short;// 16 bit

using namespace input;

namespace memory {
	
	Cartridge* cart;
	Registers* registers;
	Joypad* joypad;

	union _memory {
		byte whole[65536];
		struct _map {
			byte rom_bank_0[16384];
			byte rom_bank_n[16384];
			byte video_ram[8192];
			byte internal_ram_bank_0[8192];
			byte external_ram_bank_n[8192];
			byte ram_echo[7680];
			byte sprite_attribute_memory[160];
			byte empty_0[96];
			byte io_ports[76];
			byte empty_1[52];
			byte zero_page[127];
			byte interrupt;
		} map;
	} memory;

	void MMU::load_cartridge(Cartridge &cartridge) {
		cart = &cartridge;
		std::copy(cartridge.rom.begin(), cartridge.rom.begin() + 0x3FFF, memory.map.rom_bank_0);//5627
		std::copy(cartridge.rom.begin() + 0x4000, cartridge.rom.begin() + (0x3FFF + 0x3FFF), memory.map.rom_bank_n);
		cart->mbc.switch_rom_bank(0x1);
		cart->mbc.switch_ram_bank(0x1);
	}

	void MMU::set_registers(Registers &_registers) {
		registers = &_registers;
	}
	
	void MMU::attach_joypad(Joypad &_joypad) {
		joypad = &_joypad;
	}

	byte MMU::read(word address) {
		if(cart->get_type().mbc > 0 && address >= 0x4000 && address < 0x7FFF)
			return cart->mbc.rom_banks[cart->mbc.current_rom_bank][address - 0x4000];
		if(cart->get_type().mbc > 0 && address >= 0xA000 && address < 0xBFFF)
			return cart->mbc.ram_banks[cart->mbc.current_ram_bank][address - 0xA000];
		if(address == Registers::JOYPAD) 
			return joypad->get();
		return memory.whole[address];
	}

	byte MMU::next() {
		if(cart->get_type().mbc > 0 && registers->pc >= 0x4000 && registers->pc <= 0x7FFF)
			return cart->mbc.rom_banks[cart->mbc.current_rom_bank][registers->pc++ - 0x4000];
		if(cart->get_type().mbc > 0 && registers->pc >= 0xA000 && registers->pc <= 0xBFFF)
			return cart->mbc.ram_banks[cart->mbc.current_ram_bank][registers->pc++ - 0xA000];
		return memory.whole[registers->pc++];
	}

	void MMU::write(word address, byte value) {
		if(address >= 0x2000 && address <= 0x7FFF) {
			cart->mbc.handle(address, value);
			return;
		}
		if(address >= 0xC000 && address <= 0xDE00) {
			memory.whole[address] = value;
			memory.whole[0xFE00 + (address - 0xE000)];
			return;
		}
		if(address >= 0xFE00 && address <= 0xE000) {
			memory.whole[address] = value;
			memory.whole[0xC000 + (address - 0xDE00)];
			return;
		}
		if(address == Registers::DIV) { 
			memory.whole[Registers::DIV] = 0; 
			return;
		}
		if(address == Registers::JOYPAD) {
			joypad->set_mode((value & 0x30) == Joypad::DIRECTION ? Joypad::DIRECTION : Joypad::ACTION);
			return;
		}
		if(cart->get_type().mbc > 0 && address >= 0xA000 && address <= 0xBFFF) {
			cart->mbc.ram_banks[cart->mbc.current_ram_bank][address - 0xA000] = value;
			return; 
		}
		memory.whole[address] = value;
	}

	word MMU::get_rom_bank() {
		return cart->mbc.current_rom_bank;
	}	
	word MMU::get_ram_bank() {
		return cart->mbc.current_ram_bank;
	}

	MMU::MMU() {}
	MMU::~MMU() { cart = nullptr; }
}