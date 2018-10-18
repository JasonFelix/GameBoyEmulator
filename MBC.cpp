#include "stdafx.h"
#include "MBC.h"
#include <memory>
#include <vector>
#include "Cartridge.h"

using byte = unsigned char;//8 bit
using word = unsigned short;// 16 bit

char current_ram_bank;
char current_rom_bank;

std::vector<std::vector<byte>> rom_banks = std::vector<std::vector<byte>>();
std::vector<std::vector<byte>> ram_banks = std::vector<std::vector<byte>>();

MBC::MBC(const Cartridge &cartridge) {
}

MBC::MBC() {
}


MBC::~MBC() {
}
