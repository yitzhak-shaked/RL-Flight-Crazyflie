#ifndef LEARNING_TO_FLY_POLICY_SWITCHING_H
#define LEARNING_TO_FLY_POLICY_SWITCHING_H

#include <rl_tools/operations/cpu_mux.h>
#include "constants.h"
#include <cmath>

namespace learning_to_fly {
namespace policy_switching {

    /**
     * Calculate distance from current position to target position
     */
    template<typename T>
    T calculate_distance_to_target(const T position[3]) {
        T dx = position[0] - constants::TARGET_POSITION_X<T>;
        T dy = position[1] - constants::TARGET_POSITION_Y<T>;
        T dz = position[2] - constants::TARGET_POSITION_Z<T>;
        return std::sqrt(dx * dx + dy * dy + dz * dz);
    }

    /**
     * Transform observation to be relative to target position
     * This allows the hover actor (trained at origin) to hover at the target
     */
    template<typename DEVICE, typename OBSERVATION_SPEC>
    void transform_observation_to_target_relative(
        DEVICE& device,
        rlt::Matrix<OBSERVATION_SPEC>& observation,
        const typename OBSERVATION_SPEC::T target_pos[3]
    ) {
        using T = typename OBSERVATION_SPEC::T;
        
        // Observation structure (from multirotor environment):
        // [0-2]: position (x, y, z)
        // [3-5]: orientation (quaternion w, x, y)
        // [6]: orientation z
        // [7-9]: linear velocity
        // [10-12]: angular velocity
        // ... (rest of observation)
        
        // Transform position to be relative to target (indices 0-2)
        for(int i = 0; i < 3; i++) {
            T absolute_pos = rlt::get(observation, 0, i);
            T relative_pos = absolute_pos - target_pos[i];
            rlt::set(observation, 0, i, relative_pos);
        }
        
        // All other observations (orientation, velocities, etc.) remain unchanged
        // as they are already in the body frame
    }

    /**
     * Evaluate actor with policy switching based on distance to target
     * 
     * @param use_hover If true and distance < threshold, use hover_actor
     * @param threshold Distance threshold for switching (in meters)
     */
    template<typename DEVICE, typename SPEC, typename ACTOR_TYPE, typename HOVER_ACTOR_TYPE, 
             typename ACTION_SPEC, typename OBSERVATION_SPEC, typename ACTOR_BUFFERS, typename HOVER_BUFFERS, typename RNG>
    void evaluate_with_policy_switching(
        DEVICE& device,
        const rlt::rl::environments::Multirotor<SPEC>& env,
        ACTOR_TYPE& navigation_actor,
        HOVER_ACTOR_TYPE& hover_actor,
        const rlt::Matrix<OBSERVATION_SPEC>& observation,
        rlt::Matrix<ACTION_SPEC>& action,
        ACTOR_BUFFERS& nav_buffers,
        HOVER_BUFFERS& hover_buffers,
        RNG& rng,
        bool use_hover,
        typename DEVICE::SPEC::MATH::T threshold
    ) {
        using T = typename DEVICE::SPEC::MATH::T;
        
        if (!use_hover) {
            // No policy switching - just use navigation actor
            rlt::evaluate(device, navigation_actor, observation, action, nav_buffers, rng);
            return;
        }
        
        // Calculate distance to target
        const auto& state = env.state;
        T distance = calculate_distance_to_target<T>(state.position);
        
        if (distance < threshold) {
            // HOVER MODE: Use hover actor with transformed observations
            // Create a copy of observations for transformation
            rlt::Matrix<OBSERVATION_SPEC> transformed_observation;
            rlt::malloc(device, transformed_observation);
            rlt::copy(device, device, observation, transformed_observation);
            
            // Transform position to be relative to target
            T target_pos[3];
            constants::get_target_position<T>(target_pos);
            transform_observation_to_target_relative(device, transformed_observation, target_pos);
            
            // Use hover actor with transformed observation
            rlt::evaluate(device, hover_actor, transformed_observation, action, hover_buffers, rng);
            
            rlt::free(device, transformed_observation);
        } else {
            // NAVIGATION MODE: Use navigation actor with normal observations
            rlt::evaluate(device, navigation_actor, observation, action, nav_buffers, rng);
        }
    }

} // namespace policy_switching
} // namespace learning_to_fly

#endif // LEARNING_TO_FLY_POLICY_SWITCHING_H
