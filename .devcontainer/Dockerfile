FROM mcr.microsoft.com/devcontainers/base:ubi9

# Install Assembly, Linkers, C/C++, Java, Go, Python, and Developer utilities via dnf
RUN dnf install -y \
    nasm \
    binutils \
    gcc \
    gcc-c++ \
    make \
    cmake \
    gdb \
    golang \
    java-latest-openjdk-devel \
    maven \
    gradle \
    dotnet-sdk-8.0 \
    python3 \
    python3-pip \
    python3-devel \
    nodejs \
    npm \
    jq \
    tmux \
    ripgrep \
    htop \
    && dnf clean all \
    && rm -rf /var/cache/dnf

# Install TypeScript globally
RUN npm install -g typescript ts-node yarn pnpm