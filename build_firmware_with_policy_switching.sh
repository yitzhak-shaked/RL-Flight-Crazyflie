#!/bin/bash
# Policy Switching Firmware Build Script
# Usage: ./build_firmware_with_policy_switching.sh [nav_actor.h] [hover_actor.h]

set -e

# Default actors if not specified
NAV_ACTOR=${1:-"actors/actor_000000000300000.h"}
HOVER_ACTOR=${2:-"actors/hoverActor_000000000300000.h"}

# Colors for output
GREEN='\033[0;32m'
BLUE='\033[0;34m'
RED='\033[0;31m'
NC='\033[0m' # No Color

echo -e "${BLUE}================================================${NC}"
echo -e "${BLUE}Policy Switching Firmware Build${NC}"
echo -e "${BLUE}================================================${NC}"

# Verify actors exist
if [ ! -f "$NAV_ACTOR" ]; then
    echo -e "${RED}Error: Navigation actor not found: $NAV_ACTOR${NC}"
    exit 1
fi

if [ ! -f "$HOVER_ACTOR" ]; then
    echo -e "${RED}Error: Hover actor not found: $HOVER_ACTOR${NC}"
    exit 1
fi

echo -e "${GREEN}✓ Navigation actor: $NAV_ACTOR${NC}"
echo -e "${GREEN}✓ Hover actor: $HOVER_ACTOR${NC}"

# Create deployment directory
mkdir -p deployment_actors
cp "$NAV_ACTOR" deployment_actors/actor.h
cp "$HOVER_ACTOR" deployment_actors/hover_actor.h

echo -e "${GREEN}✓ Actors copied to deployment_actors/${NC}"

# Build firmware with Docker
echo -e "${BLUE}Building firmware with policy switching enabled...${NC}"

docker run --rm -it \
  -v $(pwd)/deployment_actors/actor.h:/controller/data/actor.h \
  -v $(pwd)/deployment_actors/hover_actor.h:/controller/data/hover_actor.h \
  -v $(pwd)/build_firmware:/output \
  -e ENABLE_POLICY_SWITCHING=1 \
  arpllab/learning_to_fly_build_firmware

if [ -f "build_firmware/cf2.bin" ]; then
    echo -e "${GREEN}✓ Firmware built successfully: build_firmware/cf2.bin${NC}"
    echo -e "${BLUE}================================================${NC}"
    echo -e "${BLUE}Next Steps:${NC}"
    echo -e "1. Flash firmware: ${GREEN}sudo cfloader flash build_firmware/cf2.bin stm32-fw -w radio://0/80/2M${NC}"
    echo -e "2. Enable switching: ${GREEN}Set rlt.ps_enable=1 in cfclient${NC}"
    echo -e "3. Test flight: ${GREEN}python scripts/trigger.py --mode takeoff_and_switch --height 0.3${NC}"
    echo -e "${BLUE}================================================${NC}"
else
    echo -e "${RED}Error: Build failed - cf2.bin not found${NC}"
    exit 1
fi
