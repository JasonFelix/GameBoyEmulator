#include "stdafx.h"
#include "Cartridge.h"
#include "MBC1.h"
#include <string>
#include <iostream>
#include <fstream>
#include <vector>
#include <sstream>

using namespace std;


vector<char> rom;
MBC mbc;

Cartridge::Cartridge(std::string filename) {
	ifstream ifs(filename, ios::binary | ios::ate);
	ifstream::pos_type size = ifs.tellg();
	rom = vector<char>(size);
	ifs.seekg(0, ios::beg);
	ifs.read(&rom[0], size);
	set_type();
}


Cartridge::~Cartridge() {
}

string Cartridge::get_title() {
	return string(rom.data() + 0x134, 16);
}

short Cartridge::get_size() {
	return 32 << rom.at(0x148);
}

bool Cartridge::is_japanese() {
	return rom.at(0x14A) == 1;
}


void Cartridge::set_type() {
	switch(rom.at(0x147)) {
		case 0x0: type.rom = true;
			break;
		case 0x1: type.mbc = 1;
			break;
		case 0x2: type.mbc = 1; type.ram = true;
			break;
		case 0x3: type.mbc = 1; type.ram = true; type.battery = true;
			break;
		case 0x5: type.mbc = 2;
			break;
		case 0x6: type.mbc = 2; type.battery = true;
			break;
		case 0x8: type.rom = true; type.ram = true;
			break;
		case 0x9: type.rom = true; type.ram = true; type.battery = true;
			break;
		case 0xB: type.mmo1 = true;
			break;
		case 0xC: type.mmo1 = true; type.ram = true;
			break;
		case 0xD: type.mmo1 = true; type.ram = true; type.battery = true;
			break;
		case 0xF: type.mbc = 3; type.timer = true; type.battery = true;
			break;
		case 0x10: type.mbc = 3; type.timer = true; type.ram = true; type.battery = true;
			break;
		case 0x11: type.mbc = 3;
			break;
		case 0x12: type.mbc = 3; type.ram = true;
			break;
		case 0x13: type.mbc = 3; type.ram = true; type.battery = true;
			break;
		case 0x15: type.mbc = 4;
			break;
		case 0x16: type.mbc = 4; type.ram = true;
			break;
		case 0x17: type.mbc = 4; type.ram = true; type.battery = true;
			break;
		case 0x19: type.mbc = 5;
			break;
		case 0x1A: type.mbc = 5; type.ram = true;
			break;
		case 0x1B: type.mbc = 5; type.ram = true; type.battery = true;
			break;
		case 0x1C: type.mbc = 5; type.rumble = true;
			break;
		case 0x1D: type.mbc = 5; type.rumble = true; type.ram = true;
			break;
		case 0x1E: type.mbc = 5; type.rumble = true; type.ram = true; type.battery = true;
			break;
		case 0xFC: type.pocket_camera = true;
			break;
		case 0xFD: type.bandai_tama5 = true;
			break;
		case 0xFE: type.huc3 = true;
			break;
		case 0xFF: type.huc1 = true; type.ram = true; type.battery = true;
			break;

		default:
			cout << "Uh oh.. Cartridge type not found.\n";
			break;
	}
}

string Cartridge::to_string() {
	std::stringstream buffer;

	buffer << "Game name:" << get_title()
		<< "\nSize:" << get_size() << "KB" << std::endl
		<< "\nJapanese:" << is_japanese() << std::endl
		<< "Cartridge Type: "
		<< "\n rom:" << type.rom
		<< "\n mbc:" << type.mbc
		<< "\n ram:" << type.ram
		<< "\n battery:" << type.battery
		<< "\n timer:" << type.timer
		<< "\n pocket camera:" << type.pocket_camera
		<< "\n huc1:" << type.huc1
		<< "\n huc3:" << type.huc3
		<< "\n mmo1:" << type.mmo1
		<< "\n bandai tama5:" << type.bandai_tama5;
	return buffer.str();
}