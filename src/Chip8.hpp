#pragma once

#include "Chip8Observers.hpp"
#include <array>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <map>
#include <mutex>
#include <iostream>
#include <string>
#include <thread>


/**
 * @brief Standard exception class for errors pertaining to emulation.
 */
struct Chip8Error : public std::runtime_error
{
	Chip8Error(const std::string& what_arg);
};


/**
 * @brief Asynchronous Chip-8 virtual machine. Only compatible with the original
 * Chip-8 language.
 */
class Chip8
{
public:
	// Type of std::chrono::duration to store the duration of execution.
	typedef std::chrono::nanoseconds _TimeType;
	// The number of nanoseconds in a second.
	static constexpr long long _billion {1000000000U};

protected:
	uint16_t	_pc {0};					// Program counter.
	uint16_t	_sp {0};					// Stack pointer.
	uint16_t	_index {0};					// Memory index register.
	uint8_t		_delay {0};					// Delay timer.
	uint8_t		_sound {0};					// Sound timer.
	bool		_sounding {false};			// Set if sound is playing.
	bool		_crashed {false};			// Set if the VM crashed.
	bool		_programmed {false};		// Set if a program is loaded.
	bool		_can_draw {true};			// Set just after a "screen refesth".
	std::atomic<bool> _key_wait {false};	// Set if in_keyd is waiting.
	_TimeType	_time_budget {0};			// Time available to execute cycles.
	_TimeType	_timer {0};					// Duration remaining for timers.
	std::array<uint8_t, 16>		_gprf;		// General purpose register file.
	std::array<uint8_t, 4096>	_mem;		// VM memory.
	std::array<uint64_t, 32>	_screen;	// Screen memory (1 dword = 1 row).

public:
	// First address of the program space in Chip-8 memory.
	static constexpr uint16_t _Prog_Start {0x200};
	// Largest legal program size.
	static constexpr uint16_t _Max_Prog_Size {sizeof(_mem) - _Prog_Start};

	/**
	 * @brief Construct a new Chip-8 VM. A program will have to be loaded or a
	 * state restored before start() can be called.
	 * 
	 * @param key A reference to the keyboard input delegate to be used by this
	 * VM.
	 * @param disp A reference to the display output delegate to be used by this
	 * VM.
	 * @param snd A reference to the sound output delegate to be used by this
	 * VM.
	 */
	Chip8(Chip8Keyboard* key, Chip8Display* disp, Chip8Sound* snd);

	/**
	 * @brief Initializes the VM's state to be empty (unprogrammed).
	 */
	void clear_state();

	/**
	 * @brief Loads in the passed program and initializes the VM to run from its
	 * start.
	 * 
	 * Blocks if any blocking operating is being used by another thread.
	 * 
	 * @param program A string that contains the program to be loaded. The
	 * contents of the steam are assumed to be in "compiled" Chip-8 byte code,
	 * each instruction being two bytes with nothing in between each.
	 * @throws std::invalid_argument if the loaded program is too large.
	 */
	void load_program(std::string& program);

	/**
	 * @brief Stream insertion operator override.
	 * 
	 * @param os The output stream to write to.
	 * @param st The instance of Chip8 whose state will be output.
	 * @return std::ostream& The output stream written to.
	 * @throws std::ios_base::failure if an error occurs while writing.
	 */
	friend std::ostream& operator<<(std::ostream& os, Chip8& st);

	/**
	 * @brief Stream extraction operator override.
	 * 
	 * @param is The input stream to read from.
	 * @param st The instance of Chip8 whose state will be overwritten.
	 * @return std::istream& The input stread read from.
	 * @throws std::ios_base::failure if the end of the stream is encountered
	 * early or an error occurs while reading.
	 */
	friend std::istream& operator>>(std::istream& is, Chip8& st);

	/**
	 * @return true if the VM crashed; false otherwise.
	 */
	bool is_crashed();

	/**
	 * @return true if the VM is making sound; false otherwise.
	 */
	bool is_sounding();

	/**
	 * @return true if the VM has a program loaded; false otherwise.
	 */
	bool is_programmed();

	/**
	 * @brief Run the emulator for the specified duration.
	 * 
	 * Blocks if any blocking operating is being used by another thread.
	 * 
	 * @param elapsed_time The number of miliseconds to run the emulation
	 * forward.
	 * @throws Chip8Error if the virtual machine crashes or has already crashed.
	 */
	void execute_batch(_TimeType elapsed_time);

