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
#include <boost/beast/websocket.hpp>
#include <filesystem>
#include <fstream>
#include <rl_tools/operations/cpu_mux.h>
#include <learning_to_fly/simulator/operations_cpu.h>
#include <learning_to_fly/simulator/ui.h>
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
class websocket_session : public std::enable_shared_from_this<websocket_session> {
    beast::websocket::stream<tcp::socket> ws_;

    using ABLATION_SPEC = learning_to_fly::config::DEFAULT_ABLATION_SPEC;
    using POSITION_TO_POSITION_ABLATION_SPEC = learning_to_fly::config::POSITION_TO_POSITION_ABLATION_SPEC;
    
    struct CONFIG: learning_to_fly::config::Config<ABLATION_SPEC>{
        using DEV_SPEC = rlt::devices::cpu::Specification<rlt::devices::math::CPU, rlt::devices::random::CPU, rlt::devices::logging::CPU>;
        using DEVICE = rlt::DEVICE_FACTORY<DEV_SPEC>;
        static constexpr TI STEP_LIMIT = 300001;
        static constexpr bool DETERMINISTIC_EVALUATION = false;
        static constexpr TI BASE_SEED = 0;
    };
    
    struct POSITION_TO_POSITION_CONFIG: learning_to_fly::config::Config<POSITION_TO_POSITION_ABLATION_SPEC>{
        using DEV_SPEC = rlt::devices::cpu::Specification<rlt::devices::math::CPU, rlt::devices::random::CPU, rlt::devices::logging::CPU>;
        using DEVICE = rlt::DEVICE_FACTORY<DEV_SPEC>;
        static constexpr TI STEP_LIMIT = 300001;
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

public:
    explicit websocket_session(tcp::socket socket) : ws_(std::move(socket)), timer_(ws_.get_executor()) {
        env.parameters = parameters::environment<T, TI, CONFIG::ABLATION_SPEC>::parameters;
        rlt::malloc(device, action);
    }

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
            message["data"]["progress"] = ((T)current_step)/CONFIG::STEP_LIMIT;
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
                        rlt::copy(this->ts_position_to_position.device, this->ts_position_to_position.device, rl_tools::checkpoint::actor::model, this->ts_position_to_position.actor_critic.actor);
                        std::cout << "Actor weights loaded successfully." << std::endl;
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
                        rlt::copy(this->ts_hover.device, this->ts_hover.device, rl_tools::checkpoint::actor::model, this->ts_hover.actor_critic.actor);
                        std::cout << "Actor weights loaded successfully." << std::endl;
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
                    << "  }\n"
                    << "}\n";
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