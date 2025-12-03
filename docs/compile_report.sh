#!/bin/bash

# LaTeX实验报告编译脚本

echo "======================================"
echo "编译ESP定位User结构实验报告"
echo "======================================"
echo ""

REPORT_FILE="experiment_report"

# 检查是否安装了xelatex
if ! command -v xelatex &> /dev/null; then
    echo "错误: 未找到xelatex编译器"
    echo ""
    echo "请安装TeX Live:"
    echo "  Ubuntu/Debian: sudo apt-get install texlive-xetex texlive-lang-chinese"
    echo "  CentOS/RHEL:   sudo yum install texlive texlive-xetex"
    echo "  macOS:         brew install --cask mactex"
    echo ""
    exit 1
fi

# 编译LaTeX文档（需要编译2-3次以生成目录和交叉引用）
echo "第1次编译..."
xelatex -interaction=nonstopmode ${REPORT_FILE}.tex > /dev/null 2>&1

echo "第2次编译（生成目录）..."
xelatex -interaction=nonstopmode ${REPORT_FILE}.tex > /dev/null 2>&1

echo "第3次编译（更新交叉引用）..."
xelatex -interaction=nonstopmode ${REPORT_FILE}.tex > /dev/null 2>&1

# 检查是否成功生成PDF
if [ -f "${REPORT_FILE}.pdf" ]; then
    echo ""
    echo "✓ 编译成功!"
    echo ""
    echo "生成的文件:"
    echo "  - ${REPORT_FILE}.pdf (实验报告)"
    echo ""

    # 显示文件大小
    FILE_SIZE=$(du -h "${REPORT_FILE}.pdf" | cut -f1)
    echo "PDF大小: ${FILE_SIZE}"

    # 清理辅助文件
    echo ""
    read -p "是否清理编译产生的辅助文件? (y/n) " -n 1 -r
    echo ""
    if [[ $REPLY =~ ^[Yy]$ ]]; then
        rm -f ${REPORT_FILE}.aux ${REPORT_FILE}.log ${REPORT_FILE}.out \
              ${REPORT_FILE}.toc ${REPORT_FILE}.lof ${REPORT_FILE}.lot
        echo "✓ 辅助文件已清理"
    fi

    echo ""
    echo "查看PDF: evince ${REPORT_FILE}.pdf"
    echo "或使用你喜欢的PDF阅读器打开"

else
    echo ""
    echo "✗ 编译失败"
    echo ""
    echo "请查看日志文件: ${REPORT_FILE}.log"
    echo "或手动编译: xelatex ${REPORT_FILE}.tex"
    exit 1
fi

echo ""
echo "======================================"
