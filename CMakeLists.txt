cmake_minimum_required(VERSION 3.23)

project(wx-example)

add_subdirectory(lib/wx)
add_executable(wx-example Main.cpp)
target_link_libraries(wx-example wx::net wx::core wx::base)	