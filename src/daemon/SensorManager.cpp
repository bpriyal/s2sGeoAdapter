/**
 * @file SensorManager.cpp
 * @brief Sensor manager implementation
 */

#include "SensorManager.hpp"
#include <chrono>
#include <cmath>

namespace s2sgeo {

LocationFix SensorManager::pollGPS() {
    // Mock GPS implementation
    // In production, would use platform-specific GPS APIs
    LocationFix fix;
    
    auto now = std::chrono::system_clock::now();
    fix.timestamp_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    // Simulate a location (San Francisco area)
    fix.latitude = 37.7749 + (std::sin(fix.timestamp_ms / 1000.0) * 0.0001);
    fix.longitude = -122.4194 + (std::cos(fix.timestamp_ms / 1000.0) * 0.0001);
    fix.altitude = 50.0;
    fix.accuracy = 10.0;
    fix.speed = 5.0;
    fix.heading = 90.0;
    
    return fix;
}

void SensorManager::pollIMU(LocationFix& fix) {
    // Mock IMU implementation
    auto now = std::chrono::system_clock::now();
    int64_t ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    // Simulate walking motion (1Hz oscillation at ~9.81 m/s^2 gravity + step)
    double t = ms / 1000.0;
    fix.accel_x = std::sin(t * 2 * M_PI) * 2.0;
    fix.accel_y = 0.0;
    fix.accel_z = 9.81 + std::sin(t * 2 * M_PI) * 3.0;  // Gravity + step motion
    
    fix.gyro_x = 0.0;
    fix.gyro_y = 0.0;
    fix.gyro_z = std::cos(t * 2 * M_PI) * 0.5;
}

} // namespace s2sgeo
