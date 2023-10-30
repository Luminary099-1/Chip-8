#include "Chip8.hpp"

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <stdexcept>

// Macros for accessing specific parts of instructions.
#define INSTR_A ((instruction & 0xf000) >> 12)
#define INSTR_B ((instruction & 0x0f00) >> 8)
#define INSTR_C ((instruction & 0x00f0) >> 4)
#define INSTR_D (instruction & 0x000f)
#define INSTR_ADDR (instruction & 0x0fff)
#define INSTR_IMM (instruction & 0x00ff)


Chip8SaveState::Chip8SaveState() {}


Chip8SaveState::Chip8SaveState(Chip8SaveState& state) {
	memcpy(_gprf, state._gprf, sizeof(_gprf));
	_pc = state._pc;
	_sp = state._sp;
	_index = state._index;
	_delay = state._delay;
	_sound = state._sound;
	memcpy(_mem, state._mem, sizeof(_mem));
	memcpy(_screen, state._screen, sizeof(_screen));
	_sounding = state._sounding;
	_crashed = state._crashed;
	_key_wait = state._key_wait.load();
	_time_budget = state._time_budget;
	_timer = state._timer;
}


// Instructions with leading half bytes of 1, 2, A, and B.
const std::map<uint8_t, Chip8::_InstrFunc> Chip8::_INSTRUCTIONS1 = { // kNNN
	{0x1, in_jump},
	{0x2, in_call},
	{0xa, in_loadi},
	{0xb, in_jumpi},
};


// Instructions with leading half bytes of 3, 4, 6, 7, and C.
const std::map<uint8_t, Chip8::_InstrFunc> Chip8::_INSTRUCTIONS2 = { // kXNN
	{0x3, in_ske},
	{0x4, in_skne},
	{0x6, in_load},
	{0x7, in_add},
	{0xc, in_rand},
};


// Instructions with leading half bytes of 5, 8, and 9.
const std::map<uint8_t, Chip8::_InstrFunc> Chip8::_INSTRUCTIONS3 = { // kXYk
	{0x50, in_skre},
	{0x80, in_move},
	{0x81, in_or},
	{0x82, in_and},
	{0x83, in_xor},
	{0x84, in_addr},
	{0x85, in_sub},
	{0x86, in_shr},
	{0x87, in_suba},
	{0x8e, in_shl},
	{0x90, in_skrne},
};


// Instructions with leading half bytes of E and F.
const std::map<uint16_t, Chip8::_InstrFunc> Chip8::_INSTRUCTIONS4 = { // kXkk
	{0x0ea1, in_skup},
	{0x0e9e, in_skpr},
	{0x0f33, in_bcd},
	{0x0f15, in_loadd},
	{0x0f55, in_stor},
	{0x0f65, in_read},
	{0x0f07, in_moved},
	{0x0f18, in_loads},
	{0x0f29, in_ldspr},
	{0x0f0a, in_keyd},
	{0x0f1e, in_addi},
};


Chip8::Chip8(Chip8Keyboard* key, Chip8Display* disp,
	Chip8Sound* snd, Chip8Message* msg)
	: _keyboard(key), _display(disp), _speaker(snd), _error(msg) {
	_freq = 500;
	_programmed = false;
	_display->_vm = this;
}


void Chip8::load_program(std::string& program) {
	_access_lock.lock();
	// Verify the program isn't odd or too large.
	if (program.size() > _Max_Prog_Size)
		throw std::invalid_argument("Program is too large.");

	// Zero initialize memory and registers. Load the font.
	memset(_mem, 0, sizeof(_mem));
	memset(_screen, 0, sizeof(_screen));
	memcpy(_mem + FONT_OFF, FONT, sizeof(FONT));
	memset(_gprf, 0, sizeof(_gprf));
	_sp = 0;
	_pc = _Prog_Start;
	_index = 0;
	_delay = 0;
	_sound = 0;
	
	// Copy the program into memory.
	memcpy(_mem + _Prog_Start, (void*) program.data(), program.length());

	// Set VM data to defaults.
	_crashed = false;
	_programmed = true;
	_key_wait = false;
	_sounding = false;

	_access_lock.unlock();
}


Chip8SaveState Chip8::get_state() {
	_access_lock.lock();
	Chip8SaveState state {static_cast<Chip8SaveState>(*this)};
	_access_lock.unlock();
	return state;
}


void Chip8::set_state(Chip8SaveState& source) {
	_access_lock.lock();
	memcpy(static_cast<Chip8SaveState*>(this), &source, sizeof(Chip8SaveState));
	_access_lock.unlock();
}


