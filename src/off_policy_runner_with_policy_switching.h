#ifndef LEARNING_TO_FLY_OFF_POLICY_RUNNER_WITH_POLICY_SWITCHING_H
#define LEARNING_TO_FLY_OFF_POLICY_RUNNER_WITH_POLICY_SWITCHING_H

#include <rl_tools/rl/components/off_policy_runner/operations_generic.h>
#include "policy_switching.h"
#include "constants.h"

namespace learning_to_fly {
namespace off_policy_runner {

    /**
     * Custom step function for off-policy runner that supports policy switching
     * This uses the prologue/interlude_with_switching/epilogue pattern from RL-Tools
     */
    template <typename DEVICE, typename SPEC, typename POLICY, typename HOVER_POLICY, typename POLICY_BUFFER, typename HOVER_BUFFER>
    void interlude_with_policy_switching(
        DEVICE& device, 
        rlt::rl::components::OffPolicyRunner<SPEC>& runner,
        POLICY& policy,
        HOVER_POLICY& hover_policy,
        POLICY_BUFFER& policy_buffer,
        HOVER_BUFFER& hover_buffer,
        bool use_policy_switching,
        typename SPEC::T threshold
    ) {
        using T = typename SPEC::T;
        using TI = typename SPEC::TI;
        using ENVIRONMENT = typename SPEC::ENVIRONMENT;
        constexpr TI N_ENVIRONMENTS = SPEC::N_ENVIRONMENTS;

        if (!use_policy_switching) {
            // Standard evaluation without policy switching
            rlt::evaluate(device, policy, runner.buffers.observations, runner.buffers.actions, policy_buffer);
            return;
        }

        // Policy switching: evaluate each environment separately
        for(TI env_i = 0; env_i < N_ENVIRONMENTS; env_i++){
            auto& env = runner.envs[env_i];
            auto observation = rlt::view<DEVICE, typename decltype(runner.buffers.observations)::SPEC, 1, ENVIRONMENT::OBSERVATION_DIM>(
                device, runner.buffers.observations, env_i, 0
            );
            auto action = rlt::view<DEVICE, typename decltype(runner.buffers.actions)::SPEC, 1, ENVIRONMENT::ACTION_DIM>(
                device, runner.buffers.actions, env_i, 0
            );

            // Calculate distance to target
            auto& state = rlt::get(runner.states, 0, env_i);
            T distance = policy_switching::calculate_distance_to_target<T>(state.position);
            
            if (distance < threshold) {
                // Use hover actor with transformed observations
                rlt::MatrixDynamic<rlt::matrix::Specification<T, TI, 1, ENVIRONMENT::OBSERVATION_DIM>> transformed_obs;
                rlt::malloc(device, transformed_obs);
                rlt::copy(device, device, observation, transformed_obs);
                
                T target_pos[3];
                constants::get_target_position<T>(target_pos);
                policy_switching::transform_observation_to_target_relative(device, transformed_obs, target_pos);
                
                rlt::evaluate(device, hover_policy, transformed_obs, action, hover_buffer);
                rlt::free(device, transformed_obs);
            } else {
                // Use navigation actor
                rlt::evaluate(device, policy, observation, action, policy_buffer);
            }
        }
    }

    template <typename DEVICE, typename OFF_POLICY_RUNNER_SPEC, typename POLICY, typename HOVER_POLICY, typename POLICY_BUFFER, typename HOVER_BUFFER, typename RNG>
    void step_with_policy_switching(
        DEVICE& device, 
        rlt::rl::components::OffPolicyRunner<OFF_POLICY_RUNNER_SPEC>& off_policy_runner, 
        POLICY& policy,
        HOVER_POLICY& hover_policy,
        POLICY_BUFFER& policy_buffer,
        HOVER_BUFFER& hover_buffer,
        RNG& rng,
        bool use_policy_switching,
        typename OFF_POLICY_RUNNER_SPEC::T threshold
    ) {
        // Use RL-Tools prologue/interlude/epilogue pattern
        rlt::rl::components::off_policy_runner::prologue(device, off_policy_runner, rng);
        interlude_with_policy_switching(device, off_policy_runner, policy, hover_policy, policy_buffer, hover_buffer, use_policy_switching, threshold);
        rlt::rl::components::off_policy_runner::epilogue(device, off_policy_runner, rng);
    }

} // namespace off_policy_runner
} // namespace learning_to_fly

#endif // LEARNING_TO_FLY_OFF_POLICY_RUNNER_WITH_POLICY_SWITCHING_H
