#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio.hpp>
#include <chrono>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <vector>
#include <algorithm>
#include <boost/beast/websocket.hpp>
#include <filesystem>
#include <fstream>
#include <highfive/H5File.hpp>
#include <rl_tools/operations/cpu_mux.h>
#include <rl_tools/devices/cpu.h>
#include <learning_to_fly/simulator/operations_cpu.h>
#include <learning_to_fly/simulator/ui.h>
#include <rl_tools/nn_models/operations_cpu.h>
#include <rl_tools/nn_models/persist.h>
namespace rlt = rl_tools;

//#include "../td3/parameters.h"
#include "../training.h"
#include "../constants.h"

// Include checkpoint file if path is specified at compile time
#ifdef ACTOR_CHECKPOINT_FILE
#include ACTOR_CHECKPOINT_FILE
#endif

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

namespace my_program_state
{
    std::size_t
    request_count()
    {
        static std::size_t count = 0;
        return ++count;
    }

    std::time_t
    now()
    {
        return std::time(0);
    }
}

// Helper function for C++17 compatibility (since ends_with is C++20)
bool ends_with(const std::string& str, const std::string& suffix) {
    if (suffix.length() > str.length()) return false;
    return str.compare(str.length() - suffix.length(), suffix.length(), suffix) == 0;
}

class websocket_session : public std::enable_shared_from_this<websocket_session> {
    beast::websocket::stream<tcp::socket> ws_;

    using ABLATION_SPEC = learning_to_fly::config::DEFAULT_ABLATION_SPEC;
    using POSITION_TO_POSITION_ABLATION_SPEC = learning_to_fly::config::POSITION_TO_POSITION_ABLATION_SPEC;
    
    struct CONFIG: learning_to_fly::config::Config<ABLATION_SPEC>{
        using DEV_SPEC = rlt::devices::cpu::Specification<rlt::devices::math::CPU, rlt::devices::random::CPU, rlt::devices::logging::CPU>;
        using DEVICE = rlt::DEVICE_FACTORY<DEV_SPEC>;
        // Use STEP_LIMIT from config.h to ensure UI matches training configuration
        static constexpr TI STEP_LIMIT = learning_to_fly::config::Config<ABLATION_SPEC>::STEP_LIMIT;
        static constexpr bool DETERMINISTIC_EVALUATION = false;
        static constexpr TI BASE_SEED = 0;
    };
    
    struct POSITION_TO_POSITION_CONFIG: learning_to_fly::config::Config<POSITION_TO_POSITION_ABLATION_SPEC>{
        using DEV_SPEC = rlt::devices::cpu::Specification<rlt::devices::math::CPU, rlt::devices::random::CPU, rlt::devices::logging::CPU>;
        using DEVICE = rlt::DEVICE_FACTORY<DEV_SPEC>;
        // Use STEP_LIMIT from config.h to ensure UI matches training configuration
        static constexpr TI STEP_LIMIT = learning_to_fly::config::Config<POSITION_TO_POSITION_ABLATION_SPEC>::STEP_LIMIT;
        static constexpr bool DETERMINISTIC_EVALUATION = false;
        static constexpr TI BASE_SEED = 0;
    };
    
    using TI = CONFIG::TI;

    learning_to_fly::TrainingState<CONFIG> ts_hover;
    learning_to_fly::TrainingState<POSITION_TO_POSITION_CONFIG> ts_position_to_position;
    boost::asio::steady_timer timer_;
    std::chrono::time_point<std::chrono::high_resolution_clock> training_start, training_end;
    std::thread t;
    std::vector<std::vector<CONFIG::ENVIRONMENT::State>> ongoing_trajectories;
    std::vector<TI> ongoing_drones;
    std::vector<TI> idle_drones;
    TI drone_id_counter = 0;
    using T = CONFIG::T;
    using ENVIRONMENT = typename parameters::environment<CONFIG::T, TI, CONFIG::ABLATION_SPEC>::ENVIRONMENT;
    ENVIRONMENT env;
    rlt::devices::DefaultCPU device;
    rlt::MatrixDynamic<rlt::matrix::Specification<T, TI, 1, ENVIRONMENT::ACTION_DIM>> action;
    
    // Track current training mode
    std::string current_training_mode = "hover";
    
    // Actor evaluation state
    bool is_evaluating = false;
    std::string current_actor_path = "";
    std::thread evaluation_thread;
    std::atomic<bool> stop_evaluation{false};
    
