# MiniC 实现报告

本报告基于当前仓库中的 `README`、源码与测试用例整理实现现状，并对照“MiniC 前端 + 中间表示 + Maple/Ark handoff”这一开题方向说明当前完成度与差距。仓库内未包含开题报告原文，因此下文不引用未见到的原始表述，而是按现有目标链路归纳。

## 1. 编译器总体架构

当前编译器主流程位于 [src/main.cpp](../src/main.cpp)，整体链路如下：

```text
MiniC source
  -> Lexer
  -> Parser / AST
  -> Semantic Analyzer
  -> two downstream paths
       A. AST -> TAC IR -> pseudo Codegen
       B. AST -> Maple IR -> external Ark backend command -> .abc
```

对应模块分工如下：

| 阶段 | 主要文件 | 作用 |
| --- | --- | --- |
| Driver | [src/main.cpp](../src/main.cpp) | 解析命令行模式，串联编译流程 |
| Lexer | [src/lexer/Lexer.h](../src/lexer/Lexer.h), [src/lexer/Lexer.cpp](../src/lexer/Lexer.cpp), [src/lexer/Token.h](../src/lexer/Token.h) | 词法分析与 token 定义 |
| Parser / AST | [src/parser/Parser.h](../src/parser/Parser.h), [src/parser/Parser.cpp](../src/parser/Parser.cpp), [src/ast/AST.h](../src/ast/AST.h) | 递归下降语法分析与 AST 构建 |
| Semantic | [src/semantic/Semantic.h](../src/semantic/Semantic.h), [src/semantic/Semantic.cpp](../src/semantic/Semantic.cpp), [src/semantic/SymbolTable.h](../src/semantic/SymbolTable.h) | 作用域管理、符号表、类型检查 |
| TAC IR | [src/ir/IRModule.h](../src/ir/IRModule.h), [src/ir/IRGenerator.cpp](../src/ir/IRGenerator.cpp) | 生成字符串形式的三地址码 |
| Pseudo Codegen | [src/codegen/CodeGenerator.cpp](../src/codegen/CodeGenerator.cpp) | 将 TAC IR 进一步打印为伪目标代码 |
| Maple IR / ABC | [src/maple/IRGenerator.h](../src/maple/IRGenerator.h), [src/maple/IRGenerator.cpp](../src/maple/IRGenerator.cpp) | 将 AST 或 TAC IR 降低为 Maple IR，并交给外部 Ark 命令 |

从毕业设计展示角度看，这个仓库已经具备“前端可运行、IR 可展示、Maple handoff 可演示”的基本框架，重点差距集中在真实后端接入、IR 强类型化和运行时支持上。

## 2. 各阶段实现说明

### 2.1 Lexer

Lexer 支持以下 token 类别：

- 关键字：`int`、`float`、`char`、`if`、`else`、`while`、`for`、`return`
- 标识符：字母或下划线开头，后续可包含数字
- 字面量：整数、浮点数、字符字面量
- 运算符：`+ - * / % = == != > >= < <= ! && || ++ -- += -= *= /= %=`
- 分隔符：`; , ( ) { }`
- 注释：`//` 单行注释与 `/* ... */` 多行注释

实现特点：

- 记录行列号，错误信息可定位到源文件位置。
- 多字符运算符优先匹配，避免 `++` 被拆成两个 `+`。
- 对未终止块注释和非法字符字面量会直接抛出异常。

### 2.2 Parser

Parser 采用递归下降实现，直接构建 AST。当前支持的结构包括：

- 顶层：函数定义，或少量顶层语句
- 语句：变量声明、表达式语句、`return`、`if-else`、`while`、`for`、块语句
- 表达式：赋值、复合赋值、比较、逻辑、四则运算、一元 `!` / `-`、前后缀 `++/--`、函数调用、括号表达式

实现特点：

- 通过 `parseAssignment -> parseEquality -> parseComparison -> parseExpr -> parseTerm -> parseUnary -> parsePostfix -> parsePrimary` 建立优先级。
- `for` 初始化部分同时支持变量声明和普通表达式。
- 前后缀 `++/--` 被单独建模为 `IncDecExpr`，便于语义分析和 Maple lowering 保持表达式值语义。

当前未支持：

- `void` 关键字
- 函数声明原型
- 数组下标、指针解引用、成员访问等更复杂语法

### 2.3 Semantic

