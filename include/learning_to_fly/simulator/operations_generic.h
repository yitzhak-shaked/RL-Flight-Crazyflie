#ifndef LEARNING_TO_FLY_IN_SECONDS_SIMULATOR_OPERATIONS_GENERIC_H
#define LEARNING_TO_FLY_IN_SECONDS_SIMULATOR_OPERATIONS_GENERIC_H

#include "multirotor.h"

#include <rl_tools/utils/generic/vector_operations.h>
#include "quaternion_helper.h"

#include <rl_tools/utils/generic/typing.h>

#include <rl_tools/rl/environments/operations_generic.h>

#ifndef RL_TOOLS_FUNCTION_PLACEMENT
#define RL_TOOLS_FUNCTION_PLACEMENT
#endif

namespace rl_tools {
    // State arithmetic for RK4 integration
    // scalar multiply
    template<typename DEVICE, typename T, typename TI, typename T2>
    RL_TOOLS_FUNCTION_PLACEMENT static void scalar_multiply(DEVICE& device, const typename rl::environments::multirotor::StateBase<T, TI>& state, T2 scalar, typename rl::environments::multirotor::StateBase<T, TI>& out) {
        for(int i = 0; i < 3; ++i) {
            out.position[i]         = scalar * state.position[i]        ;
            out.orientation[i]      = scalar * state.orientation[i]     ;
            out.linear_velocity[i]  = scalar * state.linear_velocity[i] ;
            out.angular_velocity[i] = scalar * state.angular_velocity[i];
        }
        out.orientation[3] = scalar * state.orientation[3];
    }
    template<typename DEVICE, typename T, typename TI, typename T2, typename NEXT_COMPONENT>
    RL_TOOLS_FUNCTION_PLACEMENT static void scalar_multiply(DEVICE& device, const typename rl::environments::multirotor::StateRotors<T, TI, NEXT_COMPONENT>& state, T2 scalar, typename rl::environments::multirotor::StateRotors<T, TI, NEXT_COMPONENT>& out){
        scalar_multiply(device, (const NEXT_COMPONENT&)state, scalar, (NEXT_COMPONENT&)out);
        for(int i = 0; i < 4; ++i){
            out.rpm[i] = scalar * state.rpm[i];
        }
    }
    template<typename DEVICE, typename STATE, typename T2>
    RL_TOOLS_FUNCTION_PLACEMENT static void scalar_multiply(DEVICE& device, const STATE& state, T2 scalar, STATE& out, utils::typing::enable_if_t<!STATE::REQUIRES_INTEGRATION, bool> disable = false){
        static_assert(!STATE::REQUIRES_INTEGRATION);
        scalar_multiply(device, (const typename STATE::NEXT_COMPONENT&)state, scalar, (typename STATE::NEXT_COMPONENT&)out);
    }
    // scalar multiply in place
    template<typename DEVICE, typename T, typename TI, typename T2>
    RL_TOOLS_FUNCTION_PLACEMENT static void scalar_multiply(DEVICE& device, typename rl::environments::multirotor::StateBase<T, TI>& state, T2 scalar){
        scalar_multiply(device, state, scalar, state);
    }
    template<typename DEVICE, typename T, typename TI, typename T2, typename NEXT_COMPONENT>
    RL_TOOLS_FUNCTION_PLACEMENT static void scalar_multiply(DEVICE& device, typename rl::environments::multirotor::StateRotors<T, TI, NEXT_COMPONENT>& state, T2 scalar){
        scalar_multiply(device, state, scalar, state);
    }
    template<typename DEVICE, typename STATE, typename T2>
    RL_TOOLS_FUNCTION_PLACEMENT static void scalar_multiply(DEVICE& device, STATE& state, T2 scalar, utils::typing::enable_if_t<!STATE::REQUIRES_INTEGRATION, bool> disable = false){
        static_assert(!STATE::REQUIRES_INTEGRATION);
        scalar_multiply(device, (typename STATE::NEXT_COMPONENT&)state, scalar);
    }

    template<typename DEVICE, typename T, typename TI, typename T2>
    RL_TOOLS_FUNCTION_PLACEMENT static void scalar_multiply_accumulate(DEVICE& device, const typename rl::environments::multirotor::StateBase<T, TI>& state, T2 scalar, typename rl::environments::multirotor::StateBase<T, TI>& out){
        for(int i = 0; i < 3; ++i){
            out.position[i]         += scalar * state.position[i]        ;
            out.orientation[i]      += scalar * state.orientation[i]     ;
            out.linear_velocity[i]  += scalar * state.linear_velocity[i] ;
            out.angular_velocity[i] += scalar * state.angular_velocity[i];
        }
        out.orientation[3] += scalar * state.orientation[3];
    }
    template<typename DEVICE, typename T, typename TI, typename T2, typename NEXT_COMPONENT>
    RL_TOOLS_FUNCTION_PLACEMENT static void scalar_multiply_accumulate(DEVICE& device, const typename rl::environments::multirotor::StateRotors<T, TI, NEXT_COMPONENT>& state, T2 scalar, typename rl::environments::multirotor::StateRotors<T, TI, NEXT_COMPONENT>& out){
        scalar_multiply_accumulate(device, static_cast<const NEXT_COMPONENT&>(state), scalar, static_cast<NEXT_COMPONENT&>(out));
        for(int i = 0; i < 4; ++i){
            out.rpm[i] += scalar * state.rpm[i];
        }
    }
    template<typename DEVICE, typename STATE, typename T2>
    RL_TOOLS_FUNCTION_PLACEMENT static void scalar_multiply_accumulate(DEVICE& device, const STATE& state, T2 scalar, STATE& out, utils::typing::enable_if_t<!STATE::REQUIRES_INTEGRATION, bool> disable = false){
        static_assert(!STATE::REQUIRES_INTEGRATION);
        scalar_multiply_accumulate(device, static_cast<const typename STATE::NEXT_COMPONENT&>(state), scalar, static_cast<typename STATE::NEXT_COMPONENT&>(out));
    }

    template<typename DEVICE, typename T, typename TI>
    RL_TOOLS_FUNCTION_PLACEMENT static void add_accumulate(DEVICE& device, const typename rl::environments::multirotor::StateBase<T, TI>& s1, const typename rl::environments::multirotor::StateBase<T, TI>& s2, typename rl::environments::multirotor::StateBase<T, TI>& out){
        for(int i = 0; i < 3; ++i){
            out.position[i]         = s1.position[i] + s2.position[i];
            out.orientation[i]      = s1.orientation[i] + s2.orientation[i];
            out.linear_velocity[i]  = s1.linear_velocity[i] + s2.linear_velocity[i];
            out.angular_velocity[i] = s1.angular_velocity[i] + s2.angular_velocity[i];
        }
        out.orientation[3] = s1.orientation[3] + s2.orientation[3];
    }
    template<typename DEVICE, typename T, typename TI, typename NEXT_COMPONENT>
    RL_TOOLS_FUNCTION_PLACEMENT static void add_accumulate(DEVICE& device, const typename rl::environments::multirotor::StateRotors<T, TI, NEXT_COMPONENT>& s1, const typename rl::environments::multirotor::StateRotors<T, TI, NEXT_COMPONENT>& s2, typename rl::environments::multirotor::StateRotors<T, TI, NEXT_COMPONENT>& out){
        add_accumulate(device, static_cast<const NEXT_COMPONENT&>(s1), static_cast<const NEXT_COMPONENT&>(s2), static_cast<NEXT_COMPONENT&>(out));
        for(int i = 0; i < 4; ++i){
            out.rpm[i] = s1.rpm[i] + s2.rpm[i];
        }
    }
    template<typename DEVICE, typename STATE>
    RL_TOOLS_FUNCTION_PLACEMENT static void add_accumulate(DEVICE& device, const STATE& s1, const STATE& s2, STATE& out, utils::typing::enable_if_t<!STATE::REQUIRES_INTEGRATION, bool> disable = false){
        static_assert(!STATE::REQUIRES_INTEGRATION);
        add_accumulate(device, static_cast<const typename STATE::NEXT_COMPONENT&>(s1), static_cast<const typename STATE::NEXT_COMPONENT&>(s2), static_cast<typename STATE::NEXT_COMPONENT&>(out));
    }
    template<typename DEVICE, typename T, typename TI>
    RL_TOOLS_FUNCTION_PLACEMENT static void add_accumulate(DEVICE& device, const typename rl::environments::multirotor::StateBase<T, TI>& s, typename rl::environments::multirotor::StateBase<T, TI>& out){
        add_accumulate(device, s, out, out);
    }
    template<typename DEVICE, typename T, typename TI, typename NEXT_COMPONENT>
    RL_TOOLS_FUNCTION_PLACEMENT static void add_accumulate(DEVICE& device, const typename rl::environments::multirotor::StateRotors<T, TI, NEXT_COMPONENT>& s, typename rl::environments::multirotor::StateRotors<T, TI, NEXT_COMPONENT>& out){
        add_accumulate(device, s, out, out);
    }
    template<typename DEVICE, typename STATE>
    RL_TOOLS_FUNCTION_PLACEMENT static void add_accumulate(DEVICE& device, const STATE& s, STATE& out, utils::typing::enable_if_t<!STATE::REQUIRES_INTEGRATION, bool> disable = false){
        static_assert(!STATE::REQUIRES_INTEGRATION);
        add_accumulate(device, static_cast<const typename STATE::NEXT_COMPONENT&>(s), static_cast<const typename STATE::NEXT_COMPONENT&>(out), static_cast<typename STATE::NEXT_COMPONENT&>(out));
    }
}

