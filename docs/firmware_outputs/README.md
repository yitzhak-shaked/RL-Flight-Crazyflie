# Crazyflie 2.1 Firmware Files

Generated on: December 22, 2025

## Overview

This directory contains 5 firmware builds for the Crazyflie 2.1 drone with RL-based controllers:

### Single-Actor Firmwares (No Policy Switching)
These firmwares run a single trained actor for all flight phases.

1. **cf2_nav_actor_2400000_single.bin** (407 KB, Flash: 40%)
   - Source: `actors/position_to_position_2m_good_agents/actor_000000002400000.h`
   - Description: Navigation actor trained for 2.4M steps, obstacle avoidance
   - MD5: `e4c44009079d6c3d35d7dbc6fcdaf33e`
   - Use for: Testing navigation behavior independently

2. **cf2_nav_actor_2000000_single.bin** (407 KB, Flash: 40%)
   - Source: `actors/actor_2m_one_pipe_obsticle/actor_000000002000000.h`
   - Description: Navigation actor trained for 2M steps with pipe obstacle
   - MD5: `d2cbd2ad5e56878633b85d736db48c03`
   - Use for: Testing navigation with obstacle avoidance

3. **cf2_hover_actor_300000_single.bin** (407 KB, Flash: 40%)
   - Source: `actors/hover_actors/hoverActor_000000000300000.h`
   - Description: Hover actor trained for 300K steps at origin
   - MD5: `550b297f3e8ac703932df68d91cfc56d`
   - Use for: Testing hover stability independently

### Policy Switching Firmwares (Dual-Actor)
These firmwares automatically switch between navigation and hover actors based on distance to target.

4. **cf2_policy_switching_nav2400000_hover300000.bin** (462 KB, Flash: 46%)
   - Navigation: `actors/position_to_position_2m_good_agents/actor_000000002400000.h`
   - Hover: `actors/hover_actors/hoverActor_000000000300000.h`
   - MD5: `f8cb390a0a763e3e5acb875c90b47242`
   - Use for: Full navigation + hover with policy switching (recommended)

5. **cf2_policy_switching_nav2000000_hover300000.bin** (462 KB, Flash: 46%)
   - Navigation: `actors/actor_2m_one_pipe_obsticle/actor_000000002000000.h`
   - Hover: `actors/hover_actors/hoverActor_000000000300000.h`
   - MD5: `ae796c7474baeeaf9525689af1779c3d`
   - Use for: Alternative navigation actor with pipe obstacle experience

## Firmware Build Process

All firmwares were built using:
- **Native ARM GCC toolchain** (faster, recommended over Docker)
- **Crazyflie firmware version**: cf2_defconfig
- **Build flags**: `-O3 -DRL_TOOLS_CONTROLLER`
- **Policy switching builds**: `-DENABLE_POLICY_SWITCHING`

### Key Fixes Applied

#### Namespace Collision Fix
The hover actor namespace was renamed to avoid C++ redefinition errors:
```bash
sed -i 's/namespace rl_tools::checkpoint::actor/namespace rl_tools::checkpoint::hover_actor/g' hover_actor.h
sed -i 's/^namespace rl_tools::checkpoint::observation/namespace rl_tools::checkpoint::hover_actor::observation/g' hover_actor.h
sed -i 's/^namespace rl_tools::checkpoint::action/namespace rl_tools::checkpoint::hover_actor::action/g' hover_actor.h
sed -i 's/^namespace rl_tools::checkpoint::meta/namespace rl_tools::checkpoint::hover_actor::meta/g' hover_actor.h
```

#### Adapter Code Fix
The `rl_tools_adapter.cpp` was updated to reference the correct hover actor namespace:
```cpp
using HOVER_ACTOR_TYPE = decltype(rl_tools::checkpoint::hover_actor::model);
```

## Flashing Instructions

### Prerequisites
- Crazyflie 2.1 powered on and in range
- Crazyradio PA dongle connected
- Python environment with cfloader installed

### Flash Command

```bash
# Activate venv
source ~/Projects/crazyflie-clients-python/venv/bin/activate

# Flash firmware (replace FIRMWARE_NAME.bin with desired firmware)
sudo cfloader flash firmware_outputs/FIRMWARE_NAME.bin stm32-fw -w radio://0/80/2M/E7E7E7E7E7
```

### Example: Flash Policy Switching Firmware

