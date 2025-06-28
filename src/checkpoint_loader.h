#ifndef LEARNING_TO_FLY_CHECKPOINT_LOADER_H
#define LEARNING_TO_FLY_CHECKPOINT_LOADER_H

#include <iostream>
#include <filesystem>

namespace learning_to_fly {
namespace checkpoint {

    // Helper function to copy weights from a checkpoint actor to a training actor
    template<typename DEVICE, typename TRAINING_ACTOR, typename CHECKPOINT_ACTOR>
    void copy_weights_from_checkpoint(DEVICE& device, TRAINING_ACTOR& training_actor, const CHECKPOINT_ACTOR& checkpoint_actor) {
        // Copy input layer weights and biases
        rlt::copy(device, device, checkpoint_actor.content.input_layer.weights.parameters, training_actor.content.input_layer.weights.parameters);
        rlt::copy(device, device, checkpoint_actor.content.input_layer.biases.parameters, training_actor.content.input_layer.biases.parameters);
        
        // Copy hidden layer weights and biases (both actors should have same structure)
        constexpr auto NUM_HIDDEN_LAYERS = TRAINING_ACTOR::NUM_HIDDEN_LAYERS;
        static_assert(NUM_HIDDEN_LAYERS == CHECKPOINT_ACTOR::NUM_HIDDEN_LAYERS, "Checkpoint and training actor must have same number of hidden layers");
        
        for(typename DEVICE::index_t layer_i = 0; layer_i < NUM_HIDDEN_LAYERS; layer_i++) {
            rlt::copy(device, device, checkpoint_actor.content.hidden_layers[layer_i].weights.parameters, training_actor.content.hidden_layers[layer_i].weights.parameters);
            rlt::copy(device, device, checkpoint_actor.content.hidden_layers[layer_i].biases.parameters, training_actor.content.hidden_layers[layer_i].biases.parameters);
        }
        
        // Copy output layer weights and biases
        rlt::copy(device, device, checkpoint_actor.content.output_layer.weights.parameters, training_actor.content.output_layer.weights.parameters);
        rlt::copy(device, device, checkpoint_actor.content.output_layer.biases.parameters, training_actor.content.output_layer.biases.parameters);
        
        std::cout << "Successfully copied weights from checkpoint to training actor" << std::endl;
    }

    // Function to check if a checkpoint file exists and is valid
    template<typename CONFIG>
    bool validate_checkpoint_path(const std::string& checkpoint_path) {
        if (checkpoint_path.empty()) {
            return false;
        }
        
        if (!std::filesystem::exists(checkpoint_path)) {
            std::cerr << "Error: Checkpoint file does not exist: " << checkpoint_path << std::endl;
            return false;
        }
        
        if (checkpoint_path.size() < 2 || checkpoint_path.substr(checkpoint_path.size() - 2) != ".h") {
            std::cerr << "Error: Checkpoint file must be a .h file: " << checkpoint_path << std::endl;
            return false;
        }
        
        return true;
    }

} // namespace checkpoint
} // namespace learning_to_fly

#endif // LEARNING_TO_FLY_CHECKPOINT_LOADER_H
