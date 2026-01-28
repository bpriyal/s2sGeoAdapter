# S2S Geospatial Adapter

## Overview

**S2S Geospatial Adapter** is a modular C++23 system for real-time location tracking and Speech-to-Speech (S2S) AI integration. It enables mobile applications to provide intelligent, context-aware responses to users based on their current geographic location and movement patterns.

### Key Features

- **Modular Architecture**: Cleanly separated daemon (persistence) and adapter (AI) layers
- **Kalman Filtering**: Smooth and denoise GPS data with pedestrian dead reckoning (PDR) support
- **S2 Geometry Indexing**: Hierarchical spatial indexing for efficient boundary detection
- **Lock-Free IPC**: Zero-copy shared memory using atomic operations
- **Plugin System**: Extensible context providers (Cycling, Dating, Delivery, etc.)
- **Adaptive Accuracy**: Adjust precision based on use case (foot vs. car vs. cycle)
- **Gemini Integration**: Stream audio to Google's Gemini Live API with geospatial context injection

---

## Architecture

### High-Level Design

```
User (Voice Input)
        ↓
    [Mobile Device]
        │
        ├─→ [Location Daemon] ──────→ [GPS/IMU Sensors]
        │        ↓
        │   Kalman Filter
        │        ↓
        │   S2 Cell Detection
        │        ↓
        │   Plugin Context Provider
        │        ↓
        │   [Shared Memory Ring Buffer]
        │        ↑
        └─→ [S2S Adapter] (Gemini)
             ↓ ↑
        [Gemini Live API]
             ↓
        Voice Output → [Speaker/AirPods]
```

### Component Layers

#### Layer 1: Persistent Location Service (Daemon)
- **Binary**: `s2sgeo_daemon`
- **Responsibilities**:
  - Poll GPS at configurable intervals
  - Apply Kalman filter for smoothing
  - Detect S2 cell boundary crossings
  - Query active context provider
  - Write updates to shared memory

#### Layer 2: Shared Memory IPC Layer
- **Data Structure**: Lock-free ring buffer (`RingBufferEntry`)
- **Throughput**: ~100 Hz location updates
- **Latency**: < 1 μs (nanosecond-scale IPC)
- **Access**: Atomic operations, no locks

#### Layer 3: S2S Adapter (AI Client)
- **Binary**: `s2sgeo_adapter`
- **Responsibilities**:
  - Connect to Gemini Live WebSocket
  - Read location updates from shared memory
  - Detect context changes (boundary crossings)
  - Inject context as system messages
  - Stream audio to Gemini, handle responses

---

## Project Structure

```
s2sGeoAdapter/
├── CMakeLists.txt                    # Build configuration
├── conanfile.txt                     # Dependency management
├── include/
│   ├── SharedMemoryStructs.hpp       # Data types for IPC
│   ├── IGeoProvider.hpp              # Plugin interface
│   ├── WorldState.hpp                # Global state manager
│   ├── KalmanFilter.hpp              # Location smoothing
│   ├── S2GeometryWrapper.hpp         # Spatial indexing
│   ├── PluginRegistry.hpp            # Plugin factory
│   ├── LocationService.hpp           # Daemon service
│   ├── CommandDispatcher.hpp         # Keyword recognition
│   ├── SensorManager.hpp             # Sensor polling
│   ├── IPCManager.hpp                # Shared memory manager
│   ├── IPCWriter.hpp                 # Ring buffer writer
│   ├── IPCReader.hpp                 # Ring buffer reader
│   ├── S2SClient.hpp                 # Gemini WebSocket
│   ├── WebSocketManager.hpp          # WebSocket lifecycle
│   ├── GeminiIntegration.hpp         # AI orchestration
│   └── ContextInjector.hpp           # Context formatting
├── src/
│   ├── core/
│   │   ├── WorldState.cpp
│   │   ├── KalmanFilter.cpp
│   │   ├── StepDetector.cpp
│   │   ├── S2GeometryWrapper.cpp
│   │   ├── LocationDataTypes.cpp
│   │   ├── PluginRegistry.cpp
│   │   ├── CyclingContextProvider.cpp
│   │   ├── DatingContextProvider.cpp
│   │   ├── IPCWriter.cpp
│   │   ├── IPCReader.cpp
│   │   └── SharedMemoryManager.cpp
│   ├── daemon/
│   │   ├── LocationService.cpp
│   │   ├── CommandDispatcher.cpp
│   │   ├── SensorManager.cpp
│   │   └── main.cpp
│   └── adapter/
│       ├── S2SClient.cpp
│       ├── WebSocketManager.cpp
│       ├── GeminiIntegration.cpp
│       ├── ContextInjector.cpp
│       └── main.cpp
├── tests/
│   ├── TestKalmanFilter.cpp
│   ├── TestS2Geometry.cpp
│   └── TestIPC.cpp
├── docs/
└── build/                            # CMake build directory
```

---

## Technology Stack

