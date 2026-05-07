# MiniC 项目最终验收报告

## 1. 项目概述

当前 MiniC 项目已经形成一条可运行的编译原型链路，覆盖 MiniC 安全子集的词法分析、语法分析、抽象语法树构建、语义检查、字符串形式的 TAC IR 生成、AST 到 Maple IR 的 lowering，以及面向外部 Ark 后端命令模板的 handoff 接口。

本次最终验收期间，已重新执行构建、全量 `ctest`、基础样例编译以及 `--emit-maple` 冒烟验证。`README`、`docs` 与 `examples` 也已按当前实现状态完成一致性检查与必要的文档同步。

## 2. 支持的 MiniC 特性

当前项目支持的 MiniC 子集包括：

- 基本类型：`int`、`float`、`char`
- 语句结构：块语句、变量声明、表达式语句、`return`
- 控制流：`if-else`、`while`、`for`
- 表达式：四则运算、比较、逻辑、赋值、复合赋值、一元负号、逻辑非、前后缀 `++/--`
- 函数相关：函数定义、参数、函数调用、嵌套函数调用
- 语义检查：重复定义、未声明标识符、参数匹配、返回类型、条件类型、初始化与赋值类型匹配
- Maple IR 输出：AST-to-Maple 路径下保留 `int -> i32`、`float -> f32`、`char -> i8`
- ABC handoff：`--emit-abc` 会先生成 `.mpl`，再调用外部 `MINIC_ARK_BACKEND_CMD` 模板

当前明确不在支持范围内的内容包括：标准库、运行时、数组、指针、结构体、字符串、`switch`、`break`、`continue`、`do-while` 以及真实固定的 OpenArkCompiler 后端。

## 3. 编译流水线

当前编译流水线可概括为：

```text
MiniC Source
  -> Lexer
  -> Parser
  -> AST
  -> Semantic Analyzer
  -> TAC IR
  -> two downstream paths
       A. AST -> Maple IR (.mpl)
       B. AST -> Maple IR (.mpl) -> external Ark backend command -> output path
```

需要特别说明：

- `--emit-maple` 直接走 AST-to-Maple IR 路径。
- `--emit-abc` 的实际流程是 `MiniC -> Maple IR (.mpl) -> external Ark backend command -> output path`。
- `MINIC_ARK_BACKEND_CMD` 是外部命令模板，不是仓库内置真实 OpenArkCompiler 后端。
- 若未配置并验证真实 Ark 后端，则生成的 `.abc` 只能视为 handoff artifact。

## 4. 测试结果

本次验收实际执行结果如下：

### 4.1 构建与回归

- `cmake -S . -B build`：成功
- `cmake --build build`：成功
- `ctest --test-dir build --output-on-failure`：`23/23` 通过，`0` 失败

当前自动化回归覆盖：

- Maple lowering 关键模式
- AST-to-Maple 标量类型输出
- `--emit-abc` 外部后端错误处理
- `float/char` 类型路径与代表性语义负例

### 4.2 基础样例验证

- `./build/minic tests/basic.mc`：成功，正常输出 AST、TAC IR 和伪目标代码
- `./build/minic --emit-maple tests/basic.mc build/basic.mpl`：成功，生成了 `build/basic.mpl`

生成的 `build/basic.mpl` 片段如下：

```text
func &main () i32 {
  var %a i32
  var %b i32
  var %c i32

  # BB entry
  @entry
  dassign %a (constval i32 1)
  dassign %b (constval i32 2)
  dassign %c (add i32 (dread i32 %a, mul i32 (dread i32 %b, constval i32 3)))
  return (dread i32 %c)
}
```

### 4.3 `--emit-abc` 验证

本次验收环境中未设置 `MINIC_ARK_BACKEND_CMD`，因此没有执行可选的 `./build/minic --emit-abc tests/basic.mc build/basic.abc` 成功路径验证。

不过，当前 `ctest` 已覆盖 `--emit-abc` 的四类错误处理回归：

- 环境变量未设置
- 模板缺少 `{input}`
- 模板缺少 `{output}`
- 外部命令返回非零退出码

## 5. 已知限制

当前项目的主要限制如下：

- TAC IR 为字符串形式，缺少强类型元数据；TAC-to-Maple 路径仍可能退化为 `i32`
- 真实 OpenArkCompiler 后端尚未固定接入
- 当前无法证明 `.abc` 产物是已验证可运行的真实 Ark 字节码
- 未集成运行时环境
- 不支持标准库函数与外部符号链接
- 不支持数组、指针、结构体、字符串等复杂数据模型
- 默认 `IR -> Codegen` 路径仅输出伪目标代码，不是可执行机器码或稳定虚拟机代码

## 6. 与开题报告的对应关系

可按如下方式将当前实现与开题目标对应：

- 已实现：MiniC 安全子集设计、Lexer、Parser、AST、Semantic Analyzer、TAC IR、AST-to-Maple IR lowering、Ark Backend Handoff、CTest 回归测试
- 部分实现：`float/char` 类型路径、Maple IR 类型映射、ABC 生成链路
- 未实现：真实 OpenArkCompiler 后端固定接入、真实可运行 ABC 验证、运行时集成、标准库支持、数组/指针/结构体

从论文或答辩表述角度，当前项目适合定位为：

- 一个可运行的 MiniC 安全子集编译器原型
- 一个已经完成前端与 Maple lowering 主链路的实现
- 一个具备外部 Ark 后端 handoff 接口、但尚未完成真实官方后端闭环验证的实验平台

## 7. 后续工作

后续工作建议优先级如下：

1. 确认真正可用的 OpenArkCompiler 或相关工具名称、输入语法要求和 `.abc` 输出参数。
2. 固定真实 Ark 后端接入方式，并验证当前 `.mpl` 是否可直接被接受。
3. 建立真实 `.abc` 的官方检查流程或运行时验证流程。
4. 如需继续扩展语言范围，再考虑运行时、标准库以及复杂数据类型支持。
5. 若后续需要提升后端一致性，再评估 TAC IR 的强类型化，而不是在当前验收阶段做功能重构。

## 8. 文档与样例一致性结论

本次验收中已检查：

- `README.md`
- `docs/implementation_report.md`
- `docs/testing_report.md`
- `docs/ark_backend_investigation.md`
- `docs/thesis_mapping.md`
- `examples/`

结论如下：

- `README`、`docs` 与当前实现总体一致
- 文档中关于 Ark handoff、Maple IR 类型输出和自动化测试覆盖范围已同步到当前状态
- `examples/` 中现有样例文件与 `README` 的引用关系一致，未发现缺失引用的样例文件