	/**
	 * @return The current emulation instruction cycle frequency in Hz. 
	 */
	uint16_t frequency();

	/**
	 * @brief Set the emulation instruction cycle frequency.
	 * 
	 * Blocks if any blocking operating is being used by another thread.
	 * 
	 * @param value The new frequency in Hz.
	 */
	void frequency(uint16_t value);

	/**
	 * @brief Call to indicate the passed key was just pressed. A corresponding
	 * call to key_released must be made after  every call to this function.
	 * 
	 * @param key The value of the key that was just pressed.
	 * 
	 * @throws std::domain_error("Key value too large.") if the specifed value
	 * is greater than 15.
	 */
	void key_pressed(uint8_t key);


	/**
	 * @brief Indicates the specified key has been released.
	 * 
	 * @param key The value of the key that was just released.
	 * 
	 * @throws std::domain_error("Key value too large.") if the specifed value
	 * is greater than 15.
	 */
	void key_released(uint8_t key);

	/**
	 * @brief Provides access to the VM's screen buffer for drawing.
	 * 
	 * Blocks if any blocking operating is being used by another thread until
	 * release_screen_buf() is called.
	 * 
	 * @return A pointer to the 64-bit value containing the first line of screen
	 * data.
	 */
	uint64_t* get_screen_buf();

protected:
	// Type of instruction implementing functions.
	typedef void (*_InstrFunc) (Chip8& vm, uint16_t instruction);
	// Invalid key sentinel.
	static constexpr uint8_t _no_key {0x10};

	Chip8Keyboard*	_keyboard;	// Handles input (keyboard).
	Chip8Display*	_display;	// Handles output (screen).
	Chip8Sound*		_speaker;	// Handles output (sound).
	uint16_t	_freq {1200};	// Instruction cycle frequency.
	std::mutex	_access_lock;	// Protects asynchronous access.
	uint8_t _pressed_key {_no_key}; // The key value waiting to be released.
	
	// VM font memory offset.
	static constexpr uint16_t _font_off {32};
	// VM font data.
	static constexpr uint8_t _font[80]
	{
		0xf0, 0x90, 0x90, 0x90, 0xf0,    0x20, 0x60, 0x20, 0x20, 0x70,  // 0, 1
		0xf0, 0x10, 0xf0, 0x80, 0xf0,    0xf0, 0x10, 0xf0, 0x10, 0xf0,  // 2, 3
		0x90, 0x90, 0xf0, 0x10, 0x10,    0xf0, 0x80, 0xf0, 0x10, 0xf0,  // 4, 5
		0xf0, 0x80, 0xf0, 0x90, 0xf0,    0xf0, 0x10, 0x20, 0x40, 0x40,  // 6, 7
		0xf0, 0x90, 0xf0, 0x90, 0xf0,    0xf0, 0x90, 0xf0, 0x10, 0xf0,  // 8, 9
		0xf0, 0x90, 0xf0, 0x90, 0x90,    0xe0, 0x90, 0xe0, 0x90, 0xe0,  // A, B
		0xf0, 0x80, 0x80, 0x80, 0xf0,    0xe0, 0x90, 0x90, 0x90, 0xe0,  // C, D
		0xf0, 0x80, 0xf0, 0x80, 0xf0,    0xf0, 0x80, 0xf0, 0x80, 0x80,  // E, F
	};

	// Lookup table for instructions of the form kNNN.
	static const std::map<uint8_t, _InstrFunc> _INSTRUCTIONS1;
	// Lookup table for instructions of the form kXNN.
	static const std::map<uint8_t, _InstrFunc> _INSTRUCTIONS2;
	// Lookup table for instructions of the form kXYk.
	static const std::map<uint8_t, _InstrFunc> _INSTRUCTIONS3;
	// Lookup table for instructions of the form kXkk.
	static const std::map<uint16_t, _InstrFunc> _INSTRUCTIONS4;

	/**
	 * @brief Returns the function that implements the passed Chip-8
	 * instruction.
	 * 
	 * @param instruction A Chip-8 instruction.
	 * @return A pointer to the instruction implementing function that
	 * corresponds to the passed instruction.
	 * @throws Chip8Error if the passed instruction is not a valid Chip-8
	 * instruction.
	 */
	static _InstrFunc get_instr_func(uint16_t instruction);

