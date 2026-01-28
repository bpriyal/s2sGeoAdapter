# Quick Start Guide - S2S Geospatial Adapter

## 5-Minute Setup

### 1. Clone and Navigate

```bash
cd /path/to/s2sGeoAdapter
```

### 2. Install Dependencies

**macOS:**
```bash
brew install cmake conan boost eigen s2geometry nlohmann-json gtest
```

**Ubuntu/Debian:**
```bash
apt install cmake conan libboost-all-dev libeigen3-dev \
  libgtest-dev s2geometry nlohmann-json
```

### 3. Build

```bash
# Create build directory
mkdir -p build && cd build

# Install dependencies with Conan
conan install .. --build=missing

# Configure and build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j$(nproc)

# Build artifacts
ls bin/
# → s2sgeo_daemon
# → s2sgeo_adapter
```

### 4. Run (Two Terminals)

**Terminal 1: Start Location Daemon**
```bash
./build/bin/s2sgeo_daemon
```

Expected Output:
```
================================
S2S Geospatial Adapter - Daemon
================================
[LocationService] Starting...
[LocationService] Service loop started
Injecting test locations...
[LocationService] Iteration 0 - Lat: 37.7749 Lon: -122.4194
[LocationService] Iteration 10 - Lat: 37.7750 Lon: -122.4194
...
```

**Terminal 2: Start S2S Adapter**
```bash
./build/bin/s2sgeo_adapter
```

Expected Output:
```
======================================
S2S Geospatial Adapter - Client
======================================
[SharedMemoryManager] Client connected successfully
Location service is alive!
S2S Geospatial Adapter running...
[Adapter 1] Lat: 37.7749 Lon: -122.4194 Moving: No Road: Main Street
[Adapter 2] Lat: 37.7750 Lon: -122.4194 Moving: Yes Road: Main Street
...
```

### 5. Run Tests

```bash
cd build
ctest --output-on-failure
```

---

## Example: Using with Gemini API

### Step 1: Get Gemini API Key

1. Visit [Google AI Studio](https://aistudio.google.com/apikey)
2. Create an API key
3. Set environment variable:

```bash
export GEMINI_API_KEY="your_key_here"
```

### Step 2: Modify Adapter Main

Edit `src/adapter/main.cpp`:

```cpp
std::string api_key = std::getenv("GEMINI_API_KEY");
if (api_key.empty()) {
    std::cerr << "Set GEMINI_API_KEY environment variable" << std::endl;
    return 1;
}
```

### Step 3: Rebuild and Run

```bash
cd build
cmake --build .
./bin/s2sgeo_adapter
```

---

## Example: Creating a Custom Plugin

### Define Your Plugin

Create `src/core/DeliveryContextProvider.hpp`:

```cpp
#include "IGeoProvider.hpp"

class DeliveryContextProvider : public s2sgeo::IContextProvider {
public:
    void initialize(const std::string& config) override {
        // Load delivery zone database
    }
    
    s2sgeo::ContextFrame getContext(double lat, double lon) override {
        s2sgeo::ContextFrame ctx;
        
        // Your API calls here
        auto zone = queryDeliveryZone(lat, lon);
        strcpy(ctx.road_name, zone.name.c_str());
        
        auto orders = getNearbyOrders(lat, lon, 1000);
        strcpy(ctx.hazards, orders.toJSON().c_str());
        
        return ctx;
    }
    
    void prefetchContext(double lat, double lon, 
                        double heading, double distance) override {
        // Optional: prefetch for performance
    }
    
    std::string getName() const override { return "delivery"; }
};
```

### Register in Daemon

Edit `src/daemon/main.cpp`:

```cpp
#include "DeliveryContextProvider.hpp"

// Register
registry.registerProvider("delivery", []() {
    return std::make_unique<DeliveryContextProvider>();
});

// Activate
registry.activateProvider("delivery");
```

### Rebuild

```bash
cd build
cmake --build .
./bin/s2sgeo_daemon  # Now supports delivery mode
```

---

## Debugging Tips

### View Shared Memory State

Create a simple debug tool:

```cpp
#include "IPCManager.hpp"
#include "IPCReader.hpp"

int main() {
    auto& mgr = s2sgeo::SharedMemoryManager::getInstance();
    mgr.connectClient();
    
    s2sgeo::WorldState state;
    s2sgeo::ContextFrame ctx;
    
    while (true) {
        if (s2sgeo::IPCReader::readLatestState(state, ctx)) {
            std::cout << "Lat: " << state.smoothed_lat 
                      << " Lon: " << state.smoothed_lon << std::endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}
```

### Check Plugin Activation

```cpp
auto active = s2sgeo::IPCReader::getActivePlugin();
std::cout << "Active: " << active << std::endl;
```

### Monitor Ring Buffer

```cpp
auto* header = mgr.getHeader();
std::cout << "Updates: " << header->total_updates << std::endl;
std::cout << "Accuracy: " << header->accuracy_level << std::endl;
```

---

## Common Issues & Solutions

| Issue | Solution |
|-------|----------|
| Daemon won't start | Check shared memory isn't already in use: `ipcrm -M 0` |
| Adapter can't connect | Ensure daemon is running and has called `initializeServer()` |
| High CPU usage | Reduce polling frequency (edit daemon config) |
| GPS jitter | Increase Kalman filter process noise |
| Context not updating | Check plugin is activated: `CommandDispatcher::processCommand()` |

---

## Next Steps

1. **Review HLD**: See [SYSTEM_DESIGN.md](SYSTEM_DESIGN.md)
2. **Integrate with Real GPS**: Modify `SensorManager::pollGPS()`
3. **Add Your Plugin**: Follow example above
4. **Deploy to Mobile**: Use NDK/Swift to wrap C++ core
5. **Connect to Gemini**: Use real API key and handle WebSocket messages

---

## Questions?

- Check [README.md](../README.md) for full documentation
- Review system design: [SYSTEM_DESIGN.md](SYSTEM_DESIGN.md)
- See code examples in `include/` and `src/`
