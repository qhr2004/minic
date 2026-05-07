# Ark 后端工具链调查记录

## 1. 结论摘要

按照要求，我在以下范围内搜索了本地可执行文件：

- `/home/qin`
- `/home/qin/graduation`
- `/opt`
- `/usr/local`
- `/mnt/c/Users`
- `/mnt/c/Program Files`

目标工具名为：

- `ark_asm`
- `ark_disasm`
- `es2abc`
- `es2panda`
- `maple`
- `mpl2mpl`
- `mplcg`
- `jbc2mpl`

当前结论：

- 在上述范围内，没有找到任何一个目标可执行文件。
- 这些工具当前也都不在 `PATH` 中。
- 因为没有找到真实可执行文件，所以不存在可供本机执行的 `--help` / `-h` 输出。
- 结合仓库中现有 `.abc` 文件内容判断，本机当前不存在一条已安装、可验证的真实 `.mpl -> .abc` 工具链。

因此，当前 MiniC 项目中的 `MINIC_ARK_BACKEND_CMD` 应被定位为“可配置 handoff 接口”，而不是“已经固定接入官方 Maple IR -> Ark 字节码后端”。

## 2. 当前 `--emit-abc` 实际流程

当前项目中，`--emit-abc` 的流程应明确描述为：

```text
MiniC -> Maple IR (.mpl) -> external Ark backend command -> output path
```

这条链路的当前含义如下：

- MiniC 前端和 Maple lowering 在仓库内实现。
- 编译器会先生成一个 `.mpl` 文件。
- `MINIC_ARK_BACKEND_CMD` 是外部命令模板，不是仓库内置后端。
- 命令模板中的 `{input}` 会被替换为生成出来的 `.mpl` 路径。
- 命令模板中的 `{output}` 会被替换为用户请求的 `.abc` 路径。
- 编译器随后调用该外部命令，并把结果写到请求的输出路径。

需要特别强调：

- 当前项目没有内置真实 OpenArkCompiler 后端。
- 因此，如果没有额外配置并验证真实 Ark 后端，`--emit-abc` 产生的 `.abc` 只能被视为 handoff artifact，而不是已经确认可运行的真实 Ark 字节码产物。

## 3. 本地搜索方法与结果

### 3.1 `PATH` 检查

执行命令：

```sh
for t in ark_asm ark_disasm es2abc es2panda maple mpl2mpl mplcg jbc2mpl; do
    printf '%s: ' "$t"
    command -v "$t" || true
done
```

结果为：

```text
ark_asm:
ark_disasm:
es2abc:
es2panda:
maple:
mpl2mpl:
mplcg:
jbc2mpl:
```

说明这些工具当前都不在系统可执行路径中。

### 3.2 精确文件名搜索

我对指定目录做了按文件名精确匹配的搜索，目标包括：

- `ark_asm` / `ark_asm.exe`
- `ark_disasm` / `ark_disasm.exe`
- `es2abc` / `es2abc.exe`
- `es2panda` / `es2panda.exe`
- `maple` / `maple.exe`
- `mpl2mpl` / `mpl2mpl.exe`
- `mplcg` / `mplcg.exe`
- `jbc2mpl` / `jbc2mpl.exe`

搜索结果：

- `/home/qin`：未命中
- `/home/qin/graduation`：未命中
- `/opt`：未命中
- `/usr/local`：未命中
- `/mnt/c/Users`：未命中
- `/mnt/c/Program Files`：未命中

### 3.3 相关但无关的命中项

搜索过程中发现了少量名字相近但并非目标工具的路径：

- `/home/qin/graduation/minic/src/maple`
- `/mnt/c/Users/qin/AppData/Roaming/QQ/arks`

这些路径不提供 Maple 编译工具，也不提供 Ark 字节码工具，不能作为 `.mpl -> .abc` 后端依据。

## 4. 各工具的本地状态与典型用途