语义分析器的核心职责是“先登记函数，再检查函数体和语句”。

当前已实现：

- 符号表作用域栈
- 函数签名预注册，支持调用出现在被调函数定义之前
- 局部变量、参数、函数名重复定义检查
- 未声明变量 / 函数检查
- `return` 类型检查
- 变量初始化与赋值的精确类型匹配
- `if` / `while` / `for` 条件必须为 `int`
- 逻辑运算符两侧必须为 `int`
- `%` 仅允许整型

当前策略偏保守：

- 不做隐式类型转换，`int y = x;` 其中 `x` 为 `float` 会报错。
- `return;` 只能出现在 `void` 返回类型函数中，但当前语法层面又没有 `void` 函数，因此它主要被当作错误样例覆盖。

### 2.4 TAC IR

TAC IR 由 [src/ir/IRModule.h](../src/ir/IRModule.h) 中的 `std::vector<std::string>` 承载，本质上是字符串三地址码容器，典型指令形态包括：

- `var x`
- `x = 1`
- `%t0 = a + b`
- `ifFalse %t0 goto L1`
- `param x`
- `%t1 = call add, 2`
- `return %t1`

优点：

- 结构简单，便于在课程或答辩中展示前端 lowering 结果。
- 控制流、调用、`++/--` 展开等都能直观看到。

限制：

- IR 不携带强类型信息，后续 Maple 降低只能将大多数值视为 `i32`。
- 函数边界没有在 TAC 层显式建模，多个函数体会被顺序写入同一模块；因此该层更适合作为教学型中间结果，而不是严谨的后端输入。
- `float` / `char` 在默认 `IR -> Codegen` 路径中缺少完善的类型保真能力。

### 2.5 Maple IR

Maple lowering 是当前仓库最完整的后端前半段实现，包含两条入口：

- `generate(const Program&)`：直接从 AST 生成 Maple IR
- `generate(const IRModule&, ...)`：从现有 TAC IR 生成 Maple IR

AST -> Maple 路径的关键实现：

- MiniC 类型映射：`int -> i32`，`float -> f32`，`char -> i8`
- 局部变量声明：`var %name type`
- 读写变量：`dread` / `dassign`
- 算术与比较：`add`、`sub`、`mul`、`div`、`rem`、`gt`、`lt`、`eq` 等
- 条件与循环：显式降低成带 label 的基本块和 `brfalse` / `goto`
- 函数调用：
  - 语句位置调用使用 `call`
  - 需要返回值时使用 `callassigned`
- 前后缀 `++/--`：通过额外内部临时变量保存 old/new value，保证表达式语义
- 顶层语句：会被包装为合成函数 `__minic_entry`

IR -> Maple 路径的特点：

- 可复用现有 TAC IR，便于展示 Maple handoff 能力。
- 由于 TAC 弱类型，默认把绑定和值看作 `i32`。
- 发现浮点字面量时会直接拒绝 lowering，避免生成错误 Maple IR。

### 2.6 ABC handoff

`--emit-abc` 模式并不在仓库内直接实现真正的 Ark 后端，而是：

1. 先生成与目标同名的 `.mpl` 文件
2. 读取 `MINIC_ARK_BACKEND_CMD`
3. 将命令模板中的 `{input}` 和 `{output}` 分别替换为 `.mpl` 与目标 `.abc` 路径
4. 使用 `std::system` 调用外部命令

因此当前仓库已经具备“Maple IR 交给外部 Ark 工具链”的接口，但还不是固定、可复现实验环境中的完整后端。

## 3. 关键技术实现

### 3.1 `++/--` 表达式语义实现

前后缀 `++/--` 不能简单反糖成 `i = i + 1` 或 `i = i - 1`，原因在于它既有副作用，也有表达式值：

- `++i` 的结果值是更新后的新值
- `i++` 的结果值是更新前的旧值
- 当它出现在更大表达式中时，例如 `int y = i++ + add(x, 2);`，编译器既要完成对 `i` 的更新，又要把正确的旧值或新值送入后续计算

因此，当前实现没有把 `++/--` 当作普通赋值语句处理，而是在 AST 中保留 `IncDecExpr` 节点，并在 Maple lowering 阶段显式区分“旧值”和“新值”。

具体做法是：

- 先为当前变量申请一个内部临时变量，例如 `__inc_old_*`
- 用 `dassign` 把原变量值保存到这个旧值临时量
- 再申请一个新的内部临时变量，例如 `__inc_new_*`
- 使用 `add` 或 `sub` 计算更新后的值并写入新值临时量
- 再把新值写回原始局部变量

