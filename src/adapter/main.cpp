/**
 * @file main.cpp (Adapter)
 * @brief S2S Geospatial Adapter entry point
 */

#include "GeminiIntegration.hpp"
#include "IPCManager.hpp"
#include "IPCReader.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    std::cout << "======================================" << std::endl;
    std::cout << "S2S Geospatial Adapter - Client" << std::endl;
    std::cout << "======================================" << std::endl;
    
    // Connect to shared memory
    auto& shm_mgr = s2sgeo::SharedMemoryManager::getInstance();
    if (!shm_mgr.connectClient()) {
        std::cerr << "Failed to connect to shared memory" << std::endl;
        std::cerr << "Make sure location daemon is running" << std::endl;
        return 1;
    }
    
    // Check if location service is alive
    std::cout << "Waiting for location service..." << std::endl;
    for (int i = 0; i < 30; ++i) {
        if (s2sgeo::IPCReader::isLocationServiceAlive()) {
            std::cout << "Location service is alive!" << std::endl;
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    if (!s2sgeo::IPCReader::isLocationServiceAlive()) {
        std::cerr << "Location service did not start" << std::endl;
        return 1;
    }
    
    // Start Gemini integration
    // NOTE: In production, use actual Gemini API key
    std::string api_key = "YOUR_GEMINI_API_KEY_HERE";
    
    auto gemini = std::make_unique<s2sgeo::GeminiIntegration>();
    if (!gemini->start(api_key)) {
        std::cerr << "Failed to start Gemini integration" << std::endl;
        return 1;
    }
    
    std::cout << "S2S Geospatial Adapter running..." << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    
    // Main loop: monitor shared memory and display status
    int iteration = 0;
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(2));
        
        s2sgeo::WorldState state;
        s2sgeo::ContextFrame context;
        
        if (s2sgeo::IPCReader::readLatestState(state, context)) {
            std::cout << "[Adapter " << (++iteration) << "] "
                      << "Lat: " << state.smoothed_lat << " "
                      << "Lon: " << state.smoothed_lon << " "
                      << "Moving: " << (state.is_moving ? "Yes" : "No")
                      << " Road: " << context.road_name << std::endl;
        }
    }
    
    return 0;
}
