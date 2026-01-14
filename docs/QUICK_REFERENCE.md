# Policy Switching Quick Reference Card

## 🚀 Quick Start (3 Commands)

```bash
# 1. Build firmware with policy switching
./build_firmware_with_policy_switching.sh

# 2. Flash to Crazyflie
sudo cfloader flash build_firmware/cf2.bin stm32-fw -w radio://0/80/2M

# 3. Enable policy switching
python scripts/configure_policy_switching.py --enable --threshold 0.5
```

## 📋 Command Reference

### Build Firmware
```bash
# With default actors
./build_firmware_with_policy_switching.sh

# With custom actors
./build_firmware_with_policy_switching.sh path/to/nav_actor.h path/to/hover_actor.h

# Manual Docker build
docker run --rm -it \
  -v $(pwd)/deployment_actors/actor.h:/controller/data/actor.h \
  -v $(pwd)/deployment_actors/hover_actor.h:/controller/data/hover_actor.h \
  -v $(pwd)/build_firmware:/output \
  -e ENABLE_POLICY_SWITCHING=1 \
  arpllab/learning_to_fly_build_firmware
```

### Flash Firmware
```bash
# Standard flash
sudo cfloader flash build_firmware/cf2.bin stm32-fw -w radio://0/80/2M

# Force flash (if connection issues)
sudo cfloader flash build_firmware/cf2.bin stm32-fw -w radio://0/80/2M --force
```

### Configure Parameters
```bash
# Enable with default threshold (0.5m)
python scripts/configure_policy_switching.py --enable

# Enable with custom threshold
python scripts/configure_policy_switching.py --enable --threshold 0.6

# Disable
python scripts/configure_policy_switching.py --disable

# Check status
python scripts/configure_policy_switching.py --status
```

### Test Flights
```bash
# Policy switching flight test (RECOMMENDED)
python scripts/trigger.py --mode policy_switching --height 0.3 --ps-threshold 0.5

# Alternative: takeoff and switch (requires manual ps_enable setup)
python scripts/trigger.py --mode takeoff_and_switch --height 0.3

# Navigation actor only (for comparison, no switching)
python scripts/trigger.py --mode hover_learned --height 0.3

# Trajectory tracking (policy switching can be enabled separately)
python scripts/trigger.py --mode trajectory_tracking --height 0.3 --trajectory-scale 0.3
```

## 🎯 Parameter Values

| Parameter | Default | Range | Description |
|-----------|---------|-------|-------------|
| `ps_enable` | 0 | 0-1 | Enable (1) or disable (0) policy switching |
| `ps_thresh` | 0.5 | 0.1-2.0 | Distance threshold in meters |

**Access via cfclient**: Parameters → rlt group → ps_enable, ps_thresh

## 📊 Logging Variables (cfclient)

Add these in the Logging tab to monitor switching:

| Variable | Type | Description |
|----------|------|-------------|
| `rltps.actor` | uint8 | Current actor (0=nav, 1=hover) |
| `rltps.dist` | float | Distance to target (meters) |
| `rltps.thresh` | float | Current threshold value |
| `rltps.enable` | uint8 | Switch enabled (0/1) |
| `stateEstimate.x` | float | X position |
| `stateEstimate.y` | float | Y position |
| `stateEstimate.z` | float | Z position |

## 🔍 Expected Flight Behavior

### Normal Operation
```
Takeoff (actor=0, distance=~2.0m)
    ↓
Navigate toward target (actor=0, distance decreasing)
    ↓
Cross threshold (actor: 0→1, distance<0.5m)
    ↓
Hover at target (actor=1, distance≈0.2m)
```

### Logging Timeline
```
t=0s:   dist=2.5m, actor=0 (nav)
t=2s:   dist=1.2m, actor=0 (nav)
t=4s:   dist=0.45m, actor=1 (hover) ← SWITCH!
t=6s:   dist=0.25m, actor=1 (hover)
t=8s:   dist=0.30m, actor=1 (hover)
```

## 🛠️ Troubleshooting

