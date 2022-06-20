cmake_minimum_required(VERSION 3.23)
project(chip-8-cpp)
# include(CTest)
# enable_testing()

add_subdirectory(lib/wx)
add_executable(chip-8-cpp WIN32 src/Main.cpp src/Chip8.cpp)
# add_executable(chip-8-test test/Chip8Test.cpp)
target_link_libraries(chip-8-cpp wx::net wx::core wx::base)	