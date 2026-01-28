/**
 * @file IPCWriter.hpp
 * @brief Write location updates to shared memory (Daemon side)
 */

#ifndef S2SGEO_IPC_WRITER_HPP
#define S2SGEO_IPC_WRITER_HPP

#include "SharedMemoryStructs.hpp"

namespace s2sgeo {

/**
 * @class IPCWriter
 * @brief Lock-free writer for the ring buffer
 */
class IPCWriter {
public:
    /**
     * @brief Write a new state to the ring buffer
     */
    static void writeState(const WorldState& state, const ContextFrame& context);
    
    /**
     * @brief Update location only (fast path)
     */
    static void updateLocation(double lat, double lon, double alt, int64_t timestamp);
    
    /**
     * @brief Signal that location service is alive
     */
    static void signalAlive();
};

} // namespace s2sgeo

#endif // S2SGEO_IPC_WRITER_HPP