| Problem | Check | Solution |
|---------|-------|----------|
| Won't flash | USB dongle | `lsusb \| grep Crazyradio` |
| Actor doesn't switch | ps_enable | Set to 1 in cfclient |
| Oscillation | Threshold | Increase ps_thresh to 0.6 |
| Drift after switch | Hover actor | Verify hover_actor.h exists |
| Build fails | Actor files | Check both .h files mounted |

## 📝 File Checklist

### Required Files
- ✅ `actors/actor_XXXXXXXXX.h` - Navigation actor
- ✅ `actors/hoverActor_XXXXXXXXX.h` - Hover actor

### Modified Firmware Files
- ✅ `controller/rl_tools_adapter.h` - Dual actor interface
- ✅ `controller/rl_tools_adapter.cpp` - Actor loading logic
- ✅ `controller/rl_tools_controller.c` - Switching implementation

### Helper Scripts
- ✅ `build_firmware_with_policy_switching.sh` - Automated build
- ✅ `scripts/configure_policy_switching.py` - Parameter setup
- ✅ `scripts/trigger.py` - Flight testing (existing)

### Documentation
- ✅ `POLICY_SWITCHING_DEPLOYMENT.md` - Full guide
- ✅ `IMPLEMENTATION_SUMMARY.md` - Technical details
- ✅ `QUICK_REFERENCE.md` - This file

## 🎓 Key Concepts

### Distance Calculation
```c
dx = x - 0.0
dy = y - 1.2  // Target position
dz = z - 0.0
distance = sqrt(dx² + dy² + dz²)
```

### Actor Selection
```c
if (distance < ps_thresh)
    current_actor = 1;  // Hover
else
    current_actor = 0;  // Navigation
```

### Observation Transform
```c
// Navigation actor: relative to setpoint
obs = position - target_pos

// Hover actor: relative to target
obs = position - POLICY_SWITCH_TARGET
```

## 🚦 Safety Checklist

Before flight:
- [ ] Firmware flashed successfully
- [ ] ps_enable = 1 confirmed
- [ ] ps_thresh = 0.5 confirmed
- [ ] Clear 3m radius around target
- [ ] Target height = 0.3m for initial test
- [ ] Emergency stop ready
- [ ] Logging active (optional but recommended)

## 📖 Related Documentation

- **Full Deployment Guide**: `POLICY_SWITCHING_DEPLOYMENT.md`
- **Implementation Details**: `IMPLEMENTATION_SUMMARY.md`
- **General Deployment**: `README.MD`
- **User Notes**: `CrazyCheetSheet.txt`
- **Training Config**: `src/config/config.h`

## 💡 Pro Tips

1. **Test Single Actor First**: Disable policy switching (`ps_enable=0`) for baseline
2. **Monitor Logs**: Always check `rltps.actor` during first flights
3. **Tune Threshold**: Start at 0.5m, adjust based on behavior
4. **Backup Firmware**: Save `cf2.bin` files with timestamps
5. **Update Actors**: Just rebuild with new .h files, no code changes needed

## 🔄 Update Workflow

```bash
# 1. Train new actors
./build/training_headless

# 2. Copy checkpoints
cp checkpoints/multirotor_td3/actor_XXXXXXXXX.h actors/
cp checkpoints/multirotor_hover/actor_XXXXXXXXX.h actors/hoverActor_XXXXXXXXX.h

# 3. Rebuild firmware
./build_firmware_with_policy_switching.sh actors/actor_XXXXXXXXX.h actors/hoverActor_XXXXXXXXX.h

# 4. Reflash
sudo cfloader flash build_firmware/cf2.bin stm32-fw -w radio://0/80/2M

# 5. Test
python scripts/trigger.py --mode takeoff_and_switch --height 0.3
```

## 📞 Getting Help

If issues persist:
1. Check `POLICY_SWITCHING_DEPLOYMENT.md` Troubleshooting section
2. Review firmware logs in cfclient console
3. Test with `ps_enable=0` to isolate switching issues
4. Verify training and deployment configurations match

---

**Last Updated**: Based on implementation in controller files
**Target Position**: (0.0, 1.2, 0.0)
**Default Threshold**: 0.5m
**Control Rate**: 500Hz
