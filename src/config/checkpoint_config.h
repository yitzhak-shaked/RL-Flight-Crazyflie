#ifndef LEARNING_TO_FLY_ACTOR_INIT_FROM_CHECKPOINT_H
#define LEARNING_TO_FLY_ACTOR_INIT_FROM_CHECKPOINT_H

#include "checkpoint_loader.h"

namespace learning_to_fly {
namespace config {

    // Specialized config that can load actor weights from a checkpoint
    template <typename T_ABLATION_SPEC, const char* CHECKPOINT_PATH_PTR>
    struct ConfigWithCheckpoint : public Base<T_ABLATION_SPEC> {
        using BASE = Base<T_ABLATION_SPEC>;
        
        struct ActorInitializationFromCheckpoint {
            static constexpr bool LOAD_FROM_CHECKPOINT = true;
            static constexpr const char* CHECKPOINT_PATH = CHECKPOINT_PATH_PTR;
        };
        using ACTOR_INIT = ActorInitializationFromCheckpoint;
        
        // Import everything else from base
        using ABLATION_SPEC = typename BASE::ABLATION_SPEC;
        using LOGGER = typename BASE::LOGGER;
        using DEV_SPEC = typename BASE::DEV_SPEC;
        using DEVICE = typename BASE::DEVICE;
        using T = typename BASE::T;
        using TI = typename BASE::TI;
        using ENVIRONMENT = typename BASE::ENVIRONMENT;
        using ENVIRONMENT_EVALUATION = typename BASE::ENVIRONMENT_EVALUATION;
        using UI = typename BASE::UI;
        using TD3_PARAMETERS = typename BASE::TD3_PARAMETERS;
        
        // All other constants from base
        static constexpr TI STEP_LIMIT = BASE::STEP_LIMIT;
        static constexpr TI EPISODE_STEP_LIMIT = BASE::EPISODE_STEP_LIMIT;
        static constexpr TI REPLAY_BUFFER_CAP = BASE::REPLAY_BUFFER_CAP;
        static constexpr TI EVALUATION_INTERVAL = BASE::EVALUATION_INTERVAL;
        static constexpr TI NUM_EVALUATION_EPISODES = BASE::NUM_EVALUATION_EPISODES;
        static constexpr bool DETERMINISTIC_EVALUATION = BASE::DETERMINISTIC_EVALUATION;
        static constexpr TI BASE_SEED = BASE::BASE_SEED;
        static constexpr bool CONSTRUCT_LOGGER = BASE::CONSTRUCT_LOGGER;
        static constexpr TI CHECKPOINT_INTERVAL = BASE::CHECKPOINT_INTERVAL;
        
        using ACTOR_CRITIC_SPEC = typename BASE::ACTOR_CRITIC_SPEC;
        using ACTOR_CRITIC_TYPE = typename BASE::ACTOR_CRITIC_TYPE;
        using OPTIMIZER = typename BASE::OPTIMIZER;
        using ACTOR_TYPE = typename BASE::ACTOR_TYPE;
        using ACTOR_CHECKPOINT_TYPE = typename BASE::ACTOR_CHECKPOINT_TYPE;
        using ACTOR_TARGET_TYPE = typename BASE::ACTOR_TARGET_TYPE;
        using CRITIC_TYPE = typename BASE::CRITIC_TYPE;
        using CRITIC_TARGET_TYPE = typename BASE::CRITIC_TARGET_TYPE;
        using OFF_POLICY_RUNNER_SPEC = typename BASE::OFF_POLICY_RUNNER_SPEC;
        static constexpr auto off_policy_runner_parameters = BASE::off_policy_runner_parameters;
        
        using SPEC = typename BASE::SPEC;
    };

} // namespace config
} // namespace learning_to_fly

#endif // LEARNING_TO_FLY_ACTOR_INIT_FROM_CHECKPOINT_H