对应 Maple IR 形态大致如下：

```text
dassign %__inc_old_4 (dread i32 %i)
dassign %__inc_new_5 (add i32 (dread i32 %i, constval i32 1))
dassign %i (dread i32 %__inc_new_5)
```

最后根据表达式种类返回不同结果：

- 后缀 `i++` / `i--` 返回 `dread` 旧值临时量
- 前缀 `++i` / `--i` 返回 `dread` 新值临时量

这就是 `IncDecExpr` 在 Maple IR lowering 中的核心处理方式：不丢失副作用，也不丢失表达式值语义。

### 3.2 `ForStmt` 的 Maple IR lowering

`for` 语句在 Maple lowering 中不会保留高级结构，而是被拆成显式控制流基本块：

- `for_init`
- `for_cond`
- `for_body`
- `for_update`
- `for_end`

其基本跳转关系可以概括为：

```text
for_init
  goto for_cond

for_cond
  brfalse for_end (condition)
  goto for_body

for_body
  ...
  goto for_update

for_update
  ...
  goto for_cond

for_end
```

其中：

- `label` 负责显式标记基本块入口
- `brfalse` 负责在条件为假时跳转到 `for_end`
- `goto` 负责串联正常路径和循环回边

这样做的优点是控制流图非常清晰，便于 Maple IR 和后续后端直接消费，不需要再解释高层 `for` 语义。

`for` 的作用域也被单独处理。当前实现会在 lowering 整个 `for` 之前建立一层循环作用域，在 `for_end` 之后再退出。这样 `for (int i = 0; ... )` 中声明的 `i` 可以同时在：

- 初始化部分
- 条件部分
- 循环体
- 更新部分

保持可见，而不会泄漏到循环外部。循环体内部如果本身是块语句，还会继续叠加更内层的块级作用域。

### 3.3 函数调用参数求值顺序与嵌套调用处理

函数调用 lowering 的核心目标有三个：

- 按固定顺序计算实参
- 正确处理嵌套调用
- 区分“丢弃返回值”和“保留返回值”两种调用形式

在 AST -> Maple 路径中，参数表达式会按从左到右的顺序递归求值，然后再生成外层调用。这样当某个参数本身是赋值、`++/--` 或另一个函数调用时，副作用会先被物化，再把得到的表达式结果拼接进外层调用参数列表。

对嵌套调用，当前实现不使用跨调用共享的 `param` 指令队列，而是让每个调用点各自完成参数求值和参数列表拼接：

- 内层调用若需要返回值，会先通过 `callassigned` 写入内部临时量，例如 `__callret_*`
- 外层调用只消费这个临时量对应的 `dread` 表达式

这样可以避免不同调用层级之间出现参数串台。对现有字符串 TAC -> Maple 路径，代码还额外使用 `pendingCallArguments` 暂存 `param`，并在每次 `call` / `callassigned` 发射完成后立即清空，防止一条调用遗留的参数污染下一条调用。

`call` 和 `callassigned` 的区别如下：

- `call`：用于语句位置的函数调用，调用结果被丢弃，只保留副作用
- `callassigned`：用于表达式位置或赋值初始化场景，调用结果会写入一个局部变量或内部临时变量，再以 `dread` 的形式参与后续表达式

因此，像 `add(sum, i);` 这种语句会降低为 `call`，而像 `int x = add(sum, 42);` 或 `i++ + add(x, 2)` 这种需要保留返回值的表达式，则会降低为 `callassigned`。

## 4. MiniC 语言子集支持情况

