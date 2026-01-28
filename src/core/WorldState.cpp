/**
 * @file WorldState.cpp
 * @brief Maintains the authoritative current location state
 */

#include "WorldState.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <mutex>

using json = nlohmann::json;

namespace s2sgeo {

class WorldStateImpl {
private:
    WorldState current_state_;
    mutable std::shared_mutex state_mutex_;
    uint32_t update_count_ = 0;
    
public:
    WorldStateImpl() {
        current_state_.smoothed_lat = 0.0;
        current_state_.smoothed_lon = 0.0;
        current_state_.smoothed_altitude = 0.0;
        current_state_.s2_cell_id = 0;
        current_state_.s2_cell_level = 0;
        current_state_.last_update_ms = 0;
        current_state_.update_sequence = 0;
        current_state_.is_moving = false;
        current_state_.step_count = 0;
        current_state_.estimated_distance_m = 0.0;
        std::memset(current_state_.context_json, 0, sizeof(current_state_.context_json));
    }
    
    void updatePosition(double lat, double lon, double altitude, int64_t timestamp) {
        std::unique_lock lock(state_mutex_);
        current_state_.smoothed_lat = lat;
        current_state_.smoothed_lon = lon;
        current_state_.smoothed_altitude = altitude;
        current_state_.last_update_ms = timestamp;
        current_state_.update_sequence = ++update_count_;
    }
    
    void updateS2Cell(uint64_t cell_id, int level) {
        std::unique_lock lock(state_mutex_);
        current_state_.s2_cell_id = cell_id;
        current_state_.s2_cell_level = level;
    }
    
    void updateContext(const std::string& context_json) {
        std::unique_lock lock(state_mutex_);
        strncpy(current_state_.context_json, context_json.c_str(), 
                sizeof(current_state_.context_json) - 1);
        current_state_.context_json[sizeof(current_state_.context_json) - 1] = '\0';
    }
    
    void setMoving(bool moving) {
        std::unique_lock lock(state_mutex_);
        current_state_.is_moving = moving;
    }
    
    void updateStepCount(uint32_t steps) {
        std::unique_lock lock(state_mutex_);
        current_state_.step_count = steps;
    }
    
    void updateEstimatedDistance(double distance) {
        std::unique_lock lock(state_mutex_);
        current_state_.estimated_distance_m = distance;
    }
    
    WorldState getState() const {
        std::shared_lock lock(state_mutex_);
        return current_state_;
    }
    
    double getLatitude() const {
        std::shared_lock lock(state_mutex_);
        return current_state_.smoothed_lat;
    }
    
    double getLongitude() const {
        std::shared_lock lock(state_mutex_);
        return current_state_.smoothed_lon;
    }
    
    uint64_t getS2CellId() const {
        std::shared_lock lock(state_mutex_);
        return current_state_.s2_cell_id;
    }
    
    std::string getContextJson() const {
        std::shared_lock lock(state_mutex_);
        return std::string(current_state_.context_json);
    }
    
    void printState() const {
        std::shared_lock lock(state_mutex_);
        std::cout << "=== WorldState ===" << std::endl;
        std::cout << "Lat: " << current_state_.smoothed_lat << std::endl;
        std::cout << "Lon: " << current_state_.smoothed_lon << std::endl;
        std::cout << "Alt: " << current_state_.smoothed_altitude << " m" << std::endl;
        std::cout << "S2 Cell: " << current_state_.s2_cell_id << " (Level " 
                  << current_state_.s2_cell_level << ")" << std::endl;
        std::cout << "Moving: " << (current_state_.is_moving ? "Yes" : "No") << std::endl;
        std::cout << "Steps: " << current_state_.step_count << std::endl;
        std::cout << "Distance: " << current_state_.estimated_distance_m << " m" << std::endl;
        std::cout << "Context: " << current_state_.context_json << std::endl;
    }
};

// Singleton instance
static WorldStateImpl* g_world_state = nullptr;

WorldStateImpl& getWorldStateInstance() {
    if (!g_world_state) {
        g_world_state = new WorldStateImpl();
    }
    return *g_world_state;
}

} // namespace s2sgeo

// C++ API functions
extern "C++" {
    s2sgeo::WorldState s2sgeo_getWorldState() {
        return s2sgeo::getWorldStateInstance().getState();
    }
    
    void s2sgeo_updateWorldPosition(double lat, double lon, double alt, int64_t ts) {
        s2sgeo::getWorldStateInstance().updatePosition(lat, lon, alt, ts);
    }
    
    void s2sgeo_updateWorldS2Cell(uint64_t cell_id, int level) {
        s2sgeo::getWorldStateInstance().updateS2Cell(cell_id, level);
    }
    
    void s2sgeo_updateWorldContext(const std::string& json) {
        s2sgeo::getWorldStateInstance().updateContext(json);
    }
}
