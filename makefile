make:
	cmake --build build

cmake:
	cmake . -B build -D wxBUILD_SHARED=0