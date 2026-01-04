# Bitvavo Connector

A C++ WebSocket client for connecting to the Bitvavo cryptocurrency exchange API.

## Cloning

This project uses [connector-network](https://github.com/AirVan21/connector-network) as a submodule.

```bash
git clone --recursive git@github.com:AirVan21/bitvavo-connector.git
```

Or if already cloned:
```bash
git submodule update --init --recursive
```

## Building the Application

### Using Docker

```bash
docker build -t bitvavo-connector .
docker run -it -v $(pwd):/bitvavo-connector bitvavo-connector
# Inside container
mkdir build && cd build && cmake .. && cmake --build .
```

### Using CMake (Local Build)

```bash
mkdir build && cd build
cmake ..
cmake --build .
```

## Running the Application

```bash
./build/BitvavoConnector
```

## Features

- WebSocket connection to Bitvavo API
- SSL/TLS encryption
- Asynchronous message handling
- Error handling and connection management