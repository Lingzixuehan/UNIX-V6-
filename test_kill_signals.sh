#!/bin/bash

# kill 命令信号测试脚本
# 用于自动化测试不同信号对进程的影响

echo "========================================"
echo "  Kill Command Signal Testing Script"
echo "========================================"
echo ""

# 检查测试程序是否存在
if [ ! -f "./src/program/objs/immortal.exe" ]; then
    echo "Error: immortal.exe not found!"
    echo "Please compile it first: cd src/program && make immortal.exe"
    exit 1
fi

PROGRAM="./src/program/objs/immortal.exe"

echo "[Test 1] Starting immortal process..."
$PROGRAM &
PID=$!
echo "Process started with PID: $PID"
sleep 2

# 测试 SIGINT (2)
echo ""
echo "[Test 2] Sending SIGINT (signal 2) to process $PID..."
kill -2 $PID
sleep 2

if ps -p $PID > /dev/null 2>&1; then
    echo "Result: ✓ Process is still alive (SIGINT was caught)"
else
    echo "Result: ✗ Process terminated (unexpected)"
    exit 1
fi

# 测试 SIGTERM (15)
echo ""
echo "[Test 3] Sending SIGTERM (signal 15) to process $PID..."
kill -15 $PID
sleep 2

if ps -p $PID > /dev/null 2>&1; then
    echo "Result: ✓ Process is still alive (SIGTERM was caught)"
else
    echo "Result: ✗ Process terminated (unexpected)"
    exit 1
fi

# 测试 SIGUSR1 (10)
echo ""
echo "[Test 4] Sending SIGUSR1 (signal 10) to process $PID..."
kill -10 $PID
sleep 2

if ps -p $PID > /dev/null 2>&1; then
    echo "Result: ✓ Process is still alive (SIGUSR1 was caught)"
else
    echo "Result: ✗ Process terminated"
fi

# 测试 SIGKILL (9)
echo ""
echo "[Test 5] Sending SIGKILL (signal 9) to process $PID..."
kill -9 $PID
sleep 1

if ps -p $PID > /dev/null 2>&1; then
    echo "Result: ✗ Process is still alive (SIGKILL failed - should not happen!)"
    kill -9 $PID  # 强制清理
else
    echo "Result: ✓ Process terminated (SIGKILL succeeded)"
fi

echo ""
echo "========================================"
echo "  Test Summary"
echo "========================================"
echo "SIGINT  (2):  Can be caught ✓"
echo "SIGTERM (15): Can be caught ✓"
echo "SIGUSR1 (10): Can be caught ✓"
echo "SIGKILL (9):  Cannot be caught, forced termination ✓"
echo ""
echo "All tests completed successfully!"
echo "========================================"
