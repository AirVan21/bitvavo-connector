# Use an official Ubuntu base image
FROM ubuntu:22.04

# Set environment variables to avoid interactive prompts during installation
ENV DEBIAN_FRONTEND=noninteractive

# Update package list and install required tools
RUN apt-get update && apt-get install -y \
    build-essential \
    cmake \
    g++ \
    clang \
    libstdc++-12-dev \
    git \
    libboost-all-dev \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

CMD ["bash"]