    // Actor evaluation types
    // CRITICAL FIX: Use ACTOR_CHECKPOINT_TYPE (inference-only, no gradients) instead of ACTOR_TYPE (training network with gradients)
    // This allows loading actor_target checkpoints which don't have gradient information
    using ACTOR_EVAL_TYPE = typename CONFIG::ACTOR_CHECKPOINT_TYPE;
    ACTOR_EVAL_TYPE evaluation_actor;  // Main actor (navigation with obstacles)
    ACTOR_EVAL_TYPE hover_actor;       // Hover actor for stability at target
    typename ACTOR_EVAL_TYPE::template DoubleBuffer<1> actor_buffer;
    typename ACTOR_EVAL_TYPE::template DoubleBuffer<1> hover_actor_buffer;
    bool actor_loaded = false;
    bool hover_actor_loaded = false;
    bool use_policy_switching = false;  // Enable/disable policy switching
    T switch_distance_threshold = T(0.3);  // Distance threshold for switching to hover policy

public:
    explicit websocket_session(tcp::socket socket) : ws_(std::move(socket)), timer_(ws_.get_executor()) {
        env.parameters = parameters::environment<T, TI, CONFIG::ABLATION_SPEC>::parameters;
        rlt::malloc(device, action);
        rlt::malloc(device, evaluation_actor);
        rlt::malloc(device, hover_actor);
        rlt::malloc(device, actor_buffer);
        rlt::malloc(device, hover_actor_buffer);
    }

private:
    bool load_actor_from_file(const std::string& actor_path) {
        try {
            if (ends_with(actor_path, ".h5")) {
                // Load HDF5 actor
                auto file = HighFive::File(actor_path, HighFive::File::ReadOnly);
                rlt::load(device, evaluation_actor, file.getGroup("actor"));
                actor_loaded = true;
                std::cout << "Successfully loaded navigation actor from: " << actor_path << std::endl;
                return true;
            } else {
                std::cerr << "Unsupported actor file format: " << actor_path << std::endl;
                return false;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error loading navigation actor from " << actor_path << ": " << e.what() << std::endl;
            return false;
        }
    }

    bool load_hover_actor_from_file(const std::string& hover_actor_path) {
        try {
            if (ends_with(hover_actor_path, ".h5")) {
                // Load HDF5 hover actor
                auto file = HighFive::File(hover_actor_path, HighFive::File::ReadOnly);
                rlt::load(device, hover_actor, file.getGroup("actor"));
                hover_actor_loaded = true;
                std::cout << "Successfully loaded hover actor from: " << hover_actor_path << std::endl;
                return true;
            } else {
                std::cerr << "Unsupported hover actor file format: " << hover_actor_path << std::endl;
                return false;
            }
        } catch (const std::exception& e) {
            std::cerr << "Error loading hover actor from " << hover_actor_path << ": " << e.what() << std::endl;
            return false;
        }
    }
    
    void enable_policy_switching(const std::string& hover_actor_path, T threshold = T(POSITION_TO_POSITION_CONFIG::POLICY_SWITCH_THRESHOLD)) {
        if (load_hover_actor_from_file(hover_actor_path)) {
            use_policy_switching = true;
            switch_distance_threshold = threshold;
            std::cout << "Policy switching enabled with threshold: " << threshold << "m" << std::endl;
        } else {
            std::cerr << "Failed to enable policy switching - hover actor not loaded" << std::endl;
            use_policy_switching = false;
        }
    }
    
    void disable_policy_switching() {
        use_policy_switching = false;
        std::cout << "Policy switching disabled" << std::endl;
    }

    void start_actor_evaluation(const std::string& actor_path) {
        if (is_evaluating) {
            stop_evaluation = true;
            if (evaluation_thread.joinable()) {
                evaluation_thread.join();
            }
        }

        current_actor_path = actor_path;
        
        if (!load_actor_from_file(actor_path)) {
            return;
        }

        stop_evaluation = false;
        is_evaluating = true;
        
        evaluation_thread = std::thread([this]() {
            evaluate_actor_loop();
        });
    }

    void stop_actor_evaluation() {
        if (is_evaluating) {
            stop_evaluation = true;
            if (evaluation_thread.joinable()) {
                evaluation_thread.join();
            }
            is_evaluating = false;
            
            // Send stop message to frontend
            nlohmann::json stop_message;
            stop_message["channel"] = "evaluationStopped";
            stop_message["data"] = {};
            
            ws_.write(net::buffer(stop_message.dump()));
        }
    }

    void evaluate_actor_loop() {
        const TI max_episode_steps = 600;
        TI episode_count = 0;
        auto rng = rlt::random::default_engine(typename CONFIG::DEVICE::SPEC::RANDOM(), 10);
        
        while (!stop_evaluation) {
            // Create environment state
            typename ENVIRONMENT::State state;
            rlt::initial_state(device, env, state);
            
            // Create drone for visualization
            TI drone_id = drone_id_counter++;
            using UI = rlt::rl::environments::multirotor::UI<decltype(env)>;
            UI ui;
            ui.id = std::to_string(drone_id);
            ui.origin[0] = 0;
            ui.origin[1] = 0;
            ui.origin[2] = 0;
            
            // Send addDrone message
            ws_.write(net::buffer(rlt::rl::environments::multirotor::model_message(device, env, ui).dump()));
            
            // Run episode
            for (TI step = 0; step < max_episode_steps && !stop_evaluation; step++) {
                // Get observation
                rlt::MatrixDynamic<rlt::matrix::Specification<T, TI, 1, ENVIRONMENT::OBSERVATION_DIM>> observation;
                rlt::malloc(device, observation);
                rlt::observe(device, env, state, observation, rng);
                
                // ============================================================================
                // POLICY SWITCHING: Distance-Based Actor Selection
                // ============================================================================
                if (use_policy_switching && hover_actor_loaded) {
                    // Calculate distance to target
                    T target_pos[3];
                    learning_to_fly::constants::get_target_position(target_pos);
                    
                    T distance_sq = T(0);
                    for (TI i = 0; i < 3; i++) {
                        T diff = state.position[i] - target_pos[i];
                        distance_sq += diff * diff;
                    }
                    T distance = rlt::math::sqrt(device.math, distance_sq);
                    
                    if (distance < switch_distance_threshold) {
                        // CLOSE TO TARGET: Use hover actor
                        // Modify observation to be relative to target (hover actor expects origin at setpoint)
                        rlt::MatrixDynamic<rlt::matrix::Specification<T, TI, 1, ENVIRONMENT::OBSERVATION_DIM>> hover_observation;
                        rlt::malloc(device, hover_observation);
                        
                        // Copy observation and shift position to be relative to target
                        for (TI i = 0; i < ENVIRONMENT::OBSERVATION_DIM; i++) {
                            rlt::set(hover_observation, 0, i, rlt::get(observation, 0, i));
                        }
                        
                        // Modify position components (first 3 elements of observation)
                        // to be relative to target instead of origin
                        for (TI i = 0; i < 3; i++) {
                            T pos_value = rlt::get(hover_observation, 0, i);
                            // Convert from absolute position to position relative to target
                            T relative_pos = pos_value - target_pos[i];
                            rlt::set(hover_observation, 0, i, relative_pos);
                        }
                        
                        // Use hover actor
                        rlt::evaluate(device, hover_actor, hover_observation, action, hover_actor_buffer);
                        
                        rlt::free(device, hover_observation);
                    } else {
                        // FAR FROM TARGET: Use navigation actor
                        rlt::evaluate(device, evaluation_actor, observation, action, actor_buffer);
                    }
                } else {
                    // NO POLICY SWITCHING: Use main evaluation actor only
                    rlt::evaluate(device, evaluation_actor, observation, action, actor_buffer);
                }
                
                // Step environment
                typename ENVIRONMENT::State next_state;
                rlt::step(device, env, state, action, next_state, rng);
                state = next_state;
                
                // Send state to frontend
                ws_.write(net::buffer(rlt::rl::environments::multirotor::state_message(device, ui, state).dump()));
                
                rlt::free(device, observation);
                
                // Sleep for visualization
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
            
            // Remove drone
            using UI = rlt::rl::environments::multirotor::UI<decltype(env)>;
            UI remove_ui;
            remove_ui.id = ui.id;
            ws_.write(net::buffer(rlt::rl::environments::multirotor::remove_drone_message(device, remove_ui).dump()));
            
            episode_count++;
            
            // Wait a bit before next episode
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
        }
        
        is_evaluating = false;
    }

public:

    template<class Body>
    void run(http::request<Body>&& req) {
        ws_.async_accept(
                req,
                beast::bind_front_handler(
                        &websocket_session::on_accept,
                        shared_from_this()
                )
        );
    }

    void on_accept(beast::error_code ec){
        if(ec) return;
        do_read();
//        rlt::set_all(device, action, 0);
//        using UI = rlt::rl::environments::multirotor::UI<ENVIRONMENT>;
//        UI ui;
//        ws_.write(
//            net::buffer(rlt::rl::environments::multirotor::model_message(device, env, ui).dump())
//        );
//        ws_.write(
//            net::buffer(rlt::rl::environments::multirotor::state_message(device, ui, state, action).dump())
//        );
    }

    void do_read() {
        ws_.async_read(
                buffer_,
                beast::bind_front_handler(
                        &websocket_session::on_read,
                        shared_from_this()
                )
        );
    }

    void start(int duration) {
        timer_.expires_after(boost::asio::chrono::milliseconds(duration));
        timer_.async_wait([this, duration](const boost::system::error_code& ec) {
            if (!ec) {
                refresh();
                start(duration);
            }
        });
    }

    void refresh(){
        TI new_trajectories = 0;
        TI current_step = 0;
        bool current_finished = false;
        
        // Handle trajectory collection and status based on current mode
        if (current_training_mode == "position-to-position") {
            {
                std::lock_guard<std::mutex> lock(ts_position_to_position.trajectories_mutex);
                while(!ts_position_to_position.trajectories.empty() && ongoing_trajectories.size() <= 100){
                    ongoing_trajectories.push_back(ts_position_to_position.trajectories.front());
                    ts_position_to_position.trajectories.pop();
                    new_trajectories++;
                }
                while(!ts_position_to_position.trajectories.empty()){
                    ts_position_to_position.trajectories.pop();
                }
            }
            current_step = ts_position_to_position.step;
            current_finished = ts_position_to_position.finished;
        } else {
            {
                std::lock_guard<std::mutex> lock(ts_hover.trajectories_mutex);
                while(!ts_hover.trajectories.empty() && ongoing_trajectories.size() <= 100){
                    ongoing_trajectories.push_back(ts_hover.trajectories.front());
                    ts_hover.trajectories.pop();
                    new_trajectories++;
                }
                while(!ts_hover.trajectories.empty()){
                    ts_hover.trajectories.pop();
                }
            }
            current_step = ts_hover.step;
            current_finished = ts_hover.finished;
        }
        while(new_trajectories > 0){
            std::sort(idle_drones.begin(), idle_drones.end(), std::greater<TI>());
            if(idle_drones.empty()){
                TI drone_id = drone_id_counter++;
                ongoing_drones.push_back(drone_id);
                using UI = rlt::rl::environments::multirotor::UI<decltype(env)>;
                UI ui;
                ui.id = std::to_string(drone_id);
                constexpr TI width = 5;
                constexpr TI height = 3;
                TI drone_sub_id = drone_id % (width * height);
                constexpr T scale = 0.3;
                ui.origin[0] = 0; // All drones at same origin
                ui.origin[1] = 0; // All drones at same origin
                ui.origin[2] = 0;
//                std::cout << "Adding drone at " << ui << std::endl;
                ws_.write(net::buffer(rlt::rl::environments::multirotor::model_message(device, env, ui).dump()));
            }
            else{
                TI drone_id = idle_drones.back();
                ongoing_drones.push_back(drone_id);
                idle_drones.pop_back();
                using UI = rlt::rl::environments::multirotor::UI<decltype(env)>;
                UI ui;
                ui.id = std::to_string(drone_id);
                constexpr TI width = 5;
                constexpr TI height = 3;
                TI drone_sub_id = drone_id % (width * height);
                constexpr T scale = 0.3;
                ui.origin[0] = 0; // All drones at same origin
                ui.origin[1] = 0; // All drones at same origin
                ui.origin[2] = 0;
//                std::cout << "Adding drone at " << ui << std::endl;
                ws_.write(net::buffer(rlt::rl::environments::multirotor::model_message(device, env, ui).dump()));
            }
            new_trajectories--;
        }
        for(TI trajectory_i=0; trajectory_i < ongoing_trajectories.size(); trajectory_i++){
            TI drone_id = ongoing_drones[trajectory_i];
            CONFIG::ENVIRONMENT::State state = ongoing_trajectories[trajectory_i].front();
            ongoing_trajectories[trajectory_i].erase(ongoing_trajectories[trajectory_i].begin());
            using UI = rlt::rl::environments::multirotor::UI<CONFIG::ENVIRONMENT>;
            UI ui;
            ui.id = std::to_string(drone_id);
            ws_.write(net::buffer(rlt::rl::environments::multirotor::state_message(device, ui, state).dump()));
            if(ongoing_trajectories[trajectory_i].empty()){
                idle_drones.push_back(drone_id);
                ongoing_trajectories.erase(ongoing_trajectories.begin() + trajectory_i);
                ongoing_drones.erase(ongoing_drones.begin() + trajectory_i);
                trajectory_i--;
                if(ongoing_trajectories.empty()){
                    break;
                }
            }
        }
        for(TI idle_i=0; idle_i < idle_drones.size(); idle_i ++){
            using UI = rlt::rl::environments::multirotor::UI<CONFIG::ENVIRONMENT>;
            UI ui;
            TI drone_id = idle_drones[idle_i];
            ui.id = std::to_string(drone_id);
            ws_.write(net::buffer(rlt::rl::environments::multirotor::remove_drone_message(device, ui).dump()));
        }
        {
            nlohmann::json message;
            message["channel"] = "status";
            // Use correct STEP_LIMIT based on training mode
            T step_limit = (current_training_mode == "position-to-position") ? 
                           POSITION_TO_POSITION_CONFIG::STEP_LIMIT : CONFIG::STEP_LIMIT;
            message["data"]["progress"] = ((T)current_step)/step_limit;
            message["data"]["time"] = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - training_start).count()/1000.0;
            message["data"]["finished"] = current_finished;
            ws_.write(net::buffer(message.dump()));
        }
        if(current_finished){
            // terminate connection here
            ws_.async_close(beast::websocket::close_code::normal,
                            beast::bind_front_handler(
                                    &websocket_session::on_close,
                                    shared_from_this()));
        }

    }

    void on_close(beast::error_code ec) {
        if(ec) {
            std::cerr << "WebSocket close failed: " << ec.message() << std::endl;
            return;
        }
        ws_.next_layer().shutdown(tcp::socket::shutdown_both, ec);
        ws_.next_layer().close(ec);
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if(ec) return;

        auto message_string = beast::buffers_to_string(buffer_.data());
        buffer_.consume(buffer_.size());
        auto message = nlohmann::json::parse(message_string);

        if(message["channel"] == "startTraining"){
            std::cout << "startTraining message received" << std::endl;
            start(10);

            // start thread in lambda for training of ts (passed as a reference)
            typename CONFIG::TI seed = message["data"]["seed"];
            
            // Check if mode is specified in the message
            current_training_mode = "hover"; // default
            if (message["data"].contains("mode")) {
                current_training_mode = message["data"]["mode"];
            }
            
            this->t = std::thread([seed, this](){
                this->training_start = std::chrono::high_resolution_clock::now();
                
                if (this->current_training_mode == "position-to-position") {
                    std::cout << "Starting Position-to-Position Training" << std::endl;
                    std::cout << "Using position-to-position reward function with target at: (" 
                              << learning_to_fly::constants::TARGET_POSITION_X<T> << ", "
                              << learning_to_fly::constants::TARGET_POSITION_Y<T> << ", "
                              << learning_to_fly::constants::TARGET_POSITION_Z<T> << ")" << std::endl;
                    learning_to_fly::init(this->ts_position_to_position, seed);
                    
                    // Load checkpoint if available
                    constexpr bool has_checkpoint_path = (std::string_view(POSITION_TO_POSITION_CONFIG::ACTOR_CHECKPOINT_INIT_PATH).length() > 0);
                    if constexpr (has_checkpoint_path) {
                        #ifdef ACTOR_CHECKPOINT_FILE
                        std::cout << "Loading actor weights from checkpoint: " << POSITION_TO_POSITION_CONFIG::ACTOR_CHECKPOINT_INIT_PATH << std::endl;
                        // Copy weights from the included checkpoint to the training actor
                        rlt::copy(this->ts_position_to_position.device, this->ts_position_to_position.device, rl_tools::checkpoint::actor::model, this->ts_position_to_position.actor_critic.actor);
                        
                        // CRITICAL FIX: Reset optimizer state when loading pre-trained weights
                        // The optimizer (Adam) has momentum and adaptive learning rates that were
                        // tuned for the previous task. We MUST reset these for the new task
                        // to avoid catastrophic forgetting and unstable learning.
                        std::cout << "Resetting optimizer states for transfer learning..." << std::endl;
                        rlt::reset_optimizer_state(this->ts_position_to_position.device, this->ts_position_to_position.actor_critic.actor_optimizer, this->ts_position_to_position.actor_critic.actor);
                        rlt::reset_optimizer_state(this->ts_position_to_position.device, this->ts_position_to_position.actor_critic.critic_optimizers[0], this->ts_position_to_position.actor_critic.critic_1);
                        rlt::reset_optimizer_state(this->ts_position_to_position.device, this->ts_position_to_position.actor_critic.critic_optimizers[1], this->ts_position_to_position.actor_critic.critic_2);
                        
                        // Copy actor to actor_target after loading weights (important for TD3 stability)
                        rlt::copy(this->ts_position_to_position.device, this->ts_position_to_position.device, this->ts_position_to_position.actor_critic.actor, this->ts_position_to_position.actor_critic.actor_target);
                        
                        std::cout << "Actor weights loaded and optimizer states reset successfully." << std::endl;
                        #else
                        std::cerr << "Warning: Checkpoint path specified (" << POSITION_TO_POSITION_CONFIG::ACTOR_CHECKPOINT_INIT_PATH 
                                  << ") but no checkpoint file included at compile time. Using random initialization." << std::endl;
                        #endif
                    }
                    
                    for(TI step_i=0; step_i < POSITION_TO_POSITION_CONFIG::STEP_LIMIT; step_i++){
                        learning_to_fly::step(this->ts_position_to_position);
                    }
                } else {
                    std::cout << "Starting Hover Training" << std::endl;
                    std::cout << "Using hover reward function (position around origin)" << std::endl;
                    learning_to_fly::init(this->ts_hover, seed);
                    
                    // Load checkpoint if available
                    constexpr bool has_checkpoint_path = (std::string_view(CONFIG::ACTOR_CHECKPOINT_INIT_PATH).length() > 0);
                    if constexpr (has_checkpoint_path) {
                        #ifdef ACTOR_CHECKPOINT_FILE
                        std::cout << "Loading actor weights from checkpoint: " << CONFIG::ACTOR_CHECKPOINT_INIT_PATH << std::endl;
                        // Copy weights from the included checkpoint to the training actor
                        rlt::copy(this->ts_hover.device, this->ts_hover.device, rl_tools::checkpoint::actor::model, this->ts_hover.actor_critic.actor);
                        
                        // CRITICAL FIX: Reset optimizer state when loading pre-trained weights
                        // The optimizer (Adam) has momentum and adaptive learning rates that were
                        // tuned for the previous task. We MUST reset these for the new task
                        // to avoid catastrophic forgetting and unstable learning.
                        std::cout << "Resetting optimizer states for transfer learning..." << std::endl;
                        rlt::reset_optimizer_state(this->ts_hover.device, this->ts_hover.actor_critic.actor_optimizer, this->ts_hover.actor_critic.actor);
                        rlt::reset_optimizer_state(this->ts_hover.device, this->ts_hover.actor_critic.critic_optimizers[0], this->ts_hover.actor_critic.critic_1);
                        rlt::reset_optimizer_state(this->ts_hover.device, this->ts_hover.actor_critic.critic_optimizers[1], this->ts_hover.actor_critic.critic_2);
                        
                        // Copy actor to actor_target after loading weights (important for TD3 stability)
                        rlt::copy(this->ts_hover.device, this->ts_hover.device, this->ts_hover.actor_critic.actor, this->ts_hover.actor_critic.actor_target);
                        
                        std::cout << "Actor weights loaded and optimizer states reset successfully." << std::endl;
                        #else
                        std::cerr << "Warning: Checkpoint path specified (" << CONFIG::ACTOR_CHECKPOINT_INIT_PATH 
                                  << ") but no checkpoint file included at compile time. Using random initialization." << std::endl;
                        #endif
                    }
                    
                    for(TI step_i=0; step_i < CONFIG::STEP_LIMIT; step_i++){
                        learning_to_fly::step(this->ts_hover);
                    }
                }
                
                training_end = std::chrono::high_resolution_clock::now();
                std::cout << "Training took " << (std::chrono::duration_cast<std::chrono::milliseconds>(training_end - training_start).count())/1000 << "s" << std::endl;

            });
            t.detach();
        }
        else if(message["channel"] == "evaluateActor"){
            std::cout << "evaluateActor message received" << std::endl;
            
            std::string action = message["data"]["action"];
            
            if (action == "restart") {
                std::string actor_path = message["data"]["actorPath"];
                std::cout << "Starting actor evaluation with: " << actor_path << std::endl;
                start_actor_evaluation(actor_path);
                start(10); // Start refresh timer
            }
            else if (action == "stop") {
                std::cout << "Stopping actor evaluation" << std::endl;
                stop_actor_evaluation();
            }
            else if (action == "enablePolicySwitching") {
                std::string hover_actor_path = message["data"]["hoverActorPath"];
                T threshold = message["data"].contains("threshold") ? 
                    T(message["data"]["threshold"].get<double>()) : 
                    T(POSITION_TO_POSITION_CONFIG::POLICY_SWITCH_THRESHOLD); // Use same threshold as training
                std::cout << "Enabling policy switching with hover actor: " << hover_actor_path 
                          << " (threshold: " << threshold << "m)" << std::endl;
                enable_policy_switching(hover_actor_path, threshold);
            }
            else if (action == "disablePolicySwitching") {
                std::cout << "Disabling policy switching" << std::endl;
                disable_policy_switching();
            }
        }

        // read message to string:
        bool send_message = false;
        if(send_message){
            std::string message = beast::buffers_to_string(buffer_.data());
            std::cout << message << std::endl;
            message = "Hello from server responding " + message;
            ws_.text(ws_.got_text());
            ws_.async_write(
                    net::buffer(message),
                    beast::bind_front_handler(
                            &websocket_session::on_write,
                            shared_from_this()
                    )
            );
        }
        else{
            do_read();
        }
    }

    void on_write(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);
        if(ec) return;


        do_read();
    }

private:
    beast::flat_buffer buffer_;
};


