/**
 * @file GeminiIntegration.cpp
 * @brief Gemini integration implementation
 */

#include "GeminiIntegration.hpp"
#include "IPCReader.hpp"
#include <iostream>
#include <nlohmann/json.hpp>
#include <chrono>
#include <thread>

using json = nlohmann::json;

namespace s2sgeo {

GeminiIntegration::GeminiIntegration()
    : s2s_client_(std::make_unique<S2SClient>()) {
}

GeminiIntegration::~GeminiIntegration() {
    stop();
}

bool GeminiIntegration::start(const std::string& api_key) {
    std::cout << "[GeminiIntegration] Starting session..." << std::endl;
    
    if (!s2s_client_->connect(api_key)) {
        std::cerr << "[GeminiIntegration] Failed to connect to Gemini" << std::endl;
        return false;
    }
    
    // Start context update thread
    running_ = true;
    context_update_thread_ = std::thread(&GeminiIntegration::contextUpdateLoop, this);
    
    std::cout << "[GeminiIntegration] Session started" << std::endl;
    return true;
}

void GeminiIntegration::stop() {
    running_ = false;
    if (context_update_thread_.joinable()) {
        context_update_thread_.join();
    }
    s2s_client_->disconnect();
    std::cout << "[GeminiIntegration] Session stopped" << std::endl;
}

void GeminiIntegration::contextUpdateLoop() {
    std::cout << "[GeminiIntegration] Context update thread started" << std::endl;
    
    while (running_) {
        try {
            // Read latest state from shared memory
            WorldState state;
            ContextFrame context;
            
            if (IPCReader::readLatestState(state, context)) {
                // Hash context to detect changes
                uint64_t context_hash = hashContext(context);
                
                if (context_hash != last_context_hash_) {
                    last_context_hash_ = context_hash;
                    
                    // Build JSON context
                    json ctx_json = {
                        {"location", {
                            {"latitude", state.smoothed_lat},
                            {"longitude", state.smoothed_lon},
                            {"altitude", state.smoothed_altitude},
                            {"s2_cell", std::to_string(state.s2_cell_id)}
                        }},
                        {"environment", {
                            {"road", context.road_name},
                            {"surface", context.road_type},
                            {"traffic", context.traffic_level},
                            {"gradient", context.gradient_percent},
                            {"elevation_gain", context.elevation_gain_m}
                        }},
                        {"movement", {
                            {"is_moving", state.is_moving},
                            {"steps", state.step_count},
                            {"distance_m", state.estimated_distance_m}
                        }}
                    };
                    
                    // Inject context into Gemini
                    std::string system_prompt = "You are an expert cycling guide. ";
                    system_prompt += "User is at elevation " + std::to_string(state.smoothed_altitude) + "m. ";
                    system_prompt += "Current gradient: " + std::to_string(context.gradient_percent) + "%. ";
                    system_prompt += "Traffic level: " + std::string(context.traffic_level) + ". ";
                    system_prompt += "Road type: " + std::string(context.road_type) + ".";
                    
                    s2s_client_->sendContext(system_prompt);
                    
                    std::cout << "[GeminiIntegration] Context updated: "
                              << state.smoothed_lat << ", " << state.smoothed_lon << std::endl;
                }
            }
            
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            
        } catch (const std::exception& e) {
            std::cerr << "[GeminiIntegration] Error in context loop: " << e.what() << std::endl;
        }
    }
}

uint64_t GeminiIntegration::hashContext(const ContextFrame& ctx) {
    // Simple hash based on key fields
    uint64_t hash = 0;
    
    // Hash from road_type
    for (int i = 0; i < 64 && ctx.road_type[i]; ++i) {
        hash = hash * 31 + ctx.road_type[i];
    }
    
    // Hash gradient
    int gradient_int = static_cast<int>(ctx.gradient_percent * 10);
    hash = hash * 31 + gradient_int;
    
    return hash;
}

} // namespace s2sgeo
