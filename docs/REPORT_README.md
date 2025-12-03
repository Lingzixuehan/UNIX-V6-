# ESP定位User结构实验报告

## 📄 文件说明

- **experiment_report.tex** - 完整的LaTeX实验报告源文件
- **compile_report.sh** - 一键编译脚本
- **REPORT_README.md** - 本说明文件

## 📋 报告内容

本实验报告完整记录了"使用ESP寄存器定位User结构"的设计与验证过程，包括：

### 报告结构
1. **摘要** - 研究概述和关键词
2. **引言** - 研究背景、目的、实验环境
3. **理论分析** - 传统方法、ESP定位原理、数学证明
4. **实验设计** - 3个层次的实验方案
5. **实验实施** - 代码实现和运行方法
6. **实验结果** - 详细的测试数据和表格
7. **性能分析** - 理论分析和实测对比
8. **讨论** - 优缺点、适用场景、改进方向
9. **结论** - 实验总结、创新点、实际意义
10. **附录** - 完整代码、原始数据、设计文档

### 特点
- ✅ 专业的学术论文格式
- ✅ 完整的数学公式和证明
- ✅ 详细的实验数据表格
- ✅ 清晰的图示和代码
- ✅ 丰富的参考文献
- ✅ 完整的附录内容

## 🛠️ 编译方法

### 方法1：使用编译脚本（推荐）

```bash
cd /home/user/UNIX-V6-/docs
./compile_report.sh
```

脚本会：
- 检查LaTeX环境
- 自动编译3次（生成目录和交叉引用）
- 生成PDF文件
- 可选清理辅助文件

### 方法2：手动编译

```bash
cd /home/user/UNIX-V6-/docs

# 编译3次
xelatex experiment_report.tex
xelatex experiment_report.tex
xelatex experiment_report.tex

# 查看PDF
evince experiment_report.pdf
```

## 📦 安装LaTeX环境

### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install texlive-xetex texlive-lang-chinese texlive-latex-extra
```

### CentOS/RHEL
```bash
sudo yum install texlive texlive-xetex texlive-latex
```

### macOS
```bash
brew install --cask mactex
```

### Windows
下载并安装 [TeX Live](https://www.tug.org/texlive/) 或 [MiKTeX](https://miktex.org/)

## 📊 报告统计

- **页数**: 约20-25页
- **字数**: 约12,000字
- **图表**: 10+个表格和图示
- **代码**: 多个完整代码示例
- **参考文献**: 10篇

## 🎨 格式特点

### 使用的LaTeX包
- **ctex** - 中文支持
- **listings** - 代码高亮
- **amsmath** - 数学公式
- **booktabs** - 专业表格
- **hyperref** - 超链接和书签
- **algorithm** - 算法伪代码

### 代码高亮
- C++代码语法高亮
- x86汇编代码高亮
- Bash脚本高亮
- 行号和边框

### 数学公式
完整的数学推导和证明，包括：
- 对齐条件公式
- 掩码计算公式
- 性能提升计算

## 📖 如何使用

### 1. 直接使用
如果你只需要阅读，编译生成PDF即可：
```bash
./compile_report.sh
```

### 2. 修改定制
如果需要修改报告内容：
1. 用任何文本编辑器打开 `experiment_report.tex`
2. 修改需要的部分（如姓名、学号、实验数据等）
3. 保存后重新编译

### 3. 填写个人信息
在文件开头修改以下内容：
```latex
\author{
    实验者：[替换为你的姓名] \\
    学号：[替换为你的学号] \\
    指导教师：[替换为教师姓名] \\
}
```

### 4. 添加实验数据
如果你进行了额外的实验，可以在相应章节添加数据和表格。

## 🎓 学术用途

本报告适用于：
- 操作系统课程实验报告
- 计算机系统课程论文
- 本科毕业设计
- 研究生课程作业
- 学术会议投稿（需要进一步精简和调整）

## 📝 修改建议

### 基本修改
```latex
% 修改标题
\title{\textbf{你的标题}}

% 修改作者信息
\author{你的姓名}

% 修改日期
\date{2025年X月X日}
```

### 添加图片
```latex
\begin{figure}[h]
\centering
\includegraphics[width=0.8\textwidth]{your_image.png}
\caption{图片说明}
\label{fig:your-label}
\end{figure}
```

### 添加表格
```latex
\begin{table}[h]
\centering
\begin{tabular}{ccc}
\toprule
列1 & 列2 & 列3 \\
\midrule
数据1 & 数据2 & 数据3 \\
\bottomrule
\end{tabular}
\caption{表格说明}
\label{tab:your-label}
\end{table}
```

## 🔧 常见问题

### Q1: 编译报错"Font not found"
**A:** 安装中文字体支持：
```bash
sudo apt-get install fonts-wqy-microhei fonts-wqy-zenhei
```

### Q2: 编译报错"Package not found"
**A:** 安装完整的TeX Live：
```bash
sudo apt-get install texlive-full
```

### Q3: 目录和交叉引用没有更新
**A:** 需要编译多次（至少2次）：
```bash
xelatex experiment_report.tex
xelatex experiment_report.tex
```

### Q4: 想要英文版本
**A:** 修改以下内容：
- 移除 `\usepackage{ctex}`
- 将 `\documentclass[a4paper,12pt]{article}` 改为英文类
- 翻译所有中文内容

### Q5: 想要双栏格式（适合会议论文）
**A:** 修改文档类：
```latex
\documentclass[a4paper,12pt,twocolumn]{article}
```

## 📚 相关文档

本报告基于以下文档：
- **ESP_USER_LOOKUP_DESIGN.md** - 完整设计文档
- **implementation/README.md** - 实现指南
- **VERIFICATION_EXPERIMENTS.md** - 实验方案
- **experiments/test_esp_basic.cpp** - 测试代码

## 📧 反馈与建议

如有问题或建议，请：
1. 查看LaTeX编译日志文件 `.log`
2. 参考TeX Live文档
3. 提交Issue到项目仓库

## ⚖️ 许可证

本报告模板采用 MIT License，可自由使用和修改。

---

**祝你实验报告顺利完成！** 🎉
