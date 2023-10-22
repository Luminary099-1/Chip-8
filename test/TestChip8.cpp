#include "TestChip8.hpp"


uint16_t TestChip8::get_pc() {
	return _pc;
}


uint16_t TestChip8::get_sp() {
	return _sp;
}


uint16_t TestChip8::get_index() {
	return _index;
}


uint8_t TestChip8::get_delay() {
	return _delay;
}


uint8_t TestChip8::get_sound() {
	return _sound;
}


uint8_t* TestChip8::get_mem() {
	return _mem;
}


uint64_t* TestChip8::get_screen() {
	return _screen;
}


bool TestChip8::get_programmed() {
	return _programmed;
}


bool TestChip8::get_key_wait() {
	return _key_wait;
}


bool TestChip8::get_sounding() {
	return _sounding;
}


bool TestChip8::get_crashed() {
	return _crashed;
}


TestChip8::_InstrFunc TestChip8::get_instr_func(uint16_t instruction) {
	return Chip8::get_instr_func(instruction);
}