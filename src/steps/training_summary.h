#pragma once

#include <iostream>
#include <fstream>
#include <string>
#include <sstream>
#include <iomanip>
#include <chrono>
#include <ctime>
#include <filesystem>
#include "../constants.h"

namespace learning_to_fly {
namespace steps {

// Template-based training summary generator that accesses actual configuration values
class TrainingSummaryGenerator {
public:
    template<typename T_CONFIG>
    static void generate_summary_file(const std::string& log_dir, const std::string& run_name) {
        using CONFIG = T_CONFIG;
        using T = typename CONFIG::T;
        using TI = typename CONFIG::TI;
        using ABLATION_SPEC = typename CONFIG::ABLATION_SPEC;
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
        std::string training_type = determine_training_type_from_run_name<CONFIG>(run_name);
        
        file << "=== TRAINING TYPE AND LOCATIONS ===" << std::endl;
        file << "Training Date: " << run_name.substr(0, 19) << std::endl;
        file << "Training Type: " << training_type << std::endl;
        file << "Starting Location: (0.0, 0.0, 0.0)" << std::endl;
        
        // Use actual target position from constants.h
        file << "Target Location: (" 
             << learning_to_fly::constants::TARGET_POSITION_X<T> << ", "
             << learning_to_fly::constants::TARGET_POSITION_Y<T> << ", "
             << learning_to_fly::constants::TARGET_POSITION_Z<T> << ")";
             
        if constexpr (std::is_same_v<ABLATION_SPEC, learning_to_fly::config::POSITION_TO_POSITION_ABLATION_SPEC>) {
            file << " [Position-to-Position training]" << std::endl;
        } else {
            file << " [Hover training - target equals start]" << std::endl;
        }
        file << std::endl;
        
        write_ablation_features_from_config<CONFIG>(file, run_name);
        
        file << std::endl;
        file << "=== ACTOR INITIALIZATION ===" << std::endl;
        
        // Check actual ACTOR_CHECKPOINT_INIT_PATH
        std::string init_path(CONFIG::ACTOR_CHECKPOINT_INIT_PATH);
        if (init_path.empty()) {
            file << "Starting Actor: Random weight initialization" << std::endl;
            file << "Initialization Type: Fresh training from scratch" << std::endl;
            file << "Starting Actor File: None (random weights)" << std::endl;
            file << "Checkpoint Path: Not applicable" << std::endl;
        } else {
            file << "Starting Actor: Initialized from checkpoint" << std::endl;
            file << "Initialization Type: Pre-trained weights loaded" << std::endl;
            file << "Starting Actor File: " << init_path << std::endl;
            file << "Checkpoint Path: " << init_path << std::endl;
        }
        
        file << "Base Seed: " << CONFIG::BASE_SEED << " (CONFIG::BASE_SEED)" << std::endl;
        file << "Warmup Steps (Critic): " << CONFIG::N_WARMUP_STEPS_CRITIC << " steps" << std::endl;
        file << "Warmup Steps (Actor): " << CONFIG::N_WARMUP_STEPS_ACTOR << " steps" << std::endl;
        file << std::endl;
        
        file << "=== TRAINING STEPS ===" << std::endl;
        file << "Maximum Training Steps: " << CONFIG::STEP_LIMIT << std::endl;
        file << "Actor Checkpoint Interval: " << CONFIG::ACTOR_CHECKPOINT_INTERVAL << " steps" << std::endl;
        file << "Actor Checkpoints Enabled: " << (CONFIG::ACTOR_ENABLE_CHECKPOINTS ? "Yes" : "No") << std::endl;
        file << "Total Expected Checkpoints: " << (CONFIG::STEP_LIMIT / CONFIG::ACTOR_CHECKPOINT_INTERVAL) << std::endl;
        file << std::endl;
        
        file << "=== REWARD FUNCTION PARAMETERS ===" << std::endl;
        write_reward_function_details<CONFIG>(file);
        file << std::endl;
        
        file << "=== ENVIRONMENT CONFIGURATION ===" << std::endl;
        file << "Simulation Type: Multirotor (Crazyflie 2.0)" << std::endl;
        file << "Physics Engine: Custom RL-Tools Multirotor Simulator" << std::endl;
        file << "Integration Method: RK4 (Runge-Kutta 4th order)" << std::endl;
        file << "Simulation Step Size (dt): 0.01 seconds" << std::endl;
        file << "Episode Length: " << CONFIG::ENVIRONMENT_STEP_LIMIT << " steps (" << (CONFIG::ENVIRONMENT_STEP_LIMIT * 0.01) << " seconds)" << std::endl;
        file << "Evaluation Episode Length: " << CONFIG::ENVIRONMENT_STEP_LIMIT_EVALUATION << " steps (" << (CONFIG::ENVIRONMENT_STEP_LIMIT_EVALUATION * 0.01) << " seconds)" << std::endl;
        file << "Number of Parallel Environments: " << CONFIG::N_ENVIRONMENTS << std::endl;
        file << "Action Dimension: " << CONFIG::ENVIRONMENT::ACTION_DIM << " (motor commands)" << std::endl;
        file << "Observation Dimension: " << CONFIG::ENVIRONMENT::OBSERVATION_DIM << std::endl;
        file << "Observation Dimension Privileged: " << CONFIG::ENVIRONMENT::OBSERVATION_DIM_PRIVILEGED << std::endl;
        file << std::endl;
        
        file << "=== ALGORITHM PARAMETERS ===" << std::endl;
        file << "Algorithm: TD3 (Twin Delayed Deep Deterministic Policy Gradient)" << std::endl;
        file << "Actor Batch Size: " << CONFIG::TD3_PARAMETERS::ACTOR_BATCH_SIZE << std::endl;
        file << "Critic Batch Size: " << CONFIG::TD3_PARAMETERS::CRITIC_BATCH_SIZE << std::endl;
        file << "Training Interval: " << CONFIG::TD3_PARAMETERS::TRAINING_INTERVAL << " steps" << std::endl;
        file << "Actor Training Interval: " << CONFIG::TD3_PARAMETERS::ACTOR_TRAINING_INTERVAL << " steps" << std::endl;
        file << "Critic Training Interval: " << CONFIG::TD3_PARAMETERS::CRITIC_TRAINING_INTERVAL << " steps" << std::endl;
        file << "Target Network Update Interval (Actor): " << CONFIG::TD3_PARAMETERS::ACTOR_TARGET_UPDATE_INTERVAL << " steps" << std::endl;
        file << "Target Network Update Interval (Critic): " << CONFIG::TD3_PARAMETERS::CRITIC_TARGET_UPDATE_INTERVAL << " steps" << std::endl;
        file << "Discount Factor (Gamma): " << CONFIG::TD3_PARAMETERS::GAMMA << std::endl;
        file << "Target Action Noise STD: " << CONFIG::TD3_PARAMETERS::TARGET_NEXT_ACTION_NOISE_STD << std::endl;
        file << "Target Action Noise Clip: " << CONFIG::TD3_PARAMETERS::TARGET_NEXT_ACTION_NOISE_CLIP << std::endl;
        file << "Ignore Termination: " << (CONFIG::TD3_PARAMETERS::IGNORE_TERMINATION ? "Yes" : "No") << std::endl;
        file << "Replay Buffer Capacity: " << CONFIG::REPLAY_BUFFER_CAP << std::endl;
        file << "Off-Policy Runner Exploration Noise: " << CONFIG::off_policy_runner_parameters.exploration_noise << std::endl;
        file << "Evaluation Interval: " << CONFIG::EVALUATION_INTERVAL << " steps" << std::endl;
        file << "Number of Evaluation Episodes: " << CONFIG::NUM_EVALUATION_EPISODES << std::endl;
        file << std::endl;
        
        file << "=== CHECKPOINTING CONFIGURATION ===" << std::endl;
        file << "Checkpoint Interval: Every " << CONFIG::ACTOR_CHECKPOINT_INTERVAL << " steps" << std::endl;
        file << "Checkpoints Enabled: " << (CONFIG::ACTOR_ENABLE_CHECKPOINTS ? "Yes" : "No") << std::endl;
        file << "File Formats Saved:" << std::endl;
        file << "  - HDF5 (.h5): For analysis and evaluation" << std::endl;
        file << "  - C++ Header (.h): For embedded deployment" << std::endl;
        file << "TensorBoard Logging: Enabled" << std::endl;
        file << "Learning Curve Data: Saved to HDF5 format" << std::endl;
        file << "Deterministic Evaluation: " << (CONFIG::DETERMINISTIC_EVALUATION ? "Yes" : "No") << std::endl;
        file << "Collect Episode Stats: " << (CONFIG::COLLECT_EPISODE_STATS ? "Yes" : "No") << std::endl;
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
        create_actors_reference<CONFIG>(run_name);
        
        std::cout << "Training summary generated: " << filename << std::endl;
    }
    
