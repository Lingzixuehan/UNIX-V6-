#!/bin/bash

# ESP定位验证实验 - 一键运行脚本

echo "================================"
echo "编译测试程序..."
echo "================================"
echo ""

# 编译测试程序
g++ -o test_esp test_esp_basic.cpp -m32 2>/dev/null

# 如果32位编译失败，尝试64位
if [ $? -ne 0 ]; then
    echo "注意: 32位编译失败，尝试64位编译..."
    g++ -o test_esp test_esp_basic.cpp

    if [ $? -ne 0 ]; then
        echo "错误: 编译失败"
        echo "请确保已安装g++编译器"
        echo ""
        echo "Ubuntu/Debian: sudo apt-get install g++ g++-multilib"
        echo "CentOS/RHEL:   sudo yum install gcc-c++"
        exit 1
    fi
fi

echo "✓ 编译成功"
echo ""
echo "================================"
echo "运行测试..."
echo "================================"
echo ""

# 运行测试
./test_esp

# 保存退出码
EXIT_CODE=$?

echo ""
echo "================================"
echo "测试完成"
echo "================================"
echo ""

if [ $EXIT_CODE -eq 0 ]; then
    echo "✓ 所有测试通过！"
    echo ""
    echo "你已经验证了ESP定位User结构的核心原理。"
    echo ""
    echo "接下来可以："
    echo "  1. 查看详细设计文档: cat ../ESP_USER_LOOKUP_DESIGN.md"
    echo "  2. 查看代码示例: ls ../implementation/"
    echo "  3. 了解更多实验: cat ../VERIFICATION_EXPERIMENTS.md"
else
    echo "✗ 测试失败"
    echo "请检查输出中的错误信息"
fi

echo ""
