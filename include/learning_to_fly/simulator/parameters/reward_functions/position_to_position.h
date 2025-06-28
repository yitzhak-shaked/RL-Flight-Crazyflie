#ifndef LEARNING_TO_FLY_IN_SECONDS_SIMULATOR_PARAMETERS_REWARD_FUNCTIONS_POSITION_TO_POSITION_H
#define LEARNING_TO_FLY_IN_SECONDS_SIMULATOR_PARAMETERS_REWARD_FUNCTIONS_POSITION_TO_POSITION_H

#include "../../multirotor.h"
#include <rl_tools/utils/generic/typing.h>
#include <rl_tools/utils/generic/vector_operations.h>
#include "squared.h"

namespace rl_tools::rl::environments::multirotor::parameters::reward_functions{
    template<typename T>
    struct PositionToPosition {
        // Base Squared parameters
        bool non_negative;
        T scale;
        T constant;
        T termination_penalty;
        T position;
        T orientation;
        T linear_velocity;
        T angular_velocity;
        T linear_acceleration;
        T angular_acceleration;
        T action_baseline;
        T action;
        
        // Additional PositionToPosition parameters
        T target_pos[3];
        T target_radius;
        T velocity_reward_scale;
        bool use_target_progress;
        
        // Use the same Components struct as Squared
        using Components = typename Squared<T>::Components;
    };

    template<typename DEVICE, typename SPEC, typename ACTION_SPEC, typename T, typename RNG>
    RL_TOOLS_FUNCTION_PLACEMENT typename rl::environments::multirotor::parameters::reward_functions::PositionToPosition<T>::Components reward_components(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const rl::environments::multirotor::parameters::reward_functions::PositionToPosition<T>& params, const typename rl::environments::Multirotor<SPEC>::State& state, const Matrix<ACTION_SPEC>& action,  const typename rl::environments::Multirotor<SPEC>::State& next_state, RNG& rng){
        using TI = typename DEVICE::index_t;
        constexpr TI ACTION_DIM = ACTION_SPEC::COLS;
        using Components = typename rl::environments::multirotor::parameters::reward_functions::Squared<T>::Components;
        
        Components components;
        
        // Calculate distance to target
        T target_distance_sq = 0;
        T target_distance = 0;
        for(TI i = 0; i < 3; i++){
            T diff = next_state.position[i] - params.target_pos[i];
            target_distance_sq += diff * diff;
        }
        target_distance = math::sqrt(device.math, target_distance_sq);
        
        // Calculate orientation error (quaternion to desired "facing target" orientation)
        T orientation_error_sq = 0;
        for(TI i = 0; i < 4; i++){
            T target_quat = (i == 0) ? 1 : 0; // Identity quaternion (hover orientation)
            T diff = next_state.orientation[i] - target_quat;
            orientation_error_sq += diff * diff;
        }
        
        // Calculate velocity costs
        T linear_vel_cost = 0;
        T angular_vel_cost = 0;
        for(TI i = 0; i < 3; i++){
            linear_vel_cost += next_state.linear_velocity[i] * next_state.linear_velocity[i];
            angular_vel_cost += next_state.angular_velocity[i] * next_state.angular_velocity[i];
        }
        
        // Calculate linear and angular acceleration costs
        T linear_acc_cost = 0;
        T angular_acc_cost = 0;
        for(TI i = 0; i < 3; i++){
            T lin_acc_diff = next_state.linear_velocity[i] - state.linear_velocity[i];
            T ang_acc_diff = next_state.angular_velocity[i] - state.angular_velocity[i];
            linear_acc_cost += lin_acc_diff * lin_acc_diff;
            angular_acc_cost += ang_acc_diff * ang_acc_diff;
        }
        
        // Calculate action cost
        T action_diff[ACTION_DIM];
        for(TI i = 0; i < ACTION_DIM; i++){
            action_diff[i] = get(action, 0, i) - params.action_baseline;
        }
        T action_cost = rl_tools::utils::vector_operations::norm<DEVICE, T, ACTION_DIM>(action_diff);
        action_cost *= action_cost;
        
        // Store individual components
        components.position_cost = target_distance_sq; // Use distance to target instead of distance from origin
        components.orientation_cost = orientation_error_sq;
        components.linear_vel_cost = linear_vel_cost;
        components.angular_vel_cost = angular_vel_cost;
        components.linear_acc_cost = linear_acc_cost;
        components.angular_acc_cost = angular_acc_cost;
        components.action_cost = action_cost;
        
        // Additional reward for progress towards target
        T progress_reward = 0;
        if(params.use_target_progress) {
            // Reward for being close to target
            if(target_distance < params.target_radius) {
                progress_reward = params.velocity_reward_scale * (params.target_radius - target_distance);
            }
            
            // Additional velocity reward when moving towards target
            T velocity_towards_target = 0;
            for(TI i = 0; i < 3; i++){
                T direction_to_target = (params.target_pos[i] - next_state.position[i]) / (target_distance + 1e-6);
                velocity_towards_target += next_state.linear_velocity[i] * direction_to_target;
            }
            
            if(velocity_towards_target > 0 && target_distance > params.target_radius) {
                progress_reward += params.velocity_reward_scale * velocity_towards_target * 0.1;
            }
        }
        
        components.weighted_cost = params.position * components.position_cost + 
                                  params.orientation * components.orientation_cost + 
                                  params.linear_velocity * components.linear_vel_cost + 
                                  params.angular_velocity * components.angular_vel_cost + 
                                  params.linear_acceleration * components.linear_acc_cost + 
                                  params.angular_acceleration * components.angular_acc_cost + 
                                  params.action * components.action_cost;
                                  
        bool terminated_flag = terminated(device, env, next_state, rng);
        components.scaled_weighted_cost = params.scale * components.weighted_cost;

        if(terminated_flag){
            components.reward = params.termination_penalty;
        }
        else{
            components.reward = -components.scaled_weighted_cost + params.constant + progress_reward;
            components.reward = (components.reward > 0 || !params.non_negative) ? components.reward : 0;
        }

        return components;
    }

