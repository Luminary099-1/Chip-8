#include "Chip8Input.hpp"
#include "Chip8Output.hpp"

#include <cstdint>

/**
 * @brief 
 */
class Chip8 {
public:
	/**
	 * @brief
	 * 
	 */
	Chip8(Chip8Input in, Chip8Output out);

	/**
	 * @brief
	 * 
	 */
	~Chip8();

	/**
	 * @brief 
	 */
	void load_program();

	/**
	 * @brief 
	 */
	void get_state();


	/**
	 * @brief 
	 */
	void set_state();

	/**
	 * @brief 
	 */
	void run();

	/**
	 * @brief 
	 */
	void stop();

private:
	Chip8Input	_input;			// Delegate to handle input (keyboard).
	Chip8Output	_output;		// Delegate to handle output (screen/sound).

	uint8_t		_gprf[16];		// General purpose register file.
	uint16_t	_pc;			// Program counter.
	uint16_t	_sp;			// Stack pointer.
	uint16_t	_index;			// Memory index register.
	uint8_t		_delay;			// Delay timer.
	uint8_t		_sound;			// Sound timer.
	uint8_t		_mem[4096];		// VM memory.
	uint32_t	_screen[256];	// Screen memory.

	/**
	 * @brief 
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
	 * @param addr The address containing the native instruction to be executed.
	 */
	void in_sys(uint16_t addr);

	/**
	 * @brief (00E0) Clears the screen to blank; 0,
	 */
	void in_clr();

	/**
	 * @brief (00EE) Return control from a subroutine.
	 */
	void in_rts();

	/**
	 * @brief (1NNN) Unconditional branch to the instruction at address NNNN.
	 * 
	 * @param addr The address containing the instruction to be jumped to.
	 */
	void in_jump(uint16_t addr);

	/**
	 * @brief (2NNN) Execute the subroutine that begins at address NNN.
	 * 
	 * @param addr The address of the first instruction of the subroutine being
	 * called.
	 */
	void in_call(uint16_t addr);

	/**
	 * @brief (3XNN) Skip the following instruction if the value of vX equals
	 * NN.
	 * 
	 * @param vx The index of a register.
	 * @param value An immediate byte to be compared to the specified register.
	 */
	void in_ske(uint8_t vx, uint8_t value);

	/**
	 * @brief (4XNN) Skip the following instruction if the value of vX does not
	 * equal NN.
	 * 
	 * @param vx The index of a register.
	 * @param value An immediate byte to be compared to the specified register.
	 */
	void in_skne(uint8_t vx, uint8_t value);

	/**
	 * @brief (5XY0) Skip the following instruction if the value of vX equals
	 * the value of vY.
	 * 
	 * @param vx The index of the first register.
	 * @param vy The index of the second register.
	 */
	void in_skre(uint8_t vx, uint8_t vy);

	/**
	 * @brief (6XNN) Load the value NN into vX.
	 * 
	 * @param vx The index of a register.
	 * @param value An immediate byte to be loaded into the specified register.
	 */
	void in_load(uint8_t vx, uint8_t value);

	/**
	 * @brief (7XNN) Add the value NN to vX.
	 * 
	 * @param vx The index of a register.
	 * @param value An immediate byte to be added to the specified register.
	 */
	void in_add(uint8_t vx, uint8_t value);

	/**
	 * @brief (8XY0) Copy the value of vY into vX.
	 * 
	 * @param vx The index of the destination register.
	 * @param vy The index of the source register.
	 */
	void in_move(uint8_t vx, uint8_t vy);

	/**
	 * @brief (8XY1) Set the value of vX to the bitwise disjunction of itself
	 * and vY.
	 * 
	 * @param vx The index of the destination register.
	 * @param vy The index of the other disjunct register.
	 */
	void in_or(uint8_t vx, uint8_t vy);

	/**
	 * @brief (8XY2) Set the value of vX to the bitwise conjunction of itself
	 * and vY.
	 * 
	 * @param vx The index of the destination register.
	 * @param vy The index of the other conjunct register.
	 */
	void in_and(uint8_t vx, uint8_t vy);

	/**
	 * @brief (8XY3) Set the value of vX to the bitwise exclusive disjunction of
	 * itself and vY.
	 * 
	 * @param vx The index of the destination register.
	 * @param vy The index of the other disjunct register.
	 */
	void in_xor(uint8_t vx, uint8_t vy);

	/**
	 * @brief (8XY4) Add the value of vY to vX. The value of vF will be set to
	 * 0x01 if an overflow occurs or 0x00 otherwise.
	 * 
	 * @param vx The index of the destination register.
	 * @param vy The index of the other summand register.
	 */
	void in_addr(uint8_t vx, uint8_t vy);

	/**
	 * @brief (8XY5) Subtract the value of the vY from vX. The value of vF will
	 * be set to 0x01 if an underflow occurs or 0x00 otherwise.
	 * 
	 * @param vx The index of the destination register.
	 * @param vy The index of the subtrahend register.
	 */
	void in_sub(uint8_t vx, uint8_t vy);

