#include <cstdint>

/**
 * @brief 
 */
class Chip8Output {
public:
	/**
	 * @brief 
	 * 
	 */
	virtual void draw();


	/**
	 * @brief 
	 */
	virtual void start_sound();


	/**
	 * @brief 
	 */
	virtual void stop_sound();
};