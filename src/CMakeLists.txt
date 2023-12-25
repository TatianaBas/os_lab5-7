set (CMAKE_CXX_STANDARD 20)
cmake_minimum_required(VERSION 3.27)
project(lab57)


add_compile_options(-pthread)

add_executable(client node.cpp)
add_executable(server main.cpp)

target_link_libraries(client zmq)
target_link_libraries(server zmq)

