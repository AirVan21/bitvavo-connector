# Use an official Ubuntu base image
FROM ubuntu:24.04

# Set environment variables to avoid interactive prompts during installation
ENV DEBIAN_FRONTEND=noninteractive

# Update package list and install required tools
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    g++ \
    clang \
    clangd \
    libstdc++-13-dev \
    git \
    openssh-client \
    python3-full \
    python3-pip \
    pipx \
    libboost-all-dev \
    libssl-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Add pipx bin to PATH
ENV PATH="/root/.local/bin:$PATH"

# Ensure pipx is properly set up
RUN pipx ensurepath

# Install Conan
RUN pipx install conan==1.64.1

# Set up Conan configuration
RUN conan profile new default --detect && \
    conan profile update settings.compiler.cppstd=20 default && \
    conan profile update settings.arch=armv8 default && \
    conan profile update settings.arch_build=armv8 default && \
    conan profile update settings.compiler=clang default && \
    conan profile update settings.compiler.libcxx=libc++ default

# Set the working directory
WORKDIR /bitvavo-connector

# Set the default command to run the built application
CMD ["bash"]