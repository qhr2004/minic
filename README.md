# MiniC Compiler

A C++17 MiniC compiler project built with CMake. The current pipeline includes lexer, parser, AST, semantic analysis, three-address-code IR, and a simplified Maple-style IR.

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

For valid programs, the compiler prints three-address-code IR first, then simplified Maple IR. For invalid programs, it prints a semantic error and exits with a non-zero status.

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
