FROM archlinux:latest

# 1. Update pacman keys, system packages, and install VS Code Server essentials
RUN pacman -Syu --noconfirm && \
    pacman -S --noconfirm \
    base-devel \
    git \
    curl \
    wget \
    tar \
    gzip \
    which \
    procps-ng \
    sudo \
    shadow \
    bash \
    zsh \
    # Assembly & Linkers
    nasm \
    binutils \
    # Compilers & Debugging
    gcc \
    gdb \
    make \
    cmake \
    # Runtimes & Languages
    go \
    jdk-openjdk \
    maven \
    gradle \
    dotnet-sdk \
    python \
    python-pip \
    nodejs \
    npm \
    # Extra developer CLI tools
    gedit \
    ripgrep \
    tmux \
    htop \
    && pacman -Scc --noconfirm

# 2. Install TypeScript & core node global tools
RUN npm install -g typescript ts-node yarn pnpm

# 3. Create non-root 'vscode' user with passwordless sudo
RUN useradd -m -s /bin/bash vscode && \
    echo "vscode ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers.d/vscode && \
    chmod 0440 /etc/sudoers.d/vscode

USER vscode
WORKDIR /home/vscode