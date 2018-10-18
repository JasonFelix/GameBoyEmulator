// GameBoy.cpp : Defines the entry point for the console application.
//
#include "stdafx.h"
#include "vector"
#include <string>
#include <iostream>
#include <fstream>
#include <cstdio>
#include "Cartridge.h"
#include "Z80.h"
#include "MMU.h"
#include "GameBoy.h"
#include "Window.h"
#include "Registers.h"
#include "Joypad.h"
#include "MBC1.h"
#include <thread>

using namespace std;
using namespace gameboy;

namespace gameboy {
	cpu::Z80 z80;
	memory::MMU mmu;
	graphics::Window window;
	memory::Registers registers;
	input::Joypad joypad;
}

int _tmain(int argc, _TCHAR* argv[]) {	
	Cartridge cart("c:/gb/sml.gb"); 
	cart.mbc = MBC1(cart);
	cout << cart.to_string();

	mmu = memory::MMU();
	mmu.load_cartridge(cart);
	registers = memory::Registers(mmu);
	z80 = cpu::Z80(mmu, registers);
	joypad = input::Joypad();
	window = graphics::Window(mmu, registers, joypad);

	mmu.attach_joypad(joypad);
	mmu.set_registers(registers);
	std::thread lcd(&graphics::Window::run, window);
	std::thread timers(&memory::Registers::handle_timers, registers);
	z80.run();

	string wait;
	std::cin >> wait;
	return 0;
}



