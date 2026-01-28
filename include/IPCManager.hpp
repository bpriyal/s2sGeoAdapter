/**
 * @file IPCManager.hpp
 * @brief Shared memory and IPC management
 */

#ifndef S2SGEO_IPC_MANAGER_HPP
#define S2SGEO_IPC_MANAGER_HPP

#include "SharedMemoryStructs.hpp"
#include <string>
#include <boost/interprocess/managed_shared_memory.hpp>

using boost::interprocess::managed_shared_memory;

namespace s2sgeo {

/**
 * @class SharedMemoryManager
 * @brief Manages the shared memory ring buffer
 */
class SharedMemoryManager {
public:
    static constexpr const char* SHARED_MEMORY_NAME = "s2sgeo_shm";
    static constexpr size_t SHARED_MEMORY_SIZE = 1024 * 1024;  // 1 MB
    
    static SharedMemoryManager& getInstance();
    
    /**
     * @brief Initialize shared memory (server side)
     */
    bool initializeServer();
    
    /**
     * @brief Connect to existing shared memory (client side)
     */
    bool connectClient();
    
    /**
     * @brief Clean up shared memory
     */
    void cleanup();
    
    /**
     * @brief Get access to shared memory header
     */
    SharedMemoryHeader* getHeader();
    
    /**
     * @brief Get access to ring buffer
     */
    RingBufferEntry* getRingBuffer();
    
    /**
     * @brief Check if initialization was successful
     */
    bool isReady() const { return is_ready_; }
    
private:
    SharedMemoryManager() = default;
    
    std::unique_ptr<managed_shared_memory> segment_;
    SharedMemoryHeader* header_ = nullptr;
    RingBufferEntry* ring_buffer_ = nullptr;
    bool is_ready_ = false;
};

} // namespace s2sgeo

#endif // S2SGEO_IPC_MANAGER_HPP
