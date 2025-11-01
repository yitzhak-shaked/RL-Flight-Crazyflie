#ifndef LEARNING_TO_FLY_STEPS_POLICY_SWITCH_H
#define LEARNING_TO_FLY_STEPS_POLICY_SWITCH_H

#include <rl_tools/operations/cpu_mux.h>
#include "../policy_switching.h"
#include "../constants.h"
#include <highfive/H5File.hpp>
#include <string>
#include <iostream>

namespace learning_to_fly {
namespace steps {

    /**
     * Load hover actor from file for policy switching
     */
    template<typename CONFIG>
    bool load_hover_actor(TrainingState<CONFIG>& ts, const std::string& hover_actor_path) {
        try {
            if (hover_actor_path.empty()) {
                std::cout << "No hover actor path specified - policy switching disabled" << std::endl;
                return false;
            }
            
            // Load hover actor from H5 file
            auto file = HighFive::File(hover_actor_path, HighFive::File::ReadOnly);
            rlt::malloc(ts.device, ts.hover_actor);
            rlt::malloc(ts.device, ts.hover_actor_buffer);
            rlt::load(ts.device, ts.hover_actor, file.getGroup("actor"));
            
            ts.hover_actor_loaded = true;
            std::cout << "✓ Successfully loaded hover actor from: " << hover_actor_path << std::endl;
            std::cout << "  Policy switching ENABLED (threshold: " << ts.policy_switch_threshold << "m)" << std::endl;
            std::cout << "  Navigation actor: obstacle avoidance + target reaching" << std::endl;
            std::cout << "  Hover actor: stable hovering at target position" << std::endl;
            
            return true;
        } catch (const std::exception& e) {
            std::cerr << "✗ Error loading hover actor from " << hover_actor_path << ": " << e.what() << std::endl;
            std::cerr << "  Policy switching DISABLED - using navigation actor only" << std::endl;
            ts.hover_actor_loaded = false;
            return false;
        }
    }

    /**
     * Enable policy switching with a specific hover actor
     */
    template<typename CONFIG>
    void enable_policy_switching(
        TrainingState<CONFIG>& ts,
        const std::string& hover_actor_path,
        typename CONFIG::T threshold = typename CONFIG::T(0.3)
    ) {
        ts.policy_switch_threshold = threshold;
        if (load_hover_actor(ts, hover_actor_path)) {
            ts.use_policy_switching = true;
        } else {
            ts.use_policy_switching = false;
        }
    }

    /**
     * Disable policy switching
     */
    template<typename CONFIG>
    void disable_policy_switching(TrainingState<CONFIG>& ts) {
        ts.use_policy_switching = false;
        if (ts.hover_actor_loaded) {
            rlt::free(ts.device, ts.hover_actor);
            rlt::free(ts.device, ts.hover_actor_buffer);
            ts.hover_actor_loaded = false;
        }
        std::cout << "Policy switching DISABLED" << std::endl;
    }

} // namespace steps
} // namespace learning_to_fly

#endif // LEARNING_TO_FLY_STEPS_POLICY_SWITCH_H
