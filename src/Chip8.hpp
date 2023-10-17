#pragma once

#include "Chip8Observers.hpp"

#include <array>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <fstream>
#include <map>
#include <mutex>
#include <string>
#include <thread>

namespace chro = std::chrono;


/**
 * @brief Standard exception class for errors pertaining to emulation.
 */
struct Chip8Error : public std::runtime_error {
	Chip8Error(const std::string& what_arg) : std::runtime_error(what_arg) {}
};


/**
 * @brief Asynchronous Chip-8 virtual machine. Only compatible with the original
 * Chip-8 language.
 */
class Chip8 {
	friend class TestChip8;
public:
	uint16_t _freq; // Instruction cycle frequency. Defaults to 500Hz.

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
	 * @param msg A reference to the error handling delegate to be used by this
	 * VM.
	 */
	Chip8(Chip8Keyboard* key, Chip8Display* disp,
		Chip8Sound* snd, Chip8Message* msg);

	/**
	 * @brief Stops the execution of the VM (if running) and destroys the
	 * instance.
	 */
	~Chip8();

	/**
	 * @brief Loads in the passed program and initializes the VM to run from its
	 * start.
	 * 
	 * @param program A string that contains the program to be loaded. The
	 * contents of the steam are assumed to be in "compiled" Chip-8 byte code,
	 * each instruction being two bytes with nothing in between each.
	 */
	void load_program(std::string& program);

	/**
	 * @brief Obtain the current state of the VM.
	 * 
	 * @return A string containing the state of the VM.
	 */
	std::string get_state();

	/**
	 * @brief Loads in the passed state into the VM to be resumed.
	 * 
	 * @param source A reference to a string containing the VM state to be
	 * loaded.
	 */
	void set_state(std::string& source);
	
	/**
	 * @brief Start the VM's execution.
	 */
	void start();

	/**
	 * @brief Stop the VM's execution.
	 */
	void stop();

	/**
	 * @brief Indicates if the VM is currently running. The VM is running even
	 * if it has not yet reacted to a call to stop().
	 * 
	 * @return Returns true if the VM is running; false otherwise.
	 */
	bool is_running();

	/**
	 * @brief Indicats if the VM has crashed.
	 * 
	 * @return Returns true if the VM crashed; false otherwise.
	 */
	bool is_crashed();

protected:
	// Type of instruction implementing functions.
	typedef void (*_InstrFunc) (Chip8& vm, uint16_t instruction);
	// Type of std::chrono::duration for keeping the timers.
	typedef chro::duration<uint64_t, std::ratio<1, 1000>> _TimeType;

	uint8_t			_gprf[16];		// General purpose register file.
	uint16_t		_pc;			// Program counter.
	uint16_t		_sp;			// Stack pointer.
	uint16_t		_index;			// Memory index register.
	uint8_t			_delay;			// Delay timer.
	uint8_t			_sound;			// Sound timer.
	uint8_t			_mem[4096];		// VM memory.
	uint64_t		_screen[32];	// Screen memory (1 dword = 1 row).

	Chip8Keyboard*	_keyboard;		// Delegate to handle input (keyboard).
	Chip8Display*	_display;		// Delegate to handle output (screen).
	Chip8Sound*		_speaker;		// Delegate to handle output (sound).
	Chip8Message*	_error;			// Delegate to receive error notifications.
	bool			_programmed;	// True if the machine has a program.
	bool			_key_wait;		// True if in_keyd (FX0A) is "blocking".
	bool			_sounding;		// True if sound is playing.
	bool			_running;		// True if the VM is running cycles.
	bool			_stopped;		// True if the runner is stopped.
	bool			_crashed;		// True if the VM crashed.
	bool			_terminating;	// True if the runner is to end execution.
	std::thread		_runner;		// Thread to handle operation of the VM.
	std::mutex		_lock_mtx;		// Mutex for the runner lock.
	// Unique lock for the runner lock.
	std::unique_lock<std::mutex> _u_lock;
	std::condition_variable	_lock;	// Blocks the runner when the VM is stopped.
	chro::steady_clock _clock;		// The clock used to maintain the timers.
	// The _clock time of the previous cycle.
	chro::time_point<chro::steady_clock, _TimeType> _prev_time;
	_TimeType		_elapsed_second;	// The time since the last timer pulse.
	static const uint16_t FONT_OFF;		// VM font memory offset.
	static const uint8_t FONT[80];		// VM font data.
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
	 */
	static _InstrFunc get_instr_func(uint16_t instruction);