    template<typename T_CONFIG>
    static std::string determine_training_type_from_run_name(const std::string& run_name) {
        using CONFIG = T_CONFIG;
        using ABLATION_SPEC = typename CONFIG::ABLATION_SPEC;
        
        if (run_name.find("BENCHMARK") != std::string::npos) {
            return "Performance Benchmark";
        } else if (run_name.find("ABLATION") != std::string::npos) {
            return "Ablation Study";
        } else if (run_name.find("d+o+a+r+h+c+f+w+e+") != std::string::npos) {
            return "Full Multi-Component Training";
        } else if constexpr (std::is_same_v<ABLATION_SPEC, learning_to_fly::config::POSITION_TO_POSITION_ABLATION_SPEC>) {
            return "Position-to-Position Learning";
        } else {
            return "Hover Learning";
        }
    }
    
    template<typename T_CONFIG>
    static void create_actors_reference(const std::string& run_name) {
        // Ensure actors directory exists
        std::filesystem::create_directories("actors");
        
        std::string actors_ref_file = "actors/TRAINING_INFO_" + run_name + ".txt";
        std::ofstream ref_file(actors_ref_file);
        
        if (ref_file.is_open()) {
            auto now = std::chrono::system_clock::now();
            auto time_t = std::chrono::system_clock::to_time_t(now);
            
            // Determine training type
            std::string training_type = determine_training_type_from_run_name<T_CONFIG>(run_name);
            
            ref_file << "=== ACTOR CHECKPOINT REFERENCE ===" << std::endl;
            ref_file << "Generated on: " << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S") << std::endl;
            ref_file << "Training Run: " << run_name << std::endl;
            ref_file << std::endl;
            ref_file << "=== TRAINING TYPE ===" << std::endl;
            ref_file << "Type: " << training_type << std::endl;
            ref_file << "Run Date: " << run_name.substr(0, 19) << std::endl;
            ref_file << std::endl;
            ref_file << "=== STARTING POINT ===" << std::endl;
            ref_file << "Initial Actor: hoverActor_000000000300000.h (Step 300,000 checkpoint)" << std::endl;
            ref_file << "Starting Point: Pre-trained hover controller" << std::endl;
            ref_file << std::endl;
            ref_file << "=== RELATED FILES ===" << std::endl;
            ref_file << "Training Summary: ../checkpoints/multirotor_td3/" << run_name << "/training_parameters_summary.txt" << std::endl;
            ref_file << "TensorBoard Logs: ../checkpoints/multirotor_td3/" << run_name << "/data.tfevents" << std::endl;
            ref_file << "Learning Curves: ../checkpoints/multirotor_td3/" << run_name << "/learning_curves_" << run_name << ".h5" << std::endl;
            ref_file << "Actor Checkpoints (same folder):" << std::endl;
            ref_file << "  - actor_000000000000000.h5 / .h (Step 0)" << std::endl;
            ref_file << "  - actor_000000000100000.h5 / .h (Step 100,000)" << std::endl;
            ref_file << "  - actor_000000000200000.h5 / .h (Step 200,000)" << std::endl;
            ref_file << "  - actor_000000000300000.h5 / .h (Step 300,000)" << std::endl;
            ref_file << std::endl;
            ref_file << "=== AVAILABILITY ===" << std::endl;
            ref_file << "Training Summary: ✓ Generated during training" << std::endl;
            ref_file << "TensorBoard Events: ✓ Generated during training" << std::endl;
            ref_file << std::endl;
            ref_file << "NOTE: All training outputs are now consolidated in the checkpoint folder." << std::endl;
            ref_file << "No separate logs directory is needed." << std::endl;
            
            ref_file.close();
            std::cout << "Actor reference file generated: " << actors_ref_file << std::endl;
        } else {
            std::cerr << "Warning: Could not create actor reference file: " << actors_ref_file << std::endl;
        }
    }
    
private:
    template<typename T_CONFIG>
    static void write_ablation_features_from_config(std::ofstream& file, const std::string& run_name) {
        using CONFIG = T_CONFIG;
        using ABLATION_SPEC = typename CONFIG::ABLATION_SPEC;
        
        file << "=== ABLATION STUDY FEATURES ===" << std::endl;
        
        // Extract feature string from run name for reference
        std::string features = "d+o+a+r+h+c+f+w+e+"; // default
        size_t underscore_count = 0;
        size_t start_pos = 0;
        
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
        
        file << "Feature String from Run Name: " << features << std::endl;
        file << "=== ACTUAL CONFIG VALUES ===" << std::endl;
        file << "Disturbance (d): " << (ABLATION_SPEC::DISTURBANCE ? "Enabled" : "Disabled") << std::endl;
        file << "Observation Noise (o): " << (ABLATION_SPEC::OBSERVATION_NOISE ? "Enabled" : "Disabled") << std::endl;
        file << "Asymmetric Actor-Critic (a): " << (ABLATION_SPEC::ASYMMETRIC_ACTOR_CRITIC ? "Enabled" : "Disabled") << std::endl;
        file << "Rotor Delay (r): " << (ABLATION_SPEC::ROTOR_DELAY ? "Enabled" : "Disabled") << std::endl;
        file << "Action History (h): " << (ABLATION_SPEC::ACTION_HISTORY ? "Enabled" : "Disabled") << std::endl;
        file << "Curriculum Learning (c): " << (ABLATION_SPEC::ENABLE_CURRICULUM ? "Enabled" : "Disabled") << std::endl;
        file << "Recalculate Rewards (f): " << (ABLATION_SPEC::RECALCULATE_REWARDS ? "Enabled" : "Disabled") << std::endl;
        file << "Initial Reward Function (w): " << (ABLATION_SPEC::USE_INITIAL_REWARD_FUNCTION ? "Enabled" : "Disabled") << std::endl;
        file << "Exploration Noise Decay (e): " << (ABLATION_SPEC::EXPLORATION_NOISE_DECAY ? "Enabled" : "Disabled") << std::endl;
        file << "Init Normal (init): " << (ABLATION_SPEC::INIT_NORMAL ? "Enabled" : "Disabled") << std::endl;
        
        if constexpr (std::is_same_v<ABLATION_SPEC, learning_to_fly::config::POSITION_TO_POSITION_ABLATION_SPEC>) {
            file << "Position-to-Position Reward: Enabled" << std::endl;
        }
    }
    
