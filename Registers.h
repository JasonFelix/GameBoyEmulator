#pragma once
#include "stdafx.h"

using byte = unsigned char;//8 bit
using word = unsigned short;// 16 bit

namespace memory {
	class MMU;
	class Registers {

	public:

		static const word JOYPAD = 0xFF00; //Joypad (R/W)
		static const word SB = 0xFF01; //Divider Register (R/W)
		static const word SC = 0xFF02; //Timer counter (R/W)
		static const word DIV = 0xFF04; //Timer Modulo (R/W)
		static const word TIMA = 0xFF05; //Timer Counter (R/W)
		static const word TMA = 0xFF06; //Timer Modulo (R/W)
		static const word TAC = 0xFF07; //Timer Control (R/W)
		static const word IF = 0xFF0F; //Interrupt Flag (R/W)
							  
		/*Loads of sound registers..*/

		static const word LCDC = 0xFF40; //LCD Control (R/W)
		static const word STAT = 0xFF41; //LCDC Status (R/W)
		static const word SCY = 0xFF42; //Scroll Y (R/W)
		static const word SCX = 0xFF43; //Scroll X (R/W)
		static const word LY = 0xFF44; //LCDC Y-Coordinate (R)
		static const word LYC = 0xFF45; //LY Compare (R/W)
		static const word DMA = 0xFF46; //DMA Transfer and Start Address (W)
		static const word BGP = 0xFF47; //BG & Window Palette Data (R/W)
		static const word OBP0 = 0xFF48; //Object Palette 0 Data (R/W)
		static const word OBP1 = 0xFF49; //Object Palette 1 Data (R/W)
							  
		static const word HDMA1 = 0xFF51; //CGB ONLY: New DMA Source, High
		static const word HDMA2 = 0xFF52; //CGB ONLY: New DMA Source, Low
		static const word HDMA3 = 0xFF53; //CGB ONLY: New DMA Destination, High
		static const word HDMA4 = 0xFF54; //CGB ONLY: New DMA Destination, Low
		static const word HDMA5 = 0xFF55; //CGB ONLY: New DMA Length / Mode / Start	
							  
		static const word WY = 0xFF4A; //Window Y Position (R/W)
		static const word WX = 0xFF4B; //Window X Position (R/W)
							  
		static const word IE = 0xFFFF; //Interrupt Enable (R/W)

		Registers(MMU &_mmu);
		Registers();
		~Registers();

		void handle_timers();
		byte get_register(word _register);
		bool get_register(word _register, byte bit);
		void set_register(word _register, byte bit, bool set);

		enum FLAG_MASK {
			/*Z-N-H-C*/
			ZERO = 0x80,		//Zero
			N_SUBTRACT = 0x40,	//Subtract
			HALF_CARRY = 0x20,	//Half Carry
			CARRY = 0x10		//Carry
		};

		union _af {
			word w = 0x1B0;
			struct {
				byte flags;				
				byte a; //Accumulator
			};
		} af;

		union _bc {
			word w = 0x13;
			struct {
				byte c;				
				byte b;
			};
		} bc;

		union _de {
			word w = 0xD8;
			struct {
				byte e;				
				byte d;
			};
		} de;

		union _hl {
			word w = 0x14D;
			struct {
				byte l;				
				byte h;
			};
		} hl;

		bool get_flag(FLAG_MASK m) {
			return (af.flags & m) == m;
		}

		inline void set(FLAG_MASK m, bool b) {
			b ? on(m) : off(m);
		}

		void off(FLAG_MASK m) {
			af.flags = af.flags & (0xF0 ^ m);
		}

		void off(char c) {
			af.flags &= (0xF0 ^ c);
		}

		void on(FLAG_MASK m) {
			af.flags |= m;
		}

		void on(char c) {
			af.flags = af.flags | c;
		}

		void toggle(FLAG_MASK m) {
			af.flags ^= m;
		}



		word pc = 0x100; //Program counter
		word sp = 0xFFFE; //Stack pointer
		byte interrupt_master_enable; //IME 
		byte refresh_counter;
	};

}