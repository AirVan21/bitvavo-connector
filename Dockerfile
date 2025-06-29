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
    python3 \
    python3-pip \
    && apt-get clean \
    && rm -rf /var/lib/apt/lists/*

# Install Conan
RUN pip3 install conan

# Set up Conan configuration
RUN conan profile new default --detect && conan profile update settings.compiler.libcxx=libstdc++11 default

# Set the working directory
WORKDIR /bitvavo-connector

# Create a directory for dependencies and build
RUN mkdir -p build && cd build

# Install dependencies using Conan
RUN conan install .. --build=missing

# Build the project
#RUN cmake .. && make

# Set the default command to run the built application
CMD ["bash"]