FROM rockylinux:9

# 1. Enable EPEL and CRB (CodeReady Builder) for extra dev packages
RUN dnf install -y epel-release \
    && dnf config-manager --set-enabled crb \
    && dnf clean all \
    && dnf makecache

# 2. Install toolchain with --allowerasing to resolve the curl-minimal conflict cleanly
RUN dnf install -y --allowerasing \
    git \
    curl \
    wget \
    tar \
    unzip \
    which \
    sudo \
    procps-ng \
    util-linux \
    shadow-utils \
    # Assembly & Linkers
    nasm \
    binutils \
    # Compilers & Build Tools
    gcc \
    gcc-c++ \
    make \
    cmake \
    gdb \
    # Languages & Runtimes
    golang \
    java-21-openjdk-devel \
    maven \
    dotnet-sdk-8.0 \
    python3 \
    python3-pip \
    python3-devel \
    nodejs \
    npm \
    # Utilities
    gedit \
    tmux \
    htop \
    ripgrep \
    && dnf clean all \
    && rm -rf /var/cache/dnf

# 3. Global Node tooling
RUN npm install -g typescript ts-node yarn pnpm

# 4. Install Starship prompt
RUN curl -sS https://starship.rs/install.sh | sh -s -- -y

# 5. Create vscode user
RUN useradd -m -s /bin/bash vscode \
    && echo "vscode ALL=(ALL) NOPASSWD:ALL" >> /etc/sudoers.d/vscode \
    && chmod 0440 /etc/sudoers.d/vscode

USER vscode
WORKDIR /home/vscode

RUN echo 'eval "$(starship init bash)"' >> ~/.bashrc \
    && echo 'alias ls="ls --color=auto"' >> ~/.bashrc \
    && echo 'alias ll="ls -lah --color=auto"' >> ~/.bashrc