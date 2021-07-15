cmake_minimum_required(VERSION 3.20)
enable_language(CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
set(THREADS_PREFER_PTHREAD_FLAG TRUE)

project(zkz VERSION 0.1)
find_package(Threads REQUIRED)
include_directories(include libs/libelfin)
file(GLOB_RECURSE SRC_FILES ./src/*.cpp)
add_executable(${PROJECT_NAME} ${SRC_FILES})

add_custom_target(libelfin COMMAND make WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/libs/libelfin)
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME} ${CMAKE_CURRENT_SOURCE_DIR}/libs/libelfin/elf/libelf++.so ${CMAKE_CURRENT_SOURCE_DIR}/libs/libelfin/dwarf/libdwarf++.so Threads::Threads)
add_dependencies(${PROJECT_NAME} libelfin)
