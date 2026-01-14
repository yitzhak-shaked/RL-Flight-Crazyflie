# RL-Flight-Crazyflie — Project Manual (From Scratch)

This document is the **comprehensive user manual** for this workspace: how to set it up from a clean machine, train policies in simulation, understand the most important folders and configuration files, and deploy a trained policy to a Crazyflie.

> This repo contains both **host-side training/simulation code** (CMake/C++) and **Crazyflie firmware/controller code** (Bitcraze firmware build system under `controller/`).

---

## Table of Contents

- [What this project does](#what-this-project-does)
- [Repository structure (what each folder is for)](#repository-structure-what-each-folder-is-for)
- [Setup from scratch (Ubuntu Linux)](#setup-from-scratch-ubuntu-linux)
  - [Get the code + submodules](#get-the-code--submodules)
  - [Install system dependencies](#install-system-dependencies)
  - [Build host binaries (training/UI)](#build-host-binaries-trainingui)
- [Training in simulation](#training-in-simulation)
  - [Where outputs go (checkpoints is the source of truth)](#where-outputs-go-checkpoints-is-the-source-of-truth)
  - [Run headless training](#run-headless-training)
  - [Run the web UI training](#run-the-web-ui-training)
  - [TensorBoard](#tensorboard)
- [Actors and artifacts (.h5 vs .h)](#actors-and-artifacts-h5-vs-h)
- [Deploying to Crazyflie](#deploying-to-crazyflie)
  - [Prerequisites](#prerequisites)
  - [Native firmware build (recommended)](#native-firmware-build-recommended)
  - [Policy-switching firmware (two actors)](#policy-switching-firmware-two-actors)
  - [Flashing firmware](#flashing-firmware)
  - [Configuring policy switching at runtime](#configuring-policy-switching-at-runtime)
  - [Test flights (trigger script)](#test-flights-trigger-script)
- [Important configuration files](#important-configuration-files)
  - [`src/constants.h`](#srcconstantsh)
  - [`src/config/config.h`](#srcconfigconfigh)
  - [`include/.../parameters/init/default.h`](#includeparametersinitdefaulth)
  - [`include/.../parameters/termination/default.h`](#includeparametersterminationdefaulth)
  - [`include/.../parameters/reward_functions/default.h`](#includeparametersreward_functionsdefaulth)
- [Troubleshooting & common pitfalls](#troubleshooting--common-pitfalls)

---

## What this project does

- Trains **end-to-end neural network flight controllers** for a multirotor in simulation using RLtools.
- Exports trained actors (policies) and deploys them to a **Crazyflie 2.x** by compiling them into the firmware.
- Supports a **policy switching** deployment mode (firmware level): switch between a navigation actor and a hover actor based on distance to target.

---

## Repository structure (what each folder is for)

The most important folders you will interact with:

- `src/` — **Main host-side code** (training loop, UI server, step logic, ablations).
- `include/` — “Library style” simulator headers and parameter presets (init/termination/reward functions).
- `controller/` — **Crazyflie firmware controller module** + build system integration (native firmware builds live here).
- `scripts/` — Python helpers for flight testing and firmware parameter configuration.

Artifacts and outputs:

- `checkpoints/` — **Source of truth for training outputs**.
  - New runs create folders under `checkpoints/multirotor_td3/<run_name>/`.
  - Each run folder typically contains: `actor_*.h5`, `actor_*.h`, `data.tfevents*`, and `training_parameters_summary.txt`.
- `actors/` — A curated place to keep **chosen / known-good actor files** you want to reuse (e.g., for deployment).
  - Think of this as “pinned policies”; training outputs live under `checkpoints/`.
- `build/` — CMake build output for host binaries (e.g. `build/src/training_headless`, `build/src/ui`).
- `deployment_actors/` — Staging directory for deployment: the files that will be compiled into the firmware (`actor.h`, `hover_actor.h`).
- `build_firmware/` — Output directory used by the Docker firmware build script (writes `build_firmware/cf2.bin`).

Other folders you may see:

- `firmware_outputs/` — Prebuilt firmware `.bin` files (useful for quick testing).
- `logs/` — Legacy/old runs; **deprecated** (new training outputs are consolidated into `checkpoints/`).
- `external/` — RLtools submodule and other third-party code.

---

## Setup from scratch (Ubuntu Linux)

This section assumes a clean Ubuntu machine (tested on recent Ubuntu LTS).

### Get the code + submodules

If you are cloning fresh:

```bash
git clone https://github.com/arplaboratory/learning-to-fly learning_to_fly
cd learning_to_fly

# Recommended: bring everything in one go
git submodule update --init --recursive
```

> If you prefer the minimal submodule set described in the original README, you can follow the `external/rl_tools`-focused commands there. `--recursive` is simplest and avoids missing test/UI deps.

### Install system dependencies

For host-side builds (training/UI):

```bash
sudo apt update
sudo apt install -y \
  libhdf5-dev \
  libopenblas-dev \
  protobuf-compiler \
  libprotobuf-dev \
  libboost-all-dev
```

(Optional)

- For TensorBoard: `python3 -m pip install tensorboard`

### Build host binaries (training/UI)

From repo root:

```bash
mkdir -p build
cd build
cmake .. -DCMAKE_BUILD_TYPE=Release -DRL_TOOLS_BACKEND_ENABLE_OPENBLAS:BOOL=ON
cmake --build . -j8
```

Host binaries will be created under `build/src/`.

---

## Training in simulation

### Where outputs go (checkpoints is the source of truth)

New training runs write everything under:

- `checkpoints/multirotor_td3/<run_name>/`

Typical contents:

- `actor_*.h5` — HDF5 checkpoints (used on the host for runtime loading / evaluation)
- `actor_*.h` — C++ header checkpoints (used for deployment / compile-time includes)
- `data.tfevents*` — TensorBoard event logs
- `training_parameters_summary.txt` — auto-generated run summary

### Run headless training

From repo root:

```bash
./build/src/training_headless
```

### Run the web UI training

One-time: fetch UI JS dependencies:

```bash
cd src/ui
./get_dependencies.sh
cd ../..
```

Run the UI server:

```bash
./build/src/ui 0.0.0.0 8000
```

Then open `http://0.0.0.0:8000`.

### TensorBoard

Run TensorBoard over all runs:

```bash
tensorboard --logdir=checkpoints/multirotor_td3
```

Or for a single run:

```bash
tensorboard --logdir=checkpoints/multirotor_td3/<run_name>
```

---

## Actors and artifacts (.h5 vs .h)

This repo uses two main actor formats:

- **`.h5` (HDF5)**
  - Used primarily for **simulation/runtime loading** on the host.
  - Example: `HOVER_ACTOR_PATH` in `src/config/config.h` points to an `.h5` file.

- **`.h` (C/C++ header checkpoint)**
  - Used for **deployment** (compiled into Crazyflie firmware).
  - Also used optionally for **compile-time weight initialization** during training via `ACTOR_CHECKPOINT_INIT_PATH`.

Rule of thumb:

- If you’re deploying to Crazyflie → you want `actor_*.h`.
- If you’re loading/swapping policies at runtime in the simulator/UI → you want `actor_*.h5`.

---

## Deploying to Crazyflie

### Prerequisites

Hardware/software typically required:

- Crazyflie 2.1 + Crazyradio USB dongle
- A position estimation solution (Flow Deck v2 or motion capture)
- Bitcraze Python client tools (`cfloader`, `cfclient`) from `crazyflie-clients-python`

#### Windows + WSL: attaching the Crazyradio dongle

If you are running the tooling inside WSL, you must attach the USB dongle into WSL.

1) On Windows (PowerShell), list USB devices and identify the Crazyradio dongle bus id:

```bash
usbipd list
```

2) Attach the device to WSL (example bus id `1-8`):

```bash
usbipd attach --busid 1-8 --wsl
```

3) In WSL, verify the dongle is visible:

```bash
lsusb
```

If `lsusb` does not show the dongle, re-check the bus id and confirm `usbipd` is installed/enabled on Windows.

#### Recommended: verify radio link with cfclient before flashing

Before deploying or flashing any firmware, verify that your host can reliably connect to the Crazyflie:

1) Start `cfclient` and connect to your URI (example `radio://0/80/2M`).
2) Confirm you can see state estimates / sensor values updating.
3) Only then proceed to flashing and flight testing.

This isolates “radio/USB/permissions” problems early and saves a lot of time when debugging deployment issues.
The connection errors will be printed to the console from which you launched `cfclient` after clicking Connect.

### Native firmware build (recommended)

Native builds are the important workflow here (Docker firmware files are not considered up-to-date).

1) Install ARM toolchain:

```bash
sudo apt update
sudo apt install -y gcc-arm-none-eabi
```

2) Initialize Crazyflie firmware submodule:

```bash
cd controller/external
git submodule update --init --recursive
cd ../..
```

3) Configure target + build:

```bash
cd controller/external/crazyflie_firmware
make cf2_defconfig
cd ../..

make clean
make -j8
```

Output binary:

- `controller/build/cf2.bin`

### Policy-switching firmware (two actors)

Policy switching requires **two actors** compiled into the firmware:

- Navigation actor: `controller/data/actor.h`
- Hover actor: `controller/data/hover_actor.h`

Workflow:

1) Copy your chosen deployment headers into `deployment_actors/`:

```bash
mkdir -p deployment_actors
cp <path/to/nav_actor.h> deployment_actors/actor.h
cp <path/to/hover_actor.h> deployment_actors/hover_actor.h
```

2) Namespace collision note (important):

RLtools-generated checkpoint headers usually share the same namespace, so compiling two headers into one firmware may require renaming the hover actor namespace (see `FIRMWARE_BUILD_VERIFICATION.md` and `POLICY_SWITCHING_DEPLOYMENT.md`).

3) Build with policy switching enabled:

```bash
cd controller
make clean
make ENABLE_POLICY_SWITCHING=1 -j8
```

### Flashing firmware

Flash the binary using Bitcraze `cfloader`:

```bash
# Single actor
sudo cfloader flash controller/build/cf2.bin stm32-fw -w radio://0/80/2M
```

If you built using the Docker firmware script (legacy):

```bash
sudo cfloader flash build_firmware/cf2.bin stm32-fw -w radio://0/80/2M
```

### Configuring policy switching at runtime

Firmware exposes parameters in the `rlt` group:

- `rlt.ps_enable` (0/1)
- `rlt.ps_thresh` (meters)

Use the helper script:

```bash
python3 scripts/configure_policy_switching.py --enable --threshold 0.5 --uri radio://0/80/2M
```

Or disable:

```bash
python3 scripts/configure_policy_switching.py --disable --uri radio://0/80/2M
```

### Test flights (trigger script)

The trigger script provides keyboard dead-man’s switch operation.

Recommended first flight:

```bash
python3 scripts/trigger.py --mode takeoff_and_switch --height 0.3
```

Policy switching mode:

```bash
python3 scripts/trigger.py --mode policy_switching --height 0.3 --ps-threshold 0.5
```

---

## Important configuration files

### `src/constants.h`

Defines global constants used by the simulator/training code.

Key items:

- `TARGET_POSITION_X/Y/Z` — target for position-to-position tasks.
- Obstacle definitions (`OBSTACLES`, `PLANAR_OBSTACLES`) for scenarios with obstacles.

Where it matters:

- Reward functions include these constants (so reward matches the intended target).
- Training-side policy switching distance computations should use the same target.

### `src/config/config.h`

This is the **main training configuration** (hyperparameters + run behavior).

Key items you will likely edit:

- `ACTOR_CHECKPOINT_INIT_PATH`
  - Empty string means random init.
  - If set to a header path, initializes actor weights from that `.h` checkpoint.
- `ENABLE_POLICY_SWITCHING`
  - Enables the training-side switching logic.
- `HOVER_ACTOR_PATH`
  - Path to hover actor `.h5` for runtime loading (simulation/training-side switching).
- `POLICY_SWITCH_THRESHOLD`
  - The distance threshold used in training-side switching.

### `include/.../parameters/init/default.h`

Initial state distributions / presets for the simulator.

Typical use:

- Adjust initial position ranges and curriculum knobs to shape exploration.

### `include/.../parameters/termination/default.h`

Termination conditions for episodes (position/velocity/angular limits).

Typical use:

- Tighten termination for precision hover.
- Loosen termination for long-range position-to-position tasks.

### `include/.../parameters/reward_functions/default.h`

Defines reward function variants (weights/scales for position/orientation/velocity/action penalties).

Typical use:

- Select a reward variant that matches your training goal.
- Verify the reward uses the same target definition as `src/constants.h`.

---

## Troubleshooting & common pitfalls

- **Training outputs not found**: new runs are under `checkpoints/multirotor_td3/<run_name>/` (legacy `logs/` is deprecated).
- **Which actor format do I deploy?** Use `.h` for Crazyflie firmware, `.h5` for simulator/UI runtime loading.
- **Policy switching doesn’t activate**: confirm `rlt.ps_enable=1` and tune `rlt.ps_thresh`.
- **Target mismatch between training and firmware**: ensure the firmware-side `POLICY_SWITCH_TARGET` and training target (in `src/constants.h`) are set consistently.
- **Two-actor build fails**: you likely hit the RLtools namespace collision; see the sed-based namespace fix described in the deployment docs.
