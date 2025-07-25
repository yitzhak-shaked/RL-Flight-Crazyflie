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
# Enable checkpoint initialization for UI target
target_compile_definitions(ui PRIVATE ACTOR_CHECKPOINT_FILE="../actors/hoverActor_000000000300000.h")
endif()

add_executable(training_headless training.cpp)
target_link_libraries(
        training_headless
        PRIVATE
        rl_tools
        learning_to_fly
)
# Uncomment and modify the next line to enable checkpoint initialization
target_compile_definitions(training_headless PRIVATE ACTOR_CHECKPOINT_FILE="../actors/hoverActor_000000000300000.h")

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
endif()