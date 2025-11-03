#include "../../multirotor.h"

// CRITICAL FIX: Increased from 0.2 to 0.6 to cover full range to target at (0,1,0)
// Agent needs to experience states across the full 1m journey, not just ±0.2m!
// With 0.2m init, agent never saw positions beyond 0.4m during training -> extrapolation failure
#define RL_TOOLS_RL_ENVIRONMENTS_MULTIROTOR_PARAMETERS_INIT_POSITION (0.6)
#define RL_TOOLS_RL_ENVIRONMENTS_MULTIROTOR_PARAMETERS_INIT_LINEAR_VELOCITY (1)
#define RL_TOOLS_RL_ENVIRONMENTS_MULTIROTOR_PARAMETERS_INIT_ANGULAR_VELOCITY (1)

namespace rl_tools::rl::environments::multirotor::parameters::init{
    template<typename T, typename TI, TI ACTION_DIM, typename REWARD_FUNCTION>
    constexpr typename ParametersBase<T, TI, ACTION_DIM, REWARD_FUNCTION>::MDP::Initialization all_around = {
            0.1, // guidance
            0.3, // position
            3.14,   // orientation
            1,   // linear velocity
            10,  // angular velocity
            true,// relative rpm
            -1,  // min rpm
            +1,  // max rpm
    };
    template<typename T, typename TI, TI ACTION_DIM, typename REWARD_FUNCTION>
    constexpr typename ParametersBase<T, TI, ACTION_DIM, REWARD_FUNCTION>::MDP::Initialization all_around_2 = {
            0.1, // guidance
            RL_TOOLS_RL_ENVIRONMENTS_MULTIROTOR_PARAMETERS_INIT_POSITION,   // position
            3.14,   // orientation
            RL_TOOLS_RL_ENVIRONMENTS_MULTIROTOR_PARAMETERS_INIT_LINEAR_VELOCITY,   // linear velocity
            RL_TOOLS_RL_ENVIRONMENTS_MULTIROTOR_PARAMETERS_INIT_ANGULAR_VELOCITY,   // angular velocity
            true,// relative rpm
            -0,  // min rpm
            +0,  // max rpm
    };
    template<typename T, typename TI, TI ACTION_DIM, typename REWARD_FUNCTION>
    constexpr typename ParametersBase<T, TI, ACTION_DIM, REWARD_FUNCTION>::MDP::Initialization orientation_all_around = {
            0.1, // guidance
            RL_TOOLS_RL_ENVIRONMENTS_MULTIROTOR_PARAMETERS_INIT_POSITION,   // position
            3.14,   // orientation
            RL_TOOLS_RL_ENVIRONMENTS_MULTIROTOR_PARAMETERS_INIT_LINEAR_VELOCITY,   // linear velocity
            RL_TOOLS_RL_ENVIRONMENTS_MULTIROTOR_PARAMETERS_INIT_ANGULAR_VELOCITY,   // angular velocity
            true,// relative rpm
            0,  // min rpm
            0,  // max rpm
    };
    template<typename T, typename TI, TI ACTION_DIM, typename REWARD_FUNCTION>
    constexpr typename ParametersBase<T, TI, ACTION_DIM, REWARD_FUNCTION>::MDP::Initialization orientation_small_angle = {
            0.1, // guidance
            RL_TOOLS_RL_ENVIRONMENTS_MULTIROTOR_PARAMETERS_INIT_POSITION,   // position
            10.0/180.0*3.14,   // orientation
            RL_TOOLS_RL_ENVIRONMENTS_MULTIROTOR_PARAMETERS_INIT_LINEAR_VELOCITY,   // linear velocity
            RL_TOOLS_RL_ENVIRONMENTS_MULTIROTOR_PARAMETERS_INIT_ANGULAR_VELOCITY,   // angular velocity
            true,// relative rpm
            0,  // min rpm
            0,  // max rpm
    };
    template<typename T, typename TI, TI ACTION_DIM, typename REWARD_FUNCTION>
    constexpr typename ParametersBase<T, TI, ACTION_DIM, REWARD_FUNCTION>::MDP::Initialization orientation_big_angle = {
            0.1, // guidance
            RL_TOOLS_RL_ENVIRONMENTS_MULTIROTOR_PARAMETERS_INIT_POSITION,   // position
            20.0/180.0 * 3.14,   // orientation
            RL_TOOLS_RL_ENVIRONMENTS_MULTIROTOR_PARAMETERS_INIT_LINEAR_VELOCITY,   // linear velocity
            RL_TOOLS_RL_ENVIRONMENTS_MULTIROTOR_PARAMETERS_INIT_ANGULAR_VELOCITY,   // angular velocity
            true,// relative rpm
            0,  // min rpm
            0,  // max rpm
    };
    template<typename T, typename TI, TI ACTION_DIM, typename REWARD_FUNCTION>
    constexpr typename ParametersBase<T, TI, ACTION_DIM, REWARD_FUNCTION>::MDP::Initialization orientation_bigger_angle = {
            0.1, // guidance
            RL_TOOLS_RL_ENVIRONMENTS_MULTIROTOR_PARAMETERS_INIT_POSITION,   // position
            45.0/180.0 * 3.14,   // orientation
            RL_TOOLS_RL_ENVIRONMENTS_MULTIROTOR_PARAMETERS_INIT_LINEAR_VELOCITY,   // linear velocity
            RL_TOOLS_RL_ENVIRONMENTS_MULTIROTOR_PARAMETERS_INIT_ANGULAR_VELOCITY,   // angular velocity
            true,// relative rpm
            0,  // min rpm
            0,  // max rpm
    };
    template<typename T, typename TI, TI ACTION_DIM, typename REWARD_FUNCTION>
    constexpr typename ParametersBase<T, TI, ACTION_DIM, REWARD_FUNCTION>::MDP::Initialization orientation_biggest_angle = {
            0.1, // guidance
            RL_TOOLS_RL_ENVIRONMENTS_MULTIROTOR_PARAMETERS_INIT_POSITION,   // position
            90.0/180.0 * 3.14,   // orientation
            RL_TOOLS_RL_ENVIRONMENTS_MULTIROTOR_PARAMETERS_INIT_LINEAR_VELOCITY,   // linear velocity
            RL_TOOLS_RL_ENVIRONMENTS_MULTIROTOR_PARAMETERS_INIT_ANGULAR_VELOCITY,   // angular velocity
            true,// relative rpm
            0,  // min rpm
            0,  // max rpm
    };
    template<typename T, typename TI, TI ACTION_DIM, typename REWARD_FUNCTION>
    constexpr typename ParametersBase<T, TI, ACTION_DIM, REWARD_FUNCTION>::MDP::Initialization all_around_simplified = {
            0.1, // guidance
            0.3, // position
            0,   // orientation
            1,   // linear velocity
            10,  // angular velocity
            true,// relative rpm
            -1,  // min rpm
            +1,  // max rpm
    };
    template<typename T, typename TI, TI ACTION_DIM, typename REWARD_FUNCTION>
    constexpr typename ParametersBase<T, TI, ACTION_DIM, REWARD_FUNCTION>::MDP::Initialization simple = {
            0,   // guidance
            0,   // position
            0,   // orientation
            0,   // linear velocity
            0,   // angular velocity
            true,// relative rpm
            0,   // min rpm
            0,   // max rpm
    };
    template<typename T, typename TI, TI ACTION_DIM, typename REWARD_FUNCTION>
    constexpr typename ParametersBase<T, TI, ACTION_DIM, REWARD_FUNCTION>::MDP::Initialization all_positions = {
            0.5, // guidance
            0.3, // position
            0,   // orientation
            0,   // linear velocity
            0,  // angular velocity
            true,// relative rpm
            0,  // min rpm
            0,  // max rpm
    };
    
    // Precision hover training initialization - CRITICAL: Uses hardcoded 0.2m like original!
    // The global INIT_POSITION define is 0.6m for position-to-position training
    // For hover training, we MUST use 0.2m to train near the origin for precise hovering
    template<typename T, typename TI, TI ACTION_DIM, typename REWARD_FUNCTION>
    constexpr typename ParametersBase<T, TI, ACTION_DIM, REWARD_FUNCTION>::MDP::Initialization precision_hover = {
            0.1, // guidance
            0.2, // position - HARDCODED 0.2m like original successful hover actor (NOT the 0.6m global)
            90.0/180.0 * 3.14,   // orientation - 90 degrees like original orientation_biggest_angle
            1.0,   // linear velocity - same as original (global define value)
            1.0,   // angular velocity - same as original (global define value)
            true,// relative rpm
            0,  // min rpm
            0,  // max rpm
    };
}
