cmake_minimum_required(VERSION 3.23)
project(chip-8-cpp)

add_subdirectory(lib/wx)
add_subdirectory(lib/sfml)
set(wxBUILD_SHARED ON)
set(wxUSE_ACCESSIBILITY OFF)

add_executable(chip-8-cpp WIN32 src/Main.cpp src/Chip8.cpp)
target_link_libraries(chip-8-cpp wx::net wx::core wx::base sfml-audio)
configure_file(440T.wav ${CMAKE_CURRENT_BINARY_DIR}/440T.wav COPYONLY)
