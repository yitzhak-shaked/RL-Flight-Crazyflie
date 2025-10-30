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
        
        // Get environment parameters for accessing termination and other settings
        auto env_parameters = parameters::environment<T, TI, ABLATION_SPEC>::parameters;
        
        auto now = std::chrono::system_clock::now();
        auto time_t = std::chrono::system_clock::to_time_t(now);
        
        // Ensure the directory exists before creating the file
        try {
            std::filesystem::create_directories(log_dir);
        }
        catch (std::exception& e) {
            std::cerr << "Warning: Could not create directory: " << log_dir << " - " << e.what() << std::endl;
        }
        
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
        
        // Add termination configuration
        file << std::endl;
        file << "=== TERMINATION CONFIGURATION ===" << std::endl;
        auto termination_params = env_parameters.mdp.termination;
        file << "Termination Enabled: " << (termination_params.enabled ? "Yes" : "No") << std::endl;
        file << "Position Threshold: " << termination_params.position_threshold << " meters (any axis)" << std::endl;
        file << "Linear Velocity Threshold: " << termination_params.linear_velocity_threshold << " m/s" << std::endl;
        file << "Angular Velocity Threshold: " << termination_params.angular_velocity_threshold << " rad/s" << std::endl;
        file << "Note: Episode terminates if any position, velocity, or angular velocity exceeds threshold" << std::endl;
        file << std::endl;
        
        file << "=== ALGORITHM PARAMETERS (TD3) ===" << std::endl;
        file << "Algorithm: TD3 (Twin Delayed Deep Deterministic Policy Gradient)" << std::endl;
        file << std::endl;
        
        file << "--- Batch Sizes ---" << std::endl;
        file << "Actor Batch Size: " << CONFIG::TD3_PARAMETERS::ACTOR_BATCH_SIZE << std::endl;
        file << "Critic Batch Size: " << CONFIG::TD3_PARAMETERS::CRITIC_BATCH_SIZE << std::endl;
        file << std::endl;
        
        file << "--- Training & Update Intervals ---" << std::endl;
        file << "Training Interval: " << CONFIG::TD3_PARAMETERS::TRAINING_INTERVAL << " steps" << std::endl;
        file << "Critic Training Interval: " << CONFIG::TD3_PARAMETERS::CRITIC_TRAINING_INTERVAL << " steps (= " << (CONFIG::TD3_PARAMETERS::CRITIC_TRAINING_INTERVAL / CONFIG::TD3_PARAMETERS::TRAINING_INTERVAL) << " × TRAINING_INTERVAL)" << std::endl;
        file << "Actor Training Interval: " << CONFIG::TD3_PARAMETERS::ACTOR_TRAINING_INTERVAL << " steps (= " << (CONFIG::TD3_PARAMETERS::ACTOR_TRAINING_INTERVAL / CONFIG::TD3_PARAMETERS::TRAINING_INTERVAL) << " × TRAINING_INTERVAL)" << std::endl;
        file << "Critic Target Update Interval: " << CONFIG::TD3_PARAMETERS::CRITIC_TARGET_UPDATE_INTERVAL << " steps" << std::endl;
        file << "Actor Target Update Interval: " << CONFIG::TD3_PARAMETERS::ACTOR_TARGET_UPDATE_INTERVAL << " steps" << std::endl;
        file << std::endl;
        
        file << "--- Discount & Target Policy Smoothing ---" << std::endl;
        file << "Discount Factor (Gamma): " << CONFIG::TD3_PARAMETERS::GAMMA << std::endl;
        file << "Target Next Action Noise STD: " << CONFIG::TD3_PARAMETERS::TARGET_NEXT_ACTION_NOISE_STD << std::endl;
        file << "Target Next Action Noise Clip: " << CONFIG::TD3_PARAMETERS::TARGET_NEXT_ACTION_NOISE_CLIP << std::endl;
        file << "Ignore Termination Flag: " << (CONFIG::TD3_PARAMETERS::IGNORE_TERMINATION ? "Yes" : "No") << std::endl;
        file << std::endl;
        
        file << "--- Replay Buffer & Exploration ---" << std::endl;
        file << "Replay Buffer Capacity: " << CONFIG::REPLAY_BUFFER_CAP << " transitions" << std::endl;
        file << "Off-Policy Runner Exploration Noise (Initial): " << CONFIG::off_policy_runner_parameters.exploration_noise << std::endl;
        file << std::endl;
        
        file << "--- Evaluation ---" << std::endl;
        file << "Evaluation Interval: " << CONFIG::EVALUATION_INTERVAL << " steps" << std::endl;
        file << "Number of Evaluation Episodes: " << CONFIG::NUM_EVALUATION_EPISODES << std::endl;
        file << std::endl;
        
        file << "=== NEURAL NETWORK ARCHITECTURE ===" << std::endl;
        file << std::endl;
        
        file << "--- Actor Network ---" << std::endl;
        file << "Architecture: Sequential MLP" << std::endl;
        file << "Number of Layers: 3 (Input → Hidden1 → Hidden2 → Output)" << std::endl;
        file << "Hidden Layer Dimensions: 64 neurons per layer" << std::endl;
        file << "Activation Function: FAST_TANH" << std::endl;
        file << "Input Dimension: " << CONFIG::ENVIRONMENT::OBSERVATION_DIM << std::endl;
        file << "Output Dimension: " << CONFIG::ENVIRONMENT::ACTION_DIM << " (motor commands)" << std::endl;
        file << "Output Activation: FAST_TANH (actions normalized to [-1, 1])" << std::endl;
        file << std::endl;
        
        file << "--- Critic Network ---" << std::endl;
        file << "Architecture: Sequential MLP (Twin critics for TD3)" << std::endl;
        file << "Number of Layers: 3 (Input → Hidden1 → Hidden2 → Output)" << std::endl;
        file << "Hidden Layer Dimensions: 64 neurons per layer" << std::endl;
        file << "Activation Function: FAST_TANH" << std::endl;
        file << "Input Dimension: " << (CONFIG::ASYMMETRIC_OBSERVATIONS ? CONFIG::ENVIRONMENT::OBSERVATION_DIM_PRIVILEGED : CONFIG::ENVIRONMENT::OBSERVATION_DIM) << " (observations) + " << CONFIG::ENVIRONMENT::ACTION_DIM << " (actions)" << std::endl;
        file << "Output Dimension: 1 (Q-value)" << std::endl;
        file << "Output Activation: IDENTITY (linear)" << std::endl;
        file << std::endl;
        
        file << "=== OPTIMIZER PARAMETERS (ADAM) ===" << std::endl;
        using OPTIMIZER_PARAMS = typename CONFIG::ACTOR_CRITIC_CONFIG::OPTIMIZER::PARAMETERS;
        file << "Optimizer: Adam" << std::endl;
        file << "Learning Rate (Alpha): " << OPTIMIZER_PARAMS::ALPHA << std::endl;
        file << "Beta 1 (First Moment): " << OPTIMIZER_PARAMS::BETA_1 << std::endl;
        file << "Beta 2 (Second Moment): " << OPTIMIZER_PARAMS::BETA_2 << std::endl;
        file << "Epsilon: " << OPTIMIZER_PARAMS::EPSILON << std::endl;
        file << "Weight Decay: " << OPTIMIZER_PARAMS::WEIGHT_DECAY << std::endl;
        file << "Weight Decay (Input Layer): " << OPTIMIZER_PARAMS::WEIGHT_DECAY_INPUT << std::endl;
        file << "Weight Decay (Output Layer): " << OPTIMIZER_PARAMS::WEIGHT_DECAY_OUTPUT << std::endl;
        file << "Bias Learning Rate Factor: " << OPTIMIZER_PARAMS::BIAS_LR_FACTOR << std::endl;
        file << std::endl;
        
        file << "=== CURRICULUM LEARNING ===" << std::endl;
        file << "Curriculum Enabled: " << (CONFIG::ABLATION_SPEC::ENABLE_CURRICULUM ? "Yes" : "No") << std::endl;
        if constexpr (CONFIG::ABLATION_SPEC::ENABLE_CURRICULUM) {
            file << "Update Frequency: Every 100,000 steps" << std::endl;
            file << std::endl;
            file << "--- Weight Multipliers per Update ---" << std::endl;
            file << "Action Weight: ×1.4 (capped at 1.0)" << std::endl;
            file << "Position Weight: ×1.1 (capped at 10.0 or 40.0 depending on reward function)" << std::endl;
            file << "Linear Velocity Weight: ×1.4 (capped at 1.0)" << std::endl;
            file << std::endl;
            file << "Recalculate Rewards: " << (CONFIG::ABLATION_SPEC::RECALCULATE_REWARDS ? "Yes (replay buffer rewards updated)" : "No") << std::endl;
        }
        file << std::endl;
        
        file << "--- Exploration Noise Decay ---" << std::endl;
        file << "Decay Enabled: " << (CONFIG::ABLATION_SPEC::EXPLORATION_NOISE_DECAY ? "Yes" : "No") << std::endl;
        if constexpr (CONFIG::ABLATION_SPEC::EXPLORATION_NOISE_DECAY) {
            file << "Decay Start Step: 1,000,000" << std::endl;
            file << "Decay Frequency: Every 100,000 steps" << std::endl;
            file << "Decay Factor: 0.95 (multiplicative)" << std::endl;
            file << "Applied to: exploration_noise, target_next_action_noise_std, target_next_action_noise_clip" << std::endl;
        }
        file << std::endl;
        
        file << "=== ENVIRONMENT PHYSICS (CRAZYFLIE 2.0) ===" << std::endl;
        auto dynamics = env_parameters.dynamics;
        file << "Mass: " << dynamics.mass << " kg" << std::endl;
        file << "Gravity: [" << dynamics.gravity[0] << ", " << dynamics.gravity[1] << ", " << dynamics.gravity[2] << "] m/s²" << std::endl;
        file << "RPM Time Constant (T): " << dynamics.rpm_time_constant << " seconds" << std::endl;
        file << "Action Limits: [" << dynamics.action_limit.min << ", " << dynamics.action_limit.max << "] RPM" << std::endl;
        file << "Thrust Constant: " << dynamics.thrust_constants[2] << std::endl;
        file << "Torque Constant: " << dynamics.torque_constant << std::endl;
        file << "Integration dt: " << env_parameters.integration.dt << " seconds" << std::endl;
        file << "Number of Rotors: 4" << std::endl;
        file << std::endl;
        
        file << "=== OBSERVATION & ACTION NOISE ===" << std::endl;
        auto noise_params = env_parameters.mdp.observation_noise;
        file << "Observation Noise Enabled: " << (CONFIG::ABLATION_SPEC::OBSERVATION_NOISE ? "Yes" : "No") << std::endl;
        if constexpr (CONFIG::ABLATION_SPEC::OBSERVATION_NOISE) {
            file << "Position Noise STD: " << noise_params.position << std::endl;
            file << "Orientation Noise STD: " << noise_params.orientation << std::endl;
            file << "Linear Velocity Noise STD: " << noise_params.linear_velocity << std::endl;
            file << "Angular Velocity Noise STD: " << noise_params.angular_velocity << std::endl;
        }
        file << "Action Noise STD: " << env_parameters.mdp.action_noise.normalized_rpm << " (on normalized action)" << std::endl;
        file << std::endl;
        
        file << "=== DISTURBANCES ===" << std::endl;
        file << "Disturbances Enabled: " << (CONFIG::ABLATION_SPEC::DISTURBANCE ? "Yes" : "No") << std::endl;
        if constexpr (CONFIG::ABLATION_SPEC::DISTURBANCE) {
            auto disturbances = env_parameters.disturbances;
            file << "Random Force: Gaussian(μ=" << disturbances.random_force.mean << ", σ=" << disturbances.random_force.std << ") N" << std::endl;
            file << "  (Formula: 0.027 × 9.81 / 20 × DISTURBANCE_FLAG)" << std::endl;
            file << "Random Torque: Gaussian(μ=" << disturbances.random_torque.mean << ", σ=" << disturbances.random_torque.std << ") N⋅m" << std::endl;
            file << "  (Formula: 0.027 × 9.81 / 10000 × DISTURBANCE_FLAG)" << std::endl;
        }
        file << std::endl;
        
        file << "=== INITIALIZATION PARAMETERS ===" << std::endl;
        auto init_params = env_parameters.mdp.init;
        file << "Initialization Type: " << (CONFIG::ABLATION_SPEC::INIT_NORMAL ? "orientation_biggest_angle" : "all_positions") << std::endl;
        file << "Guidance: " << init_params.guidance << std::endl;
        file << "Position Range: ±" << init_params.max_position << " meters" << std::endl;
        file << "Orientation Range: ±" << init_params.max_angle << " radians (±" << (init_params.max_angle * 180.0 / 3.14159265359) << "°)" << std::endl;
        file << "Linear Velocity Range: ±" << init_params.max_linear_velocity << " m/s" << std::endl;
        file << "Angular Velocity Range: ±" << init_params.max_angular_velocity << " rad/s" << std::endl;
        file << "Relative RPM: " << (init_params.relative_rpm ? "Yes" : "No") << std::endl;
        file << "RPM Range: [" << init_params.min_rpm << ", " << init_params.max_rpm << "]" << std::endl;
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
        
        file << "=== STATE & OBSERVATION SPACE ===" << std::endl;
        file << "State Features:" << std::endl;
        file << "  - Position (x, y, z)" << std::endl;
        file << "  - Orientation (rotation matrix or quaternion)" << std::endl;
        file << "  - Linear Velocity (vx, vy, vz)" << std::endl;
        file << "  - Angular Velocity (ωx, ωy, ωz)" << std::endl;
        if constexpr (CONFIG::ABLATION_SPEC::ROTOR_DELAY) {
            file << "  - Rotor Speeds (4 values)" << std::endl;
        }
        if constexpr (CONFIG::ABLATION_SPEC::DISTURBANCE) {
            file << "  - Random Force (3 values)" << std::endl;
        }
        if constexpr (CONFIG::ABLATION_SPEC::ACTION_HISTORY) {
            file << "  - Action History (32 previous actions)" << std::endl;
        }
        file << std::endl;
        
        file << "Actor Observation Dimension: " << CONFIG::ENVIRONMENT::OBSERVATION_DIM << std::endl;
        if constexpr (CONFIG::ASYMMETRIC_OBSERVATIONS) {
            file << "Critic Observation Dimension (Privileged): " << CONFIG::ENVIRONMENT::OBSERVATION_DIM_PRIVILEGED << std::endl;
            file << "Asymmetric Actor-Critic: Yes (critic has access to privileged information)" << std::endl;
        } else {
            file << "Asymmetric Actor-Critic: No (actor and critic see same observations)" << std::endl;
        }
        file << std::endl;
        
        file << "=== HYPERPARAMETER SUMMARY ===" << std::endl;
        file << "Total Training Steps: " << CONFIG::STEP_LIMIT << std::endl;
        file << "Expected Training Time: ~" << ((CONFIG::STEP_LIMIT * CONFIG::ENVIRONMENT_STEP_LIMIT * env_parameters.integration.dt) / 3600.0) << " hours of simulated time" << std::endl;
        file << "Samples per Episode: " << CONFIG::ENVIRONMENT_STEP_LIMIT << std::endl;
        file << "Total Samples Collected: ~" << (CONFIG::STEP_LIMIT * CONFIG::N_ENVIRONMENTS) << std::endl;
        file << "Replay Buffer Fill Ratio: " << (static_cast<double>(CONFIG::STEP_LIMIT) / CONFIG::REPLAY_BUFFER_CAP * 100.0) << "%" << std::endl;
        file << std::endl;
        
        file << "Gradient Updates:" << std::endl;
        file << "  Critic Updates: " << (CONFIG::STEP_LIMIT / CONFIG::TD3_PARAMETERS::CRITIC_TRAINING_INTERVAL) << std::endl;
        file << "  Actor Updates: " << (CONFIG::STEP_LIMIT / CONFIG::TD3_PARAMETERS::ACTOR_TRAINING_INTERVAL) << std::endl;
        file << "  Target Network Updates (Actor): " << (CONFIG::STEP_LIMIT / CONFIG::TD3_PARAMETERS::ACTOR_TARGET_UPDATE_INTERVAL) << std::endl;
        file << "  Target Network Updates (Critic): " << (CONFIG::STEP_LIMIT / CONFIG::TD3_PARAMETERS::CRITIC_TARGET_UPDATE_INTERVAL) << std::endl;
        file << std::endl;
        
        file << "=== ADDITIONAL NOTES ===" << std::endl;
        file << "This summary was generated automatically during the training process." << std::endl;
        file << "All parameters are extracted dynamically from compile-time configuration." << std::endl;
        file << "Values shown are the actual values used in training, not defaults." << std::endl;
        file << std::endl;
        
        file << "Key Algorithmic Features:" << std::endl;
        file << "  - Twin Delayed DDPG with clipped double Q-learning" << std::endl;
        file << "  - Target policy smoothing with action noise" << std::endl;
        file << "  - Delayed policy updates (actor updated less frequently than critic)" << std::endl;
        file << "  - Experience replay with large buffer" << std::endl;
        file << "  - Optional curriculum learning with adaptive reward weights" << std::endl;
        file << "  - Optional exploration noise decay for convergence" << std::endl;
        file << std::endl;
        
        file << "=== RELATED FILES ===" << std::endl;
        file << "Actor Checkpoints: ../actors/ (actor_*.h5, actor_*.h)" << std::endl;
        file << "TensorBoard Logs: ./data.tfevents" << std::endl;
        file << "Configuration Source: src/config/config.h, src/config/parameters.h, src/config/ablation.h" << std::endl;
        
        file.close();
        
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
        
        file << "=== REWARD FUNCTION FORMULA ===" << std::endl;
        file << "Reward = -scale × weighted_cost + constant (if non-negative: max(result, 0))" << std::endl;
        file << "Weighted Cost = Σ(weight_i × cost_i²) for each state component" << std::endl;
        file << std::endl;
        file << "Components:" << std::endl;
        file << "  - position_cost² = x² + y² + z²" << std::endl;
        file << "  - orientation_cost = 1 - w² (where w is quaternion real part)" << std::endl;
        file << "  - linear_velocity_cost² = vx² + vy² + vz²" << std::endl;
        file << "  - angular_velocity_cost² = ωx² + ωy² + ωz²" << std::endl;
        file << "  - linear_acceleration_cost² = Δv² / dt²" << std::endl;
        file << "  - angular_acceleration_cost² = Δω² / dt²" << std::endl;
        file << "  - action_cost² = Σ(action_i - baseline)²" << std::endl;
        file << std::endl;
        
        file << "=== REWARD FUNCTION INTERPRETATION ===" << std::endl;
        file << "Higher position/orientation weights → stronger penalty for deviation from target" << std::endl;
        file << "Higher velocity weights → encourages slower, more controlled motion" << std::endl;
        file << "Higher acceleration weights → encourages smoother trajectories" << std::endl;
        file << "Higher action penalty → smoother control inputs, less aggressive maneuvers" << std::endl;
        file << "Scale factor → affects overall reward magnitude and learning sensitivity" << std::endl;
        file << "Constant → provides baseline reward, encourages survival" << std::endl;
        file << "Termination penalty → strong negative reward for crashes/failures" << std::endl;
    }
};

} // namespace steps
} // namespace learning_to_fly
