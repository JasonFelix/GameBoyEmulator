#include "stdafx.h"
#include "Window.h"
#include "Joypad.h"

#include <vector>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <thread>
#include <SDL.h>

using namespace memory;
using namespace std;
using namespace input;

using byte = unsigned char;//8 bit
using word = unsigned short;// 16 bit
namespace graphics {

	Registers* registers;
	Joypad* joypad;
	MMU* mmu;
	SDL_Surface* screen;

	const int SIZE_MULTIPLIER = 2;
	const int SCREEN_WIDTH = 160 * SIZE_MULTIPLIER;
	const int SCREEN_HEIGHT = 144 * SIZE_MULTIPLIER;
	void render_scanline();

	unsigned int line_offset;

	Window::Window(MMU &_mmu, Registers &_registers, Joypad &_joypad) {
		registers = &_registers;
		mmu = &_mmu;
		joypad = &_joypad;
	}

	Window::Window() {}

	void Window::run() {
		if(SDL_Init(SDL_INIT_EVERYTHING) != 0) {
			std::cout << "SDL_Init Error: " << SDL_GetError() << std::endl;
			return;
		}

		SDL_Window *win = SDL_CreateWindow("Gameboy", (1920 / 2) - (SCREEN_WIDTH / 2), (1080 / 2) - (SCREEN_HEIGHT / 2), SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_SHOWN); //| SDL_WINDOW_BORDERLESS);
		if(win == nullptr) {
			std::cout << "SDL_CreateWindow Error: " << SDL_GetError() << std::endl;
			return;
		}

		SDL_Renderer *ren = SDL_CreateRenderer(win, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
		if(ren == nullptr) {
			std::cout << "SDL_CreateRenderer Error: " << SDL_GetError() << std::endl;
			return;
		}

		SDL_Event e;
		bool quit = false;
		while(!quit) {
			auto vsync = std::chrono::steady_clock::now();
			line_offset = 0;
			if(!(registers->get_register(Registers::LCDC) & 0x80))
				continue;
			screen = SDL_CreateRGBSurface(0, SCREEN_WIDTH, SCREEN_HEIGHT, 32, 0, 0, 0, 0);
			for(int i = 0; i < 153; i++) {
				auto hsync = std::chrono::steady_clock::now();
				if(mmu->read(Registers::LYC) == mmu->read(Registers::LY))
					registers->set_register(Registers::STAT, 2, true);
				mmu->write(Registers::LY, line_offset++);
				registers->set_register(Registers::STAT, 0, false); //These two will set the STAT Register to HBlank = 0x0.
				registers->set_register(Registers::STAT, 1, false);
				if(registers->get_register(Registers::LCDC) & 0x1)
					render_background_scanline();
				if(registers->get_register(Registers::LCDC) & 0x20)
					render_window_scanline();
				if(registers->get_register(Registers::LCDC) == 0x80)
					while(std::chrono::duration_cast<std::chrono::duration<double, ratio<1, 1000000>>>(std::chrono::steady_clock::now() - hsync).count() < 48.6) {} //Wait for 48.6 microseconds
			}

			draw_sprites();

			SDL_Texture *tex = SDL_CreateTextureFromSurface(ren, screen);
			//mmu->write(Registers::JOYPAD, mmu->read(Registers::JOYPAD) | 0xFF);
			while(SDL_PollEvent(&e)) {
				if(e.type == SDL_QUIT)
					quit = true;
				switch(e.type) {
					case SDL_KEYDOWN:
						switch(e.key.keysym.sym) {
							case SDLK_UP:
							case SDLK_w:
								joypad->press(Joypad::UP, Joypad::DIRECTION);
								break;
							case SDLK_LEFT:
							case SDLK_a:
								joypad->press(Joypad::LEFT, Joypad::DIRECTION);
								break;
							case SDLK_DOWN:
							case SDLK_s:
								joypad->press(Joypad::DOWN, Joypad::DIRECTION);
								break;
							case SDLK_RIGHT:
							case SDLK_d:
								joypad->press(Joypad::RIGHT, Joypad::DIRECTION);
								break;
							case SDLK_k:
								joypad->press(Joypad::A, Joypad::ACTION);
								break;
							case SDLK_l:
								joypad->press(Joypad::B, Joypad::ACTION);
								break;
							case SDLK_o:
								joypad->press(Joypad::START, Joypad::ACTION);
								break;
							case SDLK_p:
								joypad->press(Joypad::SELECT, Joypad::ACTION);
								break;
						}
					break;
					case SDL_KEYUP:
						switch(e.key.keysym.sym) {
							case SDLK_UP:
							case SDLK_w:
								joypad->release(Joypad::UP);
								break;
							case SDLK_LEFT:
							case SDLK_a:
								joypad->release(Joypad::LEFT);
								break;
							case SDLK_DOWN:
							case SDLK_s:
								joypad->release(Joypad::DOWN);
								break;
							case SDLK_RIGHT:
							case SDLK_d:
								joypad->release(Joypad::RIGHT);
								break;
							case SDLK_k://A
								joypad->release(Joypad::A);
								break;
							case SDLK_l://B
								joypad->release(Joypad::B);
								break;
							case SDLK_o://Start
								joypad->release(Joypad::START);
								break;
							case SDLK_p://Select
								joypad->release(Joypad::SELECT);
								break;
						}
						break;
				}
			}
			SDL_RenderClear(ren);
			SDL_RenderCopy(ren, tex, NULL, NULL);
			SDL_RenderPresent(ren);

			registers->set_register(Registers::STAT, 0, false); //These two will set the STAT Register to VBlank = 0x1. 
			registers->set_register(Registers::STAT, 1, true);
			registers->set_register(Registers::IF, 0, true); //Set VBlank to true in the interrupt flag.
			while(std::chrono::duration_cast<std::chrono::duration<double, ratio<1, 1000>>>(std::chrono::steady_clock::now() - vsync).count() < 16.6) {} //Loop until 16.6ms is up.
			//cout << dec << 0 + std::chrono::duration_cast<std::chrono::duration<double, ratio<1, 1000>>>(std::chrono::steady_clock::now() - vsync).count() << endl;
			registers->set_register(Registers::IF, 0, false); //Set VBlank to true in the interrupt flag.
			//registers->set_register(Registers::IF, 4, false); //Not sure if im supposed to turn this off here..
			SDL_DestroyTexture(tex);
			SDL_FreeSurface(screen);
		}
	}

	inline void Window::draw_pixel(int x, int y, int colour) {
		SDL_Rect rect = {x * SIZE_MULTIPLIER, y * SIZE_MULTIPLIER - SIZE_MULTIPLIER, SIZE_MULTIPLIER, SIZE_MULTIPLIER};
		SDL_FillRect(screen, &rect, colour);
	}

	inline word Window::get_window_tile_map() {
		return registers->get_register(Registers::LCDC) & (1 << 6) ? 0x9C00 : 0x9800;
	}

	inline word Window::get_pattern_table() {
		return registers->get_register(Registers::LCDC) & (1 << 4) ? 0x8000 : 0x8800;
	}

	inline word Window::get_background_tile_map() {
		return registers->get_register(Registers::LCDC) & (1 << 3) ? 0x9C00 : 0x9800;
	}	

	/*
	Byte0 Y position on the screen
	Byte1 X position on the screen
	Byte2	Pattern number 0-255 (Unlike some tile numbers, sprite pattern numbers are unsigned.
	LSB is ignored (treated as 0) in 8x16 mode.)
	Byte3 Flags:
	Bit7 Priority
	If this bit is set to 0, sprite is displayed on top of background & window.
	If this bit is set to 1, then sprite will be hidden behind colors 1, 2, and 3 of the background & window. (Sprite only prevails over color 0 of BG & win.)
	Bit6 Y flip
	Sprite pattern is flipped vertically if this bit is set to 1.
	Bit5 X flip
	Sprite pattern is flipped horizontally if this bit is set to 1.
	Bit4 Palette number
	Sprite colors are taken from OBJ1PAL if this bit is set to 1 and from OBJ0PAL otherwise.
	*/

	word ATTRIBUTE_ADDRESS = 0xFE00; //$FE00-FE9F
	void Window::draw_sprites() {
		int colours[4] = {0xFFFFFF, 0x777777, 0xBBBBBB, 0x111111};
		for(int i = 0; i < 40; i++) {
			byte
				screen_y = mmu->read(ATTRIBUTE_ADDRESS + (i * 4)),
				screen_x = mmu->read(ATTRIBUTE_ADDRESS + (i * 4) + 1),
				pattern_number = mmu->read(ATTRIBUTE_ADDRESS + (i * 4) + 2),
				palette = mmu->read(ATTRIBUTE_ADDRESS + (i * 4) + 3);

			if(screen_y == 0 || screen_y >= 160 || screen_x == 0 || screen_x >= 168)
				continue;

			bool priority = palette & (1 << 7),
				y_flip = palette & (1 << 6),
				x_flip = palette & (1 << 5),
				object_palette = palette & (1 << 4); // OBJ1PAL if true, else OBJ0PAL

			word pattern_address = 0x8000 + (pattern_number * 16);
			for(int y = 0; y < 8; y++) {
				for(int x = 0; x < 8; x++) {
					byte line_0 = mmu->read(pattern_address + y * 2);
					byte line_1 = mmu->read(pattern_address + (y * 2) + 1);
					auto mod = x_flip ? x % 8 : 7 - (x % 8);
					int colour =
						line_0 & (1 << (mod)) & line_1 & (1 << (mod)) ? colour = colours[3]
						: line_0 & (1 << (mod)) ? colour = colours[2]
						: line_1 & (1 << (mod)) ? colour = colours[1]
						: colour = colours[0];
					if(colour != colours[0])
						draw_pixel((screen_x + x) - 8, (screen_y + y) - 16, colour);
				}
			}
		}
	}

	/*
	FF40 - LCDC - LCD Control (R/W)
		Bit 7: LCD Display Enable (0=Off, 1=On)
		Bit 6: Window Tile Map Display Select (0=9800-9BFF, 1=9C00-9FFF) Maps stored in these
		Bit 5: Window Display Enable (0=Off, 1=On)
		Bit 4: BG & Window Tile Data Table (0=8800-97FF, 1=8000-8FFF) Tiles stored in these, 0 is signed, 1 is unsigned
		Bit 3: BG Tile Map Display Select (0=9800-9BFF, 1=9C00-9FFF) Maps stored in these
		Bit 2: OBJ (Sprite) Size (0=8x8, 1=8x16)
		Bit 1: OBJ (Sprite) Display Enable (0=Off, 1=On)
		Bit 0: BG Display (for CGB see below) (0=Off, 1=On)
	*/
	void Window::render_background_scanline() {
		int colours[4] = {0xFFFFFF, 0x777777, 0xBBBBBB, 0x111111};
		int line = (registers->get_register(Registers::SCY) * 8) + line_offset;
		if(line_offset >= SCREEN_HEIGHT) {
			line_offset = 0;
		}
		for(int x = (registers->get_register(Registers::SCX) * 8); (x - (registers->get_register(Registers::SCX) * 8)) < 160; x++) {
			bool sign = get_pattern_table() == 0x8800;
			byte pattern_number = mmu->read(get_background_tile_map() + (((((line % (SCREEN_HEIGHT)) / 8)) * 32) + (x / 8))) + (sign ? 8 : 0); //((x / 8) - (registers->get_register(Registers::SCX) / 8))
			word base = (sign && pattern_number < 136 ? 0x9000 : 0x8000);
			word pattern_address = base + (sign ? (pattern_number * 16) - 128 : pattern_number * 16);
			byte local_y = ((line % 8) * 2);
			byte line_0 = mmu->read(pattern_address + local_y);
			byte line_1 = mmu->read(pattern_address + local_y + 1);
			auto mod = 7 - (x % 8);
			int colour =
				line_0 & (1 << (mod)) & line_1 & (1 << (mod)) ? colour = colours[3]
				: line_0 & (1 << (mod)) ? colour = colours[2]
				: line_1 & (1 << (mod)) ? colour = colours[1]
				: colour = colours[0];
			draw_pixel(x, line_offset, colour);
		}
	}
	void Window::render_window_scanline() {
		int colours[4] = {0xFFFFFF, 0x777777, 0xBBBBBB, 0x111111};
		int line = (registers->get_register(Registers::WY) * 8) + line_offset;
		if(line_offset >= SCREEN_HEIGHT) {
			line_offset = 0;
		}
		for(int x = (registers->get_register(Registers::WX) * 8); (x - (registers->get_register(Registers::WX) * 8)) < 160; x++) {
			bool sign = get_pattern_table() == 0x8800;
			byte pattern_number = mmu->read(get_window_tile_map() + (((((line % (SCREEN_HEIGHT)) / 8)) * 32) + (x / 8))) + (sign ? 8 : 0); //((x / 8) - (registers->get_register(Registers::SCX) / 8))
			word base = (sign && pattern_number < 136 ? 0x9000 : 0x8000);
			word pattern_address = base + (sign ? (pattern_number * 16) - 128 : pattern_number * 16);
			byte local_y = ((line % 8) * 2);
			byte line_0 = mmu->read(pattern_address + local_y);
			byte line_1 = mmu->read(pattern_address + local_y + 1);
			auto mod = 7 - (x % 8);
			int colour =
				line_0 & (1 << (mod)) & line_1 & (1 << (mod)) ? colour = colours[3]
				: line_0 & (1 << (mod)) ? colour = colours[2]
				: line_1 & (1 << (mod)) ? colour = colours[1]
				: colour = colours[0];
			draw_pixel((x - 7) + (registers->get_register(Registers::WX) * 8), line_offset, colour);
		}
	}

	Window::~Window() {
		/*SDL_DestroyTexture(tex);
		SDL_DestroyRenderer(ren);
		SDL_DestroyWindow(win);*/
		SDL_Quit();
	}
}