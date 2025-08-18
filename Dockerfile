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
    python3 \
    python3-pip \
    libboost-all-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Install Conan
RUN pip3 install conan==1.64.1

# Set up Conan configuration
RUN conan profile new default --detect && \
    conan profile update settings.compiler.cppstd=20 default && \
    conan profile update settings.arch=armv8 default && \
    conan profile update settings.arch_build=armv8 default && \
    conan profile update settings.compiler=gcc default

# Set the working directory
WORKDIR /bitvavo-connector

# Set the default command to run the built application
CMD ["bash"]