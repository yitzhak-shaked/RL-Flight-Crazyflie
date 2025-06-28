#ifndef LEARNING_TO_FLY_TRAINING_WITH_CHECKPOINT_H
#define LEARNING_TO_FLY_TRAINING_WITH_CHECKPOINT_H

#include "training.h"

namespace learning_to_fly {

    // Function to load actor weights from an included checkpoint
    // This should be called after standard init() if you want to override with checkpoint weights
    template <typename CONFIG>
    void load_actor_weights_from_checkpoint(TrainingState<CONFIG>& ts) {
        std::cout << "Loading actor weights from included checkpoint..." << std::endl;
        
        // Create a temporary checkpoint actor to copy weights from
        typename CONFIG::ACTOR_CHECKPOINT_TYPE checkpoint_actor;
        rlt::malloc(ts.device, checkpoint_actor);
        
        try {
            // Copy the weights from the checkpoint namespace to our temporary actor
            // This requires that the checkpoint .h file has been included
            #ifdef ACTOR_CHECKPOINT_FILE
            rlt::copy(ts.device, ts.device, rlt::checkpoint::actor::model, checkpoint_actor);
            #else
            throw std::runtime_error("ACTOR_CHECKPOINT_FILE not defined");
            #endif
            
            // Now copy from the checkpoint actor to the training actor
            learning_to_fly::checkpoint::copy_weights_from_checkpoint(ts.device, ts.actor_critic.actor, checkpoint_actor);
            
            // Also update the target actor
            rlt::copy(ts.device, ts.device, ts.actor_critic.actor, ts.actor_critic.actor_target);
            
            std::cout << "Successfully loaded actor weights from checkpoint!" << std::endl;
        } catch (const std::exception& e) {
            std::cerr << "Error loading checkpoint weights: " << e.what() << std::endl;
            std::cerr << "Make sure to include the checkpoint .h file before calling this function" << std::endl;
        }
        
        rlt::free(ts.device, checkpoint_actor);
    }

    // Specialized init function that loads actor weights from an included checkpoint
    template <typename T_CONFIG>
    void init_with_checkpoint(TrainingState<T_CONFIG>& ts, typename T_CONFIG::TI seed = 0) {
        // First do the standard initialization
        learning_to_fly::init(ts, seed);
        
        // Then load checkpoint weights
        load_actor_weights_from_checkpoint(ts);
    }

} // namespace learning_to_fly

#endif // LEARNING_TO_FLY_TRAINING_WITH_CHECKPOINT_H
