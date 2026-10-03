#!/usr/bin/env bash
set -e

echo "=== Initializing Rocky Linux Workspace ==="

# Display OS identification
cat /etc/os-release | grep -E "(NAME|VERSION)="

# Verify toolchains
echo "NASM:   $(nasm -v 2>&1)"
echo "LD:     $(ld -v 2>&1)"
echo "GCC:    $(gcc --version | head -n1)"
echo "Go:     $(go version 2>&1)"
echo "Java:   $(javac -version 2>&1)"
echo ".NET:   $(dotnet --version 2>&1)"
echo "Node:   $(node -v 2>&1)"
echo "Python: $(python3 --version 2>&1)"

if [ -f "requirements.txt" ]; then
    pip3 install --user -r requirements.txt
fi

if [ -f "package.json" ]; then
    npm install
fi

echo "=== Environment Ready! ==="