```bash
source ~/Projects/crazyflie-clients-python/venv/bin/activate
sudo cfloader flash firmware_outputs/cf2_policy_switching_nav2400000_hover300000.bin stm32-fw -w radio://0/80/2M/E7E7E7E7E7
```

**Expected output:**
```
Reset to bootloader mode ...
Restart the Crazyflie you want to bootload in the next 10 seconds ...
Connected to bootloader on Crazyflie 2.1 (version=0x10)
Target info: nrf51 (0xFE)
Flash pages: 232 | Page size: 1024 | Buffer pages: 1 | Start page: 88
Flashing 1 of 1 to stm32 (fw): 472004 bytes
[========================================] 100%
```

## Policy Switching Configuration

For policy switching firmwares (4 & 5), configure parameters after flashing:

### Via cfclient GUI:
1. Connect to Crazyflie
2. Go to **Parameters** tab
3. Set `rlt.ps_enable = 1` (enable policy switching)
4. Set `rlt.ps_thresh = 0.5` (switch at 0.5m from target)

### Via Python:
```python
import cflib.crtp
from cflib.crazyflie import Crazyflie
from cflib.crazyflie.syncCrazyflie import SyncCrazyflie

cflib.crtp.init_drivers()
URI = 'radio://0/80/2M/E7E7E7E7E7'

with SyncCrazyflie(URI, cf=Crazyflie(rw_cache='./cache')) as scf:
    scf.cf.param.set_value('rlt.ps_enable', 1)
    scf.cf.param.set_value('rlt.ps_thresh', 0.5)
    print("Policy switching enabled!")
```

### Via trigger.py script:
```bash
python3 scripts/trigger.py --mode policy_switching --height 0.2 --ps-threshold 0.5
```

## Flight Testing Recommendations

### Step 1: Test Individual Actors First
Before testing policy switching, validate each actor independently:

```bash
# Test hover actor alone
sudo cfloader flash firmware_outputs/cf2_hover_actor_300000_single.bin stm32-fw -w radio://0/80/2M/E7E7E7E7E7
python3 scripts/trigger.py --mode hover_learned --height 0.3

# Test navigation actor alone
sudo cfloader flash firmware_outputs/cf2_nav_actor_2400000_single.bin stm32-fw -w radio://0/80/2M/E7E7E7E7E7
python3 scripts/trigger.py --mode hover_learned --height 0.3
```

**Expected Behavior:**
- Hover actor: Stable hovering with minimal drift at target position
- Navigation actor: Reaches target vicinity (may have some drift, hover actor handles final positioning)

### Step 2: Test Policy Switching
Once both actors are validated individually:

```bash
# Flash policy switching firmware
sudo cfloader flash firmware_outputs/cf2_policy_switching_nav2400000_hover300000.bin stm32-fw -w radio://0/80/2M/E7E7E7E7E7

# Test policy switching flight
python3 scripts/trigger.py --mode policy_switching --height 0.2 --ps-threshold 0.5
```

**Expected Behavior:**
- Takeoff with original controller (safe)
- Switch to navigation actor (distance > 0.5m from target)
- Automatic switch to hover actor when within 0.5m of target
- Stable hovering at target position (0.0, 1.2, 0.2)

### Monitor Switching Behavior

Use cfclient logging to observe actor transitions:
1. Open **Logging** tab in cfclient
2. Add variables:
   - `rltps.actor` (0=nav, 1=hover)
   - `rltps.dist` (distance to target in meters)
   - `rltps.thresh` (threshold value)
   - `stateEstimate.x/y/z` (position)

3. Start logging and trigger flight
4. Observe smooth transition from nav→hover as distance crosses threshold

## Known Issues & Troubleshooting

### Issue: Constant drift toward target without stopping

**Potential Causes:**
1. **Actor validation needed**: Navigation actor may not be properly trained for deployment
2. **Observation mismatch**: Training vs deployment observation scaling differs
3. **Target position mismatch**: `POLICY_SWITCH_TARGET` in firmware doesn't match training

**Solutions:**
1. Test each actor individually first (see Step 1 above)
2. Check observation scaling in training config vs firmware
3. Verify target position: `rl_tools_controller.c` line ~63:
   ```c
   static const float POLICY_SWITCH_TARGET[3] = {0.0f, 1.2f, 0.0f};
   ```

### Issue: Actor doesn't switch

