# MiniC 测试报告

本报告记录当前仓库可直接演示的构建、回归与 Maple/ABC 输出命令，并说明每个测试文件覆盖的语法点。对于毕业设计展示，建议优先演示 `ctest`、`--emit-maple` 与 `--emit-abc` 三类命令。

## 1. 构建命令

首次构建：

```sh
cmake -S . -B build
cmake --build build
```

若已存在旧的 `build/` 且缓存路径不一致，可先删除后重建：

```sh
rm -rf build
cmake -S . -B build
cmake --build build
```

## 2. `ctest` 回归测试命令

```sh
ctest --test-dir build --output-on-failure
```

当前 `ctest` 主要覆盖 Maple lowering 与 ABC handoff 的关键模式匹配，而不是所有 `.mc` 样例。其底层由 [tests/assert_maple_contains.sh](../tests/assert_maple_contains.sh) 驱动。

## 3. `--emit-maple` 测试命令

推荐命令：

```sh
./build/minic --emit-maple tests/nested_increment_call.mc build/nested_increment_call.mpl
sed -n '1,160p' build/nested_increment_call.mpl
```

适合答辩展示的点：

- 函数签名与局部变量声明
- `for` / `while` / `if` 降低后的基本块标签
- `call` 与 `callassigned`
- 前后缀 `++` 展开出的内部临时变量

如果想看较简单的函数调用 lowering，可使用：

```sh
./build/minic --emit-maple tests/function_call.mc build/function_call.mpl
```

## 4. `--emit-abc` 测试命令

真实 Ark 后端命令尚未固化，因此当前有两种用法。

### 4.1 本地回归占位命令

```sh
MINIC_ARK_BACKEND_CMD='cat {input} > {output}' \
./build/minic --emit-abc tests/nested_increment_call.mc build/nested_increment_call.abc
```

说明：

- 这条命令只是把生成的 `.mpl` 拷贝成 `.abc`，用于验证 handoff 逻辑是否通畅。
- 该模式适合本地回归和毕业设计演示，不代表真实 Ark 字节码产物。

### 4.2 真实 Ark 后端占位接口

```sh
export MINIC_ARK_BACKEND_CMD='your-ark-backend --input {input} --output {output}'
./build/minic --emit-abc tests/nested_increment_call.mc build/nested_increment_call.abc
```

要求：

- 命令模板必须同时包含 `{input}` 和 `{output}`
- 编译器会先生成同名 `.mpl` 文件，再调用外部后端

## 5. ABC 生成验证说明

当前 `--emit-abc` 的工作方式并不是在仓库内直接实现真实 Ark 后端，而是：

1. 先把 MiniC AST 降低为 Maple IR
2. 把 Maple IR 写到与目标 `.abc` 同名的 `.mpl` 文件
3. 读取环境变量 `MINIC_ARK_BACKEND_CMD`
4. 用命令模板中的 `{input}` 和 `{output}` 替换实际路径
5. 调用外部命令，生成目标 `.abc`

`MINIC_ARK_BACKEND_CMD` 的模板格式要求如下：

- 必须是一个 shell 命令模板
- 必须同时包含 `{input}` 和 `{output}`
- `{input}` 表示编译器刚生成的 Maple IR 文件路径
- `{output}` 表示用户请求生成的 `.abc` 目标路径

当前最实用的冒烟测试方式是：

```sh
MINIC_ARK_BACKEND_CMD='cat {input} > {output}' \
./build/minic --emit-abc tests/nested_increment_call.mc build/nested_increment_call.abc
```

这个命令不会生成真实 Ark 字节码，它只是把 `.mpl` 文本复制为 `.abc`，用于验证：

- `--emit-abc` 路径能否走通
- `.mpl` 是否正常生成
- handoff 命令模板和输出路径替换逻辑是否正确

根据 [docs/ark_backend_investigation.md](ark_backend_investigation.md) 的调查结论，当前本地未找到以下工具：

- `ark_asm`
- `ark_disasm`
- `es2abc`
- `es2panda`
- `maple`
- `mpl2mpl`
- `mplcg`
- `jbc2mpl`

因此可以明确说明：

- 当前本地没有可确认的官方 `.mpl -> .abc` 工具链
- 当前生成出来的 `.abc` 文件不能视为可执行 Ark 字节码
- 它们应被视为 handoff 占位产物或命令链路验证产物

