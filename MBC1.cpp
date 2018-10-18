#include "stdafx.h"
#include "MBC1.h"
#include "MBC.h"
#include "Cartridge.h"
#include <iostream>
#include <fstream>
#include <cstdio>
#include <iterator>

using byte = unsigned char;//8 bit
using word = unsigned short;// 16 bit

MBC1::MBC1(const Cartridge &cartridge) {
	rom_banks = std::vector<std::vector<byte>>(125, std::vector<byte>(16384));
	ram_banks = std::vector<std::vector<byte>>(4, std::vector<byte>(8192));
	for(int bank = 0; bank < (cartridge.rom.size() / 16384); bank++)
		std::copy(cartridge.rom.begin() + (0x4000 * bank), cartridge.rom.begin() + ((0x4000 * bank) + 0x3FFF), rom_banks[bank].begin());
	switch_rom_bank(1);
	switch_ram_bank(1);
}


MBC1::~MBC1() {
}
enum Mode { _8KB_RAM_2MB_ROM, _32KB_RAM_512KB_ROM };
Mode mode = _8KB_RAM_2MB_ROM;

byte low_bank, high_bank;

void MBC::handle(word address, byte value) {
	if(address <= 0x3FFF) {
		byte bank = value & 0x1F;
		low_bank = bank;
		switch(bank) {
			case 0x0:
				bank = 1;
				break;
			case 0x20:
				bank = 0x21;
				break;
			case 0x40:
				bank = 0x41;
				break;
			case 0x60:
				bank = 0x61;
				break;
		}
		if(_32KB_RAM_512KB_ROM) {
			bank = (high_bank << 5) | low_bank;
		}
		switch_rom_bank(bank);
		return;
	} else if(address <= 0x5FFF) {
		if(mode == _32KB_RAM_512KB_ROM) {
			byte bank = value & 0x3;
			bank = (high_bank << 5) | low_bank;
			switch_rom_bank(bank); //not sure if it's actually supposed to switch the bank..
		} else {
			switch_ram_bank(value & 0x3);
		}
		return;
	} else if(address <= 0x7FFF) {
		mode = (value & 0x1) == 1 ? _32KB_RAM_512KB_ROM : _8KB_RAM_2MB_ROM;
		return;
	}

}
