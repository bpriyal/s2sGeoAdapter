cmake_minimum_required(VERSION 3.20)
project(s2sGeoAdapter VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O3")

# Find required packages
find_package(Boost REQUIRED COMPONENTS system)
find_package(s2geometry REQUIRED)
find_package(nlohmann_json 3.2.0 REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)

# ============================================================================
# CORE LIBRARY (ISOLATED DOMAIN LOGIC)
# ============================================================================
add_library(s2sgeo_core STATIC
    src/core/WorldState.cpp
    src/core/KalmanFilter.cpp
    src/core/StepDetector.cpp
    src/core/LocationDataTypes.cpp
    src/core/S2GeometryWrapper.cpp
)
target_link_libraries(s2sgeo_core PUBLIC
    Boost::system
    s2geometry
    nlohmann_json::nlohmann_json
)
target_include_directories(s2sgeo_core PUBLIC ${CMAKE_SOURCE_DIR}/include)

# ============================================================================
# PLUGIN INTERFACE LIBRARY
# ============================================================================
add_library(s2sgeo_plugins STATIC
    src/core/PluginRegistry.cpp
    src/core/CyclingContextProvider.cpp
    src/core/DatingContextProvider.cpp
)
target_link_libraries(s2sgeo_plugins PUBLIC s2sgeo_core)
target_include_directories(s2sgeo_plugins PUBLIC ${CMAKE_SOURCE_DIR}/include)

# ============================================================================
# IPC LIBRARY (SHARED MEMORY & INTER-PROCESS COMMUNICATION)
# ============================================================================
add_library(s2sgeo_ipc STATIC
    src/core/IPCWriter.cpp
    src/core/IPCReader.cpp
    src/core/SharedMemoryManager.cpp
)
target_link_libraries(s2sgeo_ipc PUBLIC s2sgeo_core Boost::system)
target_include_directories(s2sgeo_ipc PUBLIC ${CMAKE_SOURCE_DIR}/include)

# ============================================================================
# LOCATION DAEMON (PERSISTENT SERVICE)
# ============================================================================
add_executable(s2sgeo_daemon
    src/daemon/LocationService.cpp
    src/daemon/CommandDispatcher.cpp
    src/daemon/SensorManager.cpp
    src/daemon/main.cpp
)
target_link_libraries(s2sgeo_daemon PUBLIC
    s2sgeo_core
    s2sgeo_plugins
    s2sgeo_ipc
    Boost::system
    nlohmann_json::nlohmann_json
)
target_include_directories(s2sgeo_daemon PUBLIC ${CMAKE_SOURCE_DIR}/include)

# ============================================================================
# S2S GEOSPATIAL ADAPTER (CLIENT / AI INTEGRATION)
# ============================================================================
add_executable(s2sgeo_adapter
    src/adapter/S2SClient.cpp
    src/adapter/WebSocketManager.cpp
    src/adapter/GeminiIntegration.cpp
    src/adapter/ContextInjector.cpp
    src/adapter/main.cpp
)
target_link_libraries(s2sgeo_adapter PUBLIC
    s2sgeo_core
    s2sgeo_ipc
    Boost::system
    nlohmann_json::nlohmann_json
)
target_include_directories(s2sgeo_adapter PUBLIC ${CMAKE_SOURCE_DIR}/include)

# ============================================================================
# UNIT TESTS
# ============================================================================
enable_testing()

# Google Test
find_package(GTest REQUIRED)

add_executable(test_kalman
    tests/TestKalmanFilter.cpp
)
target_link_libraries(test_kalman PUBLIC
    s2sgeo_core
    GTest::gtest_main
)
add_test(NAME KalmanFilterTests COMMAND test_kalman)

add_executable(test_s2_geometry
    tests/TestS2Geometry.cpp
)
target_link_libraries(test_s2_geometry PUBLIC
    s2sgeo_core
    GTest::gtest_main
)
add_test(NAME S2GeometryTests COMMAND test_s2_geometry)

add_executable(test_ipc
    tests/TestIPC.cpp
)
target_link_libraries(test_ipc PUBLIC
    s2sgeo_core
    s2sgeo_ipc
    GTest::gtest_main
)
add_test(NAME IPCTests COMMAND test_ipc)

# ============================================================================
# INSTALLATION
# ============================================================================
install(TARGETS s2sgeo_core s2sgeo_plugins s2sgeo_ipc
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
install(DIRECTORY include/ DESTINATION include)
install(TARGETS s2sgeo_daemon s2sgeo_adapter DESTINATION bin)
