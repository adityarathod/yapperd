FROM mcr.microsoft.com/devcontainers/base:ubuntu-24.04
# update package list
RUN apt update
# install basic dependencies
RUN apt install \
    clang llvm \
    libelf-dev libpcap-dev \
    libbpf-dev \
    build-essential \
    linux-tools-$(uname -r) \
    -y
# install kernel headers
RUN apt install \
    linux-headers-$(uname -r) \
    -y
# link kernel headers in the right place
RUN ln -s /usr/include/aarch64-linux-gnu/asm /usr/include/asm
# install tools
RUN apt install \
    linux-tools-common \
    linux-tools-generic \
    tcpdump \
    xdp-tools \
    netcat-openbsd \
    -y
# install basic editing tools
RUN apt install \
    neovim \
    -y
