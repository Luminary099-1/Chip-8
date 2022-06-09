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

	/**
	 * @brief Blocks until a keypress is made for the VM and returns the half
	 * byte that corresponds to the key pressed.
	 * 
	 * @return The value of the pressed key.
	 */
	virtual uint8_t wait_key() = 0;
};


/**
 * @brief Delegate to handle display output for the Chip-8 VM.
 */
struct Chip8Display {
	/**
	 * @brief Called if the VM has updated the display output.
	 * 
	 * @param screen A pointer to the screen memory, consisting of 32 64-bit
	 * values, each bit representing a pixel.
	 */
	virtual void draw(uint64_t* screen) = 0;
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
	 */
	virtual void crashed() = 0;
};