Chip8::_InstrFunc Chip8::get_instr_func(uint16_t instruction) {
	// Grab the halfwords of the instruction.
	uint8_t a = INSTR_A;
	uint8_t b = INSTR_B;
	uint8_t c = INSTR_C;
	uint8_t d = INSTR_D;
	_InstrFunc func;
	
	try { // Use the instruction to determine which map to search.
		switch (a) {
			case 0x0: // Leading half byte 0.
				if (instruction == 0x00E0) func = in_clr;
				else if (instruction == 0x00EE) func = in_rts;
				else throw std::out_of_range("");
				break;

			case 0xD: // DXYN
				func = in_draw;
				break;

			// Leading half bytes 1, 2, A, and B.
			case 0X1:	case 0x2:	case 0xA:	case 0xB:
				func = _INSTRUCTIONS1.at(a);
				break;

			// Leading half bytes 3, 4, 6, 7, and C.
			case 0x3:	case 0x4:	case 0x6:	case 0x7:	case 0xC:
				func = _INSTRUCTIONS2.at(a);
				break;

			case 0xE:	case 0xF: // Leading half bytes E and F.
				func = _INSTRUCTIONS4.at(((uint16_t) a << 8) + (c << 4) + d);
				break;
				
			default: // Leading half bytes 5, 8, and 9.
				func = _INSTRUCTIONS3.at((a << 4) + d);
				break;
		}

	// Catch and rethrow invalid instruction accesses.
	} catch (std::out_of_range& e) {
		std::stringstream msg;
		msg << "Invalid instruction: "
			<< std::uppercase << std::hex << instruction;
		throw Chip8Error(msg.str());
	}
	// Ensure functions with addresses are valid.
	if (func == in_sys || func == in_jump || func == in_call) {
		uint16_t addr = INSTR_ADDR;
		if (addr < _Prog_Start || addr > _Prog_End)
			std::out_of_range("Illegal VM memory operation.");
	}
	return func;
}


void Chip8::run(_TimeType elapsed_time) {
	_access_lock.lock();
	_TimeType cycle_period {1000U / _freq};
	_time_budget += elapsed_time;
	_TimeType cycles {_time_budget / cycle_period.count()};
	for (int64_t i {0}; i < cycles.count(); ++i)
		execute_cycle(cycle_period);
	_time_budget -= cycles * cycle_period.count();
	_access_lock.unlock();
}


void Chip8::execute_cycle(_TimeType cycle_time) {
	static constexpr _TimeType timer_period {1000U / 60U};
	// Keep track of elapsed time to update the timers.
	_timer += cycle_time;
	// If the 60Hz timer has cycled, update the timers and reset it.
	if (_timer >= timer_period) {
		_timer -= timer_period;
		if (_delay != 0) _delay -= 1;
		if (_sound != 0) _sound -= 1;
	}

	if (_key_wait) return;

	// Grab and execute the next instruction.
	uint16_t instruction = get_hword(_pc);
	_InstrFunc instr_func = get_instr_func(instruction);
	instr_func(*this, instruction);
	// Set the sound output to reflect the value of the timer.
	if (_sounding && _sound == 0) {
		_speaker->stop_sound();
		_sounding = false;
	} else if (_sound >= 2) {
		_speaker->start_sound();
		_sounding = true;
	}
	// Increment the program counter if the instruction was not a jump.
	if (instr_func != in_rts && instr_func != in_jump
		&& instr_func != in_call && instr_func != in_jumpi) _pc += 2;
}


uint16_t Chip8::frequency() {
	return _freq;
}


void Chip8::frequency(uint16_t value) {
	_access_lock.lock();
	_freq = value;
	_access_lock.unlock();
}


bool Chip8::is_crashed() {
	return _crashed;
}


bool Chip8::is_sounding() {
	return _sounding;
}

bool Chip8::is_programmed() {
	return _programmed;
}


uint16_t Chip8::get_hword(uint16_t addr) {
	return (_mem[addr] << 8) + _mem[addr + 1];
}


void Chip8::set_hword(uint16_t addr, uint16_t hword) {
	_mem[addr] = hword >> 8;
	_mem[addr + 1] = hword & 0xff;
}


void Chip8::key_pressed(uint8_t key) {
	if (key > 0xf) throw new std::domain_error("Key value too large.");
	if (!_key_wait) return;
	_access_lock.lock();
	uint16_t instruction = get_hword(_pc - 2);
	uint8_t x = INSTR_B;
	_gprf[x] = key;
	_access_lock.unlock();
}


uint64_t* Chip8::get_screen_buf() {
	return _screen;
}


// Instruction Implementing Methods ============================================
void Chip8::in_sys(Chip8& vm, uint16_t instruction) { // 0NNN
	// Originally called machine code instruction, does nothing here.
}


void Chip8::in_clr(Chip8& vm, uint16_t instruction) { // 00E0
	memset(vm._screen, 0, sizeof(vm._screen));
	vm._display->draw();
}


