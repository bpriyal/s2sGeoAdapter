/**
 * @file WorldState.hpp
 * @brief Global state manager for current user location
 */

#ifndef S2SGEO_WORLD_STATE_HPP
#define S2SGEO_WORLD_STATE_HPP

#include "SharedMemoryStructs.hpp"
#include <string>
#include <shared_mutex>

namespace s2sgeo {

/**
 * @class WorldStateImpl
 * @brief Thread-safe singleton for location state management
 */
class WorldStateImpl {
public:
    void updatePosition(double lat, double lon, double altitude, int64_t timestamp);
    void updateS2Cell(uint64_t cell_id, int level);
    void updateContext(const std::string& context_json);
    void setMoving(bool moving);
    void updateStepCount(uint32_t steps);
    void updateEstimatedDistance(double distance);
    
    WorldState getState() const;
    double getLatitude() const;
    double getLongitude() const;
    uint64_t getS2CellId() const;
    std::string getContextJson() const;
    
    void printState() const;
};

// Global accessor
WorldStateImpl& getWorldStateInstance();

} // namespace s2sgeo

#endif // S2SGEO_WORLD_STATE_HPP
