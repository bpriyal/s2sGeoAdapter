/**
 * @file IPCWriter.cpp
 * @brief IPC writer implementation
 */

#include "IPCWriter.hpp"
#include "IPCManager.hpp"
#include <atomic>
#include <cstring>

namespace s2sgeo {

void IPCWriter::writeState(const WorldState& state, const ContextFrame& context) {
    auto& mgr = SharedMemoryManager::getInstance();
    if (!mgr.isReady()) return;
    
    auto* header = mgr.getHeader();
    auto* buffer = mgr.getRingBuffer();
    
    if (!header || !buffer) return;
    
    // Get write index and increment atomically
    uint32_t write_idx = header->write_index.load(std::memory_order_relaxed);
    uint32_t next_write = (write_idx + 1) % SharedMemoryHeader::RING_BUFFER_SIZE;
    
    // Prepare entry
    RingBufferEntry& entry = buffer[write_idx];
    entry.sequence.store(header->global_sequence.load(std::memory_order_relaxed),
                        std::memory_order_release);
    entry.state = state;
    entry.context = context;
    
    // Atomic increment of global sequence and write index
    header->global_sequence.fetch_add(1, std::memory_order_release);
    header->write_index.store(next_write, std::memory_order_release);
    header->total_updates.fetch_add(1, std::memory_order_relaxed);
}

void IPCWriter::updateLocation(double lat, double lon, double alt, int64_t timestamp) {
    auto& mgr = SharedMemoryManager::getInstance();
    if (!mgr.isReady()) return;
    
    auto* header = mgr.getHeader();
    auto* buffer = mgr.getRingBuffer();
    
    if (!header || !buffer) return;
    
    uint32_t write_idx = header->write_index.load(std::memory_order_relaxed);
    RingBufferEntry& entry = buffer[write_idx];
    
    entry.state.smoothed_lat = lat;
    entry.state.smoothed_lon = lon;
    entry.state.smoothed_altitude = alt;
    entry.state.last_update_ms = timestamp;
    
    // Increment
    uint32_t next_write = (write_idx + 1) % SharedMemoryHeader::RING_BUFFER_SIZE;
    header->write_index.store(next_write, std::memory_order_release);
    header->global_sequence.fetch_add(1, std::memory_order_release);
}

void IPCWriter::signalAlive() {
    auto& mgr = SharedMemoryManager::getInstance();
    if (!mgr.isReady()) return;
    
    auto* header = mgr.getHeader();
    if (header) {
        header->location_service_alive.store(true, std::memory_order_release);
    }
}

} // namespace s2sgeo
