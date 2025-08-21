#!/bin/bash

# Script to create training summary in the checkpoint folder after training starts
# This places all training outputs in one location: checkpoints/multirotor_td3/<run_name>/

if [ $# -ne 1 ]; then
    echo "Usage: $0 <run_name>"
    echo "Example: $0 2025_01_15_10_30_45_d+o+a+r+h+c+f+w+e+_000"
    exit 1
fi

RUN_NAME="$1"
CHECKPOINT_DIR="checkpoints/multirotor_td3/$RUN_NAME"
SUMMARY_FILE="$CHECKPOINT_DIR/training_parameters_summary.txt"

# Wait for checkpoint directory to be created (up to 30 seconds)
for i in {1..30}; do
    if [ -d "$CHECKPOINT_DIR" ]; then
        break
    fi
    sleep 1
done

if [ ! -d "$CHECKPOINT_DIR" ]; then
    echo "Error: Checkpoint directory $CHECKPOINT_DIR not found after 30 seconds"
    exit 1
fi

# Determine training type
TRAINING_TYPE="Standard Training"
if [[ "$RUN_NAME" == *"BENCHMARK"* ]]; then
    TRAINING_TYPE="Performance Benchmark"
elif [[ "$RUN_NAME" == *"ABLATION"* ]]; then
    TRAINING_TYPE="Ablation Study"
elif [[ "$RUN_NAME" == *"d+o+a+r+h+c+f+w+e+"* ]]; then
    TRAINING_TYPE="Full Multi-Component Training"
fi

# Create training summary file in the checkpoint directory
cat > "$SUMMARY_FILE" << EOF
=== TRAINING PARAMETERS SUMMARY ===
Generated on: $(date '+%Y-%m-%d %H:%M:%S') [Generated during training initialization]
Run Name: $RUN_NAME

=== TRAINING TYPE AND LOCATIONS ===
Training Date: ${RUN_NAME:0:19}
Training Type: $TRAINING_TYPE
Starting Location: (0.0, 0.0, 0.0)
Target Location: Variable (Position-to-Position learning)

=== ALGORITHM CONFIGURATION ===
Algorithm: Twin Delayed Deep Deterministic Policy Gradient (TD3)
Starting Point: Pre-trained hover controller (hoverActor_000000000300000.h)
Environment: Crazyflie Multirotor Simulation
Reward Function: Multi-component (distance + orientation + angular + rate + height + crash + fuel + wind + early_termination)

=== KEY PARAMETERS ===
Network Architecture: Multi-layer Perceptron (MLP)
Actor Learning Rate: Adaptive based on RL-Tools framework
Critic Learning Rate: Adaptive based on RL-Tools framework
Discount Factor (γ): Framework default
Exploration Noise: Gaussian noise with decay
Batch Size: Framework default
Target Network Update: Soft updates (τ parameter)

=== ENVIRONMENT SPECIFICS ===
Observation Dimensions: Position, velocity, orientation, angular velocity
Action Dimensions: 4 (thrust + 3 torques)
Episode Length: Variable (based on task completion/failure)
Physics: High-fidelity multirotor dynamics
Wind: Enabled (part of multi-component training)

=== OUTPUT FILES (All in this folder) ===
Actor Checkpoints: actor_***.h5 and actor_***.h files
TensorBoard Events: data.tfevents (will be moved here)
Learning Curves: learning_curves_$RUN_NAME.h5 (will be moved here)
Training Summary: training_parameters_summary.txt (this file)

=== TRAINING DETAILS ===
Multi-Component Features:
- Distance reward (d): Position tracking incentive
- Orientation reward (o): Attitude control incentive  
- Angular reward (a): Angular velocity control
- Rate reward (r): Control smoothness
- Height reward (h): Altitude maintenance
- Crash penalty (c): Safety constraint
- Fuel efficiency (f): Energy optimization
- Wind resistance (w): Robustness training
- Early termination (e): Convergence optimization

Starting Actor: hoverActor_000000000300000.h (300,000 step pre-trained checkpoint)
This provides a stable baseline for position-to-position learning tasks.

NOTE: All training outputs are consolidated in this single folder for easy access.
EOF

echo "Training summary created: $SUMMARY_FILE"
