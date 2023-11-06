#include "Chip8.hpp"

#include <algorithm>
#include <cstdlib>
#include <sstream>
#include <stdexcept>


constexpr uint8_t Chip8::instr_a(uint16_t instruction) {
	return (instruction & 0xf000U) >> 12;
}


constexpr uint8_t Chip8::instr_b(uint16_t instruction) {
	return (instruction & 0x0f00U) >> 8;
}


constexpr uint8_t Chip8::instr_c(uint16_t instruction) {
	return (instruction & 0x00f0U) >> 4;
}


constexpr uint8_t Chip8::instr_d(uint16_t instruction) {
	return instruction & 0x000fU;
}


constexpr uint16_t Chip8::instr_addr(uint16_t instruction) {
	return instruction & 0x0fffU;
}


constexpr uint8_t Chip8::instr_imm(uint16_t instruction) {
	return instruction & 0x00ffU;
}


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
	_freq = 1200;
	_programmed = false;
	_display->_vm = this;
	_pressed_key = 0x10; // An invalid key value sentinel.
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
	uint8_t a = instr_a(instruction);
	uint8_t b = instr_b(instruction);
	uint8_t c = instr_c(instruction);
	uint8_t d = instr_d(instruction);
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
		uint16_t addr = instr_addr(instruction);
		if (addr < _Prog_Start || addr > _Prog_End)
			std::out_of_range("Illegal VM memory operation.");
	}
	return func;
}


void Chip8::run(_TimeType elapsed_time) {
	_access_lock.lock();
	_TimeType cycle_period {_billion / _freq};
	_time_budget += elapsed_time;
	_TimeType cycles {_time_budget / cycle_period.count()};

	for (int64_t i {0}; i < cycles.count(); ++i) execute_cycle(cycle_period);
	
	_time_budget -= cycles * cycle_period.count();
	_access_lock.unlock();
}


void Chip8::execute_cycle(_TimeType cycle_time) {
	static constexpr _TimeType timer_period {_billion / 60U};
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
	if (_pc < _Prog_Start || _pc > _Prog_End)
		throw new Chip8Error("PC is outside of the program range.");

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
	// Increment _pc if the instruction was not a jump, call, or wait.
	if (instr_func != in_jump && instr_func != in_jumpi
		&& instr_func != in_call && instr_func != in_keyd) _pc += 2;
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
	uint16_t instruction = get_hword(_pc);
	_gprf[instr_b(instruction)] = key;
	_pc += 2;
	_pressed_key = key;
}


void Chip8::key_released(uint8_t key) {
	if (key > 0xf) throw new std::domain_error("Key value too large.");
	if (key != _pressed_key) return;
	_key_wait = false;
	_pressed_key = 0x10;
}


uint64_t* Chip8::get_screen_buf() {
	return _screen;
}


// Instruction Implementing Methods ============================================
void Chip8::in_sys(Chip8& vm, uint16_t instr) { // 0NNN
	// Originally called machine code instruction, does nothing here.
}


void Chip8::in_clr(Chip8& vm, uint16_t instr) { // 00E0
	memset(vm._screen, 0, sizeof(vm._screen));
	vm._display->draw();
}


void Chip8::in_rts(Chip8& vm, uint16_t instr) { // 00EE
	if (vm._sp == 0) throw Chip8Error("VM call stack underflow.");
	vm._sp -= 2;
	vm._pc = vm.get_hword(vm._sp);
}


void Chip8::in_jump(Chip8& vm, uint16_t instr) { // 1NNN
	vm._pc = instr_addr(instr);
}


void Chip8::in_call(Chip8& vm, uint16_t instr) { // 2NNN
	if (vm._sp == FONT_OFF) throw Chip8Error("VM call stack overflow.");
	vm.set_hword(vm._sp, vm._pc);
	vm._sp += 2;
	vm._pc = instr_addr(instr);
}


void Chip8::in_ske(Chip8& vm, uint16_t instr) { // 3XNN
	if (vm._gprf[instr_b(instr)] == instr_imm(instr)) vm._pc += 2;
}


void Chip8::in_skne(Chip8& vm, uint16_t instr) { // 4XNN
	if (vm._gprf[instr_b(instr)] != instr_imm(instr)) vm._pc += 2;
}


void Chip8::in_skre(Chip8& vm, uint16_t instr) { // 5XY0
	if (vm._gprf[instr_b(instr)] == vm._gprf[instr_c(instr)]) vm._pc += 2;
}


