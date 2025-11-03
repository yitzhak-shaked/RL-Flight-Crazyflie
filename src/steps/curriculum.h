namespace learning_to_fly {
    namespace steps {
        template <typename CONFIG>
        void curriculum(TrainingState<CONFIG>& ts){
            using T = typename CONFIG::T;
            using TI = typename CONFIG::TI;
            if constexpr(CONFIG::ABLATION_SPEC::ENABLE_CURRICULUM == true) {
                if(ts.step != 0 && ts.step % 100000 == 0 && ts.step != (CONFIG::STEP_LIMIT - 1)){
                    rlt::add_scalar(ts.device, ts.device.logger, "td3/gamma", ts.actor_critic.gamma);
                    rlt::add_scalar(ts.device, ts.device.logger, "td3/target_next_action_noise_std", ts.actor_critic.target_next_action_noise_std);
                    rlt::add_scalar(ts.device, ts.device.logger, "td3/target_next_action_noise_clip", ts.actor_critic.target_next_action_noise_clip);
                    rlt::add_scalar(ts.device, ts.device.logger, "off_policy_runner/exploration_noise", ts.off_policy_runner.parameters.exploration_noise);


                    for(auto& env : ts.off_policy_runner.envs){
                        {
                            T action_weight = env.parameters.mdp.reward.action;
                            action_weight *= 1.4;
                            T action_weight_limit = 1.0;
                            action_weight = action_weight > action_weight_limit ? action_weight_limit : action_weight;
                            env.parameters.mdp.reward.action = action_weight;
                            rlt::add_scalar(ts.device, ts.device.logger, "reward_function/action_weight", action_weight);
                        }
                        {
                            T position_weight = env.parameters.mdp.reward.position;
                            position_weight *= 1.08;  // GENTLE increase: 8% every 100k steps (was 15%)
                            T position_weight_limit = 20;  // Conservative limit (was 40)
                            position_weight = position_weight > position_weight_limit ? position_weight_limit : position_weight;
                            env.parameters.mdp.reward.position = position_weight;
                            rlt::add_scalar(ts.device, ts.device.logger, "reward_function/position_weight", position_weight);
                        }
                        {
                            T linear_velocity_weight = env.parameters.mdp.reward.linear_velocity;
                            linear_velocity_weight *= 1.2;  // Slower increase (was 1.4)
                            T linear_velocity_weight_limit = 0.5;  // Lower limit (was 1.0)
                            linear_velocity_weight = linear_velocity_weight > linear_velocity_weight_limit ? linear_velocity_weight_limit : linear_velocity_weight;
                            env.parameters.mdp.reward.linear_velocity = linear_velocity_weight;
                            rlt::add_scalar(ts.device, ts.device.logger, "reward_function/linear_velocity_weight", linear_velocity_weight);
                        }
                        {
                            // HOVER TRAINING CURRICULUM: Gradually tighten termination threshold from 1m to 20cm
                            // More conservative than before (was 10cm which was too tight)
                            T position_threshold = env.parameters.mdp.termination.position_threshold;
                            position_threshold *= 0.96;  // GENTLER decrease: 4% every 100k steps (was 7%)
                            T position_threshold_limit = 0.2;  // More forgiving: 20cm final target (was 10cm)
                            position_threshold = position_threshold < position_threshold_limit ? position_threshold_limit : position_threshold;
                            env.parameters.mdp.termination.position_threshold = position_threshold;
                            rlt::add_scalar(ts.device, ts.device.logger, "termination/position_threshold", position_threshold);
                        }
                    }
                    if constexpr(CONFIG::ABLATION_SPEC::RECALCULATE_REWARDS == true){
                        auto start = std::chrono::high_resolution_clock::now();
                        rlt::recalculate_rewards(ts.device, ts.off_policy_runner.replay_buffers[0], ts.off_policy_runner.envs[0], ts.rng_eval);
                        auto end = std::chrono::high_resolution_clock::now();
//                        std::cout << "recalculate_rewards: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms\n";
                    }
                }
            }
            if(CONFIG::ABLATION_SPEC::EXPLORATION_NOISE_DECAY == true){
                if(ts.step % 100000 == 0 && ts.step >= 1000000){  // Start decay later at 1M instead of 500k
                    constexpr T noise_decay_base = 0.95;  // More gradual: 0.95 instead of 0.90
                    ts.off_policy_runner.parameters.exploration_noise *= noise_decay_base;
                    ts.actor_critic.target_next_action_noise_std *= noise_decay_base;
                    ts.actor_critic.target_next_action_noise_clip *= noise_decay_base;
                }
            }
        }
    }
}