	/**
	 * @brief (8XY6) Stores the value of the 1 bit logical shift right of vY in
	 * vX. vF is set to the least significant bit of vY before the shift is
	 * applied and vY is unchanged.
	 * 
	 * @param vx The index of the destination register.
	 * @param vy The index of the source register.
	 */
	void in_shr(uint8_t vx, uint8_t vy);

	/**
	 * @brief (8XY7) Store value of vF minus vX in vX. The value of vF will
	 * be set to 0x01 if an underflow occurs or 0x00 otherwise.
	 * 
	 * @param vx The index of the destination register.
	 * @param vy The index of the minuend register.
	 */
	void in_suba(uint8_t vx, uint8_t vy);

	/**
	 * @brief (8XYE) Stores the value of the 1 bit logical shift left of vY in
	 * vX. vF is set to the least significant bit of vY before the shift is
	 * applied and vY is unchanged.
	 * 
	 * @param vx The index of the destination register.
	 * @param vy The index of the source register.
	 */
	void in_shl(uint8_t vx, uint8_t vy);

	/**
	 * @brief (9XY0) Skip the following instruction if the value of vX does not
	 * equal the value of vY.
	 * 
	 * @param vx The index of the first register.
	 * @param vy The index of the second register.
	 */
	void in_skrne(uint8_t vx, uint8_t vy);

	/**
	 * @brief (ANNN) Store the value at address NNN in I.
	 * 
	 * @param addr The address whose value is to be stored.
	 */
	void in_loadi(uint16_t addr);

	/**
	 * @brief (BNNN) Unconditional branch to the address NNN + v0.
	 * 
	 * @param addr The base address to which the value of v0 will be added to to
	 * determine the branch address.
	 */
	void in_jumpi(uint16_t addr);

	/**
	 * @brief (CXNN) Set vX to a random value using the mask NN.
	 * 
	 * @param vx The index of the destination register.
	 * @param mask An immediate byte to mask the bits of the destination
	 * register that will be randomized.
	 */
	void in_rand(uint8_t vx, uint8_t mask);

	/**
	 * @brief (DXYN) Draw a sprite onto the screen at the position (vX, vY)
	 * using N bytes of sprite data starting at the address stored in I. The
	 * value of vF will be set to 0x01 if any pixels are unset or 0x00
	 * otherwise.
	 * 
	 * @param vx The index of the register containing the sprite's x coordinate.
	 * @param vy The index of the register containing the sprite's y coordinate.
	 * @param n The height of the sprite to be drawn to the screen.
	 */
	void in_draw(uint8_t vx, uint8_t vy, uint8_t n);

	/**
	 * @brief (EX9E) Skip the following instruction if the key corresponding to
	 * the hex value stored in vX is pressed.
	 * 
	 * @param vx The index of the register containing the key value.
	 */
	void in_skpr(uint8_t vx);

	/**
	 * @brief (EXA1) Skip the following instruction if the key corresponding to
	 * the hex value stored in vX is not pressed.
	 * 
	 * @param vx The index of the register containing the key value.
	 */
	void in_skup(uint8_t vx);

	/**
	 * @brief (FX07) Store the value of the delay timer in vX.
	 * 
	 * @param vx The index of the destination register.
	 */
	void in_moved(uint8_t vx);

	/**
	 * @brief (FX0A) Wait for a keypress and store the value of the pressed key
	 * in vX.
	 * 
	 * @param vx The index of the destination register.
	 */
	void in_keyd(uint8_t vx);

	/**
	 * @brief (FX15) Set the delay timer to the value of vX.
	 * 
	 * @param vx The index of the source register.
	 */
	void in_loadd(uint8_t vx);

	/**
	 * @brief (FX18) Set the sound timer to the value of vX.
	 * 
	 * @param vx The index of the source register.
	 */
	void in_loads(uint8_t vx);

	/**
	 * @brief (FX1E) Add the value of vX to I.
	 * 
	 * @param vx The index of the register containing the summand.
	 */
	void in_addi(uint8_t vx);

	/**
	 * @brief (FX29) Set I to the address of the sprite data that corresponds to
	 * the digit stored in vX.
	 * 
	 * @param vx The index of the register whose corresponding sprite address is
	 * to be retrieved.
	 */
	void in_ldspr(uint8_t vx);

	/**
	 * @brief (FX33) Stores the BCD equivalent of the value of vX in memory at
	 * addresses I, I + 1, and I + 2.
	 * 
	 * @param vx The index of the source register.
	 */
	void in_bcd(uint8_t vx);

	/**
	 * @brief (FX55) Stores the values of v0 to vX in memory, starting at the
	 * address specified by I. I is set to I + X after the operation is
	 * complete.
	 * 
	 * @param vx The index of the last register to be stored in memory.
	 */
	void in_stor(uint8_t vx);

	/**
	 * @brief (FX65) Fill the registers v0 to vX with values in memory, starting
	 * with that at the address specified by I. I is set to I - X the operation
	 * is complete.
	 * 
	 * @param vx The index of the last register to be loaded from memory.
	 */
	void in_read(uint8_t vx);
};