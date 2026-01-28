/**
 * @file main.cpp (Daemon)
 * @brief Location daemon entry point
 */

#include "LocationService.hpp"
#include "CommandDispatcher.hpp"
#include "PluginRegistry.hpp"
#include "CyclingContextProvider.hpp"
#include "DatingContextProvider.hpp"
#include "IPCManager.hpp"
#include <iostream>
#include <thread>
#include <chrono>

int main(int argc, char* argv[]) {
    std::cout << "================================" << std::endl;
    std::cout << "S2S Geospatial Adapter - Daemon" << std::endl;
    std::cout << "================================" << std::endl;
    
    // Initialize shared memory
    auto& shm_mgr = s2sgeo::SharedMemoryManager::getInstance();
    if (!shm_mgr.initializeServer()) {
        std::cerr << "Failed to initialize shared memory" << std::endl;
        return 1;
    }
    
    // Register plugins
    auto& registry = s2sgeo::PluginRegistry::getInstance();
    registry.registerProvider("cycling", []() {
        return std::make_unique<s2sgeo::CyclingContextProvider>();
    });
    registry.registerProvider("dating", []() {
        return std::make_unique<s2sgeo::DatingContextProvider>();
    });
    
    // Start location service
    auto location_service = std::make_unique<s2sgeo::LocationService>();
    
    // Activate cycling by default
    if (registry.activateProvider("cycling")) {
        location_service->setContextProvider(registry.getActiveProvider());
    }
    
    location_service->start();
    
    // Simulate some location updates
    std::cout << "Injecting test locations..." << std::endl;
    for (int i = 0; i < 50; ++i) {
        double lat = 37.7749 + (i * 0.0001);
        double lon = -122.4194;
        int64_t timestamp_ms = std::chrono::system_clock::now().time_since_epoch().count() / 1000000;
        
        location_service->injectLocation(lat, lon, 50.0 + i * 0.5, timestamp_ms);
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    
    std::cout << "Location injections complete. Service running..." << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;
    
    // Keep daemon running
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    
    location_service->stop();
    shm_mgr.cleanup();
    
    return 0;
}
