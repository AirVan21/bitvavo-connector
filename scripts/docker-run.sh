#!/bin/bash
# Helper script to run Docker container with SSH keys for git push support

set -e

# Colors for output
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Starting Docker container with git push support...${NC}"

# Check if SSH key exists
SSH_KEY="${HOME}/.ssh/id_rsa"
if [ ! -f "$SSH_KEY" ]; then
    echo -e "${YELLOW}Warning: SSH key not found at $SSH_KEY${NC}"
    echo -e "${YELLOW}You can either:${NC}"
    echo -e "${YELLOW}  1. Generate one with: ssh-keygen -t rsa -b 4096 -C 'your_email@example.com'${NC}"
    echo -e "${YELLOW}  2. Use HTTPS method instead (see README)${NC}"
    echo ""
fi

# Build the image if it doesn't exist
if ! docker image inspect bitvavo-connector &>/dev/null; then
    echo "Building Docker image..."
    docker build -t bitvavo-connector .
fi

# Run container with SSH keys mounted
docker run -it \
    -v "$(pwd):/bitvavo-connector" \
    -v "${HOME}/.ssh:/root/.ssh:ro" \
    -v "${SSH_AUTH_SOCK}:/ssh-agent" \
    -e SSH_AUTH_SOCK=/ssh-agent \
    bitvavo-connector
