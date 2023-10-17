#include "TestChip8Observer.hpp"




bool TestChip8Observer::test_key(uint8_t key) {
	return _key_states.at(key);
}


uint8_t TestChip8Observer::wait_key() {
	return 0; // TODO: Decide how to do this.
}


void TestChip8Observer::draw(uint64_t* screen) {
	_screens.emplace_back();
	for (size_t i = 0; i < 32; i ++)
		_screens[_screens.size() - 1]._screen[i] = screen[i];
}


void TestChip8Observer::start_sound() {
	_sounding = true;
}


void TestChip8Observer::stop_sound() {
	_sounding = false;
}


void TestChip8Observer::crashed(const char* what) {
	_crashes.push_back(what);
}