	/**
	 * @brief Asynchronously sets VM cycles continuously, at a rate no faster
	 * than the value of _freq. If _running is set to false, the function will
	 * stop executing cycles, lock the _lock condition variable, and sets
	 * _stopped to true. To resume execution, _running must be set true and
	 * _lock released; _stopped is set by run() when execution resumes.
	 * 
	 * @param vm Chip8 reference of the VM to run cycles on.
	 */
	static void run(Chip8* vm);

	/**
	 * @brief Executes the next Chip-8 instruction cycles, given the state of
	 * the VM.
	 */
	void execute_cycle();

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

public:
	/* Instruction Implementing Methods ========================================
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
	 * @param instruction The instruction being executed.
	 */
	static void in_sys(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (00E0) Clears the screen to blank; 0,
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_clr(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (00EE) Return control from a subroutine.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_rts(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (1NNN) Unconditional branch to the instruction at address NNNN.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_jump(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (2NNN) Execute the subroutine that begins at address NNN.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 * called.
	 */
	static void in_call(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (3XNN) Skip the following instruction if the value of vX equals
	 * NN.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_ske(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (4XNN) Skip the following instruction if the value of vX does not
	 * equal NN.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_skne(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (5XY0) Skip the following instruction if the value of vX equals
	 * the value of vY.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_skre(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (6XNN) Load the value NN into vX.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_load(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (7XNN) Add the value NN to vX.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_add(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (8XY0) Copy the value of vY into vX.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_move(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (8XY1) Set the value of vX to the bitwise disjunction of itself
	 * and vY.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_or(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (8XY2) Set the value of vX to the bitwise conjunction of itself
	 * and vY.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_and(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (8XY3) Set the value of vX to the bitwise exclusive disjunction of
	 * itself and vY.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_xor(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (8XY4) Add the value of vY to vX. The value of vF will be set to
	 * 0x01 if an overflow occurs or 0x00 otherwise.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_addr(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (8XY5) Subtract the value of the vY from vX. The value of vF will
	 * be set to 0x01 if an underflow occurs or 0x00 otherwise.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_sub(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (8XY6) Stores the value of the 1 bit logical shift right of vY in
	 * vX. vF is set to the least significant bit of vY before the shift is
	 * applied and vY is unchanged.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_shr(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (8XY7) Store value of vF minus vX in vX. The value of vF will
	 * be set to 0x01 if an underflow occurs or 0x00 otherwise.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_suba(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (8XYE) Stores the value of the 1 bit logical shift left of vY in
	 * vX. vF is set to the least significant bit of vY before the shift is
	 * applied and vY is unchanged.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_shl(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (9XY0) Skip the following instruction if the value of vX does not
	 * equal the value of vY.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_skrne(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (ANNN) Store the value at address NNN in I.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_loadi(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (BNNN) Unconditional branch to the address NNN + v0.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_jumpi(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (CXNN) Set vX to a random value using the mask NN.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_rand(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (DXYN) Draw a sprite onto the screen at the position (vX, vY)
	 * using N bytes of sprite data starting at the address stored in I. The
	 * value of vF will be set to 0x01 if any pixels are unset or 0x00
	 * otherwise.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_draw(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (EX9E) Skip the following instruction if the key corresponding to
	 * the hex value stored in vX is pressed.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_skpr(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (EXA1) Skip the following instruction if the key corresponding to
	 * the hex value stored in vX is not pressed.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_skup(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (FX07) Store the value of the delay timer in vX.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_moved(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (FX0A) Wait for a keypress and store the value of the pressed key
	 * in vX.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_keyd(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (FX15) Set the delay timer to the value of vX.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_loadd(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (FX18) Set the sound timer to the value of vX.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_loads(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (FX1E) Add the value of vX to I.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_addi(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (FX29) Set I to the address of the sprite data that corresponds to
	 * the digit stored in vX.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_ldspr(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (FX33) Stores the BCD equivalent of the value of vX in memory at
	 * addresses I, I + 1, and I + 2.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_bcd(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (FX55) Stores the values of v0 to vX in memory, starting at the
	 * address specified by I. I is set to I + X after the operation is
	 * complete.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_stor(Chip8& vm, uint16_t instruction);

	/**
	 * @brief (FX65) Fill the registers v0 to vX with values in memory, starting
	 * with that at the address specified by I. I is set to I - X the operation
	 * is complete.
	 * 
	 * @param vm Chip8 reference on which to apply the instruction.
	 * @param instruction The instruction being executed.
	 */
	static void in_read(Chip8& vm, uint16_t instruction);
};