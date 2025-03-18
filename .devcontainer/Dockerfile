FROM mcr.microsoft.com/devcontainers/base:ubuntu-24.04
# update package list
RUN apt-get update
# install basic dependencies
RUN apt-get install \
    clang llvm \
    libelf-dev libpcap-dev \
    libbpf-dev \
    build-essential \
    linux-tools-$(uname -r) \
    -y
# install kernel headers
RUN apt-get install \
    linux-headers-$(uname -r) \
    -y
# link kernel headers in the right place
RUN ln -s /usr/include/aarch64-linux-gnu/asm /usr/include/asm
# install tools
RUN apt-get install \
    linux-tools-common \
    linux-tools-generic \
    tcpdump \
    xdp-tools \
    netcat-openbsd \
    ethtool \
    iputils-ping \
    -y
# install basic editing tools
RUN apt-get install \
    neovim \
    -y
# clone test env tools and add to path
RUN git clone https://github.com/xdp-project/xdp-tutorial /tutorial
RUN echo $(/tutorial/testenv/testenv.sh alias) >> /root/.bashrc
RUN echo $(/tutorial/testenv/testenv.sh alias) >> /root/.zshrc