	/**
	 * @brief Executes the next Chip-8 instruction cycles, given the state of
	 * the VM.
	 * @param cycle_time The amount of time that will pass over the execution of
	 * this cycle.
	 * @throws Chip8Error If a cycle could not be executed.
	 */
	void execute_cycle(_TimeType cycle_time);

	/**
	 * @brief Retrives the halfword in memory at the specified address.
	 * 
	 * @param addr The address of the first byte of the halfword to retrieve.
	 * @return The halfword at the specified address.
	 */
	uint16_t get_hword(uint16_t addr);

	/**
	 * @brief Stores the given halfword in memory at the specified address.
	 * 
	 * @param addr The address for the first byte of the halfword.
	 * @param hword The halfword to be stored.
	 */
	void set_hword(uint16_t addr, uint16_t hword);

	/**
	 * @param instruction A two-byt e Chip8 instruction.
	 * @return Returns the first half-byte of the passed instruction. 
	 */
	static constexpr uint8_t instr_a(uint16_t instruction);

	/**
	 * @param instruction A two-byt e Chip8 instruction.
	 * @return Returns the second half-byte of the passed instruction. 
	 */
	static constexpr uint8_t instr_b(uint16_t instruction);

	/**
	 * @param instruction A two-byt e Chip8 instruction.
	 * @return Returns the third half-byte of the passed instruction. 
	 */
	static constexpr uint8_t instr_c(uint16_t instruction);

	/**
	 * @param instruction A two-byt e Chip8 instruction.
	 * @return Returns the fourth half-byte of the passed instruction. 
	 */
	static constexpr uint8_t instr_d(uint16_t instruction);

	/**
	 * @param instruction A two-byt e Chip8 instruction.
	 * @return Returns the address encoded in the last 3 half-bytes of the
	 * passed instruction.
	 */
	static constexpr uint16_t instr_addr(uint16_t instruction);

	/**
	 * @param instruction A two-byt e Chip8 instruction.
	 * @return Returns the immediate value encoded in the second byte of the
	 * passed instruction.
	 */
	static constexpr uint8_t instr_imm(uint16_t instruction);

/* Instruction Implementing Methods ============================================
	 * Each instruction description below begins with its CHIP-8 opcode. Legend:
	 *		N = A hexadecimal digit.
	 *		vX = A register where X is a hexadecimal digit.
	 *		vY = A register where Y is a hexadecimal digit.
	 *		I = The memory index register.
	 */

	/**
	 * @brief (0NNN) Exectures the machine instruction at address NNN. Note that
	 * this is ignored by this emulation.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_sys(Chip8& vm, uint16_t instr);

	/**
	 * @brief (00E0) Clears the screen to blank; 0,
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_clr(Chip8& vm, uint16_t instr);

	/**
	 * @brief (00EE) Return control from a subroutine.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_rts(Chip8& vm, uint16_t instr);

	/**
	 * @brief (1NNN) Unconditional branch to the instruction at address NNNN.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_jump(Chip8& vm, uint16_t instr);

	/**
	 * @brief (2NNN) Execute the subroutine that begins at address NNN.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 * called.
	 */
	static void in_call(Chip8& vm, uint16_t instr);

