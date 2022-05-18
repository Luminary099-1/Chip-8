make:
	cmake --build build

cmake:
	cmake . -B build -G "MinGW Makefiles" -D wxBUILD_SHARED=0