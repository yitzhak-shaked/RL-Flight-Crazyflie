# Critical Bug Fix - December 3, 2025

## Bug Summary

**Symptom**: Navigation actors fly toward target but never stop, even with policy switching enabled.

**Root Cause**: Target position mismatch between policy switching logic and navigation actor.

## The Problem

### Code Issue (rl_tools_controller.c, line 484-486)

**BEFORE (BUGGY)**:
```c
case POSITION:
    target_pos[0] = origin[0];  // ❌ Always uses takeoff location
    target_pos[1] = origin[1];
    target_pos[2] = origin[2];
    break;
```

### What Was Happening

1. **Policy switching distance check** (line ~597):
   ```c
   distance_to_target = sqrt((x - POLICY_SWITCH_TARGET[0])² + 
                             (y - POLICY_SWITCH_TARGET[1])² + 
                             (z - POLICY_SWITCH_TARGET[2])²)
   // Where POLICY_SWITCH_TARGET = {0.0, 1.2, 0.0}
   ```

2. **Navigation actor target** (line 484-486):
   ```c
   target_pos = origin  // Takeoff location (e.g., 0.0, 0.0, 0.3)
   ```

3. **Result**:
   - Navigation actor tries to reach `origin` (0.0, 0.0, 0.3)
   - Switching logic checks distance to `POLICY_SWITCH_TARGET` (0.0, 1.2, 0.0)
   - These are **different locations** 1.2m apart!
   - Distance never crosses threshold → **never switches to hover actor**
   - Navigation actor keeps flying toward origin, which isn't the intended target

### Why Single-Actor Navigation Also Failed

Even without policy switching:
- Navigation actor was told to hover at `origin` (takeoff point)
- But actors were trained to navigate to a specific target location
- The actor expected to be navigating toward (0.0, 1.2, 0.0), not hovering at origin
- Mismatch between training expectations and deployment target

## The Fix

**AFTER (FIXED)**:
```c
case POSITION:
    if(ps_enable){
        // When policy switching is enabled, navigate to the policy switch target
        target_pos[0] = POLICY_SWITCH_TARGET[0];  // 0.0
        target_pos[1] = POLICY_SWITCH_TARGET[1];  // 1.2
        target_pos[2] = POLICY_SWITCH_TARGET[2];  // 0.0 (height set separately)
    } else {
        // Normal position mode: hover at origin (takeoff location)
        target_pos[0] = origin[0];
        target_pos[1] = origin[1];
        target_pos[2] = origin[2];
    }
    break;
```

### What Now Happens

1. **With policy switching enabled** (`ps_enable = 1`):
   - Navigation actor navigates to `POLICY_SWITCH_TARGET` (0.0, 1.2, height)
   - Distance calculation uses same target
   - When distance < threshold → switches to hover actor ✅
   - Hover actor gets observations relative to same target ✅

2. **Without policy switching** (`ps_enable = 0`):
   - Actor hovers at `origin` (backwards compatible)
   - Useful for testing single actors at takeoff location

## Impact

### Before Fix
- ❌ Navigation actors: Constant drift, never stop
- ❌ Policy switching: Never switches actors
- ✅ Hover actor alone: Works (was already at origin)

### After Fix
- ✅ Navigation actors: Should navigate to (0.0, 1.2, height) when `ps_enable=1`
- ✅ Policy switching: Should switch actors at threshold
- ✅ Hover actor alone: Still works
- ✅ Backwards compatible: Single actors can still hover at origin with `ps_enable=0`

## Testing Recommendations

### Test 1: Hover Actor (Validation - Should Still Work)
```bash
sudo cfloader flash firmware_outputs/cf2_hover_actor_300000_single.bin stm32-fw -w radio://0/80/2M/E7E7E7E7E7
python3 scripts/trigger.py --mode hover_learned --height 0.3
```
**Expected**: Stable hover at takeoff location ✅

### Test 2: Policy Switching (PRIMARY TEST)
```bash
sudo cfloader flash firmware_outputs/cf2_policy_switching_nav2400000_hover300000.bin stm32-fw -w radio://0/80/2M/E7E7E7E7E7
python3 scripts/trigger.py --mode policy_switching --height 0.2 --ps-threshold 0.5
```
**Expected**:
1. Takeoff with original controller to ~0.2m height
2. Switch to navigation actor
3. Navigate toward (0.0, 1.2, 0.2)
4. **Automatic switch to hover actor** when within 0.5m of target
5. **Stable hover at (0.0, 1.2, 0.2)**

