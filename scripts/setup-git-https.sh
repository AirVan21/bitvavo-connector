#!/bin/bash
# Script to switch git remote from SSH to HTTPS for Docker container usage
# This allows pushing using a personal access token instead of SSH keys

set -e

GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

echo -e "${GREEN}Switching git remote to HTTPS...${NC}"

# Change remote from SSH to HTTPS
git remote set-url origin https://github.com/AirVan21/bitvavo-connector.git

echo -e "${GREEN}Remote URL updated to HTTPS.${NC}"
echo -e "${YELLOW}To push, you'll need to:${NC}"
echo -e "${YELLOW}  1. Create a GitHub Personal Access Token (PAT)${NC}"
echo -e "${YELLOW}     https://github.com/settings/tokens${NC}"
echo -e "${YELLOW}  2. Use it as password when git prompts for credentials${NC}"
echo -e "${YELLOW}  3. Or configure credential helper:${NC}"
echo -e "${YELLOW}     git config --global credential.helper store${NC}"
echo ""
echo -e "${GREEN}To switch back to SSH, run:${NC}"
echo -e "${GREEN}  git remote set-url origin git@github.com:AirVan21/bitvavo-connector.git${NC}"
