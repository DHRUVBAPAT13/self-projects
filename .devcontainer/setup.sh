#!/usr/bin/env bash
set -e

echo "=== Initializing Workspace Environment ==="

# Verify core tool versions
echo "OS: $(cat /etc/os-release | grep PRETTY_NAME | cut -d= -f2)"
echo "Go: $(go version 2>&1)"
echo "Java: $(javac -version 2>&1)"
echo "Node: $(node -v 2>&1)"
echo "Python: $(python3 --version 2>&1)"
echo ".NET: $(dotnet --version 2>&1)"
echo "GCC: $(gcc --version | head -n1)"
echo "NASM: $(nasm -v 2>&1)"
echo "LD: $(ld -v 2>&1)"

# Install workspace Python packages if requirements exist
if [ -f "requirements.txt" ]; then
    echo "Installing Python dependencies..."
    pip3 install --user -r requirements.txt
fi

# Install workspace Node packages if package.json exists
if [ -f "package.json" ]; then
    echo "Installing Node dependencies..."
    npm install
fi

echo "=== Workspace Setup Complete ==="