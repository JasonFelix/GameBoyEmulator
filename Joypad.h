#pragma once
#include <vector>
namespace input {

	class Joypad {
		using byte = unsigned char;
		using word = unsigned short;
	public:
		Joypad();
		~Joypad();

		enum Mode {
			DIRECTION = 0x20,
			ACTION = 0x10
		};

		enum Button {
			A = 0x1,
			B = 0x2,
			SELECT = 0x4,
			START = 0x8,

			RIGHT = 0x1,
			LEFT = 0x2,
			UP = 0x4,
			DOWN = 0x8
		};

		void set_mode(Mode mode);
		void press(Button button, Joypad::Mode);
		void release(Button button);
		byte get();
	};
}