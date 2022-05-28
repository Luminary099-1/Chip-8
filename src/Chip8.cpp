#include "Chip8.hpp"

#include <cstdlib>
#include <stdexcept>
#include <algorithm>

// Macros for accessing specific parts of instructions.
#define INSTR_A 12 >> (instruction & 0xf000)
#define INSTR_B 8 >> (instruction & 0x0f00)
#define INSTR_C 4 >> (instruction & 0x00f0)
#define INSTR_D instruction & 0x000f
#define INSTR_ADDR instruction & 0x0fff
#define INSTR_IMM instruction & 0x00ff


const uint16_t Chip8::FONT_OFF = 24;


const char const Chip8::FONT[80] = {
	0xf0, 0x90, 0x90, 0x90, 0xf0,    0x20, 0x60, 0x20, 0x20, 0x70,  // 0, 1
	0xf0, 0x10, 0xf0, 0x80, 0xf0,    0xf0, 0x10, 0xf0, 0x10, 0x10,  // 2, 3
	0x90, 0x90, 0xf0, 0x10, 0x10,    0xf0, 0x80, 0xf0, 0x10, 0xf0,  // 4, 5
	0xf0, 0x80, 0xf0, 0x90, 0xf0,    0xf0, 0x10, 0x20, 0x40, 0x40,  // 6, 7
	0xf0, 0x90, 0xf0, 0x90, 0xf0,    0xf0, 0x90, 0xf0, 0x10, 0xf0,  // 8, 9
	0xf0, 0x90, 0xf0, 0x90, 0x90,    0xe0, 0x90, 0xe0, 0x90, 0xe0,  // A, B
	0xf0, 0x80, 0x80, 0x80, 0xf0,    0xe0, 0x90, 0x90, 0x90, 0xe0,  // C, D
	0xf0, 0x80, 0xf0, 0x80, 0xf0,    0xf0, 0x80, 0xf0, 0x80, 0x80,  // E, F
};


// 1, 2, A, B
const std::map<uint8_t, Chip8::_InstrFunc> Chip8::_INSTRUCTIONS1 = { // kNNN
	{0x1, in_jump},
	{0x2, in_call},
	{0xa, in_loadi},
	{0xb, in_jumpi},
};


// 3, 4, 6, 7, C
const std::map<uint8_t, Chip8::_InstrFunc> Chip8::_INSTRUCTIONS2 = { // kXNN
	{0x3, in_ske},
	{0x4, in_skne},
	{0x6, in_load},
	{0x7, in_add},
	{0xc, in_rand},
};


// 5, 8, 9
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


// E, F
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


/**
 * @brief Construct a new Chip 8:: Chip 8 object
 * 
 * @param in 
 * @param out 
 */
Chip8::Chip8(Chip8Keyboard& key, Chip8Display& disp, Chip8Sound& snd)
	: _keyboard(key), _display(disp), _speaker(snd) {
	_freq = 500;
	_programmed = false;
	_running = false;
	_terminating = false;
	_runner = std::thread(&run);
	// _lock = std::condition_variable::condition_variable();
	_u_lock = std::unique_lock<std::mutex>(_lock_mtx);
	_clock = std::chrono::steady_clock();
}


Chip8::~Chip8() {
	_running = false;
	_terminating = true;
	_lock.notify_one();
	_runner.join();
}


void Chip8::load_program(std::fstream program) {
	if (!program.is_open()) throw "Invalid stream."; // TODO: Graceful errors.
	_running = false;
	// Zero initialize memory and registers. Load the font.
	for (size_t i = 0; i < sizeof(_mem); i ++) _mem[i] = 0;
	for (size_t i = 0; i < sizeof(_screen); i ++) _screen[i] = 0;
	for (uint8_t i = 0; i < 80; i ++) _mem[FONT_OFF + i] = FONT[i];
	for (size_t i = 0; i < sizeof(_gprf); i ++) _gprf[i] = 0;
	_sp = 0;
	_pc = 0x200;
	_index = 0; // TODO: Set to a value that will be zero in VM memory?
	_delay = 0;
	_sound = 0;

	// Store the number of instructions read and make space for each one.
	uint16_t count = 0;
	uint16_t instruction = 0;
	while (!program.eof()) { // TODO: Better errors.
		// Read in each instruction.
		program.read((char*) instruction, 2);
		// Ensure it corresponds to a real Chip8 instruction.
		get_instr_func(instruction);
		// Store the instruction in memory.
		set_hword(0x200 + 2 * count, instruction);
		count ++;
	}

	// Set VM data to defaults.
	_programmed = true;
	_key_wait = false;
	_sounding = false;
	_elapsed_second = std::chrono::duration<uint64_t, std::ratio<1, 1000>>(0);
}