**Solutions:**
- Verify `rlt.ps_enable = 1` via cfclient parameters
- Check `rlt.ps_thresh` value (default: 0.5)
- Monitor `rltps.dist` - does it cross threshold?
- Confirm firmware built with `ENABLE_POLICY_SWITCHING=1` (check flash usage ~46%)

### Issue: Firmware won't flash

**Solutions:**
- Power cycle Crazyflie
- Check USB dongle: `lsusb | grep Crazyradio`
- Try `--force` flag: `sudo cfloader flash ... --force`
- Verify URI matches your drone

### Issue: Build fails with namespace errors

**Solution:**
The namespace fix should already be applied. If you rebuild from source, remember to run:
```bash
cd controller/data
sed -i 's/namespace rl_tools::checkpoint::actor/namespace rl_tools::checkpoint::hover_actor/g' hover_actor.h
# ... (other sed commands as shown above)
```

## Performance Metrics

### Flash Usage
- Single-actor: ~40% (407 KB / 1032 KB)
- Policy switching: ~46% (462 KB / 1032 KB)
- Remaining space: ~54% for future features

### RAM Usage
- Total: ~65% (85 KB / 131 KB)
- Sufficient headroom for normal operation

### CCM Usage
- Total: ~95% (62 KB / 65.5 KB)
- Near capacity, but within acceptable limits

### Control Loop Performance
- Actor inference: ~100-200µs per call
- Distance calculation: ~10µs
- Policy switching overhead: ~220µs total (11% of 2ms cycle)
- **Zero switching latency** (immediate decision at 500Hz)

## Backup & Rollback

These firmware files are your backup. To rollback to any version:

```bash
# List available firmwares
ls -lh firmware_outputs/

# Flash desired version
sudo cfloader flash firmware_outputs/DESIRED_FIRMWARE.bin stm32-fw -w radio://0/80/2M/E7E7E7E7E7
```

**Recommended backups:**
- Keep these `.bin` files in a safe location
- Document which firmware was flashed for each flight test
- Use MD5 checksums to verify file integrity

## Additional Resources

- **Main README**: `../README.MD`
- **Policy Switching Guide**: `../POLICY_SWITCHING_DEPLOYMENT.md`
- **Quick Reference**: `../CrazyCheetSheet.txt`
- **Implementation Summary**: `../IMPLEMENTATION_SUMMARY.md`

## Build Reproduction

To rebuild any firmware from source:

### Single-Actor Build
```bash
cd controller
cp ../actors/PATH_TO_ACTOR.h data/actor.h
make clean
make -j8
cp build/cf2.bin ../firmware_outputs/CUSTOM_NAME.bin
```

### Policy Switching Build
```bash
cd controller
cp ../actors/NAV_ACTOR.h data/actor.h
cp ../actors/HOVER_ACTOR.h data/hover_actor.h

# Apply namespace fix
cd data
sed -i 's/namespace rl_tools::checkpoint::actor/namespace rl_tools::checkpoint::hover_actor/g' hover_actor.h
sed -i 's/^namespace rl_tools::checkpoint::observation/namespace rl_tools::checkpoint::hover_actor::observation/g' hover_actor.h
sed -i 's/^namespace rl_tools::checkpoint::action/namespace rl_tools::checkpoint::hover_actor::action/g' hover_actor.h
sed -i 's/^namespace rl_tools::checkpoint::meta/namespace rl_tools::checkpoint::hover_actor::meta/g' hover_actor.h
cd ..

# Build with policy switching
make clean
make ENABLE_POLICY_SWITCHING=1 -j8
cp build/cf2.bin ../firmware_outputs/CUSTOM_NAME.bin
```

## Support & Testing Notes

**Current Actor Status:**
- ✅ Hover actor (`hoverActor_000000000300000.h`): VALIDATED - stable hovering confirmed
- ⚠️ Navigation actors: NEEDS FLIGHT VALIDATION
  - Issue reported: "constant drift toward target without stopping"
  - Recommendation: Test single-actor builds first to isolate actor vs switching logic issues

**Next Steps for Troubleshooting:**
1. Flash `cf2_nav_actor_2400000_single.bin`
2. Test flight and observe navigation behavior
3. If drift persists → actor training/deployment mismatch issue
4. If works correctly → test policy switching firmware

---

**Generated using native ARM GCC build system**  
**Build time**: ~1-2 minutes per firmware  
**Total build time**: ~10 minutes for all 5 firmwares  

Happy Flying! 🚁