Watch for console message: "Actor switch: 0 -> 1 (distance: 0.XXXm)"

### Test 3: Monitor Switching (Debugging)
In cfclient, log these variables:
- `rltps.actor` (0=nav, 1=hover)
- `rltps.dist` (distance to target)
- `rltps.thresh` (switching threshold)
- `stateEstimate.x/y/z` (position)

**Expected behavior**:
- `rltps.dist` starts > 0.5m (far from target)
- `rltps.actor` = 0 (navigation)
- As drone flies, `rltps.dist` decreases
- When `rltps.dist` < 0.5m → `rltps.actor` switches to 1 (hover)
- Drone stabilizes at target position

## Files Changed

### Modified
- `controller/rl_tools_controller.c` (lines 484-496)

### Rebuilt Firmware (All 5 files)
All firmware files in `firmware_outputs/` have been rebuilt with the fix:

**NEW MD5 Checksums** (December 3, 2025, 16:50-16:53):
```
6bada0541800bd088b5d47bda370b96c  cf2_nav_actor_2400000_single.bin
a59f6c650adf9573983b19c25df7ea55  cf2_nav_actor_2000000_single.bin
b8460ceeb5a601949aac1e2661292883  cf2_hover_actor_300000_single.bin
78ed2a0849caca1f1ca8b2080969232b  cf2_policy_switching_nav2400000_hover300000.bin
1fc416e884756d4a139920898c6d3d01  cf2_policy_switching_nav2000000_hover300000.bin
```

**OLD MD5 Checksums** (November 30, 2025 - BUGGY):
```
77449171c4962f844385e2386f27ca8e  cf2_nav_actor_2400000_single.bin (OLD - BUGGY)
ff0f284df777134e3c4f312d4dad91fb  cf2_nav_actor_2000000_single.bin (OLD - BUGGY)
3cd30dd9ece25c5464dea40e32b4c241  cf2_hover_actor_300000_single.bin (OLD - BUGGY)
ecebf18334f2221389245b2590faba51  cf2_policy_switching_nav2400000_hover300000.bin (OLD - BUGGY)
8a81d0702c24f16ef61a92eef4c24388  cf2_policy_switching_nav2000000_hover300000.bin (OLD - BUGGY)
```

⚠️ **DELETE OLD FIRMWARE FILES** - They have the bug and won't work correctly!

## Technical Details

### Policy Switching Target
Defined in `rl_tools_controller.c` line 63:
```c
static const float POLICY_SWITCH_TARGET[3] = {0.0f, 1.2f, 0.0f};
```

This is the global position where:
- Navigation actor navigates TO
- Hover actor hovers AT (after transformation to relative coordinates)
- Distance is measured FROM (for switching logic)

### Observation Transformation (Still Correct)

In `update_state()` (lines 156-163):
```c
float ref_pos[3];
if(ps_enable && current_actor == 1){
    ref_pos = POLICY_SWITCH_TARGET;  // Hover actor: relative to target
} else {
    ref_pos = target_pos;            // Nav actor: relative to target_pos
}
```

With the fix:
- When `ps_enable=1`: `target_pos` = `POLICY_SWITCH_TARGET` for both actors ✅
- Navigation actor: observations relative to `POLICY_SWITCH_TARGET`
- Hover actor: observations relative to `POLICY_SWITCH_TARGET`
- Both actors see the same reference frame ✅

## Lesson Learned

**Always verify target position consistency** across:
1. Actor training target
2. Firmware target position setting
3. Policy switching distance calculation
4. Observation reference frame

All four must use the **same target location** for policy switching to work.

## Next Steps

1. ✅ Firmware rebuilt with fix
2. ⏳ Test policy switching firmware
3. ⏳ Verify actor transitions occur
4. ⏳ Confirm stable hovering at target

---

**Fixed by**: GitHub Copilot  
**Date**: December 3, 2025, 16:49-16:53 UTC  
**Impact**: Critical - Makes policy switching functional  
**Backwards Compatibility**: Yes (with `ps_enable=0`)