void Chip8::in_rts(Chip8& vm, uint16_t instruction) { // 00EE
	if (vm._sp == 0)
		throw Chip8Error("VM stack underflow.");
	vm._pc = vm.get_hword(vm._sp);
	vm._sp -= 2;
}


void Chip8::in_jump(Chip8& vm, uint16_t instruction) { // 1NNN
	vm._index = INSTR_ADDR;
}


void Chip8::in_call(Chip8& vm, uint16_t instruction) { // 2NNN
	if (vm._sp == FONT_OFF - 1)
		throw Chip8Error("VM stack overflow.");
	vm._sp += 2;
	vm.set_hword(vm._sp, vm._pc);
	vm._pc = INSTR_ADDR;
}


void Chip8::in_ske(Chip8& vm, uint16_t instruction) { // 3XNN
	if (vm._gprf[INSTR_B] == INSTR_IMM) vm._pc += 2;
}


void Chip8::in_skne(Chip8& vm, uint16_t instruction) { // 4XNN
	if (vm._gprf[INSTR_B] != INSTR_IMM) vm._pc += 2;
}


void Chip8::in_skre(Chip8& vm, uint16_t instruction) { // 5XY0
	if (vm._gprf[INSTR_B] == vm._gprf[INSTR_C]) vm._pc += 2;
}


void Chip8::in_load(Chip8& vm, uint16_t instruction) { // 6XNN
	vm._gprf[INSTR_B] = INSTR_IMM;
}


void Chip8::in_add(Chip8& vm, uint16_t instruction) { // 7XNN
	vm._gprf[INSTR_B] += INSTR_IMM;
}


void Chip8::in_move(Chip8& vm, uint16_t instruction) { // 8XY0
	vm._gprf[INSTR_B] = vm._gprf[INSTR_C];
}


void Chip8::in_or(Chip8& vm, uint16_t instruction) { // 8XY1
	vm._gprf[INSTR_B] = vm._gprf[INSTR_B] | vm._gprf[INSTR_C];
}


void Chip8::in_and(Chip8& vm, uint16_t instruction) { // 8XY2
	vm._gprf[INSTR_B] = vm._gprf[INSTR_B] & vm._gprf[INSTR_C];
}


void Chip8::in_xor(Chip8& vm, uint16_t instruction) { // 8XY3
	vm._gprf[INSTR_B] = vm._gprf[INSTR_B] ^ vm._gprf[INSTR_C];
}


void Chip8::in_addr(Chip8& vm, uint16_t instruction) { // 8XY4
	if ((int16_t) vm._gprf[INSTR_B] + (int16_t) vm._gprf[INSTR_C] > UINT8_MAX)
		vm._gprf[0xf] = 0x01;
	else vm._gprf[0xf] = 0x00;
	vm._gprf[INSTR_B] += vm._gprf[INSTR_C];
}


void Chip8::in_sub(Chip8& vm, uint16_t instruction) { // 8XY5
	if ((int8_t) vm._gprf[INSTR_B] - (int8_t) vm._gprf[INSTR_C] < 0)
		vm._gprf[0xf] = 0x01;
	else vm._gprf[0xf] = 0x00;
	vm._gprf[INSTR_B] -= vm._gprf[INSTR_C];
}


void Chip8::in_shr(Chip8& vm, uint16_t instruction) { // 8XY6
	vm._gprf[0xf] = vm._gprf[INSTR_C] & 0x01;
	vm._gprf[INSTR_B] = vm._gprf[INSTR_C] >> 1;
}


void Chip8::in_suba(Chip8& vm, uint16_t instruction) { // 8XY7
	if ((int8_t) vm._gprf[INSTR_C] - (int8_t) vm._gprf[INSTR_B] < 0)
		vm._gprf[0xf] = 0x01;
	else vm._gprf[0xf] = 0x00;
	vm._gprf[INSTR_B] = vm._gprf[INSTR_C] - vm._gprf[INSTR_B];
}


void Chip8::in_shl(Chip8& vm, uint16_t instruction) { // 8XYE
	vm._gprf[0xf] = (vm._gprf[INSTR_C] & 0x80) >> 7;
	vm._gprf[INSTR_B] = vm._gprf[INSTR_C] << 1;
}


void Chip8::in_skrne(Chip8& vm, uint16_t instruction) { // 9XY0
	if (vm._gprf[INSTR_B] != vm._gprf[INSTR_C]) vm._pc += 2;
}


void Chip8::in_loadi(Chip8& vm, uint16_t instruction) { // ANNN
	vm._index = INSTR_ADDR;
}


