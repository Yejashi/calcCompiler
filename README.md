# calcCompiler
A small educational compiler for a minimal arithmetic expression language called **calc**.

---

## Overview

The **calc** language is a simple expression-based language that supports:
- Declaring variables with the keyword `with`
- Using arithmetic operations (`+`, `-`, `*`, `/`)
- Grouping expressions with parentheses
- Evaluating user-provided expressions at runtime

The goal of this project is to **build a complete compilation pipeline** — from lexical analysis (tokenization) to parsing, semantic analysis, and eventually code generation.

Each stage is implemented as a standalone module with its own driver program inside `drivers/`.

---

## Language Definition

### Syntax
`with <var1>, <var2>, ... : <expression>`
- Variables must be declared after the `with` keyword and separated by commas.
- The colon `:` separates the declaration list from the expression.
- `<expression>` may include:
  - numeric literals (e.g., `42`, `3.14`)
  - variable names (declared after `with`)
  - arithmetic operators: `+`, `-`, `*`, `/`
  - parentheses for grouping

### Grammar (EBNF-style)
```
calc ::= ("with" ident ("," ident)* ":")? expr ;
expr ::= term (("+" | "-") term)* ;
term ::= factor (("" | "/") factor) ;
factor ::= ident | number | "(" expr ")" ;
ident ::= [a-zA-Z]+ ;
number ::= [0-9]+ ;
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
CalcCompiler/
├── lexer/
│ ├── lexer.cpp
│ └── lexer.h
├── drivers/
│ └── lexer/
│ └── main.cpp
├── CMakeLists.txt
└── README.md
```

### Future Modules
| Stage | Directory | Description |
|:------|:-----------|:------------|
| Lexer | `lexer/` | Converts input text into tokens |
| Parser | `parser/` | Converts tokens into an Abstract Syntax Tree (AST) |
| Semantic Analyzer | `semantics/` | Validates variable usage and typing |
| Codegen | `codegen/` | Generates executable code or LLVM IR |
| Runtime | `runtime/` | Provides user input and evaluation logic |

## Build Instructions
### Requirements
- LLVM (≥ 15.0)
- CMake (≥ 3.16)
- A C++17 compiler (GCC, Clang, etc.)

### Build
```bash
mkdir build && cd build
cmake .. -DBUILD_LEXER=ON
make
```

### Run
`./lexer_driver`
Example output:
```
KW_with [with]
ident [a]
comma [,]
ident [b]
colon [:]
ident [a]
star [*]
l_paren [(]
number [4]
plus [+]
ident [b]
r_paren [)]
eoi
```
## Goals
- [x] Implement lexer and token definitions
- [ ] Add recursive-descent parser
- [ ] Implement AST and evaluator
- [ ] Add error handling and diagnostics
- [ ] Integrate LLVM IR code generation
- [ ] Build end-to-end compiler driver



