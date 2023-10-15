#include "../src/Chip8.hpp"
#include <vector>


/**
 * @brief A mock interface object for testing the CHIP-8 emulation.
 */
struct TestChip8Observer : public Chip8Keyboard, public Chip8Display,
public Chip8Sound, public Chip8Message {
	// Stores the states of all CHIP-8 keyboard keys.
	std::array<bool, 16> _key_states;
	// Stores a history of each frame produced by the emulated display.
	std::vector<uint64_t[32]> _screens;
	// Indicates whether the emulator is outputting sound.
	bool _sounding = false;
	// Stores emulator crash messages.
	std::vector<std::string> _crashes;


	/**
	 * @brief Test if a key for the VM is currently pressed.
	 * 
	 * @param key The value of the key to test.
	 * @return True if the key is pressed; false otherwise.
	 */
	bool test_key(uint8_t key) override;


	/**
	 * @brief Blocks until a keypress is made for the VM and returns the half
	 * byte that corresponds to the key pressed.
	 * 
	 * @return The value of the pressed key.
	 */
	uint8_t wait_key() override;


	/**
	 * @brief Called if the VM has updated the display output.
	 * 
	 * @param screen A pointer to the screen memory, consisting of 32 64-bit
	 * values, each bit representing a pixel.
	 */
	void draw(uint64_t* screen) override;


	/**
	 * @brief Called if the VM is to start emitting sound.
	 */
	void start_sound() override;


	/**
	 * @brief Called if the VM is to stop emitting sound.
	 */
	void stop_sound() override;


	/**
	 * @brief Called if the VM crashed. The VM can crash if: memory is accessed
	 * illegally, the call stack overflows, or the call stack underflows.
	 * 
	 * @param what The message corresponding to the exception.
	 */
	void crashed(const char* what) override;
};