    template<typename DEVICE, typename SPEC, typename ACTION_SPEC, typename T, typename RNG>
    RL_TOOLS_FUNCTION_PLACEMENT static typename SPEC::T reward(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const rl::environments::multirotor::parameters::reward_functions::PositionToPosition<T>& params, const typename rl::environments::Multirotor<SPEC>::State& state, const Matrix<ACTION_SPEC>& action,  const typename rl::environments::Multirotor<SPEC>::State& next_state, RNG& rng) {
        auto components = reward_components(device, env, params, state, action, next_state, rng);
        return components.reward;
    }

    template<typename DEVICE, typename SPEC, typename ACTION_SPEC, typename T, typename RNG>
    RL_TOOLS_FUNCTION_PLACEMENT void log_reward(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const rl::environments::multirotor::parameters::reward_functions::PositionToPosition<T>& params, const typename rl::environments::Multirotor<SPEC>::State& state, const Matrix<ACTION_SPEC>& action,  const typename rl::environments::Multirotor<SPEC>::State& next_state, RNG& rng) {
        constexpr typename SPEC::TI cadence = 1;
        auto components = reward_components(device, env, params, state, action, next_state, rng);
        
        // Log target-specific metrics
        add_scalar(device, device.logger, "reward/target_distance", math::sqrt(device.math, components.position_cost), cadence);
        add_scalar(device, device.logger, "reward/position_cost",    components.position_cost, cadence);
        add_scalar(device, device.logger, "reward/orientation_cost", components.orientation_cost, cadence);
        add_scalar(device, device.logger, "reward/linear_vel_cost",  components.linear_vel_cost, cadence);
        add_scalar(device, device.logger, "reward/angular_vel_cost", components.angular_vel_cost, cadence);
        add_scalar(device, device.logger, "reward/linear_acc_cost",  components.linear_acc_cost, cadence);
        add_scalar(device, device.logger, "reward/angular_acc_cost", components.angular_acc_cost, cadence);
        add_scalar(device, device.logger, "reward/action_cost",      components.action_cost, cadence);
        add_scalar(device, device.logger, "reward/weighted_cost",        components.weighted_cost, cadence);
        add_scalar(device, device.logger, "reward/scaled_weighted_cost", components.scaled_weighted_cost, cadence);
        add_scalar(device, device.logger, "reward/reward",               components.reward, cadence);
        add_scalar(device, device.logger, "reward/reward_zero",          components.reward == 0, cadence);
    }
}

#endif
