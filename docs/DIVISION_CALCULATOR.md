# Unix V6++ 除法计算器 (divcalc)

## 概述

`divcalc` 是一个为 Unix V6++ 系统设计的除法计算器程序，提供完整的整数除法运算功能，包括商和余数的计算。

## 功能特性

- ✅ **双模式运行**：支持命令行参数和交互式两种使用模式
- ✅ **完整除法运算**：同时计算并显示商和余数
- ✅ **正负数支持**：正确处理正数和负数的除法运算
- ✅ **除零检测**：自动检测并防止除零错误
- ✅ **详细输出**：以清晰的格式显示运算结果和数学公式
- ✅ **自定义实现**：使用自定义的 `str_to_int()` 函数（Unix V6++ 没有标准 `atoi()`）

## 使用方法

### 1. 命令行模式

直接在命令行提供两个参数：被除数和除数

```bash
divcalc <被除数> <除数>
```

**示例：**

```bash
# 计算 17 除以 5
divcalc 17 5

# 输出：
# ================================
#   Division Calculation Result
# ================================
# Dividend  : 17
# Divisor   : 5
# Quotient  : 3
# Remainder : 2
# --------------------------------
# Formula   : 17 = 5 * 3 + 2
# Expression: 17 / 5 = 3 ... 2
# ================================
```

```bash
# 支持负数
divcalc -20 3

# 计算大数
divcalc 1000 7
```

### 2. 交互模式

不带参数运行程序，进入交互式计算模式：

```bash
divcalc
```

程序将提示您输入被除数和除数：

```
====================================
  Unix V6++ Division Calculator
====================================
Interactive Mode
Enter 'q' or '0 0' to exit
====================================

Enter dividend (or 'q' to quit): 100
Enter divisor: 7

================================
  Division Calculation Result
================================
Dividend  : 100
Divisor   : 7
Quotient  : 14
Remainder : 2
--------------------------------
Formula   : 100 = 7 * 14 + 2
Expression: 100 / 7 = 14 ... 2
================================

Enter dividend (or 'q' to quit): q
Thank you for using the calculator!
```

**退出方式：**
- 输入 `q` 或 `Q`
- 输入 `0 0`（两个零）

### 3. 查看帮助

输入错误的参数数量将显示使用帮助：

```bash
divcalc --help
# 或
divcalc 1 2 3  # 参数过多
```

## 实现细节

### 核心功能

1. **str_to_int()** - 字符串到整数转换
   - 支持正负号
   - 跳过前导空格和制表符
   - 手动实现（因为 Unix V6++ 缺少标准 `atoi()`）

2. **perform_division()** - 执行除法运算
   - 除零检测
   - 计算商（quotient）
   - 计算余数（remainder）
   - 格式化输出结果

3. **interactive_mode()** - 交互式循环
   - 持续接受用户输入
   - 支持多种退出方式
   - 友好的用户界面

4. **command_line_mode()** - 命令行参数处理
   - 一次性计算
   - 快速输出结果

### 数学公式

程序使用标准的除法公式：

```
被除数 = 除数 × 商 + 余数
dividend = divisor × quotient + remainder
```

**示例：**
- `17 = 5 × 3 + 2`
- `100 = 7 × 14 + 2`
- `-20 = 3 × (-7) + 1`  （注意：C 语言的余数符号与被除数相同）

### 异常处理

程序会检测并处理以下情况：

1. **除零错误**
   ```
   Error: Division by zero is not allowed!
   Divisor cannot be 0.
   ```

2. **参数错误**
   - 显示详细的使用帮助
   - 返回错误码 -1

## 技术规格

- **源文件**：`src/program/divcalc.c`
- **编译目标**：`objs/divcalc.exe`
- **入口函数**：`main1(int argc, char* argv[])`
- **依赖库**：`Lib_V6++.a`
- **头文件**：
  - `<stdio.h>` - 输入输出函数
  - `<sys.h>` - 系统调用

## 编译

程序通过项目的 Makefile 编译：

```bash
cd src/program
make divcalc.exe
```

或者编译所有程序：

```bash
make all
```

## 与 divzero 的比较

| 特性 | divzero | divcalc |
|------|---------|---------|
| 主要功能 | 演示除零异常处理 | 实用的除法计算器 |
| 运行模式 | 仅交互式 | 交互式 + 命令行 |
| 输出内容 | 仅商 | 商 + 余数 |
| 信号处理 | SIGFPE 信号演示 | 无（专注于计算） |
| 输出格式 | 简单 | 详细的格式化输出 |
| 使用场景 | 教学演示 | 实际计算工具 |

## 示例会话

### 例1：基本除法

```bash
$ divcalc 25 4

====================================
  Unix V6++ Division Calculator
====================================
Command Line Mode
====================================

================================
  Division Calculation Result
================================
Dividend  : 25
Divisor   : 4
Quotient  : 6
Remainder : 1
--------------------------------
Formula   : 25 = 4 * 6 + 1
Expression: 25 / 4 = 6 ... 1
================================
```

### 例2：负数除法

```bash
$ divcalc -17 5

================================
  Division Calculation Result
================================
Dividend  : -17
Divisor   : 5
Quotient  : -3
Remainder : -2
--------------------------------
Formula   : -17 = 5 * -3 + -2
Expression: -17 / 5 = -3 ... -2
================================
```

### 例3：除零错误

```bash
$ divcalc 10 0

================================
  Division Calculation Result
================================
Error: Division by zero is not allowed!
Divisor cannot be 0.
```

## 教育价值

这个程序展示了以下 Unix V6++ 编程概念：

1. **系统调用使用**：`printf()`, `gets()`
2. **参数处理**：`argc`, `argv[]`
3. **字符串处理**：手动实现字符串到整数转换
4. **错误处理**：除零检测和参数验证
5. **用户界面设计**：交互式和命令行两种模式
6. **代码组织**：功能模块化设计

## 扩展思路

未来可能的增强功能：

- [ ] 浮点数除法支持
- [ ] 批量计算模式
- [ ] 计算历史记录
- [ ] 更多算术运算（加、减、乘）
- [ ] 表达式解析器
- [ ] 科学计数法支持

## 作者注释

本程序设计为 Unix V6++ 系统的思考题作业，演示了在资源受限的历史操作系统中实现实用工具的方法。代码遵循 Unix V6++ 的编程约定，使用 `main1()` 作为入口点，并链接自定义的 `Lib_V6++.a` 库。

## 相关文件

- 源代码：`src/program/divcalc.c`
- 构建规则：`src/program/Makefile` (第 161-163 行)
- 可执行文件：`objs/divcalc.exe`
- 系统镜像：将被复制到 `tools/v6pp-fs-edit-2022/workspace/programs/bin/divcalc`
