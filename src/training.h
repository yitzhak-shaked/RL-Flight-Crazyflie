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
#include "steps/validation.h"
#include "steps/training_summary.h"
#include "steps/training_summary.h"
#include "steps/policy_switch.h"
#include "policy_switching.h"
#include "off_policy_runner_with_policy_switching.h"
#include "steps/trajectory_collection.h"  // Must come after policy_switching.h

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

        // Initialize policy switching if enabled
        if constexpr (CONFIG::ENABLE_POLICY_SWITCHING) {
            std::cout << "\n=== POLICY SWITCHING ENABLED ===" << std::endl;
            steps::enable_policy_switching(ts, std::string(CONFIG::HOVER_ACTOR_PATH), CONFIG::POLICY_SWITCH_THRESHOLD);
            std::cout << "================================\n" << std::endl;
        } else {
            ts.use_policy_switching = false;
        }

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
        using SPEC = CONFIG;
        
        if(ts.step % 10000 == 0){
            std::cout << "Step: " << ts.step << std::endl;
        }
        
        steps::logger(ts);
        steps::validation(ts);
        steps::curriculum(ts);
        
        // =====================================================================
        // CUSTOM TD3 STEP WITH POLICY SWITCHING
        // =====================================================================
        // This is a modified version of rlt::rl::algorithms::td3::loop::step
        // that uses policy switching for data collection
        
        bool finished = false;
        rlt::set_step(ts.device, ts.device.logger, ts.step);
        
        // Evaluation
        if constexpr(SPEC::DETERMINISTIC_EVALUATION == true){
            if(ts.step % SPEC::EVALUATION_INTERVAL == 0){
                auto result = rlt::evaluate(ts.device, ts.env_eval, ts.ui, ts.actor_critic.actor_target, 
                    rlt::rl::utils::evaluation::Specification<SPEC::NUM_EVALUATION_EPISODES, SPEC::ENVIRONMENT_STEP_LIMIT_EVALUATION>(), 
                    ts.observations_mean, ts.observations_std, ts.actor_deterministic_evaluation_buffers, ts.rng_eval, false);
                rlt::logging::text(ts.device, ts.device.logger, "Step: ", ts.step, " (mean return: ", result.returns_mean, ", mean episode length: ", result.episode_length_mean, ")");
                TI current_evaluation_i = ts.step / SPEC::EVALUATION_INTERVAL;
                assert(current_evaluation_i < TrainingState<CONFIG>::N_EVALUATIONS);
                ts.evaluation_results[current_evaluation_i] = result;
                rlt::add_scalar(ts.device, ts.device.logger, "evaluation/returns_mean", result.returns_mean);
                rlt::add_scalar(ts.device, ts.device.logger, "evaluation/returns_std", result.returns_std);
                rlt::add_scalar(ts.device, ts.device.logger, "evaluation/episode_length_mean", result.episode_length_mean);
                rlt::add_scalar(ts.device, ts.device.logger, "evaluation/episode_length_std", result.episode_length_std);
            }
        }
        
        // Data collection with policy switching
        if (ts.use_policy_switching && ts.hover_actor_loaded) {
            off_policy_runner::step_with_policy_switching(
                ts.device, ts.off_policy_runner, 
                ts.actor_critic.actor, ts.hover_actor,
                ts.actor_buffers_eval, ts.hover_actor_buffer,
                ts.rng, true, ts.policy_switch_threshold
            );
        } else {
            rlt::step(ts.device, ts.off_policy_runner, ts.actor_critic.actor, ts.actor_buffers_eval, ts.rng);
        }
        
        // Critic training
        if(ts.step > SPEC::N_WARMUP_STEPS_CRITIC && ts.step % SPEC::TD3_PARAMETERS::CRITIC_TRAINING_INTERVAL == 0){
            for(TI critic_i = 0; critic_i < 2; critic_i++){
                rlt::target_action_noise(ts.device, ts.actor_critic, ts.critic_training_buffers.target_next_action_noise, ts.rng);
                rlt::gather_batch(ts.device, ts.off_policy_runner, ts.critic_batch, ts.rng);
                rlt::train_critic(ts.device, ts.actor_critic, critic_i == 0 ? ts.actor_critic.critic_1 : ts.actor_critic.critic_2, 
                    ts.critic_batch, ts.critic_optimizers[critic_i], ts.actor_buffers[critic_i], ts.critic_buffers[critic_i], ts.critic_training_buffers);
            }
            T critic_1_loss = rlt::critic_loss(ts.device, ts.actor_critic, ts.actor_critic.critic_1, ts.critic_batch, ts.actor_buffers[0], ts.critic_buffers[0], ts.critic_training_buffers);
            rlt::add_scalar(ts.device, ts.device.logger, "critic_1_loss", critic_1_loss, 100);
        }

        // Actor training
        if(ts.step > SPEC::N_WARMUP_STEPS_ACTOR && ts.step % SPEC::TD3_PARAMETERS::ACTOR_TRAINING_INTERVAL == 0){
            rlt::gather_batch(ts.device, ts.off_policy_runner, ts.actor_batch, ts.rng);
            rlt::train_actor(ts.device, ts.actor_critic, ts.actor_batch, ts.actor_optimizer, ts.actor_buffers[0], ts.critic_buffers[0], ts.actor_training_buffers);

            T actor_value = rlt::mean(ts.device, ts.actor_training_buffers.state_action_value);
            rlt::add_scalar(ts.device, ts.device.logger, "actor_value", actor_value, 100);
        }
        
        // Target updates
        if(ts.step > SPEC::N_WARMUP_STEPS_CRITIC && ts.step % SPEC::TD3_PARAMETERS::CRITIC_TARGET_UPDATE_INTERVAL == 0){
            rlt::update_critic_targets(ts.device, ts.actor_critic);
        }
        if(ts.step > SPEC::N_WARMUP_STEPS_ACTOR && ts.step % SPEC::TD3_PARAMETERS::ACTOR_TARGET_UPDATE_INTERVAL == 0) {
            rlt::update_actor_target(ts.device, ts.actor_critic);
        }

        ts.step++;
        ts.finished = (ts.step >= SPEC::STEP_LIMIT);
        // =====================================================================
        // END CUSTOM TD3 STEP
        // =====================================================================
        
        steps::trajectory_collection(ts);
        steps::checkpoint(ts);
        
        // Print evaluation results
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
