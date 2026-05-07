# 真实 Ark Backend 兼容性分析

## 1. 结论摘要

基于 2026 年 5 月 7 日对 OpenArkCompiler / OpenHarmony 公开仓库与文档的调查，当前 MiniC 的 `--emit-abc` 更准确的定位是：

```text
MiniC -> Maple IR (.mpl) -> external Ark backend command -> output path
```

它已经实现了一个可配置的 handoff 接口，但**尚不能证明当前生成的 `.mpl` 可以被真实公开 Ark `.abc` 工具链直接接受**。

当前判断如下：

- MiniC 生成的是一个 **Maple-like 文本 IR 子集**，在表面语法上接近公开 `MapleIRDesign.md` 描述的 Maple IR。
- 公开 OpenArkCompiler 文档仍然清楚描述了 `hir2mpl`、`maple`、`mpl2mpl`、`me`、`mplcg` 这一类 **Maple 编译链**，它主要面向 `.ast -> .mpl -> 优化 / 汇编 / 目标代码`。
- 公开 OpenHarmony / Ark 文档描述的 **`.abc` 生成链** 则主要围绕 `es2abc`、`ets_frontend` 或 `ark_asm` 展开，输入分别是源语言前端产物或 `*.pa` 文本汇编，而不是 `*.mpl`。
- 在本次检索到的公开资料中，我**没有找到一个已文档化、当前可依赖的独立 `mpl2abc` 或 “maple driver 直接产出 .abc” 的公开链路**。

因此，当前 MiniC 与“真实可运行 ABC”之间的最大距离，不一定是若干条 Maple 语法细节，而更可能是**接入目标本身不对**：MiniC 现在输出的是 Maple IR，而公开 Ark 字节码工具链接受的入口看起来是另一套前端/汇编格式。

## 2. 上游公开工具链调查

### 2.1 Maple IR 与 `.mpl`

公开 `MapleIRDesign.md` 将 Maple IR 描述为可读写的文本 IR，并给出了 `.mpl` / `.mplt` 的模块、变量、类型、函数、控制流和调用语法。文档中列出的要点包括：

- 模块级指令：`entryfunc`、`flavor`、`id`、`import`、`importpath`、`numfuncs`、`srclang`。
- 变量声明：`var <id-name> <storage-class> <type> <type-attributes>`。
- 函数声明：`func <func-name> <attributes> (...) <ret-type> { ... }`。
- 函数内部可选指令：`funcsize`、`framesize`、`moduleid`、`formalwordstypetagged`、`localwordstypetagged` 等。
- 平坦控制流和调用语法：`brfalse`、`goto`、`return`、`call`、`callassigned`、`dassign`、`dread`。

这说明从“文件格式”角度看，MiniC 选择 `.mpl` 作为文本输出载体并不是脱靶；但公开设计文档描述的是**完整 Maple IR 语言**，并不自动等于“当前 Ark 字节码工具链就接受这份 `.mpl`”。

### 2.2 公开 OpenArkCompiler 编译链

公开 OpenArkCompiler 资料仍然展示了典型 Maple 工具链：

- `hir2mpl` 读取 `clang` 生成的 `.ast` 并输出 `.mpl`。
- `maple --run=me:mpl2mpl:mplcg` 驱动后续优化与代码生成阶段。

从公开 release note 和 quick-start 片段来看，这条链更像：

```text
C/Java frontend -> .ast/.mpl -> Maple middle/back end -> 汇编/目标代码
```

也就是说，公开主线 Maple 文档更支持“先验证 `.mpl` 能否被 Maple 工具接受”，而不是直接支持“.mpl 一步变成 `.abc`”。

### 2.3 公开 Ark `.abc` 工具链

公开 OpenHarmony 文档与仓库信息显示：

- `arkcompiler_runtime_core` 的 `assembler` 目录被描述为“把文本格式 `*.pa` 组装成二进制 `*.abc`”。
- 同一仓库还提供 `ark_disasm`，用于把二进制 `.abc` 反汇编为文本 `*.pa`。
- `arkcompiler_ets_frontend` README 明确写到：`ets_frontend` 结合 `ace-ets2bundle` 可把 ETS 文件转换成 ARK bytecode；其 `es2abc` 用法则是把 JavaScript 输入转换成 Ark bytecode 文件。

因此，公开 Ark `.abc` 链路更接近：

```text
JS/TS/ETS source -> es2abc / ets_frontend -> .abc
```

或者：

```text
Panda assembly (.pa) -> ark_asm -> .abc
```

这与 MiniC 当前的 `.mpl` handoff 入口并不一致。

### 2.4 关于 OpenArkCompiler-lite

