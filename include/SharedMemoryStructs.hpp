/**
 * @file SharedMemoryStructs.hpp
 * @brief Shared memory data structures for IPC between Location Daemon and S2S Adapter
 * @details Lock-free SPMC (Single Producer, Multi-Consumer) queue design
 */

#ifndef S2SGEO_SHARED_MEMORY_STRUCTS_HPP
#define S2SGEO_SHARED_MEMORY_STRUCTS_HPP

#include <cstdint>
#include <array>
#include <atomic>
#include <chrono>

namespace s2sgeo {

/**
 * @struct LocationFix
 * @brief Raw sensor data from GPS and IMU
 */
struct LocationFix {
    double latitude;
    double longitude;
    double altitude;
    double accuracy;        // meters
    double speed;          // m/s
    double heading;        // degrees 0-360
    int64_t timestamp_ms;  // milliseconds since epoch
    
    // IMU data for step detection
    double accel_x, accel_y, accel_z;
    double gyro_x, gyro_y, gyro_z;
    
    LocationFix() = default;
    LocationFix(double lat, double lon, int64_t ts)
        : latitude(lat), longitude(lon), altitude(0), accuracy(10),
          speed(0), heading(0), timestamp_ms(ts),
          accel_x(0), accel_y(0), accel_z(0),
          gyro_x(0), gyro_y(0), gyro_z(0) {}
};

/**
 * @struct WorldState
 * @brief Authoritative, smoothed location state
 * @note Shared via Boost.Interprocess Ring Buffer
 */
struct WorldState {
    // Smoothed position (after Kalman filtering)
    double smoothed_lat;
    double smoothed_lon;
    double smoothed_altitude;
    
    // S2 Geometry Cell ID (for spatial indexing)
    uint64_t s2_cell_id;
    int s2_cell_level;
    
    // Context data
    char context_json[1024];  // JSON string of current context
    
    // Metadata
    int64_t last_update_ms;
    uint32_t update_sequence;
    bool is_moving;
    
    // PDR (Pedestrian Dead Reckoning) state
    uint32_t step_count;
    double estimated_distance_m;
    
    WorldState() = default;
};

/**
 * @struct ContextFrame
 * @brief Environmental data to inject into AI session
 */
struct ContextFrame {
    // Road info
    char road_name[256];
    char road_type[64];      // "paved", "gravel", "dirt", etc.
    
    // Traffic & Safety
    char traffic_level[32];  // "light", "moderate", "heavy"
    double current_speed;
    double speed_limit;
    
    // Elevation
    double elevation_gain_m;
    double gradient_percent;
    
    // POIs & Hazards
    char hazards[512];       // JSON array of nearby hazards
    
    // Timestamp
    int64_t timestamp_ms;
    
    ContextFrame() = default;
};

/**
 * @struct RingBufferEntry
 * @brief Single entry in the lock-free ring buffer
 */
struct RingBufferEntry {
    std::atomic<uint32_t> sequence;
    WorldState state;
    ContextFrame context;
};

/**
 * @struct SharedMemoryHeader
 * @brief Control structure for the shared memory ring buffer
 */
struct SharedMemoryHeader {
    static constexpr size_t RING_BUFFER_SIZE = 1024;
    
    // Ring buffer pointers
    std::atomic<uint32_t> write_index;
    std::atomic<uint32_t> read_index;
    
    // Global state
    std::atomic<uint32_t> global_sequence;
    std::atomic<bool> location_service_alive;
    
    // Configuration
    char active_plugin[64];  // "cycling", "dating", etc.
    double accuracy_level;   // 1.0 = full, 0.5 = degraded
    
    // Statistics
    uint64_t total_updates;
    uint64_t total_context_updates;
    
    SharedMemoryHeader() 
        : write_index(0), read_index(0), global_sequence(0),
          location_service_alive(false), accuracy_level(1.0),
          total_updates(0), total_context_updates(0) {}
};

} // namespace s2sgeo

#endif // S2SGEO_SHARED_MEMORY_STRUCTS_HPP
