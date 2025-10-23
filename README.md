# Bitvavo Connector

A C++ WebSocket client for connecting to the Bitvavo cryptocurrency exchange API.

## Building the Application

### Using Docker

```bash
docker build -t bitvavo-connector .
```

This command builds a Docker image named 'bitvavo-connector' using the Dockerfile in the current directory.

### Using CMake (Local Build)

```bash
mkdir build && cd build
cmake ..
make
```

## Running the Application

### Using Docker

```bash
docker run -it --name bitvavo-connector -v $(pwd):/bitvavo-connector bitvavo-connector
```

This command runs a container named 'bitvavo-connector' from the 'bitvavo-connector' image.
It mounts the current directory ($(pwd)) to '/bitvavo-connector' inside the container.
The '-it' flag enables interactive mode with a terminal.

### Running Locally

```bash
./BitvavoConnector
```

## Features

- WebSocket connection to Bitvavo API
- SSL/TLS encryption
- Asynchronous message handling
- Error handling and connection management