本次调查没有找到一套独立、当前公开且文档化的 `OpenArkCompiler-lite` `.mpl -> .abc` 接入说明，足以推翻上面的判断。公开资料里可以看到一些 lite 相关分支/命名痕迹，但在本次检索范围内，没有找到一条清晰公开的“lite Maple IR 直接产出可运行 `.abc`”流程。

因此，本报告对 “OpenArkCompiler-lite” 的判断与主线一致：

- 目前**不能据公开资料认定** MiniC 现有 `.mpl` 已兼容一个真实 lite `.abc` 后端。
- 如果后续要走 lite 路线，仍然需要先拿到该路线当前真正接受的输入格式与工具入口。

## 3. MiniC 当前 `.mpl` 输出形态

当前 MiniC 的 `--emit-maple` 路径来自 AST-to-Maple lowering，而不是优先依赖 TAC-to-Maple。相关实现位于：

- `src/main.cpp`
- `src/maple/IRGenerator.cpp`

当前 emitter 的已知行为包括：

- 模块头会输出 `flavor 1`、`srclang "MiniC"`、`numfuncs`、`entryfunc`。
- 函数以 `func &name (...) retType {` 形式输出。
- `int -> i32`、`float -> f32`、`char -> i8`。
- 局部变量以 `var %name type` 声明。
- 表达式和语句会生成 `dread`、`dassign`、`constval`、`add/sub/mul/div/rem`、`eq/ne/lt/le/gt/ge`。
- 控制流会生成显式 label、`brfalse`、`goto`。
- 函数调用会生成 `call` 或 `callassigned`。

一个代表性输出片段如下：

```text
flavor 1
srclang "MiniC"
numfuncs 2
entryfunc &main

func &add (var %a i32, var %b i32) i32 {
  # BB entry
  @entry
  return (add i32 (dread i32 %a, dread i32 %b))
}
func &main () i32 {
  var %sum i32
  var %i i32
  ...
  brfalse @for_end_3 (lt i32 i32 (dread i32 %i, constval i32 5))
  ...
  callassigned &add (dread i32 %sum, constval i32 42) { dassign %x 0 }
}
```

## 4. 与公开 Maple IR 语法的对比

### 4.1 已基本对齐的部分

如果只看公开 `MapleIRDesign.md` 的文本语法，MiniC 当前输出至少在下面这些点上是**形式上对齐**的：

- 文本 `.mpl` 模块头：`flavor`、`srclang`、`numfuncs`、`entryfunc`
- 函数声明关键字：`func`
- 局部变量声明关键字：`var`
- 基本标识符前缀：`&` 用于函数，`%` 用于局部变量，`@` 用于标签
- 基元类型：`i32`、`f32`、`i8`
- 基础表达式/语句：`constval`、`dread`、`dassign`
- 显式控制流：`brfalse`、`goto`、`return`
- 调用形式：`call`、`callassigned`

也就是说，MiniC 当前 emitter 不是随意拼接的私有格式，而是明显参考了 Maple IR 的公开文本风格。

### 4.2 当前缺失或未覆盖的部分

但与公开 Maple IR 设计相比，MiniC 现在仍然缺失多类信息。

#### 模块 metadata

MiniC 当前没有输出这些公开设计中出现的模块级信息：

- `id`
- `import`
- `importpath`
- `globalmemsize`
- `globalmemmap`
- `globalwordstypetagged`
- `globalwordsrefcounted`

对于当前只含局部标量与简单函数的 MiniC 子集，这些字段未必每一个都立即必需；但如果真实工具链依赖其中部分信息，当前 `.mpl` 就仍然不完整。

#### function attribute / function directive

MiniC 当前输出的是：

```text
func &name (...) retType {
```

但没有附带公开文档允许的函数 attribute 或函数内部 directive，例如：

- `static`
- `extern`
- `funcsize`
- `framesize`
- `moduleid`
- `upformalsize`
- `formalwordstypetagged`
- `formalwordsrefcounted`
- `localwordstypetagged`
- `localwordsrefcounted`

这意味着即使“裸语法”能被某个 Maple 解析器接受，也未必满足更后续阶段对栈帧、模块归属、类型标记或引用计数位图的要求。

#### type annotation 的现状

对当前 MiniC 已支持的 `int / float / char` 子集来说，primitive type annotation **并没有完全丢失**：

- 参数、局部变量、返回值已经显式输出为 `i32 / f32 / i8`
- 表达式运算也显式带 primitive type

因此，“当前问题不是完全没有类型注解”，而是：