#include <rl_tools/utils/generic/integrators.h>


namespace rl_tools::rl::environments::multirotor {
    template<typename DEVICE, typename T, typename TI, typename PARAMETERS>
    RL_TOOLS_FUNCTION_PLACEMENT void multirotor_dynamics(DEVICE& device, const PARAMETERS& params, const StateBase<T, TI>& state, const T* action, StateBase<T, TI>& state_change) {
        using STATE = StateBase<T, TI>;

        T thrust[3];
        T torque[3];
        thrust[0] = 0;
        thrust[1] = 0;
        thrust[2] = 0;
        torque[0] = 0;
        torque[1] = 0;
        torque[2] = 0;
        // flops: N*23 => 4 * 23 = 92
        for(typename DEVICE::index_t i_rotor = 0; i_rotor < 4; i_rotor++){
            // flops: 3 + 1 + 3 + 3 + 3 + 4 + 6 = 23
            T rpm = action[i_rotor];
            T thrust_magnitude = params.dynamics.thrust_constants[0] + params.dynamics.thrust_constants[1] * rpm + params.dynamics.thrust_constants[2] * rpm * rpm;
            T rotor_thrust[3];
            utils::vector_operations::scalar_multiply<DEVICE, T, 3>(params.dynamics.rotor_thrust_directions[i_rotor], thrust_magnitude, rotor_thrust);
            utils::vector_operations::add_accumulate<DEVICE, T, 3>(rotor_thrust, thrust);

            utils::vector_operations::scalar_multiply_accumulate<DEVICE, T, 3>(params.dynamics.rotor_torque_directions[i_rotor], thrust_magnitude * params.dynamics.torque_constant, torque);
            utils::vector_operations::cross_product_accumulate<DEVICE, T>(params.dynamics.rotor_positions[i_rotor], rotor_thrust, torque);
        }

        // linear_velocity_global
        state_change.position[0] = state.linear_velocity[0];
        state_change.position[1] = state.linear_velocity[1];
        state_change.position[2] = state.linear_velocity[2];

        // angular_velocity_global
        // flops: 16
        quaternion_derivative<DEVICE, T>(state.orientation, state.angular_velocity, state_change.orientation);

        // linear_acceleration_global
        // flops: 21
        rotate_vector_by_quaternion<DEVICE, T>(state.orientation, thrust, state_change.linear_velocity);
        // flops: 4
        utils::vector_operations::scalar_multiply<DEVICE, T, 3>(state_change.linear_velocity, 1 / params.dynamics.mass);
        utils::vector_operations::add_accumulate<DEVICE, T, 3>(params.dynamics.gravity, state_change.linear_velocity);

        T vector[3];
        T vector2[3];

        // angular_acceleration_local
        // flops: 9
        utils::vector_operations::matrix_vector_product<DEVICE, T, 3, 3>(params.dynamics.J, state.angular_velocity, vector);
        // flops: 6
        utils::vector_operations::cross_product<DEVICE, T>(state.angular_velocity, vector, vector2);
        utils::vector_operations::sub<DEVICE, T, 3>(torque, vector2, vector);
        // flops: 9
        utils::vector_operations::matrix_vector_product<DEVICE, T, 3, 3>(params.dynamics.J_inv, vector, state_change.angular_velocity);
        // total flops: (quadrotor): 92 + 16 + 21 + 4 + 9 + 6 + 9 = 157
//        multirotor_dynamics<DEVICE, T, TI, PARAMETERS>(device, params, (const typename STATE::LATENT_STATE&)state, action, state_change);
//        multirotor_dynamics(device, params, (const typename STATE::LATENT_STATE&)state, action, state_change);
    }
    template<typename DEVICE, typename T, typename TI, typename PARAMETERS, typename NEXT_COMPONENT>
    RL_TOOLS_FUNCTION_PLACEMENT void multirotor_dynamics(DEVICE& device, const PARAMETERS& params, const StateRandomForce<T, TI, NEXT_COMPONENT>& state, const T* action, StateRandomForce<T, TI, NEXT_COMPONENT>& state_change){
        multirotor_dynamics(device, params, static_cast<const NEXT_COMPONENT&>(state), action, static_cast<NEXT_COMPONENT&>(state_change));

        state_change.linear_velocity[0] += state.force[0] / params.dynamics.mass;
        state_change.linear_velocity[1] += state.force[1] / params.dynamics.mass;
        state_change.linear_velocity[2] += state.force[2] / params.dynamics.mass;

        T angular_acceleration[3];

        utils::vector_operations::matrix_vector_product<DEVICE, T, 3, 3>(params.dynamics.J_inv, state.torque, angular_acceleration);
        utils::vector_operations::add_accumulate<DEVICE, T, 3>(angular_acceleration, state_change.angular_velocity);
    }
    template<typename DEVICE, typename T, typename TI, typename NEXT_COMPONENT, typename PARAMETERS>
    RL_TOOLS_FUNCTION_PLACEMENT void multirotor_dynamics(DEVICE& device, const PARAMETERS& params, const StateRotors<T, TI, NEXT_COMPONENT>& state, const T* action, StateRotors<T, TI, NEXT_COMPONENT>& state_change) {
        multirotor_dynamics(device, params, static_cast<const NEXT_COMPONENT&>(state), state.rpm, static_cast<NEXT_COMPONENT&>(state_change));
        for(typename DEVICE::index_t i_rotor = 0; i_rotor < 4; i_rotor++){
            state_change.rpm[i_rotor] = (action[i_rotor] - state.rpm[i_rotor]) * 1/params.dynamics.rpm_time_constant;
        }

    }
    template<typename DEVICE, typename T, typename PARAMETERS, typename STATE>
    RL_TOOLS_FUNCTION_PLACEMENT void multirotor_dynamics_dispatch(DEVICE& device, const PARAMETERS& params, const STATE& state, const T* action, STATE& state_change) {
        // this dispatch function is required to pass the multirotor dynamics function to the integrator (euler, rk4) as a template parameter (so that it can be inlined/optimized at compile time)
        // If we would try to pass the multirotor_dynamics function directly the state type-based overloading would make the inference of the auto template parameter for the dynamics function in the integrator function impossible
//        multirotor_dynamics<DEVICE, T, typename DEVICE::index_t, typename STATE::LATENT_STATE, PARAMETERS>(device, params, state, action, state_change);
        multirotor_dynamics(device, params, state, action, state_change);
    }

}
RL_TOOLS_NAMESPACE_WRAPPER_END


