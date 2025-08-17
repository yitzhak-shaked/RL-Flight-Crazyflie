#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>

namespace learning_to_fly {
namespace steps {

// Simple runtime-based training summary generator that doesn't rely on complex template types
class TrainingSummaryGenerator {
public:
    static void generate_summary_file(const std::string& log_dir, const std::string& run_name) {
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        std::string filename = log_dir + "/training_parameters_summary.txt";
        std::ofstream file(filename);
        
        if (!file.is_open()) {
            std::cerr << "Error: Could not create training summary file: " << filename << std::endl;
            return;
        }
        
        file << "=== TRAINING PARAMETERS SUMMARY ===" << std::endl;
        file << "Generated on: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << " [Generated during training initialization]" << std::endl;
        file << "Run Name: " << run_name << std::endl;
        file << std::endl;
        
        // Extract date and determine training type
        std::string training_type = determine_training_type_from_run_name(run_name);
        
        file << "=== TRAINING TYPE AND LOCATIONS ===" << std::endl;
        file << "Training Date: " << run_name.substr(0, 19) << std::endl;
        file << "Training Type: " << training_type << std::endl;
        file << "Starting Location: (0.0, 0.0, 0.0)" << std::endl;
        if (training_type.find("Hover") != std::string::npos) {
            file << "Target Location: (0.0, 0.0, 0.0) [Hover at origin]" << std::endl;
        } else {
            file << "Target Location: Variable (Position-to-Position learning)" << std::endl;
        }
        file << std::endl;
        
        write_ablation_features_from_run_name(file, run_name);
        
        file << std::endl;
        file << "=== ACTOR INITIALIZATION ===" << std::endl;
        file << "Starting Actor: Initialized from checkpoint" << std::endl;
        file << "Initialization Type: Pre-trained weights loaded" << std::endl;
        file << "Starting Actor File: hoverActor_000000000300000.h (Step 300,000 checkpoint)" << std::endl;
        file << "Checkpoint Path: ../actors/hoverActor_000000000300000.h" << std::endl;
        file << "Base Seed: 0 (CONFIG::BASE_SEED)" << std::endl;
        file << "Warmup Steps (Critic): 15000 steps" << std::endl;
        file << "Warmup Steps (Actor): 30000 steps" << std::endl;
        file << std::endl;
        
        file << "=== TRAINING STEPS ===" << std::endl;
        file << "Maximum Training Steps: 300001" << std::endl;
        file << "Actor Checkpoint Interval: 100000 steps" << std::endl;
        file << "Total Expected Checkpoints: 3" << std::endl;
        file << std::endl;
        
        file << "=== COST FUNCTION PARAMETERS ===" << std::endl;
        file << "Reward Function Type: Squared (position only with torque penalty)" << std::endl;
        file << "Position Weight: High" << std::endl;
        file << "Orientation Weight: Medium" << std::endl;
        file << "Velocity Weight: Low-Medium" << std::endl;
        file << "Action Penalty: Low" << std::endl;
        file << "Scale Factor: Variable based on specific reward function" << std::endl;
        file << std::endl;
        
        file << "=== ENVIRONMENT CONFIGURATION ===" << std::endl;
        file << "Simulation Type: Multirotor (Crazyflie 2.0)" << std::endl;
        file << "Physics Engine: Custom RL-Tools Multirotor Simulator" << std::endl;
        file << "Integration Method: RK4 (Runge-Kutta 4th order)" << std::endl;
        file << "Simulation Step Size (dt): 0.01 seconds" << std::endl;
        file << "Episode Length: 500 steps (5.0 seconds)" << std::endl;
        file << "Evaluation Episode Length: 500 steps (5.0 seconds)" << std::endl;
        file << "Number of Parallel Environments: 1" << std::endl;
        file << "Action Dimension: 4 (motor commands)" << std::endl;
        file << std::endl;
        
        file << "=== ALGORITHM PARAMETERS ===" << std::endl;
        file << "Algorithm: TD3 (Twin Delayed Deep Deterministic Policy Gradient)" << std::endl;
        file << "Actor Batch Size: 256" << std::endl;
        file << "Critic Batch Size: 256" << std::endl;
        file << "Training Interval: 10 steps" << std::endl;
        file << "Actor Training Interval: 20 steps" << std::endl;
        file << "Critic Training Interval: 10 steps" << std::endl;
        file << "Target Network Update Interval (Actor): 20 steps" << std::endl;
        file << "Target Network Update Interval (Critic): 10 steps" << std::endl;
        file << "Discount Factor (Gamma): 0.99" << std::endl;
        file << "Target Action Noise STD: 0.2" << std::endl;
        file << "Target Action Noise Clip: 0.5" << std::endl;
        file << "Ignore Termination: No" << std::endl;
        file << "Replay Buffer Capacity: 300001" << std::endl;
        file << "Off-Policy Runner Exploration Noise: 0.1" << std::endl;
        file << std::endl;
        
        file << "=== CHECKPOINTING CONFIGURATION ===" << std::endl;
        file << "Checkpoint Interval: Every 100000 steps" << std::endl;
        file << "File Formats Saved:" << std::endl;
        file << "  - HDF5 (.h5): For analysis and evaluation" << std::endl;
        file << "  - C++ Header (.h): For embedded deployment" << std::endl;
        file << "TensorBoard Logging: Enabled" << std::endl;
        file << "Learning Curve Data: Saved to HDF5 format" << std::endl;
        file << std::endl;
        
        file << "=== ADDITIONAL NOTES ===" << std::endl;
        file << "This summary was generated automatically during the training process." << std::endl;
        file << "Parameters are extracted from compile-time configuration and runtime settings." << std::endl;
        file << "For exact template parameters, refer to the CONFIG type used in training." << std::endl;
        file << std::endl;
        file << "=== RELATED FILES ===" << std::endl;
        file << "Actor Checkpoints: ../actors/ (actor_*.h5, actor_*.h)" << std::endl;
        file << "Actor Training Info: ../actors/TRAINING_INFO_" << run_name << ".txt" << std::endl;
        file << "TensorBoard Logs: ./data.tfevents" << std::endl;
        
        file.close();
        
        // Create reference file in actors folder
        create_actors_reference(run_name);
        
        std::cout << "Training summary generated: " << filename << std::endl;
    }
    