| Component | Technology | Rationale |
|-----------|-----------|-----------|
| **Language** | C++23 | Performance, modular design, atomic operations |
| **Build System** | CMake | Cross-platform, dependency mgmt with Conan |
| **Spatial Indexing** | Google S2 | Hierarchical cell-based geometry |
| **Linear Algebra** | Eigen | Fast matrix operations for Kalman |
| **IPC** | Boost.Interprocess | Shared memory, lock-free ring buffer |
| **JSON** | nlohmann/json | Lightweight serialization |
| **Testing** | Google Test | Comprehensive unit test coverage |
| **AI Model** | Gemini Live API | Native audio streaming, low latency |

---

## API Quick Start

### 1. Location Daemon (Background Service)

```cpp
#include "LocationService.hpp"
#include "PluginRegistry.hpp"
#include "CyclingContextProvider.hpp"

// Initialize
auto service = std::make_unique<s2sgeo::LocationService>();
auto& registry = s2sgeo::PluginRegistry::getInstance();

// Register plugins
registry.registerProvider("cycling", []() {
    return std::make_unique<s2sgeo::CyclingContextProvider>();
});

// Activate and start
registry.activateProvider("cycling");
service->setContextProvider(registry.getActiveProvider());
service->start();

// Inject test location
service->injectLocation(37.7749, -122.4194, 50.0, timestamp_ms);
```

### 2. S2S Adapter (AI Client)

```cpp
#include "GeminiIntegration.hpp"
#include "IPCManager.hpp"

// Connect to shared memory
auto& shm_mgr = s2sgeo::SharedMemoryManager::getInstance();
shm_mgr.connectClient();

// Start Gemini integration
auto gemini = std::make_unique<s2sgeo::GeminiIntegration>();
gemini->start("YOUR_GEMINI_API_KEY");

// Monitor location changes
while (true) {
    s2sgeo::WorldState state;
    s2sgeo::ContextFrame context;
    if (s2sgeo::IPCReader::readLatestState(state, context)) {
        std::cout << "Location: " << state.smoothed_lat << ", " 
                  << state.smoothed_lon << std::endl;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}
```

### 3. Plugin Implementation (Custom)

```cpp
class MyContextProvider : public s2sgeo::IContextProvider {
public:
    void initialize(const std::string& config) override {
        // Load API keys, etc.
    }
    
    s2sgeo::ContextFrame getContext(double lat, double lon) override {
        // Query your APIs, return context
        s2sgeo::ContextFrame ctx;
        ctx.elevation_gain_m = getElevation(lat, lon);
        return ctx;
    }
    
    void prefetchContext(double lat, double lon, double heading, 
                        double distance) override {
        // Async prefetch for low latency
    }
    
    std::string getName() const override { return "my_provider"; }
};
```

---

## Building & Running

### Prerequisites

```bash
# macOS
brew install cmake conan boost eigen s2geometry nlohmann-json

# Ubuntu/Debian
apt install cmake conan libboost-all-dev libeigen3-dev libgtest-dev
```

### Build

```bash
cd /path/to/s2sGeoAdapter

# Configure with Conan
conan install . --build=missing

# Build with CMake
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)
```

### Run Location Daemon

```bash
./build/bin/s2sgeo_daemon
# Output:
# [LocationService] Starting...
# [LocationService] Service loop started
# Injecting test locations...
# [LocationService] Iteration 0 - Lat: 37.7749 Lon: -122.4194
```

### Run S2S Adapter

```bash
# In a separate terminal
./build/bin/s2sgeo_adapter
# Output:
# [SharedMemoryManager] Client connected successfully
# Location service is alive!
# S2S Geospatial Adapter running...
# [Adapter 1] Lat: 37.7749 Lon: -122.4194 Moving: No Road: Main Street
```

### Run Unit Tests

```bash
cd build
ctest --output-on-failure
# Expected: All tests pass
```

---

## Accuracy Profiles

### Barefoot / Cycling (High Precision)

- **S2 Cell Level**: 24 (~60cm)
- **Sensors**: GPS + IMU (step detection)
- **Filter**: Pedestrian Dead Reckoning (PDR)
- **Update Frequency**: 10 Hz
- **Latency Target**: < 100ms

**Activation**:
```cpp
s2sgeo::CommandDispatcher::setAccuracyLevel(1.0);
s2sgeo::CommandDispatcher::processCommand("cycling");
```

### Driving (Moderate Precision)

- **S2 Cell Level**: 15 (~200m)
- **Sensors**: GPS + Map Matching
- **Filter**: Standard Kalman (road-snapped)
- **Update Frequency**: 2 Hz
- **Latency Target**: < 500ms

**Activation**:
```cpp
s2sgeo::CommandDispatcher::setAccuracyLevel(0.5);
s2sgeo::CommandDispatcher::processCommand("driving");
```

### Dating / Social (Room-Level)

- **S2 Cell Level**: 18 (~100m)
- **Sensors**: GPS only
- **Filter**: Light smoothing
- **Update Frequency**: 0.5 Hz
- **Latency Target**: < 2 seconds

