
#include "../../multirotor.h"

#include <rl_tools/math/operations_generic.h>

namespace rl_tools::rl::environments::multirotor::parameters::termination{
    template<typename T, typename TI, TI ACTION_DIM, typename REWARD_FUNCTION>
    constexpr typename rl_tools::rl::environments::multirotor::ParametersBase<T, TI, 4, REWARD_FUNCTION>::MDP::Termination classic = {
            true,           // enable
            3.5,            // position - increased from 1.5 to 3.5 to allow reaching 2m target with margin
            10,         // linear velocity
            10 // angular velocity
    };
    template<typename T, typename TI, TI ACTION_DIM, typename REWARD_FUNCTION>
    constexpr typename rl_tools::rl::environments::multirotor::ParametersBase<T, TI, 4, REWARD_FUNCTION>::MDP::Termination fast_learning = {
        true,           // enable
        3.5,            // position - increased from 1.5 to 3.5 to allow reaching 2m target with margin
        1000,         // linear velocity
        1000 // angular velocity
    };
}