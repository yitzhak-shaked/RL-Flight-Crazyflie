namespace learning_to_fly {
    namespace steps {
        template<typename CONFIG>
        void trajectory_collection(TrainingState <CONFIG> &ts) {
            static_assert(CONFIG::N_ENVIRONMENTS == 1);
            using TI = typename CONFIG::TI;
            using T = typename CONFIG::T;
            
            // CRITICAL: Generate visualization using actor_target to match checkpoints
            // Previous approach (replay buffer) showed training actor behavior which differs from saved checkpoints
            // This runs full episodes with actor_target for accurate visualization
            
            constexpr TI VIZ_INTERVAL = 100; // Run target actor episode every 100 training steps
            static TI last_viz_step = 0;
            static bool viz_in_progress = false;
            static TI viz_step_count = 0;
            static std::vector<typename CONFIG::ENVIRONMENT::State> current_viz_trajectory;
            static typename CONFIG::ENVIRONMENT viz_env;
            static typename CONFIG::ENVIRONMENT::State viz_state;
            static typename CONFIG::ACTOR_TYPE::template DoubleBuffer<1> viz_buffer;
            static bool viz_resources_allocated = false;
            
            // One-time resource allocation
            if (!viz_resources_allocated) {
                viz_env = ts.envs[0];
                rlt::malloc(ts.device, viz_buffer);
                viz_resources_allocated = true;
            }
            
            // Start new visualization episode if interval elapsed
            if (!viz_in_progress && (ts.step - last_viz_step >= VIZ_INTERVAL)) {
                current_viz_trajectory.clear();
                viz_step_count = 0;
                rlt::sample_initial_state(ts.device, viz_env, viz_state, ts.rng_eval);
                viz_in_progress = true;
                last_viz_step = ts.step;
            }
            
            // Step through visualization episode (one step per training step to avoid blocking)
            if (viz_in_progress) {
                current_viz_trajectory.push_back(viz_state);
                
                // Evaluate actor_target (THIS IS WHAT GETS SAVED IN CHECKPOINTS!)
                // WITH POLICY SWITCHING if enabled
                rlt::MatrixDynamic<rlt::matrix::Specification<T, TI, 1, CONFIG::ENVIRONMENT::OBSERVATION_DIM>> obs;
                rlt::MatrixDynamic<rlt::matrix::Specification<T, TI, 1, CONFIG::ENVIRONMENT::ACTION_DIM>> act;
                rlt::malloc(ts.device, obs);
                rlt::malloc(ts.device, act);
                
                rlt::observe(ts.device, viz_env, viz_state, obs, ts.rng_eval);
                
                // Apply policy switching if enabled
                if (ts.use_policy_switching && ts.hover_actor_loaded) {
                    // Calculate distance to target
                    T distance = learning_to_fly::policy_switching::calculate_distance_to_target<T>(viz_state.position);
                    
                    if (distance < ts.policy_switch_threshold) {
                        // Use hover actor with transformed observations
                        rlt::MatrixDynamic<rlt::matrix::Specification<T, TI, 1, CONFIG::ENVIRONMENT::OBSERVATION_DIM>> transformed_obs;
                        rlt::malloc(ts.device, transformed_obs);
                        rlt::copy(ts.device, ts.device, obs, transformed_obs);
                        
                        T target_pos[3];
                        learning_to_fly::constants::get_target_position<T>(target_pos);
                        learning_to_fly::policy_switching::transform_observation_to_target_relative(ts.device, transformed_obs, target_pos);
                        
                        rlt::evaluate(ts.device, ts.hover_actor, transformed_obs, act, ts.hover_actor_buffer);
                        rlt::free(ts.device, transformed_obs);
                    } else {
                        // Use navigation actor_target
                        rlt::evaluate(ts.device, ts.actor_critic.actor_target, obs, act, viz_buffer);
                    }
                } else {
                    // No policy switching - use actor_target only
                    rlt::evaluate(ts.device, ts.actor_critic.actor_target, obs, act, viz_buffer);
                }
                
                typename CONFIG::ENVIRONMENT::State next_viz_state;
                rlt::step(ts.device, viz_env, viz_state, act, next_viz_state, ts.rng_eval);
                
                rlt::free(ts.device, obs);
                rlt::free(ts.device, act);
                
                bool term = rlt::terminated(ts.device, viz_env, next_viz_state, ts.rng_eval);
                viz_step_count++;
                bool done = term || (viz_step_count >= CONFIG::ENVIRONMENT_STEP_LIMIT);
                
                viz_state = next_viz_state;
                
                if (done) {
                    {
                        std::lock_guard<std::mutex> lock(ts.trajectories_mutex);
                        ts.trajectories.push(current_viz_trajectory);
                    }
                    viz_in_progress = false;
                }
            }
        }

    }
}