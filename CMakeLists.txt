
cmake_minimum_required(VERSION 3.20)

project(riverbed LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_library(riverbed_lib src/river.cpp)
target_include_directories(riverbed_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

add_executable(riverbed_bench examples/main.cpp)
target_link_libraries(riverbed_bench PRIVATE riverbed_lib)

target_compile_options(riverbed_lib PRIVATE -O3 -Wall -Wextra -Wpedantic)
target_compile_options(riverbed_bench PRIVATE -O3 -Wall -Wextra -Wpedantic)

option(RIVER_ASAN "Enable AddressSanitizer" OFF)
option(RIVER_TSAN "Enable ThreadSanitizer" OFF)

if (RIVER_ASAN)
  target_compile_options(riverbed_lib PRIVATE -fsanitize=address -fno-omit-frame-pointer)
  target_link_options(riverbed_lib PRIVATE -fsanitize=address)
  target_compile_options(riverbed_bench PRIVATE -fsanitize=address -fno-omit-frame-pointer)
  target_link_options(riverbed_bench PRIVATE -fsanitize=address)
endif()

if (RIVER_TSAN)
  target_compile_options(lib PRIVATE -fsanitize=thread -fno-omit-frame-pointer)
  target_link_options(lib PRIVATE -fsanitize=thread)
  target_compile_options(bench PRIVATE -fsanitize=thread -fno-omit-frame-pointer)
  target_link_options(bench PRIVATE -fsanitize=thread)
endif()