	/**
	 * @brief (3XNN) Skip the following instruction if the value of vX equals
	 * NN.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_ske(Chip8& vm, uint16_t instr);

	/**
	 * @brief (4XNN) Skip the following instruction if the value of vX does not
	 * equal NN.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_skne(Chip8& vm, uint16_t instr);

	/**
	 * @brief (5XY0) Skip the following instruction if the value of vX equals
	 * the value of vY.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_skre(Chip8& vm, uint16_t instr);

	/**
	 * @brief (6XNN) Load the value NN into vX.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_load(Chip8& vm, uint16_t instr);

	/**
	 * @brief (7XNN) Add the value NN to vX.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_add(Chip8& vm, uint16_t instr);

	/**
	 * @brief (8XY0) Copy the value of vY into vX.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_move(Chip8& vm, uint16_t instr);

	/**
	 * @brief (8XY1) Set the value of vX to the bitwise disjunction of itself
	 * and vY.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_or(Chip8& vm, uint16_t instr);

	/**
	 * @brief (8XY2) Set the value of vX to the bitwise conjunction of itself
	 * and vY.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_and(Chip8& vm, uint16_t instr);

	/**
	 * @brief (8XY3) Set the value of vX to the bitwise exclusive disjunction of
	 * itself and vY.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_xor(Chip8& vm, uint16_t instr);

	/**
	 * @brief (8XY4) Add the value of vY to vX. The value of vF will be set to
	 * 0x01 if an overflow occurs or 0x00 otherwise.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_addr(Chip8& vm, uint16_t instr);

	/**
	 * @brief (8XY5) Subtract the value of the vY from vX. The value of vF will
	 * be set to 0x01 if an underflow occurs or 0x00 otherwise.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_sub(Chip8& vm, uint16_t instr);

	/**
	 * @brief (8XY6) Stores the value of the 1 bit logical shift right of vY in
	 * vX. vF is set to the least significant bit of vY before the shift is
	 * applied and vY is unchanged.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_shr(Chip8& vm, uint16_t instr);

	/**
	 * @brief (8XY7) Store value of vF minus vX in vX. The value of vF will
	 * be set to 0x01 if an underflow occurs or 0x00 otherwise.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_suba(Chip8& vm, uint16_t instr);

	/**
	 * @brief (8XYE) Stores the value of the 1 bit logical shift left of vY in
	 * vX. vF is set to the least significant bit of vY before the shift is
	 * applied and vY is unchanged.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_shl(Chip8& vm, uint16_t instr);

	/**
	 * @brief (9XY0) Skip the following instruction if the value of vX does not
	 * equal the value of vY.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_skrne(Chip8& vm, uint16_t instr);

	/**
	 * @brief (ANNN) Store the value at address NNN in I.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_loadi(Chip8& vm, uint16_t instr);

	/**
	 * @brief (BNNN) Unconditional branch to the address NNN + v0.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_jumpi(Chip8& vm, uint16_t instr);

	/**
	 * @brief (CXNN) Set vX to a random value using the mask NN.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_rand(Chip8& vm, uint16_t instr);

	/**
	 * @brief (DXYN) Draw a sprite onto the screen at the position (vX, vY)
	 * using N bytes of sprite data starting at the address stored in I. The
	 * value of vF will be set to 0x01 if any pixels are unset or 0x00
	 * otherwise.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_draw(Chip8& vm, uint16_t instr);

	/**
	 * @brief (EX9E) Skip the following instruction if the key corresponding to
	 * the hex value stored in vX is pressed.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_skpr(Chip8& vm, uint16_t instr);

	/**
	 * @brief (EXA1) Skip the following instruction if the key corresponding to
	 * the hex value stored in vX is not pressed.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_skup(Chip8& vm, uint16_t instr);

	/**
	 * @brief (FX07) Store the value of the delay timer in vX.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_moved(Chip8& vm, uint16_t instr);

	/**
	 * @brief (FX0A) Wait for a keypress and store the value of the pressed key
	 * in vX.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_keyd(Chip8& vm, uint16_t instr);

	/**
	 * @brief (FX15) Set the delay timer to the value of vX.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_loadd(Chip8& vm, uint16_t instr);

	/**
	 * @brief (FX18) Set the sound timer to the value of vX.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_loads(Chip8& vm, uint16_t instr);

	/**
	 * @brief (FX1E) Add the value of vX to I.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_addi(Chip8& vm, uint16_t instr);

	/**
	 * @brief (FX29) Set I to the address of the sprite data that corresponds to
	 * the digit stored in vX.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_ldspr(Chip8& vm, uint16_t instr);

	/**
	 * @brief (FX33) Stores the BCD equivalent of the value of vX in memory at
	 * addresses I, I + 1, and I + 2.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_bcd(Chip8& vm, uint16_t instr);

	/**
	 * @brief (FX55) Stores the values of v0 to vX in memory, starting at the
	 * address specified by I. I is set to I + X after the operation is
	 * complete.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_stor(Chip8& vm, uint16_t instr);

	/**
	 * @brief (FX65) Fill the registers v0 to vX with values in memory, starting
	 * with that at the address specified by I. I is set to I - X the operation
	 * is complete.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instr The instruction being executed.
	 */
	static void in_read(Chip8& vm, uint16_t instr);
};
