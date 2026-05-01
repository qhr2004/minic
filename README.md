# MiniC Compiler

A C++17 MiniC compiler project built with CMake. The current pipeline includes lexer, parser, AST construction, semantic analysis, IR generation, a simple code generation stage, and a Maple IR lowering module that can hand off `.mpl` output to an external Ark backend command.

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

Emit Maple IR from MiniC source:

```sh
./build/minic --emit-maple tests/if_else.mc build/if_else.mpl
```

Emit ABC through a configured Ark backend command:

```sh
export MINIC_ARK_BACKEND_CMD='your-ark-backend --input {input} --output {output}'
./build/minic --emit-abc tests/if_else.mc build/if_else.abc
```

`MINIC_ARK_BACKEND_CMD` is a shell template. The compiler substitutes `{input}` with the generated `.mpl` path and `{output}` with the requested `.abc` path.

## Project Structure

```text
src/
├── lexer/     # Token definitions and lexical analysis
├── parser/    # Syntax analysis and AST construction
├── ast/       # Shared AST node hierarchy
├── semantic/  # Symbol table and semantic checks
├── ir/        # IR container and AST-to-IR lowering
├── maple/     # AST/IR-to-Maple lowering and Ark backend wrapper
├── codegen/   # IR-to-target translation
└── main.cpp   # End-to-end compiler driver
```

## Pipeline Overview

```text
MiniC Source
  ↓
Lexer
  ↓
Parser
  ↓
AST
  ↓
Semantic Analyzer
  ↓
TAC IR
  ↓
Maple IR (.mpl)
  ↓
Ark Backend Handoff
  ↓
ABC Placeholder / Future Real ABC
```

The current ABC stage depends on `MINIC_ARK_BACKEND_CMD`. The compiler first emits Maple IR, then substitutes `{input}` with the generated `.mpl` path and `{output}` with the requested `.abc` path before invoking the external command template.

## MiniC Feature Support

| Status | Feature area | Current support |
| --- | --- | --- |
| Supported | Core syntax | `int` / `float` / `char` local variables and literals, blocks, `if-else`, `while`, `for`, `return`, function definitions, function calls, prefix/postfix `++` / `--`, and compound assignments |
| Supported | Semantic checks | Nested scopes, duplicate declaration / duplicate parameter detection, undeclared identifier detection, exact type checking, and `int` conditions for `if` / `while` / `for` |
| Supported | Output stages | AST dump, string-based TAC IR, pseudo target dump, AST-to-Maple IR lowering, and external ABC handoff through `--emit-abc` |
| Partial | Type-aware IR | AST-to-Maple lowering preserves declared types, but the current TAC IR container is string-based and mostly lowered as `i32` |
| Partial | Backend integration | `--emit-abc` can invoke an external Ark backend command, but the real toolchain command line is still configured out-of-band via `MINIC_ARK_BACKEND_CMD` and the current `.abc` result may only be a handoff placeholder |
| Partial | Regression coverage | Control flow, function calls, and increment expressions are covered by `ctest`; float / char paths and several semantic negative cases are currently documented and runnable, but not all are wired into `ctest` |
| Not yet | Richer MiniC data model | Arrays, pointers, strings, structs, global storage, and function prototypes are not implemented |
| Not yet | Additional statements | `break`, `continue`, `switch`, and `do-while` are not implemented |
| Not yet | Runtime / library | Standard library functions, built-in I/O, and a fixed runtime environment are not implemented |

See [docs/implementation_report.md](docs/implementation_report.md) and [docs/testing_report.md](docs/testing_report.md) for the graduation-project oriented implementation and testing summaries.

## Maple IR / Ark Backend

`src/maple/IRGenerator.h` and `src/maple/IRGenerator.cpp` implement a dedicated Maple lowering layer. The code is split by AST node kind so future nodes such as `ForStmt`, function calls, or richer IR instructions can be added without reworking the whole backend.

Current mapping rules:

- MiniC types map to Maple primitive types as `int -> i32`, `float -> f32`, and `char -> i8`.
- `VarDecl` becomes `var %name <type>` plus `dassign` when an initializer exists.
- `Identifier` becomes `dread`, assignments become `dassign`, and arithmetic/comparison expressions become Maple opcodes such as `add`, `sub`, `mul`, `gt`, and `eq`.
- `IfStmt` and `WhileStmt` are lowered into explicit labeled basic blocks with `brfalse` and `goto`.
- `ForStmt` is lowered into explicit `for_init`, `for_cond`, `for_body`, `for_update`, and `for_end` basic blocks.
- `i++` / `i--` and `++i` / `--i` preserve expression semantics by materializing internal temporaries for the old and new values.
- AST input preserves declared types. The current string-based MiniC TAC can also be lowered, but it is treated as mostly `i32` because the existing IR container does not retain full type metadata.