class http_connection: public std::enable_shared_from_this<http_connection>
{
public:
    http_connection(tcp::socket socket): socket_(std::move(socket)){}
    void start(){
        read_request();
        check_deadline();
    }

private:
    tcp::socket socket_;
    beast::flat_buffer buffer_{8192};
    http::request<http::dynamic_body> request_;
    http::response<http::dynamic_body> response_;
    net::steady_timer deadline_{socket_.get_executor(), std::chrono::seconds(60)};
    void read_request(){
        auto self = shared_from_this();

        http::async_read(
                socket_,
                buffer_,
                request_,
                [self](beast::error_code ec,
                       std::size_t bytes_transferred)
                {
                    boost::ignore_unused(bytes_transferred);
                    if(!ec)
                        self->process_request();
                });
    }

    void process_request(){
        response_.version(request_.version());
        response_.keep_alive(false);

        switch(request_.method())
        {
            case http::verb::get:
                response_.result(http::status::ok);
                response_.set(http::field::server, "Beast");
                create_response();
                break;

            default:
                // We return responses indicating an error if
                // we do not recognize the request method.
                response_.result(http::status::bad_request);
                response_.set(http::field::content_type, "text/plain");
                beast::ostream(response_.body())
                        << "Invalid request-method '"
                        << std::string(request_.method_string())
                        << "'";
                break;
        }
        write_response();
    }