void Chip8::in_load(Chip8& vm, uint16_t instr) { // 6XNN
	vm._gprf[instr_b(instr)] = instr_imm(instr);
}


void Chip8::in_add(Chip8& vm, uint16_t instr) { // 7XNN
	vm._gprf[instr_b(instr)] += instr_imm(instr);
}


void Chip8::in_move(Chip8& vm, uint16_t instr) { // 8XY0
	vm._gprf[instr_b(instr)] = vm._gprf[instr_c(instr)];
}


void Chip8::in_or(Chip8& vm, uint16_t instr) { // 8XY1
	vm._gprf[instr_b(instr)]
		= vm._gprf[instr_b(instr)] | vm._gprf[instr_c(instr)];
}


void Chip8::in_and(Chip8& vm, uint16_t instr) { // 8XY2
	vm._gprf[instr_b(instr)]
		= vm._gprf[instr_b(instr)] & vm._gprf[instr_c(instr)];
}


void Chip8::in_xor(Chip8& vm, uint16_t instr) { // 8XY3
	vm._gprf[instr_b(instr)]
		= vm._gprf[instr_b(instr)] ^ vm._gprf[instr_c(instr)];
}


void Chip8::in_addr(Chip8& vm, uint16_t instr) { // 8XY4
	uint8_t b {vm._gprf[instr_b(instr)]};
	uint8_t c {vm._gprf[instr_c(instr)]};
	uint8_t sum {static_cast<uint8_t>(b + c)};
	vm._gprf[instr_b(instr)] = sum;

	if (sum < b) vm._gprf[0xf] = 0x01;
	else vm._gprf[0xf] = 0x00;
}


void Chip8::in_sub(Chip8& vm, uint16_t instr) { // 8XY5
	uint8_t b {vm._gprf[instr_b(instr)]};
	uint8_t c {vm._gprf[instr_c(instr)]};
	uint8_t difference {static_cast<uint8_t>(b - c)};
	vm._gprf[instr_b(instr)] = difference;

	if (difference > b) vm._gprf[0xf] = 0x00;
	else vm._gprf[0xf] = 0x01;
}


void Chip8::in_shr(Chip8& vm, uint16_t instr) { // 8XY6
	uint8_t opY {vm._gprf[instr_c(instr)]};
	vm._gprf[instr_b(instr)] = opY >> 1;
	vm._gprf[0xf] = opY & 0x01;
}


void Chip8::in_suba(Chip8& vm, uint16_t instr) { // 8XY7
	uint8_t b {vm._gprf[instr_b(instr)]};
	uint8_t c {vm._gprf[instr_c(instr)]};
	uint8_t difference {static_cast<uint8_t>(c - b)};
	vm._gprf[instr_b(instr)] = difference;

	if (difference > c) vm._gprf[0xf] = 0x00;
	else vm._gprf[0xf] = 0x01;
}


void Chip8::in_shl(Chip8& vm, uint16_t instr) { // 8XYE
	uint8_t opY {vm._gprf[instr_c(instr)]};
	vm._gprf[instr_b(instr)] = opY << 1;
	vm._gprf[0xf] = (opY & 0x80) >> 7;
}


void Chip8::in_skrne(Chip8& vm, uint16_t instr) { // 9XY0
	if (vm._gprf[instr_b(instr)] != vm._gprf[instr_c(instr)]) vm._pc += 2;
}


void Chip8::in_loadi(Chip8& vm, uint16_t instr) { // ANNN
	vm._index = instr_addr(instr);
}


void Chip8::in_jumpi(Chip8& vm, uint16_t instr) { // BNNN
	uint16_t addr { static_cast<uint16_t>(vm._gprf[0] + instr_addr(instr)) };
	if (addr < _Prog_Start || _Prog_End > _Prog_End)
		Chip8Error("Jump to address outside program range.");
	vm._pc = addr;
}


void Chip8::in_rand(Chip8& vm, uint16_t instr) { // CXNN
	vm._gprf[instr_b(instr)] = (rand() % 256) & instr_imm(instr);
}


