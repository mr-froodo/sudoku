cmake_minimum_required(VERSION 3.16)

project(sudoku LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Solver mechanics
add_library(sudoku
  src/grid.c
  src/solver.c
)
target_include_directories(sudoku PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(sudoku PRIVATE -Wall -Wextra -pedantic)
endif()

# Brute-force solver test executable
add_executable(bruteforce_solver test/bruteforce_solver.c)
target_link_libraries(bruteforce_solver PRIVATE sudoku)
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
  target_compile_options(bruteforce_solver PRIVATE -Wall -Wextra -pedantic)
endif()

include(CTest)
if(BUILD_TESTING)
  add_test(NAME solve_default COMMAND bruteforce_solver)
  add_test(NAME solve_classic COMMAND bruteforce_solver
    530070000600195000098000060800060003400803001700020006060000280000419005000080079)
  set_tests_properties(solve_default solve_classic PROPERTIES
    PASS_REGULAR_EXPRESSION "Success!")

  add_test(NAME reject_invalid_input COMMAND bruteforce_solver 12)
  set_tests_properties(reject_invalid_input PROPERTIES WILL_FAIL TRUE)
endif()