由于本机没有找到这些可执行文件，下面表格中的“作用”是工具链常见职责说明，不是本机 `--help` 的实测输出。

| 工具 | 本机是否找到 | 路径 | 是否执行 `--help` / `-h` | 典型用途 |
| --- | --- | --- | --- | --- |
| `ark_asm` | 否 | 无 | 否，本机无二进制 | Ark 汇编器，通常用于把 Ark 汇编/文本格式转成 Ark 字节码产物 |
| `ark_disasm` | 否 | 无 | 否，本机无二进制 | Ark 反汇编/检查工具，用于查看 `.abc` 等 Ark 字节码内容 |
| `es2abc` | 否 | 无 | 否，本机无二进制 | 常见于 OpenHarmony ArkTS/JS 工具链，用于把源码前端产物转成 `.abc` |
| `es2panda` | 否 | 无 | 否，本机无二进制 | Panda/Ark 前端编译工具，可从源码生成 Ark 字节码或相关中间产物 |
| `maple` | 否 | 无 | 否，本机无二进制 | Maple 驱动或封装入口 |
| `mpl2mpl` | 否 | 无 | 否，本机无二进制 | Maple IR 到 Maple IR 的优化/变换阶段 |
| `mplcg` | 否 | 无 | 否，本机无二进制 | Maple 后端代码生成阶段，通常面向汇编/目标代码方向 |
| `jbc2mpl` | 否 | 无 | 否，本机无二进制 | Java 字节码到 Maple IR 的转换工具 |

## 5. 是否存在真实的 `.mpl -> .abc` 工具链

当前判断：不存在。

理由如下：

1. Ark 侧预期工具 `ark_asm`、`ark_disasm`、`es2abc`、`es2panda` 均未找到。
2. Maple 侧预期工具 `maple`、`mpl2mpl`、`mplcg`、`jbc2mpl` 均未找到。
3. 在本机已搜索到的范围内，没有任何工具表现出“直接把 Maple IR 变成 Ark `.abc`”的能力。

结合 OpenArkCompiler / OpenHarmony 工具边界，可以明确写成：

- OpenArkCompiler 的 Maple 工具通常是 `.mpl -> 优化 / 汇编 / 目标代码` 方向。
- OpenHarmony ArkTS 的 `.abc` 通常来自 `es2abc`、`es2panda` 或 `ark_asm`。
- 因此，当前 MiniC 的 `MINIC_ARK_BACKEND_CMD` 应定位为可配置 handoff，而不是已固定接入官方 `mpl -> abc` 后端。

## 6. 对当前 MiniC 仓库的补充证据

我还检查了仓库中已经存在的 `.abc` 文件，结果表明它们并不像真实 Ark 字节码。

### 6.1 文件类型检查

执行：

```sh
file build/*.abc build/*.mpl
```

结果显示：

```text
build/if_else.abc:                  ASCII text
build/maple_demo.abc:               ASCII text
build/nested_increment_call.abc:    ASCII text
...
build/nested_increment_call.mpl:    ASCII text
```

这说明至少当前这些 `.abc` 文件是文本文件，不像真实二进制字节码容器。

### 6.2 与 `.mpl` 对比

执行：

```sh
cmp -s build/nested_increment_call.abc build/nested_increment_call.mpl; echo $?
cmp -s build/maple_demo.abc build/maple_demo.mpl; echo $?
```

结果中：

- `nested_increment_call` 返回 `0`
- `maple_demo` 返回 `0`

这表示对应 `.abc` 与 `.mpl` 完全相同，是逐字节一致的复制结果。

### 6.3 十六进制预览

执行：

```sh
xxd -l 32 build/nested_increment_call.abc
```

输出开头为：

```text
00000000: 666c 6176 6f72 2031 0a73 7263 6c61 6e67  flavor 1.srclang
00000010: 2022 4d69 6e69 4322 0a6e 756d 6675 6e63   "MiniC".numfunc
```

