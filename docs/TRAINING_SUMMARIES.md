# Training Parameter Summary Generation

This repository includes **automated C++ generation** of training parameter summary files that document all the important details of each training run during the training initialization process.

**All training outputs are now consolidated in a single location**: `checkpoints/multirotor_td3/<run_name>/` eliminating the need for a separate logs directory and cross-references.

## Features

The training parameter summaries are automatically generated and include the following information:

### 1. Training Type and Locations
- **Training Type**: Hover Learning vs Position-to-Position Learning
- **Starting Location**: Initial position of the drone
- **Target Location**: Target position (hover at origin or variable)
- **Training Date**: Timestamp of when training started

### 2. Ablation Study Features
- **Feature String**: Complete ablation configuration (e.g., d+o+a+r+h+c+f+w+e+)
- **Individual Features**: Detailed breakdown of each enabled/disabled feature:
  - Disturbance (d)
  - Observation Noise (o) 
  - Asymmetric Actor-Critic (a)
  - Rotor Delay (r)
  - Action History (h)
  - Curriculum Learning (c)
  - Recalculate Rewards (f)
  - Initial Reward Function (w)
  - Exploration Noise Decay (e)

### 3. Actor Initialization
- **Starting Actor File**: Specific checkpoint file used (e.g., hoverActor_000000000300000.h)
- **Initialization Type**: Pre-trained weights or from scratch
- **Checkpoint Path**: Full path to the starting actor file
- **Base Seed**: Random seed used for initialization
- **Warmup Steps**: Actor and critic warmup configuration

### 4. Training Steps
- **Maximum Training Steps**: Total number of training steps
- **Checkpoint Interval**: How often actor checkpoints are saved
- **Expected Checkpoints**: Total number of checkpoints that will be generated

### 5. Cost Function Parameters
- **Reward Function Type**: Specific reward function used
- **Weight Configuration**: Position, orientation, velocity, and action penalty weights
- **Scale Factors**: Normalization and scaling parameters

### 6. Environment Configuration
- **Simulation Type**: Multirotor (Crazyflie 2.0)
- **Physics Engine**: Custom RL-Tools Multirotor Simulator
- **Integration Method**: RK4 (Runge-Kutta 4th order)
- **Step Size**: Simulation timestep (dt)
- **Episode Length**: Number of steps per episode

### 7. Algorithm Parameters
- **Algorithm**: TD3 (Twin Delayed Deep Deterministic Policy Gradient)
- **Batch Sizes**: Actor and critic batch sizes
- **Training Intervals**: Update frequencies for actor and critic
- **Target Network Updates**: Soft update parameters
- **Exploration Parameters**: Noise configuration for exploration

### 8. Checkpointing Configuration
- **Checkpoint Intervals**: When checkpoints are saved
- **File Formats**: HDF5 (.h5) and C++ header (.h) formats
- **TensorBoard Logging**: Event logging configuration
- **Learning Curve Data**: Historical performance tracking

## Technical Implementation

### Unified Output Directory Structure

All training outputs are consolidated in a single directory structure:

**Directory**: `checkpoints/multirotor_td3/<run_name>/`

**Contents**:
- `actor_000000000000000.h5/.h` - Training checkpoints at various steps
- `training_parameters_summary.txt` - Detailed parameter documentation
- `data.tfevents` - TensorBoard event logs  
- `learning_curves_<run_name>.h5` - Performance metrics over time

**Benefits of Unified Structure**:
- **Simplified Navigation**: All related files in one location
- **No Cross-Platform Issues**: No symbolic links needed
- **Easy Backup/Transfer**: Single directory contains everything
- **Clear Organization**: No confusion between different output folders

### Script Integration

**Script Location**: `scripts/create_training_files.sh`

**Called From**: `src/training.h` during training initialization:
```cpp
std::string script_cmd = "bash scripts/create_training_files.sh " + ts.run_name + " &";
std::system(script_cmd.c_str());
```

The script creates the training parameter summary file in the checkpoint directory, providing complete documentation in the same location as all other training outputs.

## Example Files

### Training Summary Structure
```
=== TRAINING PARAMETERS SUMMARY ===
Generated on: 2025-08-18 00:06:27 [Generated during training initialization]
Run Name: 2025_08_18_00_06_27_d+o+a+r+h+c+f+w+e+_000

=== TRAINING TYPE AND LOCATIONS ===
Training Date: 2025-08-18 00:06:27
Training Type: Hover Learning
Starting Location: (0.0, 0.0, 0.0)
Target Location: (0.0, 0.0, 0.0) [Hover at origin]

=== ACTOR INITIALIZATION ===
Starting Actor: Initialized from checkpoint
Starting Actor File: hoverActor_000000000300000.h (Step 300,000 checkpoint)
Checkpoint Path: ../actors/hoverActor_000000000300000.h

... [complete parameter details] ...

=== RELATED FILES ===
Actor Checkpoints: ../actors/ (actor_*.h5, actor_*.h)
Actor Training Info: ../actors/TRAINING_INFO_[run_name].txt
TensorBoard Logs: ./data.tfevents
```

### Actor Reference Structure
```
=== ACTOR CHECKPOINT REFERENCE ===
Generated on: 2025-08-18 00:12:45
Training Run: 2025_08_18_00_06_27_d+o+a+r+h+c+f+w+e+_000

=== STARTING POINT ===
Initial Actor: hoverActor_000000000300000.h (Step 300,000 checkpoint)
Starting Point: Pre-trained hover controller

=== RELATED FILES ===
 Training Summary: ../checkpoints/multirotor_td3/[run_name]/training_parameters_summary.txt
 TensorBoard Logs: ../checkpoints/multirotor_td3/[run_name]/data.tfevents
 Actor Checkpoints (this folder): [checkpoint file list]
 
 Note: The legacy `logs/` folder is deprecated. New runs store summaries and TensorBoard event files directly inside:
 `checkpoints/multirotor_td3/<run_name>/`.
```

## Benefits

1. **Complete Documentation**: Every training run is automatically documented
2. **Unified Location**: All training outputs in one directory - no separate logs folder
3. **Cross-Platform Compatible**: No symbolic links that cause issues in WSL/Windows
4. **Easy Backup/Transfer**: Single directory contains everything for a training run
5. **Reproducibility**: All parameters needed to reproduce results are captured
6. **No Manual Work**: Documentation happens automatically during training
7. **Consistent Format**: Standardized structure across all training runs
8. **Simple Navigation**: Everything related to a training run in one place

## Current Status

The training documentation system has been **simplified to use a unified directory structure**:

- ✅ **Unified Output Location**: All training files in `checkpoints/multirotor_td3/<run_name>/`
- ✅ **Automatic Documentation**: Training summaries generated during initialization  
- ✅ **Complete File Coverage**: Checkpoints, logs, learning curves, and summaries
- ✅ **Cross-Platform Compatibility**: No symbolic links that break in WSL/Windows
- ✅ **Training Type Classification**: Benchmark vs standard training detection

**New Training Runs**: Will automatically have all outputs in the checkpoint folder.

**Existing Training Data**: 
- Previous runs in separate `logs/` directory remain for reference
- Future training uses the new unified structure

This eliminates the complexity of cross-references and provides a much cleaner, more portable solution for training documentation and output management.
