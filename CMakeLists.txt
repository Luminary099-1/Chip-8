cmake_minimum_required(VERSION 3.23)
project(chip-8-cpp)
include(CTest)
enable_testing()

set(BUILD_SHARED_LIBS FALSE)

add_subdirectory(lib/wx)
set(wxBUILD_SHARED OFF)
set(wxUSE_ACCESSIBILITY OFF)
# add_subdirectory(lib/catch2)

add_executable(chip-8-cpp WIN32 src/Main.cpp src/Chip8.cpp)
target_link_libraries(chip-8-cpp wx::net wx::core wx::base)

# add_executable(chip-8-test test/Chip8Test.cpp test/TestChip8Observer.cpp test/TestChip8.cpp src/Chip8.cpp)
# target_link_libraries(chip-8-test PRIVATE Catch2::Catch2WithMain)