RL_TOOLS_NAMESPACE_WRAPPER_START
namespace rl_tools{
    template<typename DEVICE, typename SPEC>
    void malloc(DEVICE&, rl::environments::Multirotor<SPEC>){

    }
    template<typename DEVICE, typename T, typename TI, typename SPEC>
    static void initial_parameters(DEVICE& device, rl::environments::Multirotor<SPEC>& env, typename rl::environments::multirotor::StateBase<T, TI>& state){
//        T J_factor = random::uniform_real_distribution(random_dev, (T)0.5, (T)2, rng);
//        env.current_dynamics.J[0][0] *= J_factor;
//        env.current_dynamics.J[1][1] *= J_factor;
//        env.current_dynamics.J[2][2] *= J_factor;
//        env.current_dynamics.J_inv[0][0] /= J_factor;
//        env.current_dynamics.J_inv[1][1] /= J_factor;
//        env.current_dynamics.J_inv[2][2] /= J_factor;
//        T mass_factor = random::uniform_real_distribution(random_dev, (T)0.5, (T)1.5, rng);
//        env.current_dynamics.mass *= mass_factor;
//        printf("initial state: %f %f %f %f %f %f %f %f %f %f %f %f %f\n", state.state[0], state.state[1], state.state[2], state.state[3], state.state[4], state.state[5], state.state[6], state.state[7], state.state[8], state.state[9], state.state[10], state.state[11], state.state[12]);
        env.current_dynamics = env.parameters.dynamics;
    }
    template<typename DEVICE, typename T, typename TI, typename SPEC>
    static void initial_state(DEVICE& device, rl::environments::Multirotor<SPEC>& env, typename rl::environments::multirotor::StateBase<T, TI>& state){
        using STATE = typename rl::environments::Multirotor<SPEC>::State;
        for(typename DEVICE::index_t i = 0; i < 3; i++){
            state.position[i] = 0;
        }
        state.orientation[0] = 1;
        for(typename DEVICE::index_t i = 1; i < 4; i++){
            state.orientation[i] = 0;
        }
        for(typename DEVICE::index_t i = 0; i < 3; i++){
            state.linear_velocity[i] = 0;
        }
        for(typename DEVICE::index_t i = 0; i < 3; i++){
            state.angular_velocity[i] = 0;
        }
        initial_parameters(device, env, state);
    }
    template<typename DEVICE, typename T, typename TI, typename SPEC, typename NEXT_COMPONENT>
    static void initial_state(DEVICE& device, rl::environments::Multirotor<SPEC>& env, typename rl::environments::multirotor::StateRandomForce<T, TI, NEXT_COMPONENT>& state){
        initial_state(device, env, static_cast<NEXT_COMPONENT&>(state));
        state.force[0] = 0;
        state.force[1] = 0;
        state.force[2] = 0;
        state.torque[0] = 0;
        state.torque[1] = 0;
        state.torque[2] = 0;
    }
    template<typename DEVICE, typename T, typename TI, typename SPEC, typename NEXT_COMPONENT>
    static void initial_state(DEVICE& device, rl::environments::Multirotor<SPEC>& env, typename rl::environments::multirotor::StateRotors<T, TI, NEXT_COMPONENT>& state){
        initial_state(device, env, static_cast<NEXT_COMPONENT&>(state));
        for(typename DEVICE::index_t i = 0; i < 4; i++){
            state.rpm[i] = (env.parameters.dynamics.action_limit.max - env.parameters.dynamics.action_limit.min) / 2 + env.parameters.dynamics.action_limit.min;
        }
    }
    template<typename DEVICE, typename T, typename TI_H, TI_H HISTORY_LENGTH, typename SPEC, typename NEXT_COMPONENT>
    static void initial_state(DEVICE& device, rl::environments::Multirotor<SPEC>& env, typename rl::environments::multirotor::StateRotorsHistory<T, TI_H, HISTORY_LENGTH, NEXT_COMPONENT>& state){
        using TI = typename DEVICE::index_t;
        using MULTIROTOR = rl::environments::Multirotor<SPEC>;
        initial_state(device, env, static_cast<rl::environments::multirotor::StateRotors<T, TI, NEXT_COMPONENT>&>(state));
        for(TI step_i = 0; step_i < HISTORY_LENGTH; step_i++){
            for(TI action_i = 0; action_i < MULTIROTOR::ACTION_DIM; action_i++){
                state.action_history[step_i][action_i] = (state.rpm[action_i] - env.parameters.dynamics.action_limit.min) / (env.parameters.dynamics.action_limit.max - env.parameters.dynamics.action_limit.min) * 2 - 1;
            }
        }
    }
    template<typename DEVICE, typename T, typename TI, typename SPEC, typename RNG, bool INHERIT_GUIDANCE = false>
    RL_TOOLS_FUNCTION_PLACEMENT static void sample_initial_state(DEVICE& device, rl::environments::Multirotor<SPEC>& env, typename rl::environments::multirotor::StateBase<T, TI>& state, RNG& rng, bool inherited_guidance = false){
        typename DEVICE::SPEC::MATH math_dev;
        typename DEVICE::SPEC::RANDOM random_dev;
        using STATE = typename rl::environments::multirotor::StateBase<T, TI>;
        bool guidance;
        guidance = random::uniform_real_distribution(random_dev, (T)0, (T)1, rng) < env.parameters.mdp.init.guidance;
        if(!guidance){
            for(TI i = 0; i < 3; i++){
                state.position[i] = random::uniform_real_distribution(random_dev, -env.parameters.mdp.init.max_position, env.parameters.mdp.init.max_position, rng);
            }
            // https://web.archive.org/web/20181126051029/http://planning.cs.uiuc.edu/node198.html
        }
        else{
            for(TI i = 0; i < 3; i++){
                state.position[i] = 0;
            }
        }
        if(env.parameters.mdp.init.max_angle > 0 && !guidance){
            do{
                T u[3];
                for(TI i = 0; i < 3; i++){
                    u[i] = random::uniform_real_distribution(random_dev, (T)0, (T)1, rng);
                }
                state.orientation[0] = math::sqrt(math_dev, 1-u[0]) * math::sin(math_dev, 2*math::PI<T>*u[1]);
                state.orientation[1] = math::sqrt(math_dev, 1-u[0]) * math::cos(math_dev, 2*math::PI<T>*u[1]);
                state.orientation[2] = math::sqrt(math_dev,   u[0]) * math::sin(math_dev, 2*math::PI<T>*u[2]);
                state.orientation[3] = math::sqrt(math_dev,   u[0]) * math::cos(math_dev, 2*math::PI<T>*u[2]);
            } while(math::abs(math_dev, 2*math::acos(math_dev, state.orientation[0])) > env.parameters.mdp.init.max_angle);
        }
        else{
            state.orientation[0] = 1;
            state.orientation[1] = 0;
            state.orientation[2] = 0;
            state.orientation[3] = 0;
        }
        for(TI i = 0; i < 3; i++){
            state.linear_velocity[i] = random::uniform_real_distribution(random_dev, -env.parameters.mdp.init.max_linear_velocity, env.parameters.mdp.init.max_linear_velocity, rng);
        }
        for(TI i = 0; i < 3; i++){
            state.angular_velocity[i] = random::uniform_real_distribution(random_dev, -env.parameters.mdp.init.max_angular_velocity, env.parameters.mdp.init.max_angular_velocity, rng);
        }
        initial_parameters(device, env, state);
    }
    template<typename DEVICE, typename T_S, typename TI_S, typename SPEC, typename NEXT_COMPONENT, typename RNG>
    RL_TOOLS_FUNCTION_PLACEMENT static void sample_initial_state(DEVICE& device, rl::environments::Multirotor<SPEC>& env, typename rl::environments::multirotor::StateRandomForce<T_S, TI_S, NEXT_COMPONENT>& state, RNG& rng){
        typename DEVICE::SPEC::RANDOM random_dev;
        using T = typename SPEC::T;
//        bool guidance = random::uniform_real_distribution(random_dev, (T)0, (T)1, rng) < env.parameters.mdp.init.guidance;
        sample_initial_state(device, env, static_cast<NEXT_COMPONENT&>(state), rng);
//        if(!guidance){
        {
            auto distribution = env.parameters.disturbances.random_force;
            state.force[0] = random::normal_distribution::sample(random_dev, (T)distribution.mean, (T)distribution.std, rng);
            state.force[1] = random::normal_distribution::sample(random_dev, (T)distribution.mean, (T)distribution.std, rng);
            state.force[2] = random::normal_distribution::sample(random_dev, (T)distribution.mean, (T)distribution.std, rng);
        }
        {
            auto distribution = env.parameters.disturbances.random_torque;
            state.torque[0] = random::normal_distribution::sample(random_dev, (T)distribution.mean, (T)distribution.std, rng);
            state.torque[1] = random::normal_distribution::sample(random_dev, (T)distribution.mean, (T)distribution.std, rng);
            state.torque[2] = random::normal_distribution::sample(random_dev, (T)distribution.mean, (T)distribution.std, rng);
        }
//        }
//        else{
//            state.force[0] = 0;
//            state.force[1] = 0;
//            state.force[2] = 0;
//            state.torque[0] = 0;
//            state.torque[1] = 0;
//            state.torque[2] = 0;
//        }

    }
    template<typename DEVICE, typename T, typename TI, typename SPEC, typename NEXT_COMPONENT, typename RNG>
    RL_TOOLS_FUNCTION_PLACEMENT static void sample_initial_state(DEVICE& device, rl::environments::Multirotor<SPEC>& env, typename rl::environments::multirotor::StateRotors<T, TI, NEXT_COMPONENT>& state, RNG& rng){
        sample_initial_state(device, env, static_cast<NEXT_COMPONENT&>(state), rng);
        T min_rpm, max_rpm;
        if(env.parameters.mdp.init.relative_rpm){
            min_rpm = (env.parameters.mdp.init.min_rpm + 1)/2 * (env.parameters.dynamics.action_limit.max - env.parameters.dynamics.action_limit.min) + env.parameters.dynamics.action_limit.min;
            max_rpm = (env.parameters.mdp.init.max_rpm + 1)/2 * (env.parameters.dynamics.action_limit.max - env.parameters.dynamics.action_limit.min) + env.parameters.dynamics.action_limit.min;
        }
        else{
            min_rpm = env.parameters.mdp.init.min_rpm < 0 ? env.parameters.dynamics.action_limit.min : env.parameters.mdp.init.min_rpm;
            max_rpm = env.parameters.mdp.init.max_rpm < 0 ? env.parameters.dynamics.action_limit.max : env.parameters.mdp.init.max_rpm;
            if(max_rpm > env.parameters.dynamics.action_limit.max){
                max_rpm = env.parameters.dynamics.action_limit.max;
            }
            if(min_rpm > max_rpm){
                min_rpm = max_rpm;
            }
        }
        for(TI i = 0; i < 4; i++){
            state.rpm[i] = random::uniform_real_distribution(typename DEVICE::SPEC::RANDOM(), min_rpm, max_rpm, rng);
        }
    }
    template<typename DEVICE, typename T_S, typename TI_S, TI_S HISTORY_LENGTH, typename NEXT_COMPONENT, typename SPEC, typename RNG>
    RL_TOOLS_FUNCTION_PLACEMENT static void sample_initial_state(DEVICE& device, rl::environments::Multirotor<SPEC>& env, typename rl::environments::multirotor::StateRotorsHistory<T_S, TI_S, HISTORY_LENGTH, NEXT_COMPONENT>& state, RNG& rng){
        using MULTIROTOR = rl::environments::Multirotor<SPEC>;
        using TI = typename DEVICE::index_t;
        sample_initial_state(device, env, static_cast<typename rl::environments::multirotor::StateRotors<T_S, TI_S, NEXT_COMPONENT>&>(state), rng);
        for(TI step_i = 0; step_i < HISTORY_LENGTH; step_i++){
            for(TI action_i = 0; action_i < MULTIROTOR::ACTION_DIM; action_i++){
                state.action_history[step_i][action_i] = (state.rpm[action_i] - env.parameters.dynamics.action_limit.min) / (env.parameters.dynamics.action_limit.max - env.parameters.dynamics.action_limit.min) * 2 - 1;
            }
        }
    }
    namespace rl::environments::multirotor{
        template<typename DEVICE, typename SPEC, typename STATE, typename OBSERVATION_TI, typename OBS_SPEC, typename RNG>
        RL_TOOLS_FUNCTION_PLACEMENT static void observe(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const STATE& state, rl::environments::multirotor::observation::LastComponent<OBSERVATION_TI>, Matrix<OBS_SPEC>& observation, RNG& rng){
            static_assert(OBS_SPEC::COLS == 0);
            static_assert(OBS_SPEC::ROWS == 1);
        }
        template<typename DEVICE, typename SPEC, typename OBSERVATION_SPEC, typename OBS_SPEC, typename RNG>
        RL_TOOLS_FUNCTION_PLACEMENT static void observe(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const typename rl::environments::Multirotor<SPEC>::State& state, rl::environments::multirotor::observation::Position<OBSERVATION_SPEC>, Matrix<OBS_SPEC>& observation, RNG& rng){
            using OBSERVATION = rl::environments::multirotor::observation::Position<OBSERVATION_SPEC>;
            static_assert(OBS_SPEC::COLS >= OBSERVATION::CURRENT_DIM);
            static_assert(OBS_SPEC::ROWS == 1);
            using T = typename SPEC::T;
            using TI = typename DEVICE::index_t;

            for(TI i = 0; i < 3; i++){
                if constexpr(OBSERVATION_SPEC::PRIVILEGED && !SPEC::STATIC_PARAMETERS::PRIVILEGED_OBSERVATION_NOISE){
                    set(observation, 0, i, state.position[i]);
                }
                else{
                    T noise = random::normal_distribution::sample(typename DEVICE::SPEC::RANDOM{}, (T)0, env.parameters.mdp.observation_noise.position, rng);
                    set(observation, 0, i, state.position[i] + noise);
                }
            }
            auto next_observation = view(device, observation, matrix::ViewSpec<1, OBS_SPEC::COLS - OBSERVATION::CURRENT_DIM>{}, 0, OBSERVATION::CURRENT_DIM);
            observe(device, env, state, typename OBSERVATION::NEXT_COMPONENT{}, next_observation, rng);
        }
        template<typename DEVICE, typename SPEC, typename OBSERVATION_SPEC, typename OBS_SPEC, typename RNG>
        RL_TOOLS_FUNCTION_PLACEMENT static void observe(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const typename rl::environments::Multirotor<SPEC>::State& state, rl::environments::multirotor::observation::OrientationQuaternion<OBSERVATION_SPEC>, Matrix<OBS_SPEC>& observation, RNG& rng){
            using T = typename SPEC::T;
            using TI = typename DEVICE::index_t;
            using OBSERVATION = rl::environments::multirotor::observation::OrientationQuaternion<OBSERVATION_SPEC>;
            static_assert(OBS_SPEC::COLS >= OBSERVATION::CURRENT_DIM);
            static_assert(OBS_SPEC::ROWS == 1);
            for(TI i = 0; i < OBSERVATION::CURRENT_DIM; i++){
                if constexpr(OBSERVATION_SPEC::PRIVILEGED && !SPEC::STATIC_PARAMETERS::PRIVILEGED_OBSERVATION_NOISE){
                    set(observation, 0, i, state.orientation[i]);
                }
                else{
                    T noise = random::normal_distribution::sample(typename DEVICE::SPEC::RANDOM{}, (T)0, env.parameters.mdp.observation_noise.orientation, rng);
                    set(observation, 0, i, state.orientation[i] + noise);
                }
            }
            auto next_observation = view(device, observation, matrix::ViewSpec<1, OBS_SPEC::COLS - OBSERVATION::CURRENT_DIM>{}, 0, OBSERVATION::CURRENT_DIM);
            observe(device, env, state, typename OBSERVATION::NEXT_COMPONENT{}, next_observation, rng);
        }
        template<typename DEVICE, typename SPEC, typename OBSERVATION_SPEC, typename OBS_SPEC, typename RNG>
        RL_TOOLS_FUNCTION_PLACEMENT static void observe(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const typename rl::environments::Multirotor<SPEC>::State& state, rl::environments::multirotor::observation::OrientationRotationMatrix<OBSERVATION_SPEC>, Matrix<OBS_SPEC>& observation, RNG& rng){
            using T = typename SPEC::T;
            using TI = typename DEVICE::index_t;
            using OBSERVATION = rl::environments::multirotor::observation::OrientationRotationMatrix<OBSERVATION_SPEC>;
            static_assert(OBS_SPEC::COLS >= OBSERVATION::CURRENT_DIM);
            static_assert(OBS_SPEC::ROWS == 1);
            const typename SPEC::T* q = state.orientation;
            set(observation, 0, 0, (1 - 2*q[2]*q[2] - 2*q[3]*q[3]));
            set(observation, 0, 1, (    2*q[1]*q[2] - 2*q[0]*q[3]));
            set(observation, 0, 2, (    2*q[1]*q[3] + 2*q[0]*q[2]));
            set(observation, 0, 3, (    2*q[1]*q[2] + 2*q[0]*q[3]));
            set(observation, 0, 4, (1 - 2*q[1]*q[1] - 2*q[3]*q[3]));
            set(observation, 0, 5, (    2*q[2]*q[3] - 2*q[0]*q[1]));
            set(observation, 0, 6, (    2*q[1]*q[3] - 2*q[0]*q[2]));
            set(observation, 0, 7, (    2*q[2]*q[3] + 2*q[0]*q[1]));
            set(observation, 0, 8, (1 - 2*q[1]*q[1] - 2*q[2]*q[2]));
            if constexpr(!OBSERVATION_SPEC::PRIVILEGED || SPEC::STATIC_PARAMETERS::PRIVILEGED_OBSERVATION_NOISE){
                for(TI i = 0; i < OBSERVATION::CURRENT_DIM; i++){
                    T noise;
                    noise = random::normal_distribution::sample(typename DEVICE::SPEC::RANDOM(), (T)0, env.parameters.mdp.observation_noise.orientation, rng);
                    increment(observation, 0, i, noise);
                }
            }
            auto next_observation = view(device, observation, matrix::ViewSpec<1, OBS_SPEC::COLS - OBSERVATION::CURRENT_DIM>{}, 0, OBSERVATION::CURRENT_DIM);
            observe(device, env, state, typename OBSERVATION::NEXT_COMPONENT{}, next_observation, rng);
        }
        template<typename DEVICE, typename SPEC, typename OBSERVATION_SPEC, typename OBS_SPEC, typename RNG>
        RL_TOOLS_FUNCTION_PLACEMENT static void observe(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const typename rl::environments::Multirotor<SPEC>::State& state, rl::environments::multirotor::observation::LinearVelocity<OBSERVATION_SPEC>, Matrix<OBS_SPEC>& observation, RNG& rng){
            using T = typename SPEC::T;
            using TI = typename DEVICE::index_t;
            using OBSERVATION = rl::environments::multirotor::observation::LinearVelocity<OBSERVATION_SPEC>;
            static_assert(OBS_SPEC::COLS >= OBSERVATION::CURRENT_DIM);
            static_assert(OBS_SPEC::ROWS == 1);
            for(TI i = 0; i < OBSERVATION::CURRENT_DIM; i++){
                if constexpr(OBSERVATION_SPEC::PRIVILEGED && !SPEC::STATIC_PARAMETERS::PRIVILEGED_OBSERVATION_NOISE){
                    set(observation, 0, i, state.linear_velocity[i]);
                }
                else{
                    T noise = random::normal_distribution::sample(typename DEVICE::SPEC::RANDOM{}, (T)0, env.parameters.mdp.observation_noise.linear_velocity, rng);
                    set(observation, 0, i, state.linear_velocity[i] + noise);
                }
            }
            auto next_observation = view(device, observation, matrix::ViewSpec<1, OBS_SPEC::COLS - OBSERVATION::CURRENT_DIM>{}, 0, OBSERVATION::CURRENT_DIM);
            observe(device, env, state, typename OBSERVATION::NEXT_COMPONENT{}, next_observation, rng);
        }
        template<typename DEVICE, typename SPEC, typename OBSERVATION_SPEC, typename OBS_SPEC, typename RNG>
        RL_TOOLS_FUNCTION_PLACEMENT static void observe(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const typename rl::environments::Multirotor<SPEC>::State& state, rl::environments::multirotor::observation::AngularVelocity<OBSERVATION_SPEC>, Matrix<OBS_SPEC>& observation, RNG& rng){
            using T = typename SPEC::T;
            using TI = typename DEVICE::index_t;
            using OBSERVATION = rl::environments::multirotor::observation::AngularVelocity<OBSERVATION_SPEC>;
            static_assert(OBS_SPEC::COLS >= OBSERVATION::CURRENT_DIM);
            static_assert(OBS_SPEC::ROWS == 1);
            for(TI i = 0; i < OBSERVATION::CURRENT_DIM; i++){
                if constexpr(OBSERVATION_SPEC::PRIVILEGED && !SPEC::STATIC_PARAMETERS::PRIVILEGED_OBSERVATION_NOISE){
                    set(observation, 0, i, state.angular_velocity[i]);
                }
                else{
                    T noise = random::normal_distribution::sample(typename DEVICE::SPEC::RANDOM{}, (T)0, env.parameters.mdp.observation_noise.angular_velocity, rng);
                    set(observation, 0, i, state.angular_velocity[i] + noise);
                }
            }
            auto next_observation = view(device, observation, matrix::ViewSpec<1, OBS_SPEC::COLS - OBSERVATION::CURRENT_DIM>{}, 0, OBSERVATION::CURRENT_DIM);
            observe(device, env, state, typename OBSERVATION::NEXT_COMPONENT{}, next_observation, rng);
        }
        template<typename DEVICE, typename SPEC, typename OBSERVATION_SPEC, typename OBS_SPEC, typename RNG>
        RL_TOOLS_FUNCTION_PLACEMENT static void observe(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const typename rl::environments::Multirotor<SPEC>::State& state, rl::environments::multirotor::observation::RotorSpeeds<OBSERVATION_SPEC>, Matrix<OBS_SPEC>& observation, RNG& rng){
            using T = typename SPEC::T;
            using TI = typename DEVICE::index_t;
            using OBSERVATION = rl::environments::multirotor::observation::RotorSpeeds<OBSERVATION_SPEC>;
            static_assert(OBS_SPEC::COLS >= OBSERVATION::CURRENT_DIM);
            static_assert(OBS_SPEC::ROWS == 1);
            for(TI action_i = 0; action_i < OBSERVATION::CURRENT_DIM; action_i++){
                T action_value = (state.rpm[action_i] - env.parameters.dynamics.action_limit.min)/(env.parameters.dynamics.action_limit.max - env.parameters.dynamics.action_limit.min) * 2 - 1;
                set(observation, 0, action_i, action_value);
            }
            auto next_observation = view(device, observation, matrix::ViewSpec<1, OBS_SPEC::COLS - OBSERVATION::CURRENT_DIM>{}, 0, OBSERVATION::CURRENT_DIM);
            observe(device, env, state, typename OBSERVATION::NEXT_COMPONENT{}, next_observation, rng);
        }
        template<typename DEVICE, typename SPEC, typename OBSERVATION_SPEC, typename OBS_SPEC, typename RNG>
        RL_TOOLS_FUNCTION_PLACEMENT static void observe(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const typename rl::environments::Multirotor<SPEC>::State& state, rl::environments::multirotor::observation::ActionHistory<OBSERVATION_SPEC>, Matrix<OBS_SPEC>& observation, RNG& rng){
            using T = typename SPEC::T;
            using TI = typename DEVICE::index_t;
            using OBSERVATION = rl::environments::multirotor::observation::ActionHistory<OBSERVATION_SPEC>;
            static_assert(OBS_SPEC::COLS >= OBSERVATION::CURRENT_DIM);
            static_assert(OBS_SPEC::ROWS == 1);
            static_assert(rl::environments::Multirotor<SPEC>::State::HISTORY_LENGTH == OBSERVATION::HISTORY_LENGTH);
            static_assert(rl::environments::Multirotor<SPEC>::State::ACTION_DIM == OBSERVATION::ACTION_DIM);
            static_assert(rl::environments::Multirotor<SPEC>::ACTION_DIM == OBSERVATION::ACTION_DIM);
            for(TI step_i = 0; step_i < OBSERVATION::HISTORY_LENGTH; step_i++){
                for(TI action_i = 0; action_i < OBSERVATION::ACTION_DIM; action_i++){
                    set(observation, 0, step_i*OBSERVATION::ACTION_DIM + action_i, state.action_history[step_i][action_i]);
                }
            }
            auto next_observation = view(device, observation, matrix::ViewSpec<1, OBS_SPEC::COLS - OBSERVATION::CURRENT_DIM>{}, 0, OBSERVATION::CURRENT_DIM);
            observe(device, env, state, typename OBSERVATION::NEXT_COMPONENT{}, next_observation, rng);
        }
        template<typename DEVICE, typename SPEC, typename OBSERVATION_SPEC, typename OBS_SPEC, typename RNG>
        RL_TOOLS_FUNCTION_PLACEMENT static void observe(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const typename rl::environments::Multirotor<SPEC>::State& state, rl::environments::multirotor::observation::RandomForce<OBSERVATION_SPEC>, Matrix<OBS_SPEC>& observation, RNG& rng){
            using T = typename SPEC::T;
            using TI = typename DEVICE::index_t;
            using OBSERVATION = rl::environments::multirotor::observation::RandomForce<OBSERVATION_SPEC>;
            static_assert(OBS_SPEC::COLS >= OBSERVATION::CURRENT_DIM);
            static_assert(OBS_SPEC::ROWS == 1);
            for(TI i = 0; i < 3; i++){
                set(observation, 0, i, state.force[i]);
                set(observation, 0, 3 + i, state.torque[i]);
            }
            auto next_observation = view(device, observation, matrix::ViewSpec<1, OBS_SPEC::COLS - OBSERVATION::CURRENT_DIM>{}, 0, OBSERVATION::CURRENT_DIM);
            observe(device, env, state, typename OBSERVATION::NEXT_COMPONENT{}, next_observation, rng);
        }
    }
    template<typename DEVICE, typename SPEC, typename STATE, typename OBS_SPEC, typename RNG>
    RL_TOOLS_FUNCTION_PLACEMENT static void observe(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const STATE& state, Matrix<OBS_SPEC>& observation, RNG& rng){
        using ENVIRONMENT = rl::environments::Multirotor<SPEC>;
        static_assert(OBS_SPEC::COLS == ENVIRONMENT::OBSERVATION_DIM);
        static_assert(OBS_SPEC::ROWS == 1);
        rl::environments::multirotor::observe(device, env, state, typename ENVIRONMENT::Observation{}, observation, rng);
    }
    template<typename DEVICE, typename SPEC, typename STATE, typename OBS_SPEC, typename RNG>
    RL_TOOLS_FUNCTION_PLACEMENT static void observe_privileged(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const STATE& state, Matrix<OBS_SPEC>& observation, RNG& rng){
        using ENVIRONMENT = rl::environments::Multirotor<SPEC>;
        static_assert(OBS_SPEC::COLS == ENVIRONMENT::OBSERVATION_DIM_PRIVILEGED);
        static_assert(OBS_SPEC::ROWS == 1);
        rl::environments::multirotor::observe(device, env, state, typename ENVIRONMENT::ObservationPrivileged{}, observation, rng);
    }
//    template<typename DEVICE, typename T, typename TI, typename SPEC, typename OBS_SPEC, typename RNG>
//    RL_TOOLS_FUNCTION_PLACEMENT static void observe_privileged(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const rl::environments::multirotor::StateLatentEmpty<T, TI>& state, Matrix<OBS_SPEC>& observation, RNG& rng){
//        static_assert(OBS_SPEC::COLS == 0);
//    }
//    template<typename DEVICE, typename T, typename TI, typename SPEC, typename OBS_SPEC, typename RNG>
//    RL_TOOLS_FUNCTION_PLACEMENT static void observe_privileged(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const rl::environments::multirotor::StateLatentRandomForce<T, TI>& state, Matrix<OBS_SPEC>& observation, RNG& rng){
//        static_assert(OBS_SPEC::COLS == 6);
//        for(TI i = 0; i < 3; i++){
//            set(observation, 0, 0 + i, state.force[i]);
//            set(observation, 0, 3 + i, state.torque[i]);
//        }
//    }
//    template<typename DEVICE, typename T, typename TI, typename SPEC, typename OBS_SPEC, typename LATENT_STATE, typename RNG>
//    RL_TOOLS_FUNCTION_PLACEMENT static void observe_privileged(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const rl::environments::multirotor::StateBase<T, TI, LATENT_STATE>& state, Matrix<OBS_SPEC>& observation, RNG& rng){
//        using MULTIROTOR = rl::environments::Multirotor<SPEC>;
//        static_assert(OBS_SPEC::COLS == MULTIROTOR::OBSERVATION_DIM_BASE + MULTIROTOR::OBSERVATION_DIM_ORIENTATION + MULTIROTOR::OBSERVATION_DIM_PRIVILEGED_LATENT_STATE);
//        auto base_observation = view(device, observation, matrix::ViewSpec<1, MULTIROTOR::OBSERVATION_DIM_BASE + MULTIROTOR::OBSERVATION_DIM_ORIENTATION>{}, 0, 0);
//        observe(device, env, state, base_observation, rng, true);
//        auto latent_observation = view(device, observation, matrix::ViewSpec<1, MULTIROTOR::OBSERVATION_DIM_PRIVILEGED_LATENT_STATE>{}, 0, MULTIROTOR::OBSERVATION_DIM_BASE + MULTIROTOR::OBSERVATION_DIM_ORIENTATION);
//        observe_privileged(device, env, (const LATENT_STATE&)state, latent_observation, rng);
//    }
//    template<typename DEVICE, typename T_H, typename TI_H, TI_H HISTORY_LENGTH, typename SPEC, typename OBS_SPEC, typename LATENT_STATE, typename RNG>
//    RL_TOOLS_FUNCTION_PLACEMENT static void observe(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const rl::environments::multirotor::StateBaseRotorsHistory<T_H, TI_H, HISTORY_LENGTH, LATENT_STATE>& state, Matrix<OBS_SPEC>& observation, RNG& rng){
//        using MULTIROTOR = rl::environments::Multirotor<SPEC>;
//        static_assert(OBS_SPEC::ROWS == 1);
//        static_assert(OBS_SPEC::COLS == MULTIROTOR::OBSERVATION_DIM);
//        using TI = typename DEVICE::index_t;
//        auto base_observation = view(device, observation, matrix::ViewSpec<1, MULTIROTOR::OBSERVATION_DIM_BASE + MULTIROTOR::OBSERVATION_DIM_ORIENTATION>{}, 0, 0);
//        observe(device, env, (rl::environments::multirotor::StateBaseRotors<T_H, TI_H, LATENT_STATE>&)state, base_observation, rng);
//        auto action_history_observation = view(device, observation, matrix::ViewSpec<1, MULTIROTOR::OBSERVATION_DIM_ACTION_HISTORY>{}, 0, MULTIROTOR::OBSERVATION_DIM_BASE + MULTIROTOR::OBSERVATION_DIM_ORIENTATION);
//        for(TI step_i = 0; step_i < HISTORY_LENGTH; step_i++){
//            for(TI action_i = 0; action_i < MULTIROTOR::ACTION_DIM; action_i++){
//                set(action_history_observation, 0, step_i*MULTIROTOR::ACTION_DIM + action_i, state.action_history[step_i][action_i]);
//            }
//        }
//    }
//    template<typename DEVICE, typename T_S, typename TI_S, typename SPEC, typename OBS_SPEC, typename LATENT_STATE, typename RNG>
//    RL_TOOLS_FUNCTION_PLACEMENT static void observe_privileged(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const rl::environments::multirotor::StateBaseRotors<T_S, TI_S, LATENT_STATE>& state, Matrix<OBS_SPEC>& observation, RNG& rng){
//        using MULTIROTOR = rl::environments::Multirotor<SPEC>;
//        using T = typename SPEC::T;
//        using TI = typename DEVICE::index_t;
//        static_assert(OBS_SPEC::ROWS == 1);
//        static_assert(OBS_SPEC::COLS == MULTIROTOR::OBSERVATION_DIM_PRIVILEGED);
//        auto base_observation = view(device, observation, matrix::ViewSpec<1, MULTIROTOR::OBSERVATION_DIM_PRIVILEGED - MULTIROTOR::ACTION_DIM>{}, 0, 0);
//        observe_privileged(device, env, (const rl::environments::multirotor::StateBase<T_S, TI_S, LATENT_STATE>&) state, base_observation, rng);
//        auto rpm_observation = view(device, observation, matrix::ViewSpec<1, MULTIROTOR::ACTION_DIM>{}, 0, MULTIROTOR::OBSERVATION_DIM_PRIVILEGED - MULTIROTOR::ACTION_DIM);
//        for(TI action_i = 0; action_i < MULTIROTOR::ACTION_DIM; action_i++){
//            T action_value = (state.rpm[action_i] - env.parameters.dynamics.action_limit.min)/(env.parameters.dynamics.action_limit.max - env.parameters.dynamics.action_limit.min) * 2 - 1;
//            set(rpm_observation, 0, action_i, action_value);
//        }
//    }
//    template<typename DEVICE, typename SPEC, typename T, typename TI>
//    RL_TOOLS_FUNCTION_PLACEMENT void post_integration(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, rl::environments::multirotor::StateBase<T, TI>& state) {
    template<typename DEVICE, typename SPEC, typename ACTION_SPEC, typename T_S, typename TI_S, typename RNG>
    RL_TOOLS_FUNCTION_PLACEMENT void post_integration(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const typename rl::environments::multirotor::StateBase<T_S, TI_S>& state, const Matrix<ACTION_SPEC>& action, typename rl::environments::multirotor::StateBase<T_S, TI_S>& next_state, RNG& rng) {
        using T = T_S;
        using TI = TI_S;
        T quaternion_norm = 0;
        for(TI state_i = 0; state_i < 4; state_i++){
            quaternion_norm += next_state.orientation[state_i] * next_state.orientation[state_i];
        }
        quaternion_norm = math::sqrt(device.math, quaternion_norm);
        for(TI state_i = 0; state_i < 4; state_i++){
            next_state.orientation[state_i] /= quaternion_norm;
        }
    }
//    template<typename DEVICE, typename SPEC, typename T, typename TI, typename NEXT_COMPONENT>
//    RL_TOOLS_FUNCTION_PLACEMENT void post_integration(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, rl::environments::multirotor::StateRotors<T, TI, NEXT_COMPONENT>& state) {
    template<typename DEVICE, typename SPEC, typename ACTION_SPEC, typename T_S, typename TI_S, typename NEXT_COMPONENT, typename RNG>
    RL_TOOLS_FUNCTION_PLACEMENT void post_integration(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const typename rl::environments::multirotor::StateRotors<T_S, TI_S, NEXT_COMPONENT>& state, const Matrix<ACTION_SPEC>& action, typename rl::environments::multirotor::StateRotors<T_S, TI_S, NEXT_COMPONENT>& next_state, RNG& rng) {
        post_integration(device, env, static_cast<const NEXT_COMPONENT&>(state), action, static_cast<NEXT_COMPONENT&>(next_state), rng);
        using MULTIROTOR = rl::environments::Multirotor<SPEC>;
        for(typename DEVICE::index_t rpm_i = 0; rpm_i < MULTIROTOR::ACTION_DIM; rpm_i++){
            next_state.rpm[rpm_i] = math::clamp(typename DEVICE::SPEC::MATH{}, next_state.rpm[rpm_i], env.parameters.dynamics.action_limit.min, env.parameters.dynamics.action_limit.max);
        }
    }
    template<typename DEVICE, typename SPEC, typename ACTION_SPEC, typename T_S, typename TI_S, typename NEXT_COMPONENT, typename RNG>
    RL_TOOLS_FUNCTION_PLACEMENT void post_integration(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const typename rl::environments::multirotor::StateRandomForce<T_S, TI_S, NEXT_COMPONENT>& state, const Matrix<ACTION_SPEC>& action, typename rl::environments::multirotor::StateRandomForce<T_S, TI_S, NEXT_COMPONENT>& next_state, RNG& rng) {
        post_integration(device, env, static_cast<const NEXT_COMPONENT&>(state), action, static_cast<NEXT_COMPONENT&>(next_state), rng);
        next_state.force[0] = state.force[0];
        next_state.force[1] = state.force[1];
        next_state.force[2] = state.force[2];
        next_state.torque[0] = state.torque[0];
        next_state.torque[1] = state.torque[1];
        next_state.torque[2] = state.torque[2];
    }
    template<typename DEVICE, typename T_S, typename TI_S, typename NEXT_STATE_COMPONENT, TI_S HISTORY_LENGTH, typename SPEC, typename ACTION_SPEC, typename RNG>
    RL_TOOLS_FUNCTION_PLACEMENT void post_integration(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const typename rl::environments::multirotor::StateRotorsHistory<T_S, TI_S, HISTORY_LENGTH, NEXT_STATE_COMPONENT>& state, const Matrix<ACTION_SPEC>& action, typename rl::environments::multirotor::StateRotorsHistory<T_S, TI_S, HISTORY_LENGTH, NEXT_STATE_COMPONENT>& next_state, RNG& rng) {
        using MULTIROTOR = rl::environments::Multirotor<SPEC>;
        using TI = typename DEVICE::index_t;
        static_assert(ACTION_SPEC::COLS == MULTIROTOR::ACTION_DIM);
        post_integration(device, env, static_cast<const rl::environments::multirotor::StateRotors<T_S, TI_S, NEXT_STATE_COMPONENT>&>(state), action, static_cast<rl::environments::multirotor::StateRotors<T_S, TI_S, NEXT_STATE_COMPONENT>&>(next_state), rng);
        if constexpr(HISTORY_LENGTH > 0){
            for(TI step_i = 0; step_i < HISTORY_LENGTH-1; step_i++){
                for(TI action_i = 0; action_i < MULTIROTOR::ACTION_DIM; action_i++){
                    next_state.action_history[step_i][action_i] = state.action_history[step_i+1][action_i];
                }
            }
            for(TI action_i = 0; action_i < MULTIROTOR::ACTION_DIM; action_i++){
                next_state.action_history[HISTORY_LENGTH-1][action_i] = get(action, 0, action_i);
            }
        }
    }
//    template<typename DEVICE, typename SPEC, typename ACTION_SPEC, typename T_S, typename TI_S, typename STATE, typename RNG>
//    RL_TOOLS_FUNCTION_PLACEMENT void post_integration(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const STATE& state, const Matrix<ACTION_SPEC>& action, STATE& next_state, RNG& rng) {
//        static_assert(!STATE::REQUIRES_INTEGRATION);
//        post_integration(device, env, static_cast<typename STATE::NEXT_COMPONENT&>(state), action, static_cast<typename STATE::NEXT_COMPONENT&>(next_state), rng);
//    }
    // todo: make state const again
    template<typename DEVICE, typename SPEC, typename ACTION_SPEC, typename RNG>
    RL_TOOLS_FUNCTION_PLACEMENT static typename SPEC::T step(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const typename rl::environments::Multirotor<SPEC>::State& state, const Matrix<ACTION_SPEC>& action, typename rl::environments::Multirotor<SPEC>::State& next_state, RNG& rng) {
        using STATE = typename rl::environments::Multirotor<SPEC>::State;
        using T = typename SPEC::T;
        using TI = typename DEVICE::index_t;
        constexpr auto STATE_DIM = STATE::DIM;
        constexpr auto ACTION_DIM = rl::environments::Multirotor<SPEC>::ACTION_DIM;
        static_assert(ACTION_SPEC::ROWS == 1);
        static_assert(ACTION_SPEC::COLS == ACTION_DIM);
        T action_scaled[ACTION_DIM];

        for(TI action_i = 0; action_i < ACTION_DIM; action_i++){
            T half_range = (env.parameters.dynamics.action_limit.max - env.parameters.dynamics.action_limit.min) / 2;
            T action_noisy = get(action, 0, action_i);
            action_noisy += random::normal_distribution::sample(typename DEVICE::SPEC::RANDOM(), (T)0, env.parameters.mdp.action_noise.normalized_rpm, rng);
            action_noisy = math::clamp(device.math, action_noisy, -(T)1, (T)1);
            action_scaled[action_i] = action_noisy * half_range + env.parameters.dynamics.action_limit.min + half_range;
//            state.rpm[action_i] = action_scaled[action_i];
        }
        utils::integrators::rk4  <DEVICE, typename SPEC::T, typename SPEC::PARAMETERS, STATE, ACTION_DIM, rl::environments::multirotor::multirotor_dynamics_dispatch<DEVICE, typename SPEC::T, typename SPEC::PARAMETERS, STATE>>(device, env.parameters, state, action_scaled, env.parameters.integration.dt, next_state);
//        utils::integrators::euler<DEVICE, typename SPEC::T, typename SPEC::PARAMETERS, STATE, ACTION_DIM, rl::environments::multirotor::multirotor_dynamics_dispatch<DEVICE, typename SPEC::T, typename SPEC::PARAMETERS, STATE>>(device, env.parameters, state, action_scaled, env.parameters.integration.dt, next_state);

        post_integration(device, env, state, action, next_state, rng);

        return env.parameters.integration.dt;
    }

