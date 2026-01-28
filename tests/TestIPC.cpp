/**
 * @file TestIPC.cpp
 * @brief Unit tests for IPC layer
 */

#include "IPCManager.hpp"
#include "IPCWriter.hpp"
#include "IPCReader.hpp"
#include "gtest/gtest.h"
#include <thread>
#include <chrono>

using namespace s2sgeo;

class IPCTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clean up from previous test
        SharedMemoryManager::getInstance().cleanup();
    }
    
    void TearDown() override {
        SharedMemoryManager::getInstance().cleanup();
    }
};

TEST_F(IPCTest, ServerInitializationTest) {
    auto& mgr = SharedMemoryManager::getInstance();
    EXPECT_TRUE(mgr.initializeServer());
    EXPECT_TRUE(mgr.isReady());
    EXPECT_NE(mgr.getHeader(), nullptr);
    EXPECT_NE(mgr.getRingBuffer(), nullptr);
}

TEST_F(IPCTest, ClientConnectionTest) {
    auto& mgr = SharedMemoryManager::getInstance();
    mgr.initializeServer();
    
    auto& mgr2 = SharedMemoryManager::getInstance();
    EXPECT_TRUE(mgr2.connectClient());
    EXPECT_TRUE(mgr2.isReady());
}

TEST_F(IPCTest, WriteReadTest) {
    auto& mgr = SharedMemoryManager::getInstance();
    mgr.initializeServer();
    
    // Write state
    WorldState write_state;
    write_state.smoothed_lat = 37.7749;
    write_state.smoothed_lon = -122.4194;
    write_state.smoothed_altitude = 50.0;
    write_state.is_moving = true;
    write_state.step_count = 42;
    
    ContextFrame context;
    strcpy(context.road_name, "Main St");
    
    IPCWriter::writeState(write_state, context);
    
    // Read state
    WorldState read_state;
    ContextFrame read_context;
    
    // Need small delay for write to complete
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Simulate client reader behavior (in real scenario, different process)
    auto* buffer = mgr.getRingBuffer();
    EXPECT_NE(buffer, nullptr);
    
    // Basic ring buffer sanity check
    EXPECT_EQ(buffer[0].state.smoothed_lat, 37.7749);
    EXPECT_EQ(buffer[0].state.smoothed_lon, -122.4194);
}

TEST_F(IPCTest, AliveSignalTest) {
    auto& mgr = SharedMemoryManager::getInstance();
    mgr.initializeServer();
    
    auto* header = mgr.getHeader();
    EXPECT_FALSE(header->location_service_alive.load());
    
    IPCWriter::signalAlive();
    
    EXPECT_TRUE(header->location_service_alive.load());
}

TEST_F(IPCTest, HeaderMetadataTest) {
    auto& mgr = SharedMemoryManager::getInstance();
    mgr.initializeServer();
    
    auto* header = mgr.getHeader();
    header->accuracy_level.store(0.75);
    strcpy(header->active_plugin, "cycling");
    
    EXPECT_DOUBLE_EQ(header->accuracy_level.load(), 0.75);
    EXPECT_STREQ(header->active_plugin, "cycling");
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
