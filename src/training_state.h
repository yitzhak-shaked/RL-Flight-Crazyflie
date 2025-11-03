#include <queue>
#include <vector>
#include <mutex>

namespace learning_to_fly{
    template <typename T_CONFIG>
    struct TrainingState: rlt::rl::algorithms::td3::loop::TrainingState<T_CONFIG>{
        using CONFIG = T_CONFIG;
        using T = typename CONFIG::T;
        using TI = typename CONFIG::TI;
        
        std::string run_name;
        std::queue<std::vector<typename CONFIG::ENVIRONMENT::State>> trajectories;
        std::mutex trajectories_mutex;
        std::vector<typename CONFIG::ENVIRONMENT::State> episode;
        
        // validation
        rlt::rl::utils::validation::Task<typename CONFIG::TASK_SPEC> task;
        typename CONFIG::ENVIRONMENT validation_envs[CONFIG::VALIDATION_N_EPISODES];
        typename CONFIG::ACTOR_TYPE::template DoubleBuffer<CONFIG::VALIDATION_N_EPISODES> validation_actor_buffers;
        
        // Policy switching for training
        bool use_policy_switching = false;
        T policy_switch_threshold = T(0.3);  // Distance threshold in meters
        typename CONFIG::ACTOR_TYPE hover_actor;  // Pre-trained hover actor
        typename CONFIG::ACTOR_TYPE::template DoubleBuffer<1> hover_actor_buffer;
        bool hover_actor_loaded = false;
        
        // Track per-trajectory whether hover actor has been activated (sticky switching)
        bool current_trajectory_using_hover = false;
    };
}
