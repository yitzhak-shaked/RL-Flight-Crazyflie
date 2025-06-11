// ------------ Groups 1 ------------
#include <rl_tools/operations/cuda/group_1.h>
#include <rl_tools/operations/cpu/group_1.h>
#include <rl_tools/operations/cpu_tensorboard/group_1.h>
// ------------ Groups 2 ------------
#include <rl_tools/operations/cuda/group_2.h>
#include <rl_tools/operations/cpu/group_2.h>
#include <rl_tools/operations/cpu_tensorboard/group_2.h>
// ------------ Groups 3 ------------
#include <rl_tools/operations/cuda/group_3.h>
#include <rl_tools/operations/cpu/group_3.h>
#include <rl_tools/operations/cpu_tensorboard/group_3.h>

namespace rlt = RL_TOOLS_NAMESPACE_WRAPPER ::rl_tools;

#include <rl_tools/nn/operations_cuda.h>
#include <rl_tools/nn/operations_cpu.h>
#include <learning_to_fly/simulator/operations_cpu.h>
#include <learning_to_fly/simulator/metrics.h>

// Include CUDA-specific operations
#include <rl_tools/nn_models/operations_generic.h>
#include <rl_tools/rl/components/off_policy_runner/operations_cuda.h>
#include <rl_tools/rl/algorithms/td3/operations_cuda.h>
#include <rl_tools/rl/algorithms/td3/operations_generic.h>

#include "config/config_cuda.h"

#include "training_state.h"

// Don't include the problematic step headers for now
// #include "steps/checkpoint.h"
// #include "steps/critic_reset.h" 
// #include "steps/curriculum.h"
// #include "steps/log_reward.h"
// #include "steps/logger.h"
// #include "steps/trajectory_collection.h"
// #include "steps/validation.h"

#include "helpers.h"

#include <filesystem>
#include <fstream>
#include <fstream>

namespace learning_to_fly{
    namespace cuda {

        template <typename T_CONFIG>
        void init(TrainingState<T_CONFIG>& ts, typename T_CONFIG::TI seed = 0){
            using CONFIG = T_CONFIG;
            using T = typename CONFIG::T;
            using TI = typename CONFIG::TI;
            
            if constexpr (TrainingState<CONFIG>::HAS_DUAL_DEVICE) {
                std::cout << "Initializing CUDA Training with dual-device approach..." << std::endl;
                
                // Initialize both devices
                rlt::init(ts.device);
                rlt::construct(ts.device_init, ts.device_init.logger);
                
                // Create separate RNG for each device using proper pattern from working examples
                ts.rng_init = rlt::random::default_engine(typename CONFIG::DEVICE_INIT::SPEC::RANDOM{});
                ts.rng = rlt::random::default_engine(typename CONFIG::DEVICE::SPEC::RANDOM{});
                
                // Initialize on CPU first, then copy to CUDA
                rlt::malloc(ts.device_init, ts.actor_critic_init);
                rlt::malloc(ts.device, ts.actor_critic);
                
                rlt::malloc(ts.device_init, ts.off_policy_runner_init);
                rlt::malloc(ts.device, ts.off_policy_runner);
                
                // Initialize environments on CPU
                typename CONFIG::ENVIRONMENT envs[CONFIG::N_ENVIRONMENTS];
                rlt::init(ts.device_init, ts.off_policy_runner_init, envs);
                
                // Initialize neural networks on CPU
                rlt::init(ts.device_init, ts.actor_critic_init, ts.rng_init);
                
                // Copy initialized data from CPU to CUDA
                rlt::copy(ts.device_init, ts.device, ts.actor_critic_init, ts.actor_critic);
                rlt::copy(ts.device_init, ts.device, ts.off_policy_runner_init, ts.off_policy_runner);
            } else {
                std::cout << "Initializing single-device training..." << std::endl;
                
                // Single device setup - device_init is just an alias for device
                rlt::init(ts.device);
                
                // Single RNG setup
                ts.rng = rlt::random::default_engine(typename CONFIG::DEVICE::SPEC::RANDOM{});
                ts.rng_init = ts.rng;  // Same RNG for compatibility
                
                // Single device initialization
                rlt::malloc(ts.device, ts.actor_critic);
                rlt::malloc(ts.device, ts.off_policy_runner);
                
                // Initialize environments
                typename CONFIG::ENVIRONMENT envs[CONFIG::N_ENVIRONMENTS];
                rlt::init(ts.device, ts.off_policy_runner, envs);
                
                // Initialize neural networks
                rlt::init(ts.device, ts.actor_critic, ts.rng);
                
                // For single device, init structures are aliases
                ts.actor_critic_init = ts.actor_critic;
                ts.off_policy_runner_init = ts.off_policy_runner;
            }
            
            ts.step = 0;
            
            std::cout << "CUDA Training initialized successfully!" << std::endl;
            std::cout << "\t" << "Environment Observation dim: " << CONFIG::ENVIRONMENT::OBSERVATION_DIM << std::endl;
            std::cout << "\t" << "Environment Action dim: " << CONFIG::ENVIRONMENT::ACTION_DIM << std::endl;
        }

        template <typename CONFIG>
        void step(TrainingState<CONFIG>& ts){
            using TI = typename CONFIG::TI;
            using T = typename CONFIG::T;
            
            if(ts.step % 1000 == 0){
                std::cout << "CUDA Training Step: " << ts.step << std::endl;
            }
            
            // Simplified training step without complex logging operations
            // Use basic TD3 operations directly on CUDA device
            ts.rng = rlt::random::next(typename CONFIG::DEVICE::SPEC::RANDOM{}, ts.rng);
            
            // Basic training step - simplified for now
            // TODO: Implement full training step with buffers
            
            ts.step++;
        }
        
        template <typename CONFIG>
        void destroy(TrainingState<CONFIG>& ts){
            std::cout << "Cleaning up training..." << std::endl;
            
            if constexpr (TrainingState<CONFIG>::HAS_DUAL_DEVICE) {
                // Dual device cleanup
                rlt::free(ts.device_init, ts.actor_critic_init);
                rlt::free(ts.device, ts.actor_critic);
                rlt::free(ts.device_init, ts.off_policy_runner_init);
                rlt::free(ts.device, ts.off_policy_runner);
            } else {
                // Single device cleanup
                rlt::free(ts.device, ts.actor_critic);
                rlt::free(ts.device, ts.off_policy_runner);
            }
            
            std::cout << "Training cleanup complete." << std::endl;
        }
    }
}
