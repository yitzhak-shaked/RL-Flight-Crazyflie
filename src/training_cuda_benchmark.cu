#include "training_cuda.h"

#include <chrono>

template <typename T_ABLATION_SPEC>
void run(){
    using namespace learning_to_fly::config;

    using CONFIG = learning_to_fly::config::Config<T_ABLATION_SPEC>;
    using TI = typename CONFIG::TI;

    constexpr TI NUM_RUNS = 1;

    for (TI run_i = 0; run_i < NUM_RUNS; run_i++){
        std::cout << "CUDA Benchmark Run " << run_i << "\n";
        auto start = std::chrono::high_resolution_clock::now();
        learning_to_fly::TrainingState<CONFIG> ts;
        learning_to_fly::cuda::init(ts, run_i);

        for(TI step_i=0; step_i < CONFIG::STEP_LIMIT; step_i++){
            learning_to_fly::cuda::step(ts);
        }

        learning_to_fly::cuda::destroy(ts);
        auto end = std::chrono::high_resolution_clock::now();

        std::cout << "CUDA Benchmark Training took: " << std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count() << "s" << std::endl;
    }
}

int main(){
    std::cout << "Starting CUDA-accelerated RL benchmark..." << std::endl;
    run<learning_to_fly::config::DEFAULT_ABLATION_SPEC>();
    return 0;
}
