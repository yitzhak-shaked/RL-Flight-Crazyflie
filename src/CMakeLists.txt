
if(RL_TOOLS_ENABLE_JSON AND RL_TOOLS_ENABLE_BOOST_BEAST)
find_package(Boost REQUIRED)
message(STATUS "Boost version: ${Boost_VERSION}")
add_executable(ui ui/ui.cpp)
target_link_libraries(
        ui
        PRIVATE
        rl_tools
        learning_to_fly
        Boost::boost
)
endif()

add_executable(training_headless training.cpp)
target_link_libraries(
        training_headless
        PRIVATE
        rl_tools
        learning_to_fly
)
add_executable(training_benchmark training.cpp)
target_link_libraries(
        training_benchmark
        PRIVATE
        rl_tools
        learning_to_fly
)
target_compile_definitions(training_benchmark PRIVATE LEARNING_TO_FLY_IN_SECONDS_BENCHMARK)

if(RL_TOOLS_ENABLE_HDF5)
add_executable(ablation_study ablation_study.cpp)
target_link_libraries(
        ablation_study
        PRIVATE
        rl_tools
        learning_to_fly
)
endif()

if(LEARNING_TO_FLY_ENABLE_OLD_UI)
add_executable(evaluate_actor evaluate_actor.cpp)
target_link_libraries(
        evaluate_actor
        PRIVATE
        rl_tools
        learning_to_fly
)
endif()


if(RL_TOOLS_BACKEND_ENABLE_CUDA)
    add_subdirectory(cuda)
    
    # Simple CUDA Training Executable (working baseline)
    add_executable(training_simple_cuda training_cuda_simple.cu)
    target_link_libraries(
            training_simple_cuda
            PRIVATE
            rl_tools::minimal
            rl_tools::optimizations_cuda
            learning_to_fly
    )
    set_property(TARGET training_simple_cuda PROPERTY CUDA_ARCHITECTURES "60;70;75;80;86")
    RL_TOOLS_TAG_IS_CUDA(training_simple_cuda)
    
    # CUDA Training Executables (advanced)
    add_executable(training_headless_cuda training_cuda.cu)
    target_link_libraries(
            training_headless_cuda
            PRIVATE
            rl_tools
            learning_to_fly
    )
    set_property(TARGET training_headless_cuda PROPERTY CUDA_ARCHITECTURES "60;70;75;80;86")
    
    add_executable(training_benchmark_cuda training_cuda_benchmark.cu)
    target_link_libraries(
            training_benchmark_cuda
            PRIVATE
            rl_tools
            learning_to_fly
    )
    target_compile_definitions(training_benchmark_cuda PRIVATE LEARNING_TO_FLY_IN_SECONDS_BENCHMARK)
    set_property(TARGET training_benchmark_cuda PROPERTY CUDA_ARCHITECTURES "60;70;75;80;86")
    
    if(RL_TOOLS_ENABLE_HDF5)
    add_executable(ablation_study_cuda ablation_study.cpp)
    target_link_libraries(
            ablation_study_cuda
            PRIVATE
            rl_tools
            learning_to_fly
    )
    target_compile_definitions(ablation_study_cuda PRIVATE LEARNING_TO_FLY_USE_CUDA)
    set_property(TARGET ablation_study_cuda PROPERTY CUDA_ARCHITECTURES "60;70;75;80;86")
    endif()
endif()