| 状态 | 子集能力 | 说明 |
| --- | --- | --- |
| 已支持 | 基本类型 | `int`、`float`、`char` 的声明、字面量、语义检查；其中 `float` / `char` 在 AST -> Maple 路径最完整 |
| 已支持 | 语句与控制流 | 块语句、变量声明、表达式语句、`return`、`if-else`、`while`、`for` |
| 已支持 | 表达式 | 二元算术、比较、逻辑、赋值、复合赋值、一元负号 / 逻辑非、前后缀 `++/--` |
| 已支持 | 函数 | 函数定义、参数、调用、嵌套调用 |
| 已支持 | 语义检查 | 作用域、重复声明、未声明标识符、参数重复、返回类型不匹配、初始化类型不匹配 |
| 部分支持 | 多类型后端传递 | AST -> Maple 能保留 `int/float/char` 类型；TAC -> Maple 由于弱类型以 `i32` 为主 |
| 部分支持 | 默认后端展示 | `IR -> Codegen` 可以演示 lowering 结果，但输出只是伪目标代码，不是可执行机器码或稳定虚拟机代码 |
| 部分支持 | 顶层语句 | 允许作为程序输入，并在 Maple 阶段包装成合成入口函数；这更像演示能力，不是标准 C 风格全局语义 |
| 暂不支持 | `void` 函数 | `Semantic` 层有 `Void` 类型枚举，但语法层未接受 `void` 关键字 |
| 暂不支持 | 复合数据与高级语法 | 数组、指针、字符串、结构体、成员访问、函数原型、全局变量初始化 |
| 暂不支持 | 更多控制语句 | `break`、`continue`、`switch`、`do-while` |
| 暂不支持 | 标准库 / 运行时 | `printf`、输入输出、运行时初始化、标准头文件生态 |

## 5. 测试用例说明

当前测试由两部分组成：

- `ctest` 自动回归：重点验证 Maple lowering 和 ABC handoff 关键模式
- `tests/*.mc` 手工 / 冒烟样例：覆盖语法、语义错误与边界情况

自动回归样例：

| 测试名 | 入口 | 关注点 |
| --- | --- | --- |
| `maple_increment_expression` | `tests/increment_expression.mc` | 前后缀 `++/--` 表达式值、嵌套调用、`callassigned` |
| `maple_nested_control_flow` | `tests/nested_control_flow_call.mc` | `for` / `if` / `while` 嵌套控制流与调用降低 |
| `maple_nested_increment_call` | `tests/nested_increment_call.mc` | 嵌套 `for`、`i++` 表达式值、ABC handoff、Maple 伴生文件生成 |

手工样例覆盖了以下方向：

- 前端冒烟：`basic.mc`、`if_else.mc`、`while.mc`、`for_loop.mc`
- 函数与表达式：`function_call.mc`、`increment_expression.mc`
- 复杂 lowering：`nested_control_flow_call.mc`、`nested_increment_call.mc`
- 语义错误：`undefined_var.mc`、`redeclare.mc`、`type_error.mc`、`semantic_duplicate_param.mc`、`semantic_type_mismatch.mc`、`semantic_undeclared.mc`
- 作用域正确样例：`semantic_scope_ok.mc`
- 注释与综合样例：`test.mc`

更细的逐文件覆盖说明见 [docs/testing_report.md](testing_report.md)。

## 6. 与开题目标的差距

如果将开题目标概括为“实现一个可演示的 MiniC 编译器，并能输出 Maple IR / 对接 Ark 后端”，那么当前完成情况可以总结为：

- 已完成：MiniC 前端、语义分析、TAC IR 展示、AST -> Maple IR lowering、外部 ABC handoff 接口、基础自动化回归
- 未完全完成：真实 Ark 工具链命令固化、强类型 TAC IR、标准库与运行时、可执行后端

从毕业设计陈述角度，可以将本仓库定位为：

- 一个可运行的 MiniC 子集编译器原型
- 一个以 Maple IR 为主要展示亮点的后端前半段实现
- 一个已经具备外部 Ark 接口，但尚未绑定正式工具链命令的实验平台

## 7. 当前限制

### 7.1 真实 Ark 后端命令未固定

`--emit-abc` 已经具备接口，但仍依赖环境变量 `MINIC_ARK_BACKEND_CMD` 提供具体命令模板。仓库未固定某个 Ark 工具链版本、安装路径和参数组合，因此当前更适合展示 handoff 设计，而不是开箱即用的正式后端。

### 7.2 TAC IR 弱类型

现有 TAC IR 只保存字符串指令，不保存变量和临时值的精确类型。结果是：

- AST -> Maple 路径比 TAC -> Maple 路径更可靠
- `float` / `char` 无法在 TAC lowering 中完整保真
- TAC 目前更适合做展示型中间结果，而不是唯一的后端输入格式

### 7.3 标准库未支持

仓库目前没有：

- 标准输入输出函数
- 内建运行时
- 与标准库符号相关的语义和链接支持

这意味着当前程序主要用于展示语法、语义和 IR 生成流程，而不是运行真实应用级 MiniC 程序。