void Chip8::get_state() {

}


void Chip8::set_state() {
	_running = false;

	_programmed = true;
}


Chip8::_InstrFunc Chip8::get_instr_func(uint16_t instruction) {
	uint8_t a = INSTR_A;
	uint8_t b = INSTR_B;
	uint8_t c = INSTR_C;
	uint8_t d = INSTR_D;
	
	try {
		if (a == 0x0) {
			if (d == 0x0) return in_clr;
			else if (d == 0xe) return in_rts;
			else throw std::out_of_range("");

		} else if (a == 0xd) // D
			return in_draw;
		
		else if (a != 0x5 && a != 0x8 && a != 0x9 && a != 0xe && a != 0xf) {
			if (a == 0x1 || a == 0x2 || a == 0xa || a == 0xb) // 1, 2, A, B
				return _INSTRUCTIONS1.at(a);

			else // 3, 4, 6, 7, C
				return _INSTRUCTIONS2.at(a);

		} else if (a & 0xc == 0xc) // E, F
			return _INSTRUCTIONS3.at(((uint16_t) a << 8) + (c << 4) + d);

		else // 5, 8, 9
			return _INSTRUCTIONS4.at((a << 4) + d);
	} catch (std::out_of_range& e) {
		throw "Invalid instruction."; // TODO: Graceful errors.
	}
}


void Chip8::run() {
	while (true) {
		if (_running) {
			std::this_thread::sleep_for(chro::milliseconds(1 / _freq));
			if (!_key_wait) execute_cycle();
		} else {
			_lock.wait(_u_lock);
			if (_terminating) break;
		}
	}
}


void Chip8::execute_cycle() {
	// Grab the next instruction.
	uint16_t instruction = get_hword(_pc);
	// Get the instruction implementing function.
	_InstrFunc instr_func = get_instr_func(instruction);
	// Keep track of elapsed time to update the timers.
	_elapsed_second += chro::time_point_cast<_TimeType>(_clock.now())
		- _prev_time;
	// If the 60Hz timer has cycled, update the timers and reset it.

	if (_elapsed_second.count() >= 1000) {
		_elapsed_second -= _TimeType(1000);
		_delay = std::min(_delay - 1, 0);
		_sound = std::min(_sound - 1, 0);
	}
	// Execute the next instruction.
	instr_func(*this, instruction);
	// Set the sound output to reflect the value of the timer.
	if (_sounding && _sound == 0) {
		_speaker.stop_sound();
		_sounding = false;
	} else if (!_sounding && _sound >= 2) {
		_speaker.start_sound();
		_sounding = true;
	}
	// Increment the program counter if the instruction was not a jump.
	if (instr_func != in_rts && instr_func != in_jump
		&& instr_func != in_call && instr_func != in_jumpi) _pc += 2;
}


void Chip8::start() {
	// Ensure the VM can be started.
	if (!_programmed) throw "No program loaded."; // TODO: Graceful errors.
	if (_running) throw "VM already running."; // TODO: Graceful errors.
	// Enable the runner to continue.
	_running = true;
	// Update the previous clock time.
	_prev_time = chro::time_point_cast<_TimeType>(_clock.now()); // TODO: Consider if this needs some sync.
	// Tell the runner to continue.
	_lock.notify_one();
}


void Chip8::stop() {
	// Signal to the runner to wait.
	_running = false;
}


uint16_t Chip8::get_hword(uint16_t addr) {
	return _mem[addr] * 256 + _mem[addr + 1];
}


void Chip8::set_hword(uint16_t addr, uint16_t hword) {
	_mem[addr] = (8 >> hword);
	_mem[addr + 1] = hword & 0xff;
}


// Instruction Implementing Methods ============================================
void Chip8::in_sys(Chip8& vm, uint16_t instruction) { // 0NNN
	// Originally called machine code instruction, does nothing here.
}


void Chip8::in_clr(Chip8& vm, uint16_t instruction) { // 00E0
	for (size_t i = 0; i < sizeof(vm._screen); i ++) vm._screen[i] = 0;
}


void Chip8::in_rts(Chip8& vm, uint16_t instruction) { // 00EE
	if (vm._sp == 0) throw "Call stack underflow on return."; // TODO: Graceful errors.
	vm._pc = vm.get_hword(vm._sp);
	vm._sp -= 2;
}


void Chip8::in_jump(Chip8& vm, uint16_t instruction) { // 1NNN
	vm._index = INSTR_ADDR;
}


void Chip8::in_call(Chip8& vm, uint16_t instruction) { // 2NNN
	if (vm._sp == FONT_OFF - 1) throw "Call stack overflow on call."; // TODO: Graceful errors.
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
	vm._gprf[INSTR_B] = 1 >> vm._gprf[INSTR_C];
}