这明显是 Maple 文本内容，而不是真实的 Ark 字节码二进制格式。

## 7. 关于 `MINIC_ARK_BACKEND_CMD` 的文档定位

当前文档中应把 `MINIC_ARK_BACKEND_CMD` 写成：

- 一个外部命令模板
- 一个把 `.mpl` 交接给外部 Ark 相关工具链的接口
- 一个需要由使用者在仓库外部配置的命令字符串

应避免把它写成：

- 仓库内置的真实 OpenArkCompiler 后端
- 已经确认可把当前 `.mpl` 可靠变成真实 `.abc` 的固定命令
- 已经打通的官方 Maple IR -> ABC 产物链路

## 8. 推荐的 `MINIC_ARK_BACKEND_CMD` 配置说明

### 8.1 当前本机环境推荐配置

如果只是验证 MiniC 的 handoff 机制，推荐使用占位配置：

```sh
export MINIC_ARK_BACKEND_CMD='cat {input} > {output}'
```

这个配置的意义仅限于：

- 验证 `--emit-abc` 路径可执行
- 验证 `.mpl` 已生成
- 验证输出路径和命令模板替换逻辑正常

它不应被表述为“真实 Ark 后端”。

### 8.2 未来若接入真实后端

如果后续确实接入真实后端，`MINIC_ARK_BACKEND_CMD` 也仍应只被描述为“由项目外部提供的真实命令模板”。在当前调查结果下，文档不能虚构具体工具名、不能虚构参数形式，也不能声称已经打通真实 ABC 产物。

## 9. 真实接入时仍需确认的内容

如果未来要把当前 handoff 变成真实的 Ark 后端接入，至少还需要确认以下内容：

- OpenArkCompiler 或相关工具链中，实际应该调用的工具名称是什么。
- 该工具是否接受当前 MiniC 生成的 `.mpl` 语法，还是需要额外转换步骤。
- 该工具生成 `.abc` 的精确参数和调用方式是什么。
- 生成后的 `.abc` 应如何做运行时验证，或如何借助官方检查工具验证其结构和可执行性。

在这些问题没有被逐项确认之前，文档不能把当前输出写成“真实已打通的 ABC”。

## 10. 如何验证 `.abc` 文件

在本机缺少 `ark_disasm` 等官方检查工具的情况下，建议按下面顺序验证：

1. 确认文件存在且非空。
2. 执行 `file output.abc`。
3. 执行 `cmp -s output.abc output.mpl`。
4. 执行 `xxd -l 32 output.abc` 查看头部内容。

判断标准：

- 如果 `file` 显示 `ASCII text`，高度说明这不是真实 Ark 字节码。
- 如果 `cmp` 返回 `0`，说明 `.abc` 只是 `.mpl` 的复制品。
- 如果十六进制/文本预览开头出现 `flavor 1`、`srclang "MiniC"` 等 Maple 文本，说明它不是字节码容器。

如果未来安装了 `ark_disasm`，最可靠的验证方式应是：

- 先查看 `ark_disasm --help`
- 再按其官方帮助给出的语法对 `.abc` 做反汇编或结构检查

## 11. 最终判断

针对当前这台机器和当前 MiniC 仓库状态，可以明确写成：

- 未找到任何请求中的 Ark / Maple 可执行工具
- 未发现真实的本地 `.mpl -> .abc` 转换命令
- 当前仓库中的 `.abc` 结果符合“占位 handoff”行为，不符合“官方 Ark 字节码后端产物”特征

因此，毕业设计和项目文档中的正确表述应是：

- `MINIC_ARK_BACKEND_CMD` 是一个可配置的后端交接点
- 它当前不是一个已经固定、已经验证的官方 Maple IR -> Ark `.abc` 后端
- 当前项目没有内置真实 OpenArkCompiler 后端
- 在未配置并验证真实后端之前，`--emit-abc` 的输出只能被视为 handoff artifact
