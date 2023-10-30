#pragma once
#include "Chip8.hpp"
#include <cstdint>


/**
 * @brief Delegate to handle keyboard input for the Chip-8 VM.
 */
struct Chip8Keyboard {
	/**
	 * @brief Test if a key for the VM is currently pressed.
	 * 
	 * @param key The value of the key to test.
	 * @return True if the key is pressed; false otherwise.
	 */
	virtual bool test_key(uint8_t key) = 0;
};


/**
 * @brief Delegate to handle display output for the Chip-8 VM.
 */
class Chip8Display {
protected:
	friend class Chip8;

	/**
	 * @brief The VM this object will act as display for. Will be assigned by
	 * the VM upon its construction.
	 */
	Chip8* _vm;

public:
	/**
	 * @brief Called if the VM has updated the display output.
	 */
	virtual void draw() = 0;
};


/**
 * @brief Delegate to handle sound output for the Chip-8 VM.
 */
struct Chip8Sound {
	/**
	 * @brief Called if the VM is to start emitting sound.
	 */
	virtual void start_sound() = 0;

	/**
	 * @brief Called if the VM is to stop emitting sound.
	 */
	virtual void stop_sound() = 0;
};


/**
 * @brief Observer for error messages from the Chip-8 VM.
 */
struct Chip8Message {
	/**
	 * @brief Called if the VM crashed. The VM can crash if: memory is accessed
	 * illegally, the call stack overflows, or the call stack underflows.
	 * 
	 * @param what The message corresponding to the exception.
	 */
	virtual void crashed(const char* what) = 0;
};
