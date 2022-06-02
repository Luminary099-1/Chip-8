#include <cstdint>


/**
 * @brief 
 */
struct Chip8Keyboard {
	/**
	 * @brief
	 * 
	 * @param key 
	 * @return 
	 */
	virtual bool test_key(uint8_t key);

	/**
	 * @brief 
	 * 
	 * @return uint8_t 
	 */
	virtual uint8_t wait_key();
};


/**
 * @brief 
 */
struct Chip8Display {
	/**
	 * @brief 
	 * 
	 */
	virtual void draw();
};


/**
 * @brief 
 */
struct Chip8Sound {
	/**
	 * @brief 
	 */
	virtual void start_sound();

	/**
	 * @brief 
	 */
	virtual void stop_sound();
};


/**
 * @brief 
 */
struct Chip8Error {
	/**
	 * @brief 
	 */
	virtual void crashed();
};