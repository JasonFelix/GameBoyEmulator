#pragma once
#include "MMU.h"
#include "Registers.h"

namespace cpu {
	class Z80 {
	
	public:
		Z80(memory::MMU &_mmu, memory::Registers &_registers);
		Z80();
		~Z80();
	
		void run();
		void handle_interrupts();
		void dma_transfer();
	};
}
