
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
            std::cout << "Actor weights loaded successfully." << std::endl;
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
    run<learning_to_fly::config::DEFAULT_ABLATION_SPEC>();  // Changed from POSITION_TO_POSITION for faster training
    return 0;
}