void Chip8::in_suba(Chip8& vm, uint16_t instruction) { // 8XY7
	if ((int8_t) vm._gprf[INSTR_C] - (int8_t) vm._gprf[INSTR_B] < 0)
		vm._gprf[0xf] = 0x01;
	else vm._gprf[0xf] = 0x00;
	vm._gprf[INSTR_B] = vm._gprf[INSTR_C] - vm._gprf[INSTR_B];
}


void Chip8::in_shl(Chip8& vm, uint16_t instruction) { // 8XYE
	vm._gprf[0xf] = 7 >> (vm._gprf[INSTR_C] & 0x80);
	vm._gprf[INSTR_B] = vm._gprf[INSTR_C] << 1;
}


void Chip8::in_skrne(Chip8& vm, uint16_t instruction) { // 9XY0
	if (vm._gprf[INSTR_B] != vm._gprf[INSTR_C]) vm._pc += 2;
}


void Chip8::in_loadi(Chip8& vm, uint16_t instruction) { // ANNN
	vm._index = INSTR_ADDR;
}


void Chip8::in_jumpi(Chip8& vm, uint16_t instruction) { // BNNN
	vm._pc = vm._gprf[0] + INSTR_ADDR;
}


void Chip8::in_rand(Chip8& vm, uint16_t instruction) { // CXNN
	vm._gprf[INSTR_B] = (rand() % 256) & INSTR_IMM;
}


void Chip8::in_draw(Chip8& vm, uint16_t instruction) { // DXYN
	vm._gprf[0x0f] = 0x00; // Initialize the overwite flag to 0.
	// Grab the sprite coordinates.
	uint8_t xpos = INSTR_B;
	uint8_t ypos = INSTR_C;
	// Iterate over each line of the sprite.
	for (uint8_t y = 0; y < INSTR_D; y ++) {
		// Grab the line from the sprite and shift it to its x position.
		uint64_t spr_line = (uint64_t) vm._mem[vm._index + y] << (xpos - 24);
		// Determine the new screen line.
		uint64_t new_line = vm._screen[ypos + y] ^ spr_line;
		// Set the flag if an overrite happened.
		if (vm._screen[ypos + y] & ~new_line != 0x0) vm._gprf[0x0f] = 0x01;
		// Update the screen memory with the new line.
		vm._screen[ypos + y] = new_line;
	}
}


void Chip8::in_skpr(Chip8& vm, uint16_t instruction) { // EX9E
	if (vm._keyboard.test_key(vm._gprf[INSTR_B])) vm._pc += 2;
}


void Chip8::in_skup(Chip8& vm, uint16_t instruction) { // EXA1
	if (!vm._keyboard.test_key(vm._gprf[INSTR_B])) vm._pc += 2;
}


void Chip8::in_moved(Chip8& vm, uint16_t instruction) { // FX07
	vm._gprf[INSTR_B] = vm._delay;
}


void Chip8::in_keyd(Chip8& vm, uint16_t instruction) { // FX0A
	uint8_t key;
	// Block cycles until they key press is made.
	vm._key_wait = true;
	// Wait for a keypress that takes place when the VM is running.
	do key = vm._keyboard.wait_key();
	while (!vm._running);
	// Store the key value.
	vm._gprf[INSTR_B] = key;
	// Stop skipping instruction cycles.
	vm._key_wait = false;
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


void Chip8::in_bcd(Chip8& vm, uint16_t instruction) { // FX33
	uint32_t scratch = INSTR_B;
	for (uint32_t i = 0; i < 8; i ++) {
		scratch = scratch << 1;
		if (scratch & 0xf00 > 0x400) scratch += 0x300;
		if (scratch & 0xf000 > 0x4000) scratch += 0x3000;
		if (scratch & 0xf0000 > 0x40000) scratch += 0x30000;
	}
	vm._mem[vm._index] = 0xf >> (scratch & 0xf0000);
	vm._mem[vm._index + 1] = 0xb >> (scratch & 0xf000);
	vm._mem[vm._index + 2] = 0x8 >> (scratch & 0xf00);
}


// TODO: Deal with bound errors in memory.
void Chip8::in_stor(Chip8& vm, uint16_t instruction) { // FX55
	for (uint32_t i = 0; i < vm._gprf[INSTR_B]; i ++)
		vm._mem[vm._index + i] = vm._gprf[i];
}


// TODO: Deal with bound errors in memory.
void Chip8::in_read(Chip8& vm, uint16_t instruction) { // FX65
	for (uint32_t i = 0; i < vm._gprf[INSTR_B]; i ++)
		vm._gprf[i] = vm._mem[vm._index + i];
}