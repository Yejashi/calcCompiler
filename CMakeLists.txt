cmake_minimum_required(VERSION 3.16)
project(calcCompiler LANGUAGES C CXX)

# ==========================================================
# LLVM CONFIGURATION
# ==========================================================
find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})
link_directories(${LLVM_LIBRARY_DIRS})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ==========================================================
# OPTIONS FOR DRIVERS
# ==========================================================
option(BUILD_LEXER "Build the lexer driver" ON)
option(BUILD_PARSER "Build the parser driver" OFF)
option(BUILD_COMPILER "Build the full compiler driver" OFF)

# ==========================================================
# CORE LEXER LIBRARY
# ==========================================================
add_library(Lexer
    lexer/lexer.cpp
)
target_include_directories(Lexer PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/lexer
)
target_link_libraries(Lexer PUBLIC LLVM)

# ==========================================================
# DRIVER: LEXER
# ==========================================================
if(BUILD_LEXER)
    add_executable(lexer_driver
        drivers/lexer/main.cpp
    )
    target_link_libraries(lexer_driver PRIVATE Lexer)
    target_include_directories(lexer_driver PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer
    )
    message(STATUS "Lexer driver will be built.")
endif()

# ==========================================================
# DRIVER: PARSER (Future)
# ==========================================================
if(BUILD_PARSER)
    add_executable(parser_driver
        drivers/parser/main.cpp
    )
    target_link_libraries(parser_driver PRIVATE Lexer)
    target_include_directories(parser_driver PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer
        ${CMAKE_CURRENT_SOURCE_DIR}/parser
    )
    message(STATUS "Parser driver will be built.")
endif()

# ==========================================================
# DRIVER: FULL COMPILER (Future)
# ==========================================================
if(BUILD_COMPILER)
    add_executable(compiler_driver
        drivers/compiler/main.cpp
    )
    target_link_libraries(compiler_driver PRIVATE Lexer)
    target_include_directories(compiler_driver PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/lexer
        ${CMAKE_CURRENT_SOURCE_DIR}/parser
        ${CMAKE_CURRENT_SOURCE_DIR}/compiler
    )
    message(STATUS "Full compiler driver will be built.")
endif()