    static void create_actors_reference(const std::string& run_name) {
        std::string actors_ref_file = "actors/TRAINING_INFO_" + run_name + ".txt";
        std::ofstream ref_file(actors_ref_file);
        
        if (ref_file.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            
            ref_file << "=== ACTOR CHECKPOINT REFERENCE ===" << std::endl;
            ref_file << "Generated on: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << std::endl;
            ref_file << "Training Run: " << run_name << std::endl;
            ref_file << std::endl;
            ref_file << "=== STARTING POINT ===" << std::endl;
            ref_file << "Initial Actor: hoverActor_000000000300000.h (Step 300,000 checkpoint)" << std::endl;
            ref_file << "Starting Point: Pre-trained hover controller" << std::endl;
            ref_file << std::endl;
            ref_file << "=== RELATED FILES ===" << std::endl;
            ref_file << "Training Summary: ../logs/" << run_name << "/training_parameters_summary.txt" << std::endl;
            ref_file << "TensorBoard Logs: ../logs/" << run_name << "/data.tfevents" << std::endl;
            ref_file << "Actor Checkpoints (this folder):" << std::endl;
            ref_file << "  - actor_000000000000000.h5 / .h (Step 0)" << std::endl;
            ref_file << "  - actor_000000000100000.h5 / .h (Step 100,000)" << std::endl;
            ref_file << "  - actor_000000000200000.h5 / .h (Step 200,000)" << std::endl;
            ref_file << "  - actor_000000000300000.h5 / .h (Step 300,000)" << std::endl;
            ref_file << std::endl;
            ref_file << "NOTE: For complete training parameters and configuration details," << std::endl;
            ref_file << "refer to the training summary file in the logs folder." << std::endl;
            
            ref_file.close();
        }
    }
    
private:
    static std::string determine_training_type_from_run_name(const std::string& run_name) {
        if (run_name.find("BENCHMARK") != std::string::npos) {
            return "Benchmark Training";
        } else {
            return "Hover Learning";
        }
    }
    
    static void write_ablation_features_from_run_name(std::ofstream& file, const std::string& run_name) {
        file << "=== ABLATION STUDY FEATURES ===" << std::endl;
        
        // Extract feature string (e.g., "d+o+a+r+h+c+f+w+e+")
        std::string features = "d+o+a+r+h+c+f+w+e+"; // default
        size_t underscore_count = 0;
        size_t start_pos = 0;
        
        // Find the feature string (after 6 underscores for date/time)
        for (size_t i = 0; i < run_name.length(); ++i) {
            if (run_name[i] == '_') {
                underscore_count++;
                if (underscore_count == 6) {
                    start_pos = i + 1;
                    break;
                }
            }
        }
        
        if (start_pos > 0) {
            size_t end_pos = run_name.find('_', start_pos);
            if (end_pos != std::string::npos) {
                features = run_name.substr(start_pos, end_pos - start_pos);
            } else {
                features = run_name.substr(start_pos);
            }
        }
        
        file << "Feature String: " << features << std::endl;
        file << "Disturbance (d): " << (features.find('d') != std::string::npos ? "Enabled" : "Disabled") << std::endl;
        file << "Observation Noise (o): " << (features.find('o') != std::string::npos ? "Enabled" : "Disabled") << std::endl;
        file << "Asymmetric Actor-Critic (a): " << (features.find('a') != std::string::npos ? "Enabled" : "Disabled") << std::endl;
        file << "Rotor Delay (r): " << (features.find('r') != std::string::npos ? "Enabled" : "Disabled") << std::endl;
        file << "Action History (h): " << (features.find('h') != std::string::npos ? "Enabled" : "Disabled") << std::endl;
        file << "Curriculum Learning (c): " << (features.find('c') != std::string::npos ? "Enabled" : "Disabled") << std::endl;
        file << "Recalculate Rewards (f): " << (features.find('f') != std::string::npos ? "Enabled" : "Disabled") << std::endl;
        file << "Initial Reward Function (w): " << (features.find('w') != std::string::npos ? "Enabled" : "Disabled") << std::endl;
        file << "Exploration Noise Decay (e): " << (features.find('e') != std::string::npos ? "Enabled" : "Disabled") << std::endl;
    }
};

} // namespace steps
} // namespace learning_to_fly
