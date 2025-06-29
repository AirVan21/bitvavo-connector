# bitvavo-connector

```bash
docker build -t bitvavo-connector .
```
## Build Command
This command builds a Docker image named 'bitvavo-connector' using the Dockerfile in the current directory.

```bash
docker run -it --name bitvavo-connector -v $(pwd):/bitvavo-connector bitvavo-connector
```
## Run Command
This command runs a container named 'bitvavo-connector' from the 'bitvavo-connector' image.
It mounts the current directory ($(pwd)) to '/bitvavo-connector' inside the container.
The '-it' flag enables interactive mode with a terminal.