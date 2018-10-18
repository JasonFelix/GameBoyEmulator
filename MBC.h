#pragma once
#include <memory>
#include <vector>

using byte = unsigned char;//8 bit
using word = unsigned short;// 16 bit

class Cartridge;
class MBC {
public:
	MBC(const Cartridge &cartridge);
	MBC();
	~MBC();

	byte current_ram_bank;
	byte current_rom_bank;

	std::vector<std::vector<byte>> rom_banks = std::vector<std::vector<byte>>();
	std::vector<std::vector<byte>> ram_banks = std::vector<std::vector<byte>>();

	void switch_rom_bank(byte bank) {
		current_rom_bank = bank;
	}

	void switch_ram_bank(byte bank) {
		current_ram_bank = bank;
	}

	virtual void handle(word address, byte value);
};