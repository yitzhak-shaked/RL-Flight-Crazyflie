#include <rl_tools/operations/cpu_mux.h>
#include <rl_tools/nn/operations_cpu_mux.h>
namespace rlt = RL_TOOLS_NAMESPACE_WRAPPER ::rl_tools;

#include <learning_to_fly/simulator/operations_cpu.h>
#include <learning_to_fly/simulator/metrics.h>

#include "config/config.h"
#include "constants.h"

#include <rl_tools/rl/algorithms/td3/loop.h>
#include <cstdlib>


#include "training_state.h"

#include "steps/checkpoint.h"
#include "steps/critic_reset.h"
#include "steps/curriculum.h"
#include "steps/log_reward.h"
#include "steps/logger.h"
#include "steps/trajectory_collection.h"
#include "steps/validation.h"
#include "steps/training_summary.h"
#include "steps/training_summary.h"

#include "helpers.h"

#include <filesystem>
#include <fstream>


namespace learning_to_fly{

    template <typename T_CONFIG>
    void init(TrainingState<T_CONFIG>& ts, typename T_CONFIG::TI seed = 0){
        using CONFIG = T_CONFIG;
        using T = typename CONFIG::T;
        using TI = typename CONFIG::TI;
        using ABLATION_SPEC = typename CONFIG::ABLATION_SPEC;
        auto env_parameters = parameters::environment<T, TI, ABLATION_SPEC>::parameters;
        auto env_parameters_eval = parameters::environment<T, TI, config::template ABLATION_SPEC_EVAL<ABLATION_SPEC>>::parameters;
        for (auto& env : ts.envs) {
            env.parameters = env_parameters;
        }
        ts.env_eval.parameters = env_parameters_eval;
        TI effective_seed = CONFIG::BASE_SEED + seed;
        ts.run_name = helpers::run_name<ABLATION_SPEC, CONFIG>(effective_seed);
        
        // Use checkpoint directory for all outputs instead of separate logs directory
        rlt::construct(ts.device, ts.device.logger, std::string("checkpoints/multirotor_td3"), ts.run_name);

        // Generate training parameters summary file using template-based generator
        std::string checkpoint_dir = "checkpoints/multirotor_td3/" + ts.run_name;
        steps::TrainingSummaryGenerator::generate_summary_file<CONFIG>(checkpoint_dir, ts.run_name);

        rlt::set_step(ts.device, ts.device.logger, 0);
        rlt::add_scalar(ts.device, ts.device.logger, "loop/seed", effective_seed);
        rlt::rl::algorithms::td3::loop::init(ts, effective_seed);
        ts.off_policy_runner.parameters = CONFIG::off_policy_runner_parameters;

        for(typename CONFIG::ENVIRONMENT& env: ts.validation_envs){
            env.parameters = parameters::environment<typename CONFIG::T, TI, ABLATION_SPEC>::parameters;
        }
        rlt::malloc(ts.device, ts.validation_actor_buffers);
        rlt::init(ts.device, ts.task, ts.validation_envs, ts.rng_eval);

        // info

        std::cout << "Environment Info: \n";
        std::cout << "\t" << "Observation dim: " << CONFIG::ENVIRONMENT::OBSERVATION_DIM << std::endl;
        std::cout << "\t" << "Observation dim privileged: " << CONFIG::ENVIRONMENT::OBSERVATION_DIM_PRIVILEGED << std::endl;
        std::cout << "\t" << "Action dim: " << CONFIG::ENVIRONMENT::ACTION_DIM << std::endl;
        
                // Show target position if using position-to-position training
        if constexpr (std::is_same_v<typename CONFIG::ABLATION_SPEC, learning_to_fly::config::POSITION_TO_POSITION_ABLATION_SPEC>) {
            std::cout << "\t" << "Target position: (" 
                      << learning_to_fly::constants::TARGET_POSITION_X<T> << ", "
                      << learning_to_fly::constants::TARGET_POSITION_Y<T> << ", "
                      << learning_to_fly::constants::TARGET_POSITION_Z<T> << ")" << std::endl;
            std::cout << "\t" << "Using position-to-position SMOOTH reward (V10 - ANTI-SHIVERING)" << std::endl;
            std::cout << "\t" << "  - V1 smooth flight foundation" << std::endl;
            std::cout << "\t" << "  - Exponential proximity reward (closer = more reward)" << std::endl;
            std::cout << "\t" << "  - ANTI-SHIVERING CONFIGURATION:" << std::endl;
            std::cout << "\t" << "    * Linear acceleration: 0.5 (was 0.05) - HEAVY penalty for jerky movements!" << std::endl;
            std::cout << "\t" << "    * Angular acceleration: 0.5 (was 0.05) - HEAVY penalty for rotation jerkiness!" << std::endl;
            std::cout << "\t" << "    * Linear velocity: 0.3 (reduced from 0.5) - more freedom to move smoothly" << std::endl;
            std::cout << "\t" << "    * Angular velocity: 0.05 - freedom to rotate" << std::endl;
            std::cout << "\t" << "  - Episode length: " << CONFIG::ENVIRONMENT_STEP_LIMIT << " steps (more time to reach target)" << std::endl;
        } else {
            std::cout << "\t" << "Using hover reward function" << std::endl;
        }
    }

    template <typename CONFIG>
    void step(TrainingState<CONFIG>& ts){
        using TI = typename CONFIG::TI;
        using T = typename CONFIG::T;
        if(ts.step % 10000 == 0){
            std::cout << "Step: " << ts.step << std::endl;
        }
        
        steps::logger(ts);
        steps::checkpoint(ts);
        steps::validation(ts);
        steps::curriculum(ts);
        rlt::rl::algorithms::td3::loop::step(ts);
        steps::trajectory_collection(ts);
        
        // Print evaluation results AFTER they're computed by td3::loop::step
        if constexpr (CONFIG::DETERMINISTIC_EVALUATION) {
            if(ts.step > 0 && ts.step % CONFIG::EVALUATION_INTERVAL == 0){
                TI evaluation_index = ts.step / CONFIG::EVALUATION_INTERVAL;
                if(evaluation_index > 0 && evaluation_index <= (CONFIG::STEP_LIMIT / CONFIG::EVALUATION_INTERVAL)){
                    auto& result = ts.evaluation_results[evaluation_index - 1];
                    std::cout << "📊 Evaluation at step " << ts.step << ":" << std::endl;
                    std::cout << "   Mean return: " << result.returns_mean 
                              << " (std: " << result.returns_std << ")" << std::endl;
                    std::cout << "   Mean episode length: " << result.episode_length_mean 
                              << " (std: " << result.episode_length_std << ")" << std::endl;
                }
            }
        }
    }
    template <typename CONFIG>
    void destroy(TrainingState<CONFIG>& ts){
        rlt::rl::algorithms::td3::loop::destroy(ts);
        rlt::destroy(ts.device, ts.task);
        rlt::free(ts.device, ts.validation_actor_buffers);
    }
}
