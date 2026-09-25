FROM fedora:39

# 1. Install base utilities, build toolchains, and compilers
RUN dnf install -y \
    git \
    curl \
    wget \
    tar \
    unzip \
    which \
    procps-ng \
    util-linux \
    shadow-utils \
    sudo \
    # Assembly & Linkers
    nasm \
    binutils \
    # C / C++ Toolchain & Debuggers
    gcc \
    gcc-c++ \
    make \
    cmake \
    gdb \
    lldb \
    valgrind \
    # Languages & Runtimes
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
    # Extra developer CLI tools
    jq \
    tmux \
    ripgrep \
    htop \
    && dnf clean all \
    && rm -rf /var/cache/dnf

# 2. Install TypeScript & core node global packages
RUN npm install -g typescript ts-node yarn pnpm

# 3. Create non-root 'vscode' user with passwordless sudo
RUN useradd -m -s /bin/bash vscode \
    && echo "vscode ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers.d/vscode \
    && chmod 0440 /etc/sudoers.d/vscode

USER vscode
WORKDIR /home/vscode