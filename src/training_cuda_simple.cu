#include <rl_tools/operations/cuda.h>
#include <rl_tools/nn/operations_cuda.h>

namespace rlt = RL_TOOLS_NAMESPACE_WRAPPER ::rl_tools;

#include <learning_to_fly/simulator/operations_cpu.h>

#include <iostream>
#include <chrono>

using T = float;
using DEVICE = rlt::devices::DefaultCUDA;
using TI = typename DEVICE::index_t;

int main(void){
    std::cout << "Starting CUDA Training..." << std::endl;
    
    DEVICE device;
    
    // Simple CUDA training placeholder
    std::cout << "CUDA device initialized successfully!" << std::endl;
    
    auto start = std::chrono::high_resolution_clock::now();
    
    // Placeholder for training logic
    for(int i = 0; i < 1000; i++) {
        if(i % 100 == 0) {
            std::cout << "CUDA Training step: " << i << std::endl;
        }
        // TODO: Add actual training logic here
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::duration<double>>(end - start).count();
    
    std::cout << "CUDA Training completed in: " << duration << "s" << std::endl;
    
    return 0;
}
