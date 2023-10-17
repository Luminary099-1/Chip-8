#pragma once

#include "../src/Chip8.hpp"


/**
 * @brief Extension of the Chip-8 VM to facilitate testing.
 */
struct TestChip8 : public Chip8 {
	/**
	 * @brief Constructs an instance of TestChip8 with the specified delegates.
	 * 
	 * @param key An object that will provide keyboard inputs.
	 * @param disp An object that will output the emulators display.
	 * @param snd An object that will output the emulator's sound.
	 * @param msg An object that will listen to error messages from the
	 * emulator.
	 */
	TestChip8(Chip8Keyboard* key, Chip8Display* disp, Chip8Sound* snd,
		Chip8Message* msg) : Chip8(key, disp, snd, msg) {};


	/**
	 * @return The current value of the emulated program counter.
	 */
	uint16_t get_pc();


	/**
	 * @return The current value of the emulated stack pointer.
	 */
	uint16_t get_sp();


	/**
	 * @return The current value of the emulated index (address) register.
	 */
	uint16_t get_index();


	/**
	 * @return The current value of the emulated delay timer.
	 */
	uint8_t get_delay();


	/**
	 * @return The current value of the emulated sound timer.
	 */
	uint8_t get_sound();


	/**
	 * @return A pointer to the emulated memory.
	 */
	uint8_t* get_mem();


	/**
	 * @return A pointer to the emulated screen buffer.
	 */
	uint64_t* get_screen();


	/**
	 * @return true if there is a program loaded in the emulator; false
	 * otherwise.
	 */
	bool get_programmed();


	/**
	 * @return true if the emulated machine is waiting for a key press, false
	 * otherwise.
	 */
	bool get_key_wait();


	/**
	 * @return true if the emulated machine is outputting sound, false
	 * otherwise.
	 */
	bool get_sounding();


	/**
	 * @return true if the emulated machine is to execute cycles, false
	 * otherwise.
	 */
	bool get_running();


	/**
	 * @return true if the emulated machine is stopped, false otherwise.
	 */
	bool get_stopped();


	/**
	 * @return true if the emulated machine crashed, false otherwise.
	 */
	bool get_crashed();


	/**
	 * @return true if the emulator object is being destroyed (to inform other
	 * threads to halt).
	 */
	bool get_terminating();


	/**
	 * @brief Returns the function that executes the specified CHIP-8
	 * instruction. The call is passed to the base class, exposing it for
	 * testing.
	 * 
	 * @param instruction The CHIP-8 instruction to decode.
	 * @return _InstrFunc 
	 */
	static _InstrFunc get_instr_func(uint16_t instruction);
};