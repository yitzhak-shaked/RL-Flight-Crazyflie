
#include "training.h"

#include <chrono>

// Include checkpoint file if path is specified at compile time
#ifdef ACTOR_CHECKPOINT_FILE
#include ACTOR_CHECKPOINT_FILE
#endif


template <typename T_ABLATION_SPEC>
void run() {
    using namespace learning_to_fly::config;

    using CONFIG = learning_to_fly::config::Config<T_ABLATION_SPEC>;
    using TI = typename CONFIG::TI;

#ifdef LEARNING_TO_FLY_IN_SECONDS_BENCHMARK
    constexpr TI NUM_RUNS = 1;
#else
    constexpr TI NUM_RUNS = 1;
#endif

    for (TI run_i = 0; run_i < NUM_RUNS; run_i++){
        std::cout << "Run " << run_i << "\n";
        
        // Check if checkpoint path is specified
        constexpr bool has_checkpoint_path = (std::string_view(CONFIG::ACTOR_CHECKPOINT_INIT_PATH).length() > 0);
        if constexpr (has_checkpoint_path) {
            std::cout << "Checkpoint path specified: " << CONFIG::ACTOR_CHECKPOINT_INIT_PATH << std::endl;
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        learning_to_fly::TrainingState<CONFIG> ts;
        
        // Initialize training state (always use standard initialization first)
        learning_to_fly::init(ts, run_i);
        
        // If checkpoint path is specified and checkpoint file is included, load weights
        if constexpr (has_checkpoint_path) {
            #ifdef ACTOR_CHECKPOINT_FILE
            std::cout << "Loading actor weights from checkpoint..." << std::endl;
            // Copy weights from the included checkpoint to the training actor
            // The checkpoint namespace contains the actor weights
            rlt::copy(ts.device, ts.device, rl_tools::checkpoint::actor::model, ts.actor_critic.actor);
            
            // CRITICAL FIX: Reset optimizer state when loading pre-trained weights
            // The optimizer (Adam) has momentum and adaptive learning rates that were
            // tuned for the previous task (hover). We MUST reset these for the new task
            // to avoid catastrophic forgetting and unstable learning.
            std::cout << "Resetting optimizer states for transfer learning..." << std::endl;
            rlt::reset_optimizer_state(ts.device, ts.actor_critic.actor_optimizer, ts.actor_critic.actor);
            rlt::reset_optimizer_state(ts.device, ts.actor_critic.critic_optimizers[0], ts.actor_critic.critic_1);
            rlt::reset_optimizer_state(ts.device, ts.actor_critic.critic_optimizers[1], ts.actor_critic.critic_2);
            
            // Copy actor to actor_target after loading weights (important for TD3 stability)
            rlt::copy(ts.device, ts.device, ts.actor_critic.actor, ts.actor_critic.actor_target);
            
            std::cout << "Actor weights loaded and optimizer states reset successfully." << std::endl;
            #else
            std::cerr << "Warning: Checkpoint path specified (" << CONFIG::ACTOR_CHECKPOINT_INIT_PATH 
                      << ") but no checkpoint file included at compile time. Using random initialization." << std::endl;
            #endif
        }

        for(TI step_i=0; step_i < CONFIG::STEP_LIMIT; step_i++){
            learning_to_fly::step(ts);
        }

        learning_to_fly::destroy(ts);
        auto end = std::chrono::high_resolution_clock::now();

        std::cout << "Training took: " << std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count() << "s" << std::endl;
    }
}


int main() {
    run<learning_to_fly::config::POSITION_TO_POSITION_ABLATION_SPEC>();  // Position-to-position with policy switching
    return 0;
}