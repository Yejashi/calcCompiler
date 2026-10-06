# calcCompiler
A small educational compiler for a minimal arithmetic expression language called **calc**.

---

## Overview

The **calc** language is a simple expression-based language that supports:
- Declaring variables with the keyword `with`
- Using arithmetic operations (`+`, `-`, `*`, `/`)
- Unary minus
- Grouping expressions with parentheses
- Integer and floating-point literals
- Evaluating user-provided expressions at runtime

The project implements a **complete compilation pipeline** — from lexical
analysis (tokenization) through parsing, semantic analysis, and LLVM IR code
generation down to an ORC JIT-compiled executable. A tree-walking evaluator
provides a second, JIT-free way to run programs.

Each stage is implemented as a standalone module with its own driver program
inside `drivers/`.

---

## Language Definition

### Syntax
`with <var1>, <var2>, ... : <expression>`
- Variables must be declared after the `with` keyword and separated by commas.
  At least one variable is required, and names must not repeat.
- The colon `:` separates the declaration list from the expression.
- Every variable used in the expression must be declared.
- `<expression>` may include:
  - numeric literals (e.g., `42`, `3.14`)
  - variable names (declared after `with`)
  - arithmetic operators: `+`, `-`, `*`, `/`
  - unary minus (e.g., `-5`, `2 * -3`)
  - parentheses for grouping

### Grammar (EBNF-style)
```
calc   ::= ("with" ident ("," ident)* ":")? expr ;
expr   ::= term (("+" | "-") term)* ;
term   ::= factor (("*" | "/") factor)* ;
factor ::= "-" factor | ident | number | "(" expr ")" ;
ident  ::= [a-zA-Z]+ ;
number ::= [0-9]+ ("." [0-9]+)? ;
```

## Example Programs
### Example 1: Simple Arithmetic
`with a, b: a * (4 + b)`

**Run behavior:**
```
Enter value for a: 3
Enter value for b: 2
Result = 18
```
### Example 2: Kinetic Energy
`with m, v: 0.5 * m * v * v`

**Run behavior:**
```
Enter value for m: 80
Enter value for v: 5
Result = 1000
```

## Project Structure
```
calcCompiler/
├── lexer/
│   ├── lexer.cpp        # Tokenizer (with line/column tracking)
│   └── lexer.h          # Token, SourcePosition, CalcError
├── ast/
│   ├── AST.cpp          # AST rendering (toString)
│   └── AST.h            # Number/Variable/Binary/Unary expression nodes
├── parser/
│   ├── Parser.cpp       # Recursive-descent parser
│   └── Parser.h
├── semantics/
│   ├── SemanticAnalyzer.cpp  # Undeclared-variable checking
│   └── SemanticAnalyzer.h
├── evaluator/
│   ├── Evaluator.cpp    # Tree-walking evaluator (no LLVM)
│   └── Evaluator.h
├── codegen/
│   ├── CodeGen.cpp      # LLVM IR generation
│   └── CodeGen.h
├── drivers/
│   ├── lexer/main.cpp       # lexer_driver
│   ├── parser/main.cpp      # parser_driver
│   ├── evaluator/main.cpp   # evaluator_driver
│   └── compiler/main.cpp    # compiler_driver (LLVM ORC JIT)
├── tests/
│   ├── unit_tests.cpp   # Front-end unit tests
│   └── e2e.sh           # End-to-end tests for compiler_driver
├── CMakeLists.txt
├── shell.nix            # Nix development shell (cmake + LLVM + clang)
└── README.md
```

### Pipeline
| Stage | Directory | Description |
|:------|:----------|:------------|
| Lexer | `lexer/` | Converts input text into tokens (with positions) |
| Parser | `parser/` | Converts tokens into an Abstract Syntax Tree (AST) |
| Semantic Analyzer | `semantics/` | Validates variable usage |
| Evaluator | `evaluator/` | Tree-walking evaluation (JIT-free) |
| Codegen | `codegen/` | Generates LLVM IR |
| Runtime | `drivers/compiler/` | ORC JIT, user input, and evaluation logic |

## Build Instructions
### Requirements
- LLVM (15 – 21)
- CMake (≥ 3.16)
- A C++17 compiler (GCC, Clang, etc.)

### Build
All drivers and tests are built by default:
```bash
cmake -S . -B build
cmake --build build -j
```
On NixOS, `nix develop` (via `shell.nix`) provides cmake, LLVM, and clang and
sets `LLVM_DIR`/`CXX` automatically. Otherwise point CMake at your LLVM:
`cmake -S . -B build -DLLVM_DIR=/path/to/lib/cmake/llvm`.

Individual components can be disabled with `-DBUILD_LEXER=OFF`,
`-DBUILD_PARSER=OFF`, `-DBUILD_EVALUATOR=OFF`, `-DBUILD_COMPILER=OFF`,
`-DBUILD_TESTS=OFF`.

### Run
```bash
./build/compiler_driver          # interactive JIT compiler (REPL)
./build/compiler_driver -q       # quiet mode: results/errors only
./build/compiler_driver --ir     # also print generated LLVM IR
./build/lexer_driver [file]      # print the token stream
./build/parser_driver [file]     # print the parsed AST
./build/evaluator_driver [file]  # run with the tree-walking evaluator
```
Without a file argument, each driver reads from stdin (or uses a built-in
example).

Example session:
```
$ ./build/compiler_driver
calc> with a, b: a * (4 + b)
Enter value for a: 3
Enter value for b: 2
Result = 18
calc> 1 + 2 * 3
Result = 7
calc> with a: b + 1
error: undeclared variable 'b' (line 1, column 9)
calc>
```

### Tests
```bash
ctest --test-dir build --output-on-failure
```
Runs the front-end unit tests (`unit_tests`) and the end-to-end suite for
`compiler_driver` (`e2e_tests`).

## Diagnostics
All front-end errors are reported as
`error: <message> (line L, column C)` and never crash the compiler:
- unexpected characters and malformed number literals (lexer)
- syntax errors, including malformed `with` lists and duplicate declarations (parser)
- use of undeclared variables (semantic analyzer)

## Goals
- [x] Implement lexer and token definitions
- [x] Add recursive-descent parser
- [x] Implement AST and evaluator
- [x] Add error handling and diagnostics (position-based)
- [x] Add semantic analysis (undeclared variables)
- [x] Integrate LLVM IR code generation
- [x] Build end-to-end compiler driver (ORC JIT)
- [x] Add unit and end-to-end tests
