#include "TestChip8.hpp"
#include "TestChip8Observer.hpp"
#include "../src/Chip8.hpp"

#include <iostream>
#include <sstream>
#include <catch2/catch_test_macros.hpp>


TEST_CASE("Chip-8 VM Utilities") {
	TestChip8Observer listener;
	TestChip8 vm (&listener, &listener, &listener, &listener);

	SECTION("load_program") {
		SECTION("load_program: Full size.") {
			std::stringstream prog_acc;
			for (uint16_t i = 0; i < Chip8::_Max_Prog_Size; i ++)
				prog_acc << static_cast<char>(i % 256);
			std::string program {prog_acc.str()};
			vm.load_program(program);
			std::string loaded ((char*) vm.get_mem(), Chip8::_Mem_Size);
			loaded = loaded.substr(Chip8::_Prog_Start, Chip8::_Max_Prog_Size);
			REQUIRE(loaded.compare(program) == 0);
		}

		SECTION("load_program: Too large.") {
			std::string program (Chip8::_Max_Prog_Size + 1, ' ');
			REQUIRE_THROWS_AS(vm.load_program(program), std::invalid_argument);
		}
	}

	SECTION("get_instr_func") {
		SECTION("0___") {
			REQUIRE(vm.get_instr_func(0x00E0) == vm.in_clr);
			REQUIRE(vm.get_instr_func(0x00EE) == vm.in_rts);
			REQUIRE_THROWS_AS(vm.get_instr_func(0x0000), Chip8Error);
		}
		
		SECTION("1NNN (jump)") {
			REQUIRE(vm.get_instr_func(0x1234) == vm.in_jump);
			REQUIRE(vm.get_instr_func(0x1FFF) == vm.in_jump);
		}

		SECTION("2NNN (call)") {
			REQUIRE(vm.get_instr_func(0x2234) == vm.in_call);
			REQUIRE(vm.get_instr_func(0x2FFF) == vm.in_call);
		}

		SECTION("3XNN (ske)") {
			REQUIRE(vm.get_instr_func(0x3123) == vm.in_ske);
			REQUIRE(vm.get_instr_func(0x3FDE) == vm.in_ske);
		}

		SECTION("4XNN (skne)") {
			REQUIRE(vm.get_instr_func(0x4123) == vm.in_skne);
			REQUIRE(vm.get_instr_func(0x4FDE) == vm.in_skne);
		}

		SECTION("5XY0 (skre)") {
			REQUIRE(vm.get_instr_func(0x57F0) == vm.in_skre);
			REQUIRE(vm.get_instr_func(0x53D0) == vm.in_skre);
			REQUIRE_THROWS_AS(vm.get_instr_func(0x5453), Chip8Error);
		}

		SECTION("6XNN (load)") {
			REQUIRE(vm.get_instr_func(0x6234) == vm.in_load);
			REQUIRE(vm.get_instr_func(0x6FFF) == vm.in_load);
		}

		SECTION("7XNN (add)") {
			REQUIRE(vm.get_instr_func(0x7123) == vm.in_add);
			REQUIRE(vm.get_instr_func(0x7FDE) == vm.in_add);
		}

		SECTION("8XY_") {
			REQUIRE(vm.get_instr_func(0x87F0) == vm.in_move);
			REQUIRE(vm.get_instr_func(0x87F1) == vm.in_or);
			REQUIRE(vm.get_instr_func(0x87F2) == vm.in_and);
			REQUIRE(vm.get_instr_func(0x87F3) == vm.in_xor);
			REQUIRE(vm.get_instr_func(0x87F4) == vm.in_addr);
			REQUIRE(vm.get_instr_func(0x87F5) == vm.in_sub);
			REQUIRE(vm.get_instr_func(0x87F6) == vm.in_shr);
			REQUIRE(vm.get_instr_func(0x87F7) == vm.in_suba);
			REQUIRE(vm.get_instr_func(0x87FE) == vm.in_shl);
			REQUIRE_THROWS_AS(vm.get_instr_func(0x87FA), Chip8Error);
		}

		SECTION("9XY0 (skrne)") {
			REQUIRE(vm.get_instr_func(0x97F0) == vm.in_skrne);
			REQUIRE_THROWS_AS(vm.get_instr_func(0x97FA), Chip8Error);
		}

		SECTION("ANNN (loadi)") {
			REQUIRE(vm.get_instr_func(0xA234) == vm.in_loadi);
			REQUIRE(vm.get_instr_func(0xAFFF) == vm.in_loadi);
		}

		SECTION("ANNN (jumpi)") {
			REQUIRE(vm.get_instr_func(0xB234) == vm.in_jumpi);
			REQUIRE(vm.get_instr_func(0xBFFF) == vm.in_jumpi);
		}

		SECTION("CXNN (rand)") {
			REQUIRE(vm.get_instr_func(0xC123) == vm.in_rand);
			REQUIRE(vm.get_instr_func(0xCFDE) == vm.in_rand);
		}

		SECTION("DXYN (draw)") {
			REQUIRE(vm.get_instr_func(0xDA58) == vm.in_draw);
			REQUIRE(vm.get_instr_func(0xD7C0) == vm.in_draw);
		}

		SECTION("EX__") {
			REQUIRE(vm.get_instr_func(0xE9A1) == vm.in_skup);
			REQUIRE(vm.get_instr_func(0xEE9E) == vm.in_skpr);
			REQUIRE_THROWS_AS(vm.get_instr_func(0xEFCB), Chip8Error);
		}

		SECTION("FX__") {
			REQUIRE(vm.get_instr_func(0xF233) == vm.in_bcd);
			REQUIRE(vm.get_instr_func(0xF215) == vm.in_loadd);
			REQUIRE(vm.get_instr_func(0xF255) == vm.in_stor);
			REQUIRE(vm.get_instr_func(0xF265) == vm.in_read);
			REQUIRE(vm.get_instr_func(0xF207) == vm.in_moved);
			REQUIRE(vm.get_instr_func(0xF218) == vm.in_loads);
			REQUIRE(vm.get_instr_func(0xF229) == vm.in_ldspr);
			REQUIRE(vm.get_instr_func(0xF20A) == vm.in_keyd);
			REQUIRE(vm.get_instr_func(0xF21E) == vm.in_addi);
			REQUIRE_THROWS_AS(vm.get_instr_func(0xFFFF), Chip8Error);
		}
	}
}


TEST_CASE("Chip-8 VM Instructions") {
	REQUIRE(true);
}