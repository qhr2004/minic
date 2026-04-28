# MiniC Compiler

A C++17 MiniC compiler project built with CMake. The current pipeline includes lexer, parser, AST construction, semantic analysis, IR generation, and a simple code generation stage.

## Build

```sh
cmake -S . -B build
cmake --build build
```

## Run

Run one MiniC source file:

```sh
./build/minic tests/basic.mc
```

For valid programs, the compiler prints the AST, three-address-code IR, and pseudo target code. For invalid programs, it prints an error and exits with a non-zero status.

## Project Structure

```text
src/
├── lexer/     # Token definitions and lexical analysis
├── parser/    # Syntax analysis and AST construction
├── ast/       # Shared AST node hierarchy
├── semantic/  # Symbol table and semantic checks
├── ir/        # IR container and AST-to-IR lowering
├── codegen/   # IR-to-target translation
└── main.cpp   # End-to-end compiler driver
```

## VS Code Run And Debug

This project includes VS Code configuration under `.vscode/`.

Build from VS Code:

```text
Terminal -> Run Build Task -> MiniC: build
```

Run custom MiniC source files:

```text
Terminal -> Run Task -> MiniC: run prompt source
```

The task prompts for a `.mc` file path. The default is `tests/basic.mc`.

Debug custom MiniC source files:

```text
Run and Debug -> Debug MiniC: prompt source
```

This launches `build/minic` with the source file path as the program argument. A fixed `Debug MiniC: tests/basic.mc` configuration is also available.

## System Tests

| Test file | Purpose | Expected result |
| --- | --- | --- |
| `tests/basic.mc` | Variable declarations, arithmetic expressions, and `return` | Pass |
| `tests/if_else.mc` | `if-else` control-flow IR generation | Pass |
| `tests/while.mc` | `while` loop control-flow IR generation | Pass |
| `tests/undefined_var.mc` | Use of an undeclared variable | Semantic error |
| `tests/redeclare.mc` | Duplicate declaration in the same scope | Semantic error |
| `tests/type_error.mc` | Return type mismatch in an `int` function | Semantic error |

Run passing tests:

```sh
./build/minic tests/basic.mc
./build/minic tests/if_else.mc
./build/minic tests/while.mc
```

Run error tests:

```sh
./build/minic tests/undefined_var.mc
./build/minic tests/redeclare.mc
./build/minic tests/type_error.mc
```

Run all tests while keeping the shell going after expected failures:

```sh
for f in tests/*.mc; do
    echo "===== $f ====="
    ./build/minic "$f" || true
done
```
