# Policy Switching Deployment Guide

## Overview

This guide explains how to deploy the policy switching system to your Crazyflie 2.1 drone. The system enables seamless switching between two trained actors:
- **Navigation Actor**: For obstacle avoidance and reaching the approximated target position
- **Hover Actor**: For stable hovering at the target position

The switching occurs automatically based on distance to target, with zero latency at the 500Hz control rate.

## Prerequisites

### Required Hardware
- Crazyflie 2.1 nano quadcopter
- Flow Deck v2 (for position estimation)
- Crazyradio USB dongle
- USB cable for the dongle

### Required Software
- Docker (for firmware build)
- WSL (if using Windows for USB access)
- Python 3.x with cfloader installed

### Trained Actors
You need two trained actor checkpoint files:
1. **Navigation Actor**: `actors/navigator_actor_XXXXXXXXX.h` (obstacle avoidance + target reaching)
2. **Hover Actor**: `actors/hover_actor_XXXXXXXXX.h` (trained at origin for hovering)

Example files already in this repository:
- Navigation: `actors/navigator_actor_000000000300000.h`
- Hover: `actors/hover_actor_000000000300000.h`

## System Architecture

### Training Configuration
The training system (in `src/training.h`) uses policy switching during data collection:
- Navigation actor controls when distance > 0.5m
- Hover actor controls when distance < 0.5m
- Observations transformed to be relative to target for hover actor

### Firmware Configuration
The firmware implements the same logic at 500Hz:
```
Distance Calculation: sqrt((x-0.0)² + (y-1.2)² + (z-0.0)²)
                                    ↓
                        Distance < ps_thresh?
                          ↙              ↘
                   Hover Actor      Navigation Actor
                  (at target)      (navigating)
                          ↘              ↙
                    Observation Transform
                    (hover: relative to target)
                              ↓
                      Actor Inference
                              ↓
                      Motor Commands
```

## Step-by-Step Deployment

### 1. Prepare Actor Files

Create a deployment directory with both actors:
```bash
mkdir -p deployment_actors
cp actors/actor_000000000300000.h deployment_actors/actor.h
cp actors/hoverActor_000000000300000.h deployment_actors/hover_actor.h
```

**Important Naming Convention:**
- Navigation actor MUST be named `actor.h`
- Hover actor MUST be named `hover_actor.h`

### 2. Build Firmware with Policy Switching

The Docker build process embeds both actors into the firmware:

```bash
# Build firmware with policy switching enabled
docker run --rm -it \
  -v $(pwd)/deployment_actors/actor.h:/controller/data/actor.h \
  -v $(pwd)/deployment_actors/hover_actor.h:/controller/data/hover_actor.h \
  -v $(pwd)/build_firmware:/output \
  -e ENABLE_POLICY_SWITCHING=1 \
  arpllab/learning_to_fly_build_firmware
```

This will:
1. Mount both actor checkpoint files into the container at `/controller/data/`
2. Set `ENABLE_POLICY_SWITCHING=1` which gets passed to the Makefile as `-DENABLE_POLICY_SWITCHING`
3. Compile firmware with dual-actor support (both actors embedded)
4. Output `cf2.bin` to `build_firmware/` directory (mapped from `/output` in container)

**Build Time**: ~2-5 minutes depending on your system

### 3. Connect Crazyflie

#### On WSL (Windows):
```bash
# Attach USB dongle to WSL
usbipd attach --busid 1-8 --wsl

# Verify connection
lsusb | grep Crazyradio
```

#### On Linux:
```bash
# USB dongle should be automatically detected
lsusb | grep Crazyradio
```

### 4. Flash Firmware

Flash the compiled firmware to your Crazyflie:

```bash
sudo cfloader flash build_firmware/cf2.bin stm32-fw -w radio://0/80/2M
```

**Expected Output:**
```
Restart the Crazyflie you want to bootload in the next
 10 seconds ...
Connected to bootloader on Crazyflie 2.1 (version=0x10)
Target info: nrf51 (0xFE)
Flash pages: 232 | Page size: 1024 | Buffer pages: 1 | Start page: 88
144 KBytes of flash available for firmware image.
Flashing 1 of 1 to stm32 (fw): 142826 bytes (140 pages) ..........
Flashing done!
```

**Flashing Time**: ~30 seconds