C++ API examples:

```cpp
#include "maple/IRGenerator.h"

maple::IRGenerator generator;
generator.generate(*program);
generator.writeToFile("build/demo.mpl");

std::string abcPath = maple::generate_abc(*program, "build/demo.abc");
```

Lowering from the current MiniC IR container:

```cpp
IRModule ir;
ir.addInstruction("var i");
ir.addInstruction("i = 3");
ir.addInstruction("%t0 = i > 1");
ir.addInstruction("return %t0");

maple::IRGenerator generator;
generator.generate(ir, "main");
generator.writeToFile("build/from_ir.mpl");
```

Example inputs and output are checked into [examples/maple_demo.mc](examples/maple_demo.mc), [examples/maple_demo.ir.txt](examples/maple_demo.ir.txt), [examples/maple_demo.mpl](examples/maple_demo.mpl), [examples/for_call_demo.mc](examples/for_call_demo.mc), [examples/for_call_demo.mpl](examples/for_call_demo.mpl), [tests/nested_increment_call.mc](tests/nested_increment_call.mc), and [examples/nested_increment_call.mpl](examples/nested_increment_call.mpl).

Generated Maple IR excerpt:

```text
func &main () i32 {
  var %sum i32
  var %i i32
  var %__inc_old_0 i32
  var %__inc_new_1 i32
  var %x i32

  # BB entry
  @entry
  dassign %sum (constval i32 0)
  # BB for_init
  dassign %i (constval i32 0)
  ...
  call &add (dread i32 %sum, dread i32 %i)
  # BB for_update
  dassign %__inc_old_0 (dread i32 %i)
  dassign %__inc_new_1 (add i32 (dread i32 %i, constval i32 1))
  dassign %i (dread i32 %__inc_new_1)
  ...
  callassigned &add (dread i32 %sum, constval i32 42) { dassign %x 0 }
}
```

Nested increment/call excerpt:

```text
@for_body_1
dassign %sum (add i32 (dread i32 %sum, dread i32 %i))
call &add (dread i32 %sum, dread i32 %i)
...
callassigned &add (dread i32 %sum, constval i32 42) { dassign %x 0 }
dassign %__inc_old_4 (dread i32 %i)
dassign %__inc_new_5 (add i32 (dread i32 %i, constval i32 1))
dassign %i (dread i32 %__inc_new_5)
callassigned &add (dread i32 %x, constval i32 2) { dassign %__callret_6 0 }
dassign %y (add i32 (dread i32 %__inc_old_4, dread i32 %__callret_6))
callassigned &add (dread i32 %y, dread i32 %__inc_old_7) { dassign %z 0 }
```

Run the Maple regression tests:

```sh
ctest --test-dir build --output-on-failure
```

Validate Maple IR and ABC generation for the nested regression sample:

```sh
bash tests/assert_maple_contains.sh ./build/minic tests/nested_increment_call.mc build/nested_increment_call.mpl "callassigned &add (dread i32 %sum, constval i32 42) { dassign %x 0 }"
bash tests/assert_maple_contains.sh --abc ./build/minic tests/nested_increment_call.mc build/nested_increment_call.abc "callassigned &add (dread i32 %y, dread i32 %__inc_old_"
```

Direct ABC example:

```sh
export MINIC_ARK_BACKEND_CMD='your-ark-backend --input {input} --output {output}'
./build/minic --emit-abc tests/nested_increment_call.mc build/nested_increment_call.abc
```

## Current Limitations

- TAC IR is weakly typed. AST-to-Maple lowering preserves declared types, but TAC-to-Maple lowering mostly falls back to `i32`.
- The MiniC subset does not include a standard library or runtime support such as built-in I/O.
- The real Ark backend command is not fixed in the repository and still depends on `MINIC_ARK_BACKEND_CMD`.
- The current `.abc` output should be treated as a handoff verification artifact unless a real external Ark backend is configured. See [docs/ark_backend_investigation.md](docs/ark_backend_investigation.md).

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
| `tests/for_loop.mc` | `for` loop lowering with `i++` and `+=` desugaring | Pass |
| `tests/function_call.mc` | Function call parsing, semantic checking, and IR lowering | Pass |
| `tests/increment_expression.mc` | Prefix/postfix `++/--` as expressions and nested function calls | Pass |
| `tests/nested_control_flow_call.mc` | Nested `for`/`if`/`while` control flow with function calls | Pass |
| `tests/nested_increment_call.mc` | Nested `for` loops with `i++` expression values and call-result composition | Pass |
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