// TODO: Consider trying to simplify this function.
void Chip8::in_draw(Chip8& vm, uint16_t instr) { // DXYN
	vm._gprf[0x0f] = 0x00; // Initialize the overwite flag to 0.
	// Grab the sprite coordinates, number of lines to draw, and the x shift..
	uint8_t xpos { vm._gprf[instr_b(instr)] };
 	uint8_t ypos { vm._gprf[instr_c(instr)] };
	uint8_t y_max  { std::min(instr_d(instr), static_cast<uint8_t>(32)) };
	int x_shift {56 - xpos};

	// Iterate over each line of the sprite.
	for (uint8_t y {0}; y < y_max; ++y) {
		// Grab the row from the sprite.
		uint64_t spr_line {static_cast<uint64_t>(vm._mem[vm._index + y])};
		// Shift the sprite row left or right.
		if (x_shift >= 0) spr_line = spr_line << x_shift;
		else spr_line = spr_line >> (-1 * x_shift);
		// Determine the new screen line.
		uint64_t new_line {vm._screen[ypos + y] ^ spr_line};
		// Set the flag if an overrite happened.
		if (vm._screen[ypos + y] & ~new_line != 0x0) vm._gprf[0x0f] = 0x01;
		// Update the screen memory with the new line.
		vm._screen[ypos + y] = new_line;
	}
	vm._display->draw();
}


void Chip8::in_skpr(Chip8& vm, uint16_t instr) { // EX9E
	if (vm._keyboard->test_key(vm._gprf[instr_b(instr)])) vm._pc += 2;
}


void Chip8::in_skup(Chip8& vm, uint16_t instr) { // EXA1
	if (!vm._keyboard->test_key(vm._gprf[instr_b(instr)])) vm._pc += 2;
}


void Chip8::in_moved(Chip8& vm, uint16_t instr) { // FX07
	vm._gprf[instr_b(instr)] = vm._delay;
}


void Chip8::in_keyd(Chip8& vm, uint16_t instr) { // FX0A
	vm._key_wait = true;
}


void Chip8::in_loadd(Chip8& vm, uint16_t instr) { // FX15
	vm._delay = vm._gprf[instr_b(instr)];
}


void Chip8::in_loads(Chip8& vm, uint16_t instr) { // FX18
	vm._sound = vm._gprf[instr_b(instr)];
}


void Chip8::in_addi(Chip8& vm, uint16_t instr) { // FX1E
	vm._index += vm._gprf[instr_b(instr)];
}


void Chip8::in_ldspr(Chip8& vm, uint16_t instr) { // FX29
	vm._index = FONT_OFF + vm._gprf[instr_b(instr)];
}


// Employs the Double Dabble algorithm.
void Chip8::in_bcd(Chip8& vm, uint16_t instr) { // FX33
	static constexpr uint32_t hundreds = 0xf0000U;
	static constexpr uint32_t tens = 0xf000U;
	static constexpr uint32_t ones = 0xf00U;

	// Grab the initial value from the register.
	uint32_t scratch {vm._gprf[instr_b(instr)]};

	for (size_t i {0}; i < 7; ++i) {
		scratch = scratch << 1; // Shift in each bit of the value.
		// Add 3 to each digit if greater than 4.
		if ((scratch & hundreds) > 0x40000)
			scratch += 0x30000;
		if ((scratch & tens) > 0x4000)
			scratch += 0x3000;
		if ((scratch & ones) > 0x400)
			scratch += 0x300;
	}
	
	scratch = scratch << 1; // Make the last shift.

	if (vm._index < _Prog_Start || vm._index + 2 > _Prog_End)
		Chip8Error("Illegal VM memory operation.");

	// Store each digit in memory.
	vm._mem[vm._index] = (scratch & hundreds) >> 16;
	vm._mem[vm._index + 1] = (scratch & tens) >> 12;
	vm._mem[vm._index + 2] = (scratch & ones) >> 8;
}


void Chip8::in_stor(Chip8& vm, uint16_t instr) { // FX55
	uint8_t upper_bound = instr_b(instr);

	if (vm._index < _Prog_Start || vm._index + upper_bound > _Prog_End)
		throw Chip8Error("Illegal VM memory operation.");

	for (uint32_t i {0}; i <= upper_bound; ++i)
		vm._mem[vm._index ++] = vm._gprf[i];
}


void Chip8::in_read(Chip8& vm, uint16_t instr) { // FX65
	uint8_t upper_bound = instr_b(instr);
	for (uint32_t i {0}; i <= upper_bound; ++i)
		vm._gprf[i] = vm._mem[vm._index ++];
}