    void create_response(){
        if(request_.target() == "/count"){
            response_.set(http::field::content_type, "text/html");
            beast::ostream(response_.body())
                    << "<html>\n"
                    <<  "<head><title>Request count</title></head>\n"
                    <<  "<body>\n"
                    <<  "<h1>Request count</h1>\n"
                    <<  "<p>There have been "
                    <<  my_program_state::request_count()
                    <<  " requests so far.</p>\n"
                    <<  "</body>\n"
                    <<  "</html>\n";
        }
        else if(request_.target() == "/time"){
            response_.set(http::field::content_type, "text/html");
            beast::ostream(response_.body())
                    <<  "<html>\n"
                    <<  "<head><title>Current time</title></head>\n"
                    <<  "<body>\n"
                    <<  "<h1>Current time</h1>\n"
                    <<  "<p>The current time is "
                    <<  my_program_state::now()
                    <<  " seconds since the epoch.</p>\n"
                    <<  "</body>\n"
                    <<  "</html>\n";
        }
        else if(request_.target() == "/config"){
            response_.set(http::field::content_type, "application/json");
            beast::ostream(response_.body())
                    << "{\n"
                    << "  \"targetPosition\": {\n"
                    << "    \"x\": " << learning_to_fly::constants::TARGET_POSITION_X<float> << ",\n"
                    << "    \"y\": " << learning_to_fly::constants::TARGET_POSITION_Y<float> << ",\n"
                    << "    \"z\": " << learning_to_fly::constants::TARGET_POSITION_Z<float> << "\n"
                    << "  },\n"
                    << "  \"obstacles\": [\n";
            
            // Add all cylindrical obstacles
            for(size_t i = 0; i < learning_to_fly::constants::NUM_OBSTACLES; i++) {
                const auto& obs = learning_to_fly::constants::OBSTACLES[i];
                beast::ostream(response_.body())
                        << "    {\n"
                        << "      \"type\": \"cylinder\",\n"
                        << "      \"x\": " << obs.x << ",\n"
                        << "      \"y\": " << obs.y << ",\n"
                        << "      \"radius\": " << obs.radius << ",\n"
                        << "      \"zMin\": " << obs.z_min << ",\n"
                        << "      \"zMax\": " << obs.z_max << "\n"
                        << "    }";
                if(i < learning_to_fly::constants::NUM_OBSTACLES - 1 || 
                   learning_to_fly::constants::NUM_PLANAR_OBSTACLES > 0) {
                    beast::ostream(response_.body()) << ",";
                }
                beast::ostream(response_.body()) << "\n";
            }
            
            // Add all planar obstacles
            for(size_t i = 0; i < learning_to_fly::constants::NUM_PLANAR_OBSTACLES; i++) {
                const auto& plane = learning_to_fly::constants::PLANAR_OBSTACLES[i];
                beast::ostream(response_.body())
                        << "    {\n"
                        << "      \"type\": \"plane\",\n"
                        << "      \"pointX\": " << plane.point_x << ",\n"
                        << "      \"pointY\": " << plane.point_y << ",\n"
                        << "      \"pointZ\": " << plane.point_z << ",\n"
                        << "      \"normalX\": " << plane.normal_x << ",\n"
                        << "      \"normalY\": " << plane.normal_y << ",\n"
                        << "      \"normalZ\": " << plane.normal_z << ",\n"
                        << "      \"thickness\": " << plane.thickness << ",\n"
                        << "      \"xMin\": " << plane.x_min << ",\n"
                        << "      \"xMax\": " << plane.x_max << ",\n"
                        << "      \"yMin\": " << plane.y_min << ",\n"
                        << "      \"yMax\": " << plane.y_max << ",\n"
                        << "      \"zMin\": " << plane.z_min << ",\n"
                        << "      \"zMax\": " << plane.z_max << "\n"
                        << "    }";
                if(i < learning_to_fly::constants::NUM_PLANAR_OBSTACLES - 1) {
                    beast::ostream(response_.body()) << ",";
                }
                beast::ostream(response_.body()) << "\n";
            }
            
            beast::ostream(response_.body())
                    << "  ]\n"
                    << "}\n";
        }
        else if(request_.target() == "/actors"){
            response_.result(http::status::ok);
            response_.set(http::field::content_type, "application/json");
            
            nlohmann::json actors_array = nlohmann::json::array();
            
            // Scan ./actors directory (keep these on top)
            std::filesystem::path actors_dir = "./actors";
            if(std::filesystem::exists(actors_dir) && std::filesystem::is_directory(actors_dir)){
                for(const auto& entry : std::filesystem::directory_iterator(actors_dir)){
                    if(entry.is_regular_file()){
                        std::string filename = entry.path().filename().string();
                        if(ends_with(filename, ".h5")){
                            nlohmann::json actor_obj;
                            actor_obj["name"] = "actors/" + filename;
                            actor_obj["path"] = entry.path().string();
                            actors_array.push_back(actor_obj);
                        }
                    }
                }
            }
            
            // Scan ./checkpoints/multirotor_td3 directory and sort by creation time
            std::filesystem::path checkpoints_dir = "./checkpoints/multirotor_td3";
            std::vector<std::pair<std::filesystem::file_time_type, nlohmann::json>> checkpoint_actors;
            
            if(std::filesystem::exists(checkpoints_dir) && std::filesystem::is_directory(checkpoints_dir)){
                for(const auto& exp_entry : std::filesystem::directory_iterator(checkpoints_dir)){
                    if(exp_entry.is_directory()){
                        std::string exp_name = exp_entry.path().filename().string();
                        for(const auto& file_entry : std::filesystem::directory_iterator(exp_entry)){
                            if(file_entry.is_regular_file()){
                                std::string filename = file_entry.path().filename().string();
                                if(ends_with(filename, ".h5")){
                                    nlohmann::json actor_obj;
                                    actor_obj["name"] = "checkpoints/" + exp_name + "/" + filename;
                                    actor_obj["path"] = file_entry.path().string();
                                    
                                    // Get file creation time
                                    auto file_time = std::filesystem::last_write_time(file_entry);
                                    checkpoint_actors.push_back({file_time, actor_obj});
                                }
                            }
                        }
                    }
                }
                
                // Sort checkpoint actors by creation time (newest first)
                std::sort(checkpoint_actors.begin(), checkpoint_actors.end(),
                    [](const auto& a, const auto& b) {
                        return a.first > b.first; // newer files first
                    });
                
                // Add sorted checkpoint actors to the array
                for(const auto& actor_pair : checkpoint_actors){
                    actors_array.push_back(actor_pair.second);
                }
            }
            
            std::string json_response = actors_array.dump();
            beast::ostream(response_.body()) << json_response;
        }
        else if(request_.target() == "/ws"){
            maybe_upgrade();
        }
        else{
            std::filesystem::path path(std::string(request_.target()));
            if(path.empty() || path == "/"){
                path = "/index.html";
            }
            path = "src/ui/static" + path.string();
            // check if file at path exists

            if(std::filesystem::exists(path)){
                response_.result(http::status::ok);
                // check extension and use correct content_type
                if(path.extension() == ".html")
                    response_.set(http::field::content_type, "text/html");
                else if(path.extension() == ".js")
                    response_.set(http::field::content_type, "application/javascript");
                else if(path.extension() == ".css")
                    response_.set(http::field::content_type, "text/css");
                else if(path.extension() == ".png")
                    response_.set(http::field::content_type, "image/png");
                else if(path.extension() == ".jpg")
                    response_.set(http::field::content_type, "image/jpeg");
                else if(path.extension() == ".gif")
                    response_.set(http::field::content_type, "image/gif");
                else if(path.extension() == ".ico")
                    response_.set(http::field::content_type, "image/x-icon");
                else if(path.extension() == ".txt")
                    response_.set(http::field::content_type, "text/plain");
                else
                    response_.set(http::field::content_type, "application/octet-stream");
                beast::ostream(response_.body()) << std::ifstream(path).rdbuf();
            }
            else{
                response_.result(http::status::not_found);
                response_.set(http::field::content_type, "text/plain");
                beast::ostream(response_.body()) << "File not found\r\n";
                std::cout << "File not found: " << path << " (you might need to run \"get_dependencies.sh\" to download the UI dependencies into the static folder)" << std::endl;
            }

//            response_.result(http::status::not_found);
//            response_.set(http::field::content_type, "text/plain");
//            beast::ostream(response_.body()) << "File not found\r\n";
        }
    }
    void maybe_upgrade() {
        if (beast::websocket::is_upgrade(request_)) {
            // Construct the WebSocket session and run it
            std::make_shared<websocket_session>(std::move(socket_))->run(std::move(request_));
            return;
        }
    }


