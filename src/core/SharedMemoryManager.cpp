/**
 * @file SharedMemoryManager.cpp
 * @brief Shared memory management using Boost.Interprocess
 */

#include "IPCManager.hpp"
#include <iostream>
#include <boost/interprocess/creation_tags.hpp>

using namespace boost::interprocess;

namespace s2sgeo {

SharedMemoryManager& SharedMemoryManager::getInstance() {
    static SharedMemoryManager instance;
    return instance;
}

bool SharedMemoryManager::initializeServer() {
    try {
        // Remove any existing segment
        shared_memory_object::remove(SHARED_MEMORY_NAME);
        
        // Create new managed shared memory
        segment_ = std::make_unique<managed_shared_memory>(
            create_only,
            SHARED_MEMORY_NAME,
            SHARED_MEMORY_SIZE
        );
        
        // Allocate header
        header_ = segment_->construct<SharedMemoryHeader>("header")();
        
        // Allocate ring buffer
        ring_buffer_ = segment_->construct<RingBufferEntry[]>
                       ("ring_buffer")[SharedMemoryHeader::RING_BUFFER_SIZE]();
        
        header_->location_service_alive = true;
        is_ready_ = true;
        
        std::cout << "[SharedMemoryManager] Server initialized successfully" << std::endl;
        return true;
        
    } catch (const interprocess_exception& e) {
        std::cerr << "[SharedMemoryManager] Server init failed: " << e.what() << std::endl;
        return false;
    }
}

bool SharedMemoryManager::connectClient() {
    try {
        // Open existing managed shared memory
        segment_ = std::make_unique<managed_shared_memory>(open_only, SHARED_MEMORY_NAME);
        
        // Find the header and ring buffer
        header_ = segment_->find<SharedMemoryHeader>("header").first;
        ring_buffer_ = segment_->find<RingBufferEntry[]>("ring_buffer").first;
        
        if (!header_ || !ring_buffer_) {
            std::cerr << "[SharedMemoryManager] Could not find shared memory objects" << std::endl;
            return false;
        }
        
        is_ready_ = true;
        std::cout << "[SharedMemoryManager] Client connected successfully" << std::endl;
        return true;
        
    } catch (const interprocess_exception& e) {
        std::cerr << "[SharedMemoryManager] Client connect failed: " << e.what() << std::endl;
        return false;
    }
}

void SharedMemoryManager::cleanup() {
    try {
        if (header_) {
            header_->location_service_alive = false;
        }
        segment_.reset();
        shared_memory_object::remove(SHARED_MEMORY_NAME);
        is_ready_ = false;
        std::cout << "[SharedMemoryManager] Cleaned up" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[SharedMemoryManager] Cleanup error: " << e.what() << std::endl;
    }
}

SharedMemoryHeader* SharedMemoryManager::getHeader() {
    return header_;
}

RingBufferEntry* SharedMemoryManager::getRingBuffer() {
    return ring_buffer_;
}

} // namespace s2sgeo