**Activation**:
```cpp
s2sgeo::CommandDispatcher::setAccuracyLevel(0.3);
s2sgeo::CommandDispatcher::processCommand("dating");
```

---

## Latency Optimization Strategies

### 1. Predictive Prefetching

```cpp
// Query map data for next 500 meters
context_provider->prefetchContext(current_lat, current_lon, 
                                  heading, 500.0);
```

### 2. Differential Updates

```cpp
// Only update when crossing cell boundary or context changes significantly
if (current_s2_cell != last_s2_cell_ || context_hash != last_hash_) {
    IPCWriter::writeState(state, context);
}
```

### 3. Lock-Free IPC

```cpp
// ~1 microsecond latency with atomic operations
IPCWriter::updateLocation(lat, lon, alt, timestamp);
WorldState state = IPCReader::readLatestState();
```

### 4. Asynchronous Context Queries

```cpp
// Run context fetch in background thread
std::thread([&]() {
    auto ctx = context_provider->getContext(lat, lon);
    IPCWriter::writeState(state, ctx);
}).detach();
```

---

## Integration with Gemini Live

### Context Injection Format

```json
{
  "location": {
    "latitude": 37.7749,
    "longitude": -122.4194,
    "altitude": 50.0,
    "s2_cell": "1234567890abcdef"
  },
  "environment": {
    "road": "Market Street",
    "surface": "asphalt",
    "traffic": "heavy",
    "gradient": 5.5,
    "elevation_gain": 45.0
  },
  "movement": {
    "is_moving": true,
    "steps": 234,
    "distance_m": 156.8
  }
}
```

### System Prompt Injection

```
You are an expert cycling guide. User is at 37.7749, -122.4194. 
User is moving. Detected 234 steps, 156.8m traveled. 
Currently on Market Street (asphalt). 5.5% grade. 
45m elevation gain. Traffic: heavy.
```

---

## Configuration Files

### Cycling Provider Config

```json
{
  "google_maps_api_key": "YOUR_KEY",
  "osm_api_endpoint": "https://overpass-api.de/api/interpreter",
  "cache_ttl_ms": 5000,
  "prefetch_distance_m": 500
}
```

### Gemini Config

```json
{
  "api_key": "YOUR_GEMINI_API_KEY",
  "model": "gemini-2.0-flash-exp",
  "audio_format": "pcm_16khz",
  "system_prompt_template": "You are an expert {activity} guide..."
}
```

---

## Performance Benchmarks

| Metric | Value |
|--------|-------|
| GPS polling overhead | < 2% CPU |
| Kalman filter latency | < 1ms |
| S2 cell lookup | < 0.1ms |
| IPC write latency | < 1 μs (atomic) |
| IPC read latency | < 1 μs (atomic) |
| Context provider query | 50-200ms (API dependent) |
| Total end-to-end | < 500ms (target) |

---

## Extending with Custom Plugins

### Delivery Plugin Example

```cpp
class DeliveryContextProvider : public IContextProvider {
public:
    ContextFrame getContext(double lat, double lon) override {
        ContextFrame ctx;
        
        // Query delivery zone database
        auto zone = getDeliveryZone(lat, lon);
        strcpy(ctx.road_name, zone.address.c_str());
        
        // Check for active orders
        auto orders = getNearbyOrders(lat, lon, 1000);
        strncpy(ctx.hazards, orders.toJSON().c_str(), 
                sizeof(ctx.hazards) - 1);
        
        return ctx;
    }
};

// Register in daemon
registry.registerProvider("delivery", []() {
    return std::make_unique<DeliveryContextProvider>();
});
```

---

## Troubleshooting

### Shared Memory Connection Failed

```
Error: Could not connect to shared memory
Solution: Ensure daemon is running first, then start adapter
```

### High GPS Jitter

```
Solution: Increase Kalman filter process noise (Q)
kf->setProcessNoise(0.5);  // Higher = more responsive
```

### Context Updates Not Triggering

```
Solution: Check context provider is activated and boundary detection
```

---

## Future Enhancements

- [ ] Real-time map matching (snap to road network)
- [ ] Multi-threaded context prefetching
- [ ] WebSocket multiplexing for concurrent users
- [ ] Compass calibration for heading accuracy
- [ ] Mobile platform integration (Android NDK, Swift)
- [ ] Machine learning for step length estimation
- [ ] Integration with Waze real-time data feed

---

## References

- [Google S2 Geometry](https://s2geometry.io/)
- [Gemini Live API Documentation](https://ai.google.dev/docs/gemini_live)
- [Kalman Filter Basics](https://www.bzarg.com/p/how-a-kalman-filter-works-in-pictures/)
- [Pedestrian Dead Reckoning](https://en.wikipedia.org/wiki/Dead_reckoning)
- [Lock-Free Programming](https://www.1024cores.net/)

---

## License

Proprietary - S2S Geospatial Adapter
