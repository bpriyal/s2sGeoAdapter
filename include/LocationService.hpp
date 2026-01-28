/**
 * @file LocationService.hpp
 * @brief Main location daemon service
 */

#ifndef S2SGEO_LOCATION_SERVICE_HPP
#define S2SGEO_LOCATION_SERVICE_HPP

#include "IGeoProvider.hpp"
#include "KalmanFilter.hpp"
#include "S2GeometryWrapper.hpp"
#include <memory>
#include <thread>
#include <atomic>

namespace s2sgeo {

/**
 * @class LocationService
 * @brief Persistent background service for location tracking
 * 
 * Responsibilities:
 * - Poll GPS at regular intervals
 * - Smooth with Kalman filter
 * - Detect cell boundary crossings
 * - Query context provider for environmental data
 * - Write to shared memory
 */
class LocationService {
public:
    LocationService();
    ~LocationService();
    
    /**
     * @brief Start the location service
     */
    void start();
    
    /**
     * @brief Stop the location service
     */
    void stop();
    
    /**
     * @brief Set the active context provider
     */
    void setContextProvider(IContextProvider* provider);
    
    /**
     * @brief Inject a test location (for development)
     */
    void injectLocation(double lat, double lon, double alt, int64_t timestamp);
    
private:
    std::unique_ptr<KalmanFilter> kalman_filter_;
    std::unique_ptr<S2GeometryIndex> geometry_index_;
    IContextProvider* context_provider_ = nullptr;
    
    std::atomic<bool> running_ = false;
    std::thread service_thread_;
    
    uint64_t last_s2_cell_ = 0;
    
    /**
     * @brief Main service loop
     */
    void runServiceLoop();
    
    /**
     * @brief Poll sensor data (GPS, IMU)
     */
    LocationFix pollSensors();
};

} // namespace s2sgeo

#endif // S2SGEO_LOCATION_SERVICE_HPP
