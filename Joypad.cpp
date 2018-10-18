#include "stdafx.h"
#include "Joypad.h"
#include <string>
#include <iostream>
#include <list>
#include <mutex>
#include <thread>

using byte = unsigned char; //8 bit
using word = unsigned short;//16 bit

namespace input {
	Joypad::Joypad() {}


	Joypad::~Joypad() {}

	auto mode = Joypad::DIRECTION;
	auto directional = std::list<Joypad::Button>();
	auto action = std::list<Joypad::Button>();
	std::mutex lock;

	void Joypad::press(Joypad::Button button, Joypad::Mode _mode) {
		lock.lock();
		if(_mode == Joypad::DIRECTION)
			directional.push_back(button);
		else
			action.push_back(button);
		directional.unique();
		action.unique();
		lock.unlock();
	}

	void Joypad::release(Joypad::Button button) {
		lock.lock();
		for(auto _button : directional)
		if(button == _button) {
			directional.remove(_button);
			break;
		}
		for(auto _button : action)
		if(button == _button) {
			action.remove(_button);
			break;
		}
		lock.unlock();
	}

	byte Joypad::get() {
		byte state = 0x0;
		lock.lock();
		if(mode == Joypad::DIRECTION)
			for(Button button : directional) 
				state |= button;
		else
			for(Button button : action)
				state |= button;
		state |= mode;
		lock.unlock();
		return ~state;
	}

	void Joypad::set_mode(Joypad::Mode _mode) {
		mode = _mode;
	}
}