- 只有 primitive type 信息
- 没有更高层的 derived/high-level type
- 没有与运行时或对象模型相关的 type metadata
- 没有 `formalwordstypetagged` / `localwordstypetagged` 这类按字栈布局的辅助信息

#### 运行时与语言 metadata

当前 MiniC 不生成：

- `.mplt` 类型导入
- 类 / 接口 / 对象布局
- 运行时方法 / 模块描述信息
- Panda / Ark 字节码层面的语言元数据

这对仅验证“文本 Maple 语法长得像不像”不是问题，但对“真实可运行 `.abc`”则很可能是硬缺口。

## 5. 当前 MiniC 与真实 Ark 的距离

可以把距离拆成两个层次看。

### 5.1 与 Maple 文本 IR 的距离

在“文本外形”层面，MiniC 已经接近一个很小的 Maple 子集，尤其是：

- primitive type 映射明确
- 控制流被显式 lower 成 label / branch / goto
- call / callassigned / dassign / dread 的选择也符合公开 Maple 风格

所以，如果目标只是“让一个公开 Maple parser/driver 尝试读入简单 `.mpl`”，MiniC 并非零基础，属于**可继续逼近**的状态。

### 5.2 与真实可运行 `.abc` 的距离

如果目标改成“稳定生成真实可运行 `.abc`”，距离就明显更大：

- 当前公开 Ark `.abc` 入口看起来是 `es2abc` 或 `ark_asm`，而不是 `.mpl`
- MiniC 目前没有证明其 `.mpl` 能进入任何公开 `.abc` 生成阶段
- MiniC 也没有生成 Ark runtime 所需的语言/模块 metadata
- 当前仓库的 `MINIC_ARK_BACKEND_CMD` 只是外部命令模板，不代表已接入真实官方工具

因此，当前结论应写成：

> MiniC 已完成 Maple IR handoff 原型，但距离“真实 Ark `.abc` 固定接入并可运行验证”仍有明显工程与技术缺口。

## 6. 最小可行接入路径

### 路径 A：先验证真实 Maple 工具是否接受 MiniC `.mpl`

这是最稳妥、改动最小的第一步。

建议顺序：

1. 获取当前可构建的公开 OpenArkCompiler 主机工具。
2. 先确认哪个公开工具/阶段能直接读取 `.mpl`。
3. 用 MiniC 当前最简单的 `int` / `float` / `char` 样例去喂这个工具。
4. 根据真实报错，局部修正 Maple emitter 的头部、函数 attribute、directive 或语法细节。

这条路径的价值是：

- 能尽快把“MiniC `.mpl` 是否真能被 Maple 读入”从猜测变成实测。
- 一旦这一步都不通过，就不应继续声称 `.mpl -> .abc` 只差一个命令模板。

但要注意：

- 即使 Maple 工具接受了 `.mpl`，这也**只证明 Maple 兼容性**，不等于已经能产出 `.abc`。

### 路径 B：如果目标必须是可运行 `.abc`，先确认真实 `.abc` 入口

如果毕业设计后续目标是“真实可运行 `.abc`”，那么下一步不应先大改 MiniC，而应先回答一个更根本的问题：

> 当前公开可维护的 Ark `.abc` 工具链，到底接受什么输入？

根据现有公开资料，最可能的候选入口是：

- `es2abc` / `ets_frontend`
- `ark_asm` 对应的 `*.pa` 文本汇编

如果最终确认**没有公开维护的 `mpl -> abc` 桥接器**，则“最小可行接入路径”更可能变成：

```text
MiniC AST -> 新的 Ark-accepted text/binary frontend format -> .abc -> 运行时验证
```

而不是继续坚持：

```text
MiniC AST -> .mpl -> 真实 .abc
```

## 7. 最大技术风险

最大风险不是 `i32/f32/i8` 这类 primitive type 映射，而是：

**公开 Ark 字节码链路可能根本不以 Maple `.mpl` 作为输入。**

如果这一点成立，那么：

- 当前 Maple emitter 只能作为“Maple lowering 实验与论文展示”的成果；
- 它未必是生成真实 `.abc` 的正确中间层；
- 后续如果强行围绕 `.mpl -> .abc` 做大量补丁，可能是在错误的技术方向上投入时间。

这个风险高于以下次级风险：

- module metadata 不足
- function attribute / directive 不足
- runtime metadata 缺失
- 某些表达式或控制流语法细节与真实 parser 不一致

## 8. 是否需要改 Maple IR emitter

结论要分层回答。

### 8.1 如果目标只是“更像真实 Maple IR”

需要，而且很可能是 `src/maple/IRGenerator.cpp` 的局部增强，重点可能包括：

