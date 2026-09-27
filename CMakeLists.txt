cmake_minimum_required(VERSION 3.16)

project(sudoku LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Code style: C99, no declarations after statements
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  add_compile_options(-Wall -Wextra -pedantic -Werror=declaration-after-statement)
endif()

# Tune for the build machine (enables e.g. the popcnt instruction)
option(SUDOKU_NATIVE "Optimise for the CPU of the build machine" ON)
if(SUDOKU_NATIVE)
  include(CheckCCompilerFlag)
  check_c_compiler_flag(-march=native SUDOKU_HAVE_MARCH_NATIVE)
  if(SUDOKU_HAVE_MARCH_NATIVE)
    add_compile_options(-march=native)
  endif()
endif()

# Solver mechanics
add_library(sudoku
  src/grid.c
  src/solver.c
)
target_include_directories(sudoku PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Brute-force solver test executable
add_executable(bruteforce_solver test/bruteforce_solver.c)
target_link_libraries(bruteforce_solver PRIVATE sudoku)

# Benchmark over a file of puzzles
add_executable(benchmark bench/benchmark.c)
target_link_libraries(benchmark PRIVATE sudoku)

include(CTest)
if(BUILD_TESTING)
  add_test(NAME solve_default COMMAND bruteforce_solver)
  add_test(NAME solve_classic COMMAND bruteforce_solver
    530070000600195000098000060800060003400803001700020006060000280000419005000080079)
  set_tests_properties(solve_default solve_classic PROPERTIES
    PASS_REGULAR_EXPRESSION "Success!")

  # Givens are consistent, but the top right cell can hold no digit
  add_test(NAME unsolvable COMMAND bruteforce_solver
    123456780000000009000000000000000000000000000000000000000000000000000000000000000)
  # Givens break the rules: two 1s in the first row
  add_test(NAME contradictory COMMAND bruteforce_solver
    110000000000000000000000000000000000000000000000000000000000000000000000000000000)
  set_tests_properties(unsolvable contradictory PROPERTIES
    PASS_REGULAR_EXPRESSION "no solution")

  add_test(NAME reject_invalid_input COMMAND bruteforce_solver 12)
  set_tests_properties(reject_invalid_input PROPERTIES WILL_FAIL TRUE)

  # Solves and verifies every benchmark puzzle
  add_test(NAME benchmark_puzzles
    COMMAND benchmark ${CMAKE_CURRENT_SOURCE_DIR}/bench/hard.txt)
endif()
