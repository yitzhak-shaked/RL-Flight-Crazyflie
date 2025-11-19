namespace learning_to_fly::config{
    struct DEFAULT_ABLATION_SPEC{
        static constexpr bool DISTURBANCE = true;
        static constexpr bool OBSERVATION_NOISE = true;
        static constexpr bool ASYMMETRIC_ACTOR_CRITIC = true;
        static constexpr bool ROTOR_DELAY = true;
        static constexpr bool ACTION_HISTORY = true;
        static constexpr bool ENABLE_CURRICULUM = true;
        static constexpr bool RECALCULATE_REWARDS = true;
        static constexpr bool USE_INITIAL_REWARD_FUNCTION = true;
        static constexpr bool INIT_NORMAL = true;
        static constexpr bool EXPLORATION_NOISE_DECAY = true;
    };

    // New ablation spec for position-to-position learning
    // CRITICAL: Curriculum disabled - it's designed for hover training (tightening from 1m to 20cm)
    // Position-to-position needs stable rewards throughout training for 2m navigation
    struct POSITION_TO_POSITION_ABLATION_SPEC{
        static constexpr bool DISTURBANCE = true;
        static constexpr bool OBSERVATION_NOISE = true;
        static constexpr bool ASYMMETRIC_ACTOR_CRITIC = true;
        static constexpr bool ROTOR_DELAY = true;
        static constexpr bool ACTION_HISTORY = true;
        static constexpr bool ENABLE_CURRICULUM = false;  // DISABLED: Causes policy collapse at 100k steps
        static constexpr bool RECALCULATE_REWARDS = false;  // DISABLED: Breaks replay buffer consistency
        static constexpr bool USE_INITIAL_REWARD_FUNCTION = true;
        static constexpr bool USE_POSITION_TO_POSITION_REWARD = true;
        static constexpr bool INIT_NORMAL = true;
        static constexpr bool EXPLORATION_NOISE_DECAY = true;
    };

    template <typename T_ABLATION_SPEC>
    struct ABLATION_SPEC_EVAL: T_ABLATION_SPEC{
        // override everything but ACTION_HISTORY because that changes the observation space
        static constexpr bool DISTURBANCE = true;
        static constexpr bool OBSERVATION_NOISE = true;
        static constexpr bool ASYMMETRIC_ACTOR_CRITIC = true;
        static constexpr bool ROTOR_DELAY = true;
//            static constexpr bool ACTION_HISTORY = {copied from T_ABLATION_SPEC for structural fit};
        static constexpr bool ENABLE_CURRICULUM = true;
        static constexpr bool RECALCULATE_REWARDS = true;
        static constexpr bool USE_INITIAL_REWARD_FUNCTION = false; // Use target reward function as metric
        static constexpr bool INIT_NORMAL = true;
        static constexpr bool EXPLORATION_NOISE_DECAY = true;
    };
}