- 补足必要的模块级 metadata
- 明确函数 attribute
- 在需要时补 `moduleid` / `funcsize` / `framesize` 一类 directive
- 按真实工具报错补齐语法细节

这属于“继续打磨 Maple emitter”。

### 8.2 如果目标是“真实可运行 `.abc`”

很可能**不只**需要改 Maple emitter，甚至可能说明“Maple emitter 不是最终目标格式”。

换句话说：

- 如果公开真实 `.abc` 入口不是 `.mpl`，那就不该把全部工作压在 `IRGenerator.cpp` 上。
- 这时需要的是“接入正确入口格式”的新模块，而不是只修已有 `.mpl` 输出。

## 9. 是否需要接入 runtime metadata

对“真实可运行 `.abc`”来说，大概率需要。

原因不是 MiniC 当前语言功能很多，而是：

- Ark runtime / bytecode 文件本身就是语言与运行时耦合的产物；
- 公开工具链已经体现出自己的文件格式、汇编格式与运行时校验工具；
- 就算 MiniC 只支持 `int / float / char` 和简单函数，最终要进入可执行 `.abc`，也通常需要与目标 runtime 的模块、函数、调用约定和验证器兼容。

但这一项现在还不能细化到“具体字段名必须是什么”，因为那取决于最终确认的真实入口到底是：

- Maple
- Panda assembly (`*.pa`)
- 还是某个前端专用格式

## 10. 建议修改的模块

如果后续继续推进，建议把模块修改范围分成两档。

### 10.1 低风险验证档

先只准备修改：

- `src/maple/IRGenerator.cpp`
- 与 Ark handoff 相关的文档和测试

目标是验证：

- 真实公开 Maple 工具能否读取 MiniC 当前 `.mpl`
- 真实报错究竟是语法问题、metadata 问题，还是更根本的工具入口问题

### 10.2 如果确认目标是可运行 `.abc`

则可能需要新增或调整：

- `src/maple/IRGenerator.cpp`：如果仍保留 Maple 输出用于论文/对比
- 新的 Ark backend adapter / emitter 模块：如果真实入口不是 `.mpl`
- `src/main.cpp` 中 `--emit-abc` 的接入说明与配置方式
- 运行时验证脚本 / 文档
- 与 runtime metadata 相关的生成层

当前阶段**不建议**直接承诺这些改动，因为是否需要它们，取决于第一步真实工具验证结果。

## 11. 推荐下一步

最推荐的下一步只有两件事。

1. 先拿到一套当前可运行的公开 OpenArkCompiler 主机工具，实测 MiniC 最小 `.mpl` 能否被 Maple 链读入。
2. 并行确认当前公开 Ark `.abc` 入口是否存在文档化的 `mpl -> abc` 路径；如果不存在，应尽早把“真实可运行 `.abc`”的目标改写为“接入 Ark 实际接受的输入格式”。

如果这两件事没有先做，后续继续强化 `MINIC_ARK_BACKEND_CMD` 只会让 handoff 更好用，但不会自动缩短 MiniC 与真实 `.abc` 之间的关键距离。

## 12. 本次调查使用的公开来源

以下来源均为本次结论直接参考的公开资料：

- OpenArkCompiler Maple IR 设计文档：<https://gitee.com/openarkcompiler/OpenArkCompiler/blob/master/doc/en/MapleIRDesign.md>
- OpenArkCompiler 仓库 README：<https://gitee.com/openarkcompiler/OpenArkCompiler/blob/master/Readme.md>
- OpenArkCompiler release note / Maple 1.0.0 发布说明：<https://gitee.com/openarkcompiler/OpenArkCompiler/releases>
- OpenHarmony `arkcompiler_runtime_core` README：<https://github.com/openharmony/arkcompiler_runtime_core>
- OpenHarmony `arkcompiler_ets_frontend` README：<https://github.com/openharmony/arkcompiler_ets_frontend>
- OpenArkCompiler 组织页说明 OpenArkCompiler 2.0 / 3.0 仓库分工：<https://gitee.com/openarkcompiler?skip_mobile=true>

## 13. 最终判断

本报告的最终判断是：

- **当前 MiniC 的 `.mpl` 输出可以视为“接近 Maple IR 的实验性文本 lowering”。**
- **当前没有足够公开证据证明这份 `.mpl` 能直接进入真实公开 Ark `.abc` 工具链。**
- **最小可行下一步不是先重写 MiniC，而是先做真实 Maple / Ark 工具入口实测。**
- **如果公开 `.abc` 入口最终不是 `.mpl`，则后续重点应从“修补 Maple emitter”转向“接入 Ark 真正接受的输入格式”。**