    template<typename DEVICE, typename SPEC, typename RNG>
    RL_TOOLS_FUNCTION_PLACEMENT static bool terminated(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const typename rl::environments::Multirotor<SPEC>::State& state, RNG& rng){
        using T = typename SPEC::T;
        using TI = typename DEVICE::index_t;
        if(env.parameters.mdp.termination.enabled){
            for(TI i = 0; i < 3; i++){
                if(
                    math::abs(device.math, state.position[i]) > env.parameters.mdp.termination.position_threshold ||
                    math::abs(device.math, state.linear_velocity[i]) > env.parameters.mdp.termination.linear_velocity_threshold ||
                    math::abs(device.math, state.angular_velocity[i]) > env.parameters.mdp.termination.angular_velocity_threshold
                ){
                    return true;
                }
            }
        }
        return false;
    }
}
#include "parameters/reward_functions/reward_functions.h"
namespace rl_tools{
    template<typename DEVICE, typename SPEC, typename ACTION_SPEC, typename RNG>
    RL_TOOLS_FUNCTION_PLACEMENT static typename SPEC::T reward(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const typename rl::environments::Multirotor<SPEC>::State& state, const Matrix<ACTION_SPEC>& action, const typename rl::environments::Multirotor<SPEC>::State& next_state, RNG& rng) {
        return rl::environments::multirotor::parameters::reward_functions::reward(device, env, env.parameters.mdp.reward, state, action, next_state, rng);
    }
    template<typename DEVICE, typename SPEC, typename ACTION_SPEC, typename RNG>
    RL_TOOLS_FUNCTION_PLACEMENT void log_reward(DEVICE& device, const rl::environments::Multirotor<SPEC>& env, const typename rl::environments::Multirotor<SPEC>::State& state, const Matrix<ACTION_SPEC>& action, const typename rl::environments::Multirotor<SPEC>::State& next_state, RNG& rng) {
        rl::environments::multirotor::parameters::reward_functions::log_reward(device, env, env.parameters.mdp.reward, state, action, next_state, rng);
    }
}