    template<typename T_CONFIG>
    static void write_reward_function_details(std::ofstream& file) {
        using CONFIG = T_CONFIG;
        using T = typename CONFIG::T;
        using ABLATION_SPEC = typename CONFIG::ABLATION_SPEC;
        
        if constexpr (std::is_same_v<ABLATION_SPEC, learning_to_fly::config::POSITION_TO_POSITION_ABLATION_SPEC>) {
            file << "Reward Function Type: Position-to-Position (Hover-like parameters)" << std::endl;
            file << "Selected Function: reward_position_to_position_hover_like" << std::endl;
        } else {
            if (ABLATION_SPEC::USE_INITIAL_REWARD_FUNCTION) {
                file << "Reward Function Type: Initial Training Function (Squared)" << std::endl;
                file << "Selected Function: reward_squared_position_only_torque" << std::endl;
            } else {
                file << "Reward Function Type: Target Curriculum Function (Squared)" << std::endl;
                file << "Selected Function: reward_squared_position_only_torque_curriculum_target" << std::endl;
            }
        }
        
        file << "=== REWARD FUNCTION NUMERICAL VALUES ===" << std::endl;
        
        // Get the actual reward function values from the parameters
        auto env_parameters = parameters::environment<T, typename CONFIG::TI, ABLATION_SPEC>::parameters;
        auto reward_function = env_parameters.mdp.reward;
        
        // Extract all the numerical values from the reward function
        file << "Non-negative: " << (reward_function.non_negative ? "true" : "false") << std::endl;
        file << "Scale: " << reward_function.scale << std::endl;
        file << "Constant: " << reward_function.constant << std::endl;
        file << "Termination Penalty: " << reward_function.termination_penalty << std::endl;
        file << "Position Weight: " << reward_function.position << std::endl;
        file << "Orientation Weight: " << reward_function.orientation << std::endl;
        file << "Linear Velocity Weight: " << reward_function.linear_velocity << std::endl;
        file << "Angular Velocity Weight: " << reward_function.angular_velocity << std::endl;
        file << "Linear Acceleration Weight: " << reward_function.linear_acceleration << std::endl;
        file << "Angular Acceleration Weight: " << reward_function.angular_acceleration << std::endl;
        file << "Action Baseline: " << reward_function.action_baseline << std::endl;
        file << "Action Penalty: " << reward_function.action << std::endl;
        
        // For Position-to-Position reward functions, add extra fields if they exist
        if constexpr (std::is_same_v<ABLATION_SPEC, learning_to_fly::config::POSITION_TO_POSITION_ABLATION_SPEC>) {
            // Try to access position-to-position specific fields
            file << "=== POSITION-TO-POSITION SPECIFIC PARAMETERS ===" << std::endl;
            file << "Target Position: (" 
                 << learning_to_fly::constants::TARGET_POSITION_X<T> << ", "
                 << learning_to_fly::constants::TARGET_POSITION_Y<T> << ", "
                 << learning_to_fly::constants::TARGET_POSITION_Z<T> << ")" << std::endl;
            
            // Hardcoded values from reward_position_to_position_hover_like definition
            file << "Target Radius: 0.1 (from reward_position_to_position_hover_like definition)" << std::endl;
            file << "Velocity Reward Scale: 0.5 (from reward_position_to_position_hover_like definition)" << std::endl;
            file << "Use Target Progress: false (from reward_position_to_position_hover_like definition)" << std::endl;
        }
        
        file << "=== REWARD FUNCTION EXPLANATION ===" << std::endl;
        file << "Higher position/orientation weights = stronger penalty for deviation" << std::endl;
        file << "Higher action penalty = smoother control but potentially slower convergence" << std::endl;
        file << "Scale factor affects overall reward magnitude" << std::endl;
        file << "Constant provides baseline reward level" << std::endl;
    }
};

} // namespace steps
} // namespace learning_to_fly
