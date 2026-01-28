/**
 * @file IPCReader.cpp
 * @brief IPC reader implementation
 */

#include "IPCReader.hpp"
#include "IPCManager.hpp"
#include <cstring>

namespace s2sgeo {

bool IPCReader::readLatestState(WorldState& state, ContextFrame& context) {
    auto& mgr = SharedMemoryManager::getInstance();
    if (!mgr.isReady()) return false;
    
    auto* header = mgr.getHeader();
    auto* buffer = mgr.getRingBuffer();
    
    if (!header || !buffer) return false;
    
    // Read from the last written entry
    uint32_t write_idx = header->write_index.load(std::memory_order_acquire);
    uint32_t read_idx = (write_idx - 1) % SharedMemoryHeader::RING_BUFFER_SIZE;
    
    // Ensure we don't read from the write pointer
    if (write_idx == 0) {
        read_idx = SharedMemoryHeader::RING_BUFFER_SIZE - 1;
    }
    
    const RingBufferEntry& entry = buffer[read_idx];
    state = entry.state;
    context = entry.context;
    
    return true;
}

bool IPCReader::isLocationServiceAlive() {
    auto& mgr = SharedMemoryManager::getInstance();
    if (!mgr.isReady()) return false;
    
    auto* header = mgr.getHeader();
    if (!header) return false;
    
    return header->location_service_alive.load(std::memory_order_acquire);
}

std::string IPCReader::getActivePlugin() {
    auto& mgr = SharedMemoryManager::getInstance();
    if (!mgr.isReady()) return "";
    
    auto* header = mgr.getHeader();
    if (!header) return "";
    
    return std::string(header->active_plugin);
}

double IPCReader::getAccuracyLevel() {
    auto& mgr = SharedMemoryManager::getInstance();
    if (!mgr.isReady()) return 1.0;
    
    auto* header = mgr.getHeader();
    if (!header) return 1.0;
    
    return header->accuracy_level.load(std::memory_order_acquire);
}

} // namespace s2sgeo
