#include "Chip8Observers.hpp"

Chip8Keyboard::Chip8Keyboard(Chip8& parent) {
	_parent_vm = &parent;
}