从项目完成度角度，当前仓库已经完成了 `MiniC -> Maple IR (.mpl)` 的主链路；但真实官方 `mpl -> abc` 后端尚未固定接入，`MINIC_ARK_BACKEND_CMD` 仍然只是一个可配置的外部交接点。

## 6. 当前自动化测试项

| `ctest` 名称 | 源文件 | 覆盖重点 |
| --- | --- | --- |
| `maple_increment_expression` | `tests/increment_expression.mc` | `++/--` 表达式求值、嵌套函数调用、`callassigned` |
| `maple_nested_control_flow` | `tests/nested_control_flow_call.mc` | `for` / `if` / `while` 嵌套控制流，调用结果参与表达式 |
| `maple_nested_increment_call` | `tests/nested_increment_call.mc` | 双层 `for`、调用副作用、`i++` 旧值保留、`--emit-abc` handoff |

## 7. 每个测试文件覆盖的语法点

| 测试文件 | 预期结果 | 覆盖的语法点 / 语义点 |
| --- | --- | --- |
| `tests/basic.mc` | Pass | 基本局部变量声明、整型字面量、四则运算优先级、`return` |
| `tests/if_else.mc` | Pass | `if-else`、块语句、赋值、加减表达式 |
| `tests/while.mc` | Pass | `while` 循环、循环内赋值、条件表达式 |
| `tests/for_loop.mc` | Pass | `for` 初始化/条件/更新、`int i = 0`、`i++`、`+=` 复合赋值 |
| `tests/function_call.mc` | Pass | 函数定义、参数列表、函数调用、返回调用结果 |
| `tests/increment_expression.mc` | Pass | 前缀/后缀 `++` / `--`、表达式值语义、嵌套函数调用 |
| `tests/nested_control_flow_call.mc` | Pass | `for` + `if` + `while` 嵌套、块内局部变量、函数调用语句、调用结果参与 `+=` |
| `tests/nested_increment_call.mc` | Pass | 双层 `for`、`i++` 旧值保留、函数返回值组合、复杂 Maple lowering、ABC handoff |
| `tests/undefined_var.mc` | Semantic error | 初始化表达式中使用未声明变量 |
| `tests/redeclare.mc` | Semantic error | 同一作用域重复声明变量 |
| `tests/type_error.mc` | Semantic error | `int` 函数中使用 `return;`，返回类型不匹配 |
| `tests/semantic_duplicate_param.mc` | Semantic error | 形参重名 |
| `tests/semantic_scope_ok.mc` | Pass | 参数引用、嵌套块作用域、变量遮蔽、`float` 局部变量 |
| `tests/semantic_type_mismatch.mc` | Semantic error | 禁止 `float` 到 `int` 的隐式赋值 |
| `tests/semantic_undeclared.mc` | Semantic error | `return` 表达式中使用未声明标识符 |
| `tests/test.mc` | Pass | 注释跳过、关键字组合冒烟、`if`、`while`、表达式、块语句综合样例 |

## 8. 测试驱动脚本说明

| 文件 | 作用 |
| --- | --- |
| `tests/assert_maple_contains.sh` | 统一驱动 `--emit-maple` 或 `--emit-abc` 命令，并检查目标 Maple 模式串是否出现 |

脚本中 `--abc` 模式有一个实用设计：

- 若 `MINIC_ARK_BACKEND_CMD` 未设置或不包含占位符，会回退到 `cat {input} > {output}`
- 这样可以在没有真实 Ark 工具链的环境中继续验证 `.mpl` 伴生文件与 handoff 流程

## 9. 建议的答辩演示顺序

1. `cmake -S . -B build && cmake --build build`
2. `ctest --test-dir build --output-on-failure`
3. `./build/minic tests/function_call.mc`
4. `./build/minic --emit-maple tests/nested_increment_call.mc build/nested_increment_call.mpl`
5. `MINIC_ARK_BACKEND_CMD='cat {input} > {output}' ./build/minic --emit-abc tests/nested_increment_call.mc build/nested_increment_call.abc`

这样可以依次展示：

- 前端 AST / IR / pseudo codegen
- 自动回归结果
- Maple IR lowering
- ABC handoff 机制
