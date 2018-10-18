#pragma once
#include "stdafx.h"
#include <vector>
#include "MMU.h"
#include "Registers.h"
#include "Joypad.h"

namespace graphics {
	class Window {
	public:
		Window(memory::MMU &_mmu, memory::Registers &_registers, input::Joypad &joypad);
		Window();
		~Window();
		void run();
		void Window::render_tiles();
		void fill_rect(int x, int y, int w, int h, int color);
		void draw_pixel(int x, int y, int color);
		void draw_tile(std::vector<unsigned char> tile);
		void render_background_scanline();
		void render_window_scanline();
		void draw_sprites();
		inline unsigned short get_window_tile_map();
		inline unsigned short get_pattern_table();
		inline unsigned short get_background_tile_map();
	};
	
}