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
# OPTIONS
# ==========================================================
option(BUILD_LEXER "Build the lexer driver" ON)
option(BUILD_PARSER "Build the parser driver" ON)
option(BUILD_EVALUATOR "Build the evaluator driver" ON)
option(BUILD_COMPILER "Build the full compiler driver (LLVM JIT)" ON)
option(BUILD_TESTS "Build the test suite" ON)

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
# AST LIBRARY
# ==========================================================
add_library(Ast
    ast/AST.cpp
)
target_include_directories(Ast PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/ast
)
target_link_libraries(Ast PUBLIC Lexer)

# ==========================================================
# PARSER LIBRARY
# ==========================================================
add_library(Parser
    parser/Parser.cpp
)
target_include_directories(Parser PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/parser
)
target_link_libraries(Parser PUBLIC Lexer Ast)

# ==========================================================
# SEMANTIC ANALYSIS LIBRARY
# ==========================================================
add_library(Semantics
    semantics/SemanticAnalyzer.cpp
)
target_include_directories(Semantics PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/semantics
)
target_link_libraries(Semantics PUBLIC Ast)

# ==========================================================
# EVALUATOR LIBRARY (tree-walking, no LLVM)
# ==========================================================
add_library(Evaluator
    evaluator/Evaluator.cpp
)
target_include_directories(Evaluator PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/evaluator
)
target_link_libraries(Evaluator PUBLIC Ast)

# ==========================================================
# CODEGEN LIBRARY (LLVM IR)
# ==========================================================
add_library(CodeGen
    codegen/CodeGen.cpp
)
target_include_directories(CodeGen PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/codegen
)
target_link_libraries(CodeGen PUBLIC Parser)

# ==========================================================
# DRIVER: LEXER
# ==========================================================
if(BUILD_LEXER)
    add_executable(lexer_driver
        drivers/lexer/main.cpp
    )
    target_link_libraries(lexer_driver PRIVATE Lexer)
    message(STATUS "Lexer driver will be built.")
endif()

# ==========================================================
# DRIVER: PARSER
# ==========================================================
if(BUILD_PARSER)
    add_executable(parser_driver
        drivers/parser/main.cpp
    )
    target_link_libraries(parser_driver PRIVATE Parser)
    message(STATUS "Parser driver will be built.")
endif()

# ==========================================================
# DRIVER: EVALUATOR
# ==========================================================
if(BUILD_EVALUATOR)
    add_executable(evaluator_driver
        drivers/evaluator/main.cpp
    )
    target_link_libraries(evaluator_driver PRIVATE Parser Semantics Evaluator)
    message(STATUS "Evaluator driver will be built.")
endif()

# ==========================================================
# DRIVER: FULL COMPILER (LLVM JIT)
# ==========================================================
if(BUILD_COMPILER)
    add_executable(compiler_driver
        drivers/compiler/main.cpp
    )
    target_link_libraries(compiler_driver PRIVATE
        CodeGen
        Parser
        Semantics
        Ast
        Lexer
        LLVM
    )
    message(STATUS "Full compiler driver will be built.")
endif()

# ==========================================================
# TESTS
# ==========================================================
if(BUILD_TESTS)
    enable_testing()

    # Front-end unit tests (lexer, parser, semantics, evaluator).
    add_executable(calc_tests
        tests/unit_tests.cpp
    )
    target_link_libraries(calc_tests PRIVATE Parser Semantics Evaluator)
    add_test(NAME unit_tests COMMAND calc_tests)

    # End-to-end tests for the JIT compiler driver.
    if(BUILD_COMPILER)
        add_test(NAME e2e_tests
            COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/tests/e2e.sh
                    ${CMAKE_BINARY_DIR}/compiler_driver)
    endif()
endif()