    void write_response(){
        auto self = shared_from_this();

        response_.content_length(response_.body().size());

        http::async_write(
                socket_,
                response_,
                [self](beast::error_code ec, std::size_t)
                {
                    self->socket_.shutdown(tcp::socket::shutdown_send, ec);
                    self->deadline_.cancel();
                });
    }

    void check_deadline(){
        auto self = shared_from_this();

        deadline_.async_wait(
                [self](beast::error_code ec){
                    if(!ec){
                        self->socket_.close(ec);
                    }
                });
    }
};

void http_server(tcp::acceptor& acceptor, tcp::socket& socket){
    acceptor.async_accept(socket,
                          [&](beast::error_code ec)
                          {
                              if(!ec)
                                  std::make_shared<http_connection>(std::move(socket))->start();
                              http_server(acceptor, socket);
                          });
}

int main(int argc, char* argv[]) {
    std::cout << "Note: This executable should be executed in the context (working directory) of the main repo e.g. ./build/src/rl_environments_multirotor_ui 0.0.0.0 8000" << std::endl;
    try{
        // Check command line arguments.
        if(argc != 3){
            std::cerr << "Usage: " << argv[0] << " <address> <port> (e.g. \'0.0.0.0 8000\' for localhost 8000)\n";
            return EXIT_FAILURE;
        }

        auto const address = net::ip::make_address(argv[1]);
        unsigned short port = static_cast<unsigned short>(std::atoi(argv[2]));

        net::io_context ioc{1};

        tcp::acceptor acceptor{ioc, {address, port}};
        tcp::socket socket{ioc};
        http_server(acceptor, socket);

        std::cout << "Web interface coming up at: http://" << address << ":" << port << std::endl;

        ioc.run();
    }
    catch(std::exception const& e){
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }
}