### 5. Configure Policy Switching Parameters

After flashing, configure the policy switching behavior via parameters.

#### Using cfclient:

1. Launch cfclient:
   ```bash
   sudo venv/bin/cfclient
   ```

2. Connect to your Crazyflie (radio://0/80/2M)

3. Navigate to **Parameters** tab

4. Find `rlt` group and set:
   - `ps_enable` = 1 (enable policy switching)
   - `ps_thresh` = 0.5 (distance threshold in meters)

#### Using Python Script:

```python
import cflib.crtp
from cflib.crazyflie import Crazyflie
from cflib.crazyflie.syncCrazyflie import SyncCrazyflie

cflib.crtp.init_drivers()

URI = 'radio://0/80/2M/E7E7E7E7E7'

with SyncCrazyflie(URI, cf=Crazyflie(rw_cache='./cache')) as scf:
    cf = scf.cf
    
    # Enable policy switching
    cf.param.set_value('rlt.ps_enable', 1)
    
    # Set threshold to 0.5 meters
    cf.param.set_value('rlt.ps_thresh', 0.5)
    
    print("Policy switching configured!")
```

## Flight Testing

### Safety First
- **Start low**: Begin with target height 0.3m
- **Clear space**: Ensure 3m radius around target position
- **Have cutoff**: Keep hand near emergency stop button
- **Test indoors**: Use the Flow Deck for position estimation

### Test Procedure

#### 1. Basic Hover Test (Single Actor)
First verify the navigation actor works alone:
```bash
cd scripts
python trigger.py --mode hover_learned --height 0.3
```

This should:
- Takeoff smoothly
- Navigate to (0.0, 1.2, 0.3)
- Hover stably
- **Expected**: Slight drift without hover actor

#### 2. Policy Switching Test
Use the dedicated policy switching mode:
```bash
# New policy_switching mode (recommended)
python scripts/trigger.py --mode policy_switching --height 0.3 --ps-threshold 0.5
```

**Alternative** (manual parameter setup):
```bash
# Enable via cfclient or configure_policy_switching.py first
python scripts/configure_policy_switching.py --enable --threshold 0.5
# Then use any learned policy mode
python scripts/trigger.py --mode takeoff_and_switch --height 0.3
```

This should:
- Takeoff with original controller (safe)
- Switch to learned policy after 3 seconds
- **Navigate with navigation actor** (distance > 0.5m)
- **Automatically switch to hover actor** when within 0.5m of target
- Hover stably with minimal drift at (0.0, 1.2, 0.3)
- **Expected**: Smooth transition, no oscillation at threshold

#### 3. Monitor Switching Events

Use cfclient logging to observe switching:

1. Open **Logging** tab in cfclient
2. Add log variables:
   - `rltps.actor` (0=nav, 1=hover)
   - `rltps.dist` (distance to target)
   - `rltps.thresh` (threshold value)
   - `stateEstimate.x/y/z` (position)

3. Start logging and trigger flight

4. **Expected Pattern**:
   ```
   dist: 2.5m, actor: 0 (nav)
   dist: 1.8m, actor: 0 (nav)
   dist: 1.2m, actor: 0 (nav)
   dist: 0.7m, actor: 0 (nav)
   dist: 0.45m, actor: 1 (hover) ← SWITCH!
   dist: 0.30m, actor: 1 (hover)
   dist: 0.25m, actor: 1 (hover)
   ```

## Tuning Parameters

### Threshold Adjustment

The switching threshold can be tuned in real-time via parameter `ps_thresh`:

- **Too large** (e.g., 0.8m):
  - Early switch to hover actor
  - May not reach target precisely
  - More stable hovering region

- **Too small** (e.g., 0.2m):
  - Late switch to hover actor
  - Possible overshoot/oscillation
  - Navigation actor dominates

- **Recommended**: 0.5m (matches training configuration)

### Target Position

The target position is **hardcoded** in firmware:
```c
static const float POLICY_SWITCH_TARGET[3] = {0.0f, 1.2f, 0.0f};
```

To change it:
1. Edit `controller/rl_tools_controller.c` line ~63
2. Rebuild firmware
3. Reflash drone

**Note**: Changing target requires retraining hover actor at new position!

## Troubleshooting

### Issue: Firmware won't flash
**Symptoms**: `cfloader` can't connect to bootloader
**Solutions**:
- Reset Crazyflie manually (power cycle)
- Try `--force` flag: `sudo cfloader flash build_firmware/cf2.bin stm32-fw -w radio://0/80/2M --force`
- Check USB dongle connection: `lsusb | grep Crazyradio`

### Issue: Actor doesn't switch
**Symptoms**: `rltps.actor` stays at 0 or 1
**Solutions**:
- Verify `ps_enable = 1` via cfclient parameters
- Check `ps_thresh` value (should be 0.5)
- Monitor `rltps.dist` - does it cross threshold?
- Verify firmware built with `ENABLE_POLICY_SWITCHING=1`

### Issue: Oscillation at threshold
**Symptoms**: Actor switches rapidly back/forth
**Solutions**:
- Increase threshold: `ps_thresh = 0.6`
- Add hysteresis in future implementation
- Check if hover actor is properly trained

### Issue: Drift increases after switching
**Symptoms**: Position error grows when actor=1
**Solutions**:
- Verify hover actor file is correctly named `hover_actor.h`
- Check hover actor was trained at origin (0,0,0)
- Verify observation transformation in `update_state()`
- Confirm target position matches training setup

### Issue: Build fails with "hover_actor.h not found"
**Symptoms**: Compilation error during Docker build
**Solutions**:
- Ensure `hover_actor.h` is mounted correctly
- Check Docker volume paths match your filesystem
- Verify file exists: `ls deployment_actors/hover_actor.h`
- Verify `ENABLE_POLICY_SWITCHING=1` is set in docker run command

### Issue: Policy switching not compiled in
**Symptoms**: Parameters `rlt.ps_enable` and `rlt.ps_thresh` don't exist in cfclient
**Solutions**:
- Verify Docker build command included `-e ENABLE_POLICY_SWITCHING=1`
- Check build logs for `-DENABLE_POLICY_SWITCHING` in compiler flags
- Rebuild firmware with correct environment variable
- The Makefile should show: `EXTRA_CFLAGS += -DENABLE_POLICY_SWITCHING`

## Advanced Configuration

### Disable Policy Switching

To revert to single-actor control:
```python
cf.param.set_value('rlt.ps_enable', 0)
```
Or in cfclient: set `rlt.ps_enable` = 0

The firmware will use `rl_tools_control()` (navigation actor only).

### Custom Observation Transforms

The hover actor receives observations relative to the target position.
This transformation is in `update_state()`:

```c
if(ps_enable && current_actor == 1){
  // Transform: position relative to POLICY_SWITCH_TARGET
  ref_pos[0] = POLICY_SWITCH_TARGET[0];
  ref_pos[1] = POLICY_SWITCH_TARGET[1];
  ref_pos[2] = POLICY_SWITCH_TARGET[2];
}
```

Modify this logic if your hover actor expects different observations.

### Multiple Target Positions

For multiple targets, you can:
1. Add a `target_index` parameter
2. Use array of target positions
3. Switch `POLICY_SWITCH_TARGET` based on `target_index`

Example modification:
```c
static const float TARGETS[][3] = {
  {0.0f, 1.2f, 0.0f},
  {1.0f, 1.0f, 0.0f},
  {-0.5f, 0.8f, 0.0f}
};
static uint8_t target_index = 0;

// In distance calculation:
float dx = state->position.x - TARGETS[target_index][0];
...
```

## Performance Metrics

### Computation Time
- Actor inference: ~100-200µs per call
- Distance calculation: ~10µs
- Observation transform: ~5µs
- **Total overhead**: ~220µs (11% of 2ms control cycle)

### Memory Usage
- Navigation actor: ~15-20KB
- Hover actor: ~15-20KB
- Buffers: ~2KB per actor
- **Total RAM**: ~40KB (STM32 has 128KB)

### Switching Latency
- Decision: **immediate** (single if-statement)
- No smoothing or interpolation
- Clean switch at 500Hz rate
- **Zero additional latency**

## Integration with Existing Scripts

### Python Trigger Script

The `scripts/trigger.py` has been updated with a dedicated policy switching mode:

```bash
# NEW: Policy switching mode (recommended)
python scripts/trigger.py --mode policy_switching --height 0.3 --ps-threshold 0.5
```

This mode:
- Automatically sets `rlt.ps_enable=1` and `rlt.ps_thresh=0.5`
- Takes off with original controller (safe)
- Switches to learned policy after timeout
- Learned policy automatically switches between actors

**Existing modes can also use policy switching** if you set parameters first:
```bash
# Set parameters via configure_policy_switching.py
python scripts/configure_policy_switching.py --enable --threshold 0.5

# Then use any mode:
python scripts/trigger.py --mode takeoff_and_switch --height 0.3  # Works with switching
python scripts/trigger.py --mode hover_learned --height 0.3        # Works with switching
python scripts/trigger.py --mode trajectory_tracking --height 0.3  # Works with switching
```

**Mode Comparison:**

| Mode | Policy Switching | Description |
|------|-----------------|-------------|
| `policy_switching` | ✅ Auto-enabled | **Recommended**: Sets parameters automatically |
| `takeoff_and_switch` | ⚙️ Manual setup | Works if ps_enable=1 set beforehand |
| `hover_learned` | ⚙️ Manual setup | Works if ps_enable=1 set beforehand |
| `trajectory_tracking` | ⚙️ Manual setup | Works if ps_enable=1 set beforehand |
| `hover_original` | ❌ Not applicable | Uses original controller only |

### Gamepad Control

You can also trigger flights with a gamepad:
```bash
python scripts/gamepad_trigger.py
```

Press button to takeoff, policy switching activates automatically.

## Updating Actors

To update actors (e.g., after additional training):

1. Train new actors and save checkpoints
2. Copy new `.h` files to `deployment_actors/`
3. Rebuild firmware with updated actors
4. Reflash Crazyflie

**No code changes needed** - just replace the actor files.

## Backup and Rollback

### Backup Current Firmware
```bash
# Flash doesn't support read-back from Crazyflie
# But you can keep your cf2.bin files
cp build_firmware/cf2.bin backups/cf2_policy_switching_$(date +%Y%m%d).bin
```

### Rollback to Single Actor
```bash
# Build firmware with only navigation actor
docker run --rm -it \
  -v $(pwd)/actors/actor_000000000300000.h:/learning_to_fly/controller/data/actor.h \
  -v $(pwd)/build_firmware:/learning_to_fly/build_firmware \
  arpllab/learning_to_fly_build_firmware

# Flash
sudo cfloader flash build_firmware/cf2.bin stm32-fw -w radio://0/80/2M
```

## Next Steps

After successful deployment:

1. **Collect Flight Data**: Use logging to analyze performance
2. **Fine-tune Threshold**: Adjust `ps_thresh` based on flight behavior
3. **Train with Better Rewards**: Use deployment insights to improve training
4. **Add Hysteresis**: Prevent rapid switching at threshold boundary
5. **Multiple Targets**: Extend to waypoint navigation with switching

## References

- Training Configuration: `src/config/config.h`
- Policy Switching Logic: `src/policy_switching.h`
- Firmware Controller: `controller/rl_tools_controller.c`
- Adapter Implementation: `controller/rl_tools_adapter.cpp`
- Main README: `README.MD`
- Deployment Notes: `CrazyCheetSheet.txt`

## Support

For issues:
1. Check this guide's Troubleshooting section
2. Review firmware logs via cfclient
3. Verify training configuration matches deployment
4. Test with single actor first (`ps_enable=0`)

## Summary

**Deployment Checklist:**
- [ ] Two trained actors ready (actor.h, hover_actor.h)
- [ ] Docker firmware build completed
- [ ] Firmware flashed to Crazyflie
- [ ] Policy switching enabled (`ps_enable=1`)
- [ ] Threshold configured (`ps_thresh=0.5`)
- [ ] Basic hover test passed
- [ ] Policy switching test passed
- [ ] Logging verified (actor transitions visible)
- [ ] Flight testing complete

**Expected Behavior:**
- Smooth takeoff with navigation actor
- Automatic switch to hover actor at 0.5m from target
- Stable hovering at target position
- No oscillation or instability
- Clean actor transitions without jumps

**Key Advantages:**
- Zero latency switching (500Hz)
- No code modification needed for actor updates
- Runtime tunable threshold
- Full logging and debugging support
- Backward compatible (can disable switching)

Enjoy your policy-switching Crazyflie! 🚁
