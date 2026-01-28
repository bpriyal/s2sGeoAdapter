/**
 * @file LocationService.cpp
 * @brief Location daemon implementation
 */

#include "LocationService.hpp"
#include "IPCWriter.hpp"
#include "PluginRegistry.hpp"
#include <iostream>
#include <chrono>
#include <thread>

namespace s2sgeo {

LocationService::LocationService()
    : kalman_filter_(std::make_unique<KalmanFilter>()),
      geometry_index_(std::make_unique<S2GeometryIndex>()) {
    kalman_filter_->enablePDR(true);
}

LocationService::~LocationService() {
    stop();
}

void LocationService::start() {
    if (running_.exchange(true)) return;
    
    std::cout << "[LocationService] Starting..." << std::endl;
    service_thread_ = std::thread(&LocationService::runServiceLoop, this);
}

void LocationService::stop() {
    running_ = false;
    if (service_thread_.joinable()) {
        service_thread_.join();
    }
    std::cout << "[LocationService] Stopped" << std::endl;
}

void LocationService::setContextProvider(IContextProvider* provider) {
    context_provider_ = provider;
    std::cout << "[LocationService] Set context provider: " 
              << (provider ? provider->getName() : "null") << std::endl;
}

void LocationService::injectLocation(double lat, double lon, double alt, int64_t timestamp) {
    LocationFix fix(lat, lon, timestamp);
    fix.altitude = alt;
    kalman_filter_->update(fix);
}

void LocationService::runServiceLoop() {
    std::cout << "[LocationService] Service loop started" << std::endl;
    
    int iteration = 0;
    while (running_) {
        try {
            // 1. Get smoothed state from Kalman filter
            WorldState state = kalman_filter_->getSmoothedState();
            
            // 2. Detect S2 cell
            uint64_t current_s2 = geometry_index_->latLonToCell(
                state.smoothed_lat, state.smoothed_lon, 16
            );
            state.s2_cell_id = current_s2;
            state.s2_cell_level = 16;
            
            // 3. Check if we crossed a boundary
            ContextFrame context{};
            if (current_s2 != last_s2_cell_ && context_provider_) {
                last_s2_cell_ = current_s2;
                context = context_provider_->getContext(
                    state.smoothed_lat, state.smoothed_lon
                );
                std::cout << "[LocationService] Cell boundary crossed: " << std::hex 
                          << current_s2 << std::dec << std::endl;
            }
            
            // 4. Write to shared memory
            IPCWriter::writeState(state, context);
            IPCWriter::signalAlive();
            
            // 5. Log every 10 iterations
            if (iteration % 10 == 0) {
                std::cout << "[LocationService] Iteration " << iteration 
                          << " - Lat: " << state.smoothed_lat 
                          << " Lon: " << state.smoothed_lon << std::endl;
            }
            
            iteration++;
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            
        } catch (const std::exception& e) {
            std::cerr << "[LocationService] Error in loop: " << e.what() << std::endl;
        }
    }
}

LocationFix LocationService::pollSensors() {
    // Mock implementation - would call actual GPS/IMU in production
    LocationFix fix;
    fix.latitude = 37.7749 + (iteration % 100) * 0.0001;
    fix.longitude = -122.4194;
    fix.altitude = 100.0;
    fix.timestamp_ms = std::chrono::system_clock::now().time_since_epoch().count();
    return fix;
}

} // namespace s2sgeo
