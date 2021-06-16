cmake_minimum_required(VERSION 3.20)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(zkz VERSION 0.1)
include_directories(include libs/libelfin)
file(GLOB_RECURSE SRC_FILES src/*.cpp)
add_executable(${PROJECT_NAME} Debugger.cpp ${SRC_FILES})

add_custom_target(libelfin COMMAND make WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/libs/libelfin)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/libs/libelfin/elf/libelf++.so ${CMAKE_CURRENT_SOURCE_DIR}/libs/libelfin/dwarf/libdwarf++.so)
add_dependencies(${PROJECT_NAME} libelfin)