//template<typename DEVICE, typename T, typename TI, typename SPEC, typename LATENT_STATE>
//static void deserialize(DEVICE& device, typename rl::environments::multirotor::StateBase<T, TI, LATENT_STATE>& state, Matrix<SPEC>& flat_state){
//    using STATE = typename rl::environments::multirotor::StateBase<T, TI, LATENT_STATE>;
//    static_assert(SPEC::ROWS == 1);
//    static_assert(SPEC::COLS == STATE::DIM);
//    state.position[0] = get(flat_state, 0, 0);
//    state.position[1] = get(flat_state, 0, 1);
//    state.position[2] = get(flat_state, 0, 2);
//    state.orientation[0] = get(flat_state, 0, 3);
//    state.orientation[1] = get(flat_state, 0, 4);
//    state.orientation[2] = get(flat_state, 0, 5);
//    state.orientation[3] = get(flat_state, 0, 6);
//    state.linear_velocity[0] = get(flat_state, 0, 7);
//    state.linear_velocity[1] = get(flat_state, 0, 8);
//    state.linear_velocity[2] = get(flat_state, 0, 9);
//    state.angular_velocity[0] = get(flat_state, 0, 10);
//    state.angular_velocity[1] = get(flat_state, 0, 11);
//    state.angular_velocity[2] = get(flat_state, 0, 12);
//}
//template<typename DEVICE, typename T, typename TI, typename SPEC, typename LATENT_STATE>
//static void serialize(DEVICE& device, typename rl::environments::multirotor::StateBaseRotors<T, TI, LATENT_STATE>& state, Matrix<SPEC>& flat_state){
//    using STATE = typename rl::environments::multirotor::StateBaseRotors<T, TI, LATENT_STATE>;
//    static_assert(SPEC::ROWS == 1);
//    static_assert(SPEC::COLS == STATE::DIM);
//    auto state_base_flat = view(device, flat_state, matrix::ViewSpec<1, 13>{}, 0, 0);
//    serialize(device, (rl::environments::multirotor::StateBase<T, TI, LATENT_STATE>&)state, state_base_flat);
//    constexpr TI offset = rl::environments::multirotor::StateBase<T, TI, LATENT_STATE>::DIM;
//    set(flat_state, 0, offset + 0, state.rpm[0]);
//    set(flat_state, 0, offset + 1, state.rpm[1]);
//    set(flat_state, 0, offset + 2, state.rpm[2]);
//    set(flat_state, 0, offset + 3, state.rpm[3]);
//}
//template<typename DEVICE, typename T, typename TI, typename SPEC, typename LATENT_STATE>
//static void deserialize(DEVICE& device, typename rl::environments::multirotor::StateBaseRotors<T, TI, LATENT_STATE>& state, Matrix<SPEC>& flat_state){
//    using STATE = typename rl::environments::multirotor::StateBaseRotors<T, TI, LATENT_STATE>;
//    static_assert(SPEC::ROWS == 1);
//    static_assert(SPEC::COLS == STATE::DIM);
//    constexpr TI OFFSET = rl::environments::multirotor::StateBase<T, TI, LATENT_STATE>::DIM;
//    auto state_base_rotors_flat = view(device, flat_state, matrix::ViewSpec<1, OFFSET>{}, 0, 0);
//    deserialize(device, (rl::environments::multirotor::StateBase<T, TI, LATENT_STATE>&)state, state_base_rotors_flat);
//    state.rpm[0] = get(flat_state, 0, OFFSET + 0);
//    state.rpm[1] = get(flat_state, 0, OFFSET + 1);
//    state.rpm[2] = get(flat_state, 0, OFFSET + 2);
//    state.rpm[3] = get(flat_state, 0, OFFSET + 3);
//}
//template<typename DEVICE, typename T_S, typename TI_S, TI_S HISTORY_LENGTH, typename SPEC, typename LATENT_STATE>
//static void serialize(DEVICE& device, typename rl::environments::multirotor::StateBaseRotorsHistory<T_S, TI_S, HISTORY_LENGTH, LATENT_STATE>& state, Matrix<SPEC>& flat_state){
//    using STATE = typename rl::environments::multirotor::StateBaseRotorsHistory<T_S, TI_S, HISTORY_LENGTH, LATENT_STATE>;
//    using TI = typename DEVICE::index_t;
//    static_assert(SPEC::ROWS == 1);
//    static_assert(SPEC::COLS == STATE::DIM);
//    constexpr TI OFFSET = rl::environments::multirotor::StateBaseRotors<T_S, TI_S, LATENT_STATE>::DIM;
//    auto state_base_flat = view(device, flat_state, matrix::ViewSpec<1, OFFSET>{}, 0, 0);
//    serialize(device, (rl::environments::multirotor::StateBaseRotors<T_S, TI_S, LATENT_STATE>&)state, state_base_flat);
//    for(TI step_i = 0; step_i < HISTORY_LENGTH; ++step_i){
//        set(flat_state, 0, OFFSET + step_i * 4 + 0, state.action_history[step_i][0]);
//        set(flat_state, 0, OFFSET + step_i * 4 + 1, state.action_history[step_i][1]);
//        set(flat_state, 0, OFFSET + step_i * 4 + 2, state.action_history[step_i][2]);
//        set(flat_state, 0, OFFSET + step_i * 4 + 3, state.action_history[step_i][3]);
//    }
//}
//template<typename DEVICE, typename T_S, typename TI_S, TI_S HISTORY_LENGTH, typename SPEC, typename LATENT_STATE>
//static void deserialize(DEVICE& device, typename rl::environments::multirotor::StateBaseRotorsHistory<T_S, TI_S, HISTORY_LENGTH, LATENT_STATE>& state, Matrix<SPEC>& flat_state){
//    using STATE = typename rl::environments::multirotor::StateBaseRotors<T_S, TI_S, LATENT_STATE>;
//    using TI = typename DEVICE::index_t;
//    static_assert(SPEC::ROWS == 1);
//    static_assert(SPEC::COLS == STATE::DIM);
//    auto state_base_flat = view(device, flat_state, matrix::ViewSpec<1, 13>{}, 0, 0);
//    deserialize(device, (rl::environments::multirotor::StateBaseRotors<T_S, TI_S, LATENT_STATE>&)state, state_base_flat);
//    constexpr TI offset = rl::environments::multirotor::StateBaseRotors<T_S, TI_S, LATENT_STATE>::DIM;
//    for(TI step_i = 0; step_i < HISTORY_LENGTH; ++step_i){
//        state.rpm_history[step_i][0] = get(flat_state, 0, offset + step_i * 4 + 0);
//        state.rpm_history[step_i][1] = get(flat_state, 0, offset + step_i * 4 + 1);
//        state.rpm_history[step_i][2] = get(flat_state, 0, offset + step_i * 4 + 2);
//        state.rpm_history[step_i][3] = get(flat_state, 0, offset + step_i * 4 + 3);
//    }
//}

#endif