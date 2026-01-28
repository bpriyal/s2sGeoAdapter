/**
 * @file SensorManager.hpp
 * @brief Sensor polling and fusion
 */

#ifndef S2SGEO_SENSOR_MANAGER_HPP
#define S2SGEO_SENSOR_MANAGER_HPP

#include "SharedMemoryStructs.hpp"

namespace s2sgeo {

/**
 * @class SensorManager
 * @brief Manages GPS and IMU sensor access
 */
class SensorManager {
public:
    /**
     * @brief Poll current GPS location
     */
    static LocationFix pollGPS();
    
    /**
     * @brief Poll IMU data
     */
    static void pollIMU(LocationFix& fix);
};

} // namespace s2sgeo

#endif // S2SGEO_SENSOR_MANAGER_HPP