void Chip8::in_jumpi(Chip8& vm, uint16_t instruction) { // BNNN
	uint16_t addr = vm._gprf[0] + INSTR_ADDR;
	if (addr < _Prog_Start || _Prog_End > _Prog_End)
		Chip8Error("Illegal VM memory operation.");
	vm._pc = addr;
}


void Chip8::in_rand(Chip8& vm, uint16_t instruction) { // CXNN
	vm._gprf[INSTR_B] = (rand() % 256) & INSTR_IMM;
}


// TODO: Consider trying to simplify this function.
void Chip8::in_draw(Chip8& vm, uint16_t instruction) { // DXYN
	vm._gprf[0x0f] = 0x00; // Initialize the overwite flag to 0.
	// Grab the sprite coordinates.
	uint8_t xpos {vm._gprf[INSTR_B]};
 	uint8_t ypos {vm._gprf[INSTR_C]};
	// Iterate over each line of the sprite.
	for (uint8_t y {0}; y < std::min(INSTR_D, 32); ++y) {
		// Grab the line from the sprite and shift it to its x position.
		if (vm._index + y < _Prog_Start || vm._index + y > _Prog_End)
			Chip8Error("Illegal VM memory operation.");
		uint64_t spr_line
			= static_cast<uint64_t>(vm._mem[vm._index + y]) << (56 - xpos);
		// Determine the new screen line.
		uint64_t new_line = vm._screen[ypos + y] ^ spr_line;
		// Set the flag if an overrite happened.
		if (vm._screen[ypos + y] & ~new_line != 0x0) vm._gprf[0x0f] = 0x01;
		// Update the screen memory with the new line.
		vm._screen[ypos + y] = new_line;
	}
	vm._display->draw();
}


void Chip8::in_skpr(Chip8& vm, uint16_t instruction) { // EX9E
	if (vm._keyboard->test_key(vm._gprf[INSTR_B])) vm._pc += 2;
}


void Chip8::in_skup(Chip8& vm, uint16_t instruction) { // EXA1
	if (!vm._keyboard->test_key(vm._gprf[INSTR_B])) vm._pc += 2;
}


void Chip8::in_moved(Chip8& vm, uint16_t instruction) { // FX07
	vm._gprf[INSTR_B] = vm._delay;
}


void Chip8::in_keyd(Chip8& vm, uint16_t instruction) { // FX0A
	vm._key_wait = true;
}


void Chip8::in_loadd(Chip8& vm, uint16_t instruction) { // FX15
	vm._delay = vm._gprf[INSTR_B];
}


void Chip8::in_loads(Chip8& vm, uint16_t instruction) { // FX18
	vm._sound = vm._gprf[INSTR_B];
}


void Chip8::in_addi(Chip8& vm, uint16_t instruction) { // FX1E
	vm._index += vm._gprf[INSTR_B];
}


void Chip8::in_ldspr(Chip8& vm, uint16_t instruction) { // FX29
	vm._index = FONT_OFF + vm._gprf[INSTR_B];
}


// Makes use of the Double Dabble algorithm.
void Chip8::in_bcd(Chip8& vm, uint16_t instruction) { // FX33
	// Grab the initial value from the register.
	uint32_t scratch = vm._gprf[INSTR_B];
	for (size_t i = 0; i < 8; i ++) {
		scratch = scratch << 1; // Shift in each bit of the value.
		// Add 3 to each digit if greater than 4.
		if (scratch & 0xf00 > 0x400) scratch += 0x300;
		if (scratch & 0xf000 > 0x4000) scratch += 0x3000;
		if (scratch & 0xf0000 > 0x40000) scratch += 0x30000;
	}
	// Ensure the destination memory is valid.
	if (vm._index < _Prog_Start || vm._index + 2 > _Prog_End)
		Chip8Error("Illegal VM memory operation.");
	// Store each digit in memory.
	vm._mem[vm._index] = (scratch & 0xf0000) >> 0xf;
	vm._mem[vm._index + 1] = (scratch & 0xf000) >> 0xb;
	vm._mem[vm._index + 2] = (scratch & 0xf00) >> 0x8;
}


void Chip8::in_stor(Chip8& vm, uint16_t instruction) { // FX55
	for (uint32_t i = 0; i < vm._gprf[INSTR_B]; i ++) {
		if (vm._index + i < _Prog_Start || vm._index > _Prog_End)
			throw Chip8Error("Illegal VM memory operation.");
		vm._mem[vm._index + i] = vm._gprf[i];
	}
}


void Chip8::in_read(Chip8& vm, uint16_t instruction) { // FX65
	for (uint32_t i = 0; i < vm._gprf[INSTR_B]; i ++) {
		if (vm._index + i < _Prog_Start || vm._index > _Prog_End)
			throw Chip8Error("Illegal VM memory operation.");
		vm._gprf[i] = vm._mem[vm._index + i];
	}
}
