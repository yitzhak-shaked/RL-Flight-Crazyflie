// Example of how to initialize training with weights from a checkpoint file
// To use this example:
// 1. Copy your desired checkpoint .h file (e.g., hoverActor_000000000300000.h) to this directory
// 2. Modify the include line below to match your checkpoint file name
// 3. Compile and run this file instead of the regular training.cpp

#include "training_with_checkpoint.h"
#include <chrono>

// Include the checkpoint file you want to use for initialization
// Replace this with the path to your specific checkpoint file
#include "../actors/hover_actors/hoverActor_000000000300000.h"

template <typename T_ABLATION_SPEC>
void run_with_checkpoint() {
    using namespace learning_to_fly::config;

    using CONFIG = learning_to_fly::config::Config<T_ABLATION_SPEC>;
    using TI = typename CONFIG::TI;

#ifdef LEARNING_TO_FLY_IN_SECONDS_BENCHMARK
    constexpr TI NUM_RUNS = 1;
#else
    constexpr TI NUM_RUNS = 1;
#endif

    for (TI run_i = 0; run_i < NUM_RUNS; run_i++){
        std::cout << "Run " << run_i << " (with checkpoint initialization)\n";
        auto start = std::chrono::high_resolution_clock::now();
        
        learning_to_fly::TrainingState<CONFIG> ts;
        
        // Use the checkpoint initialization instead of regular init
        learning_to_fly::init_with_checkpoint(ts, run_i);

        for(TI step_i=0; step_i < CONFIG::STEP_LIMIT; step_i++){
            learning_to_fly::step(ts);
        }

        learning_to_fly::destroy(ts);
        auto end = std::chrono::high_resolution_clock::now();

        std::cout << "Training took: " << std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count() << "s" << std::endl;
    }
}

int main() {
    std::cout << "Training with checkpoint initialization from: " << rlt::checkpoint::meta::name << std::endl;
    run_with_checkpoint<learning_to_fly::config::DEFAULT_ABLATION_SPEC>();
    return 0;
}
