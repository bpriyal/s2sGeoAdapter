/**
 * @file IPCReader.hpp
 * @brief Read location updates from shared memory (Adapter side)
 */

#ifndef S2SGEO_IPC_READER_HPP
#define S2SGEO_IPC_READER_HPP

#include "SharedMemoryStructs.hpp"

namespace s2sgeo {

/**
 * @class IPCReader
 * @brief Lock-free reader for the ring buffer
 */
class IPCReader {
public:
    /**
     * @brief Read latest state from ring buffer
     */
    static bool readLatestState(WorldState& state, ContextFrame& context);
    
    /**
     * @brief Check if location service is alive
     */
    static bool isLocationServiceAlive();
    
    /**
     * @brief Get active plugin name
     */
    static std::string getActivePlugin();
    
    /**
     * @brief Get accuracy level
     */
    static double getAccuracyLevel();
};

} // namespace s2sgeo

#endif // S2SGEO_IPC_READER_HPP
