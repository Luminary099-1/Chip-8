#include <cstdint>

/**
 * @brief 
 */
class Chip8Keyboard {
public:
	/**
	 * @brief
	 * 
	 * @param key 
	 * @return true 
	 * @return false 
	 */
	virtual bool test_key(uint8_t key);

	/**
	 * @brief 
	 * 
	 * @return uint8_t 
	 */
	virtual uint8_t wait_key();
};