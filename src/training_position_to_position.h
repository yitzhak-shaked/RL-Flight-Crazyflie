#include <rl_tools/operations/cpu_mux.h>
#include <rl_tools/nn/operations_cpu_mux.h>
namespace rlt = RL_TOOLS_NAMESPACE_WRAPPER ::rl_tools;

#include <learning_to_fly/simulator/operations_cpu.h>
#include <learning_to_fly/simulator/metrics.h>

#include "config/config.h"
#include "constants.h"

#include <rl_tools/rl/algorithms/td3/loop.h>

#include "training_state.h"

#include "steps/checkpoint.h"
#include "steps/critic_reset.h"
#include "steps/curriculum.h"
#include "steps/log_reward.h"
#include "steps/logger.h"
#include "steps/trajectory_collection.h"
#include "steps/validation.h"

#include "helpers.h"

#include <filesystem>
#include <fstream>

namespace learning_to_fly{
    // Position-to-position training uses the same functions as regular training
    // but with the POSITION_TO_POSITION_ABLATION_SPEC which selects the appropriate reward function
    // No custom functions needed - just use learning_to_fly::init() and learning_to_fly::step() with the right config
}
