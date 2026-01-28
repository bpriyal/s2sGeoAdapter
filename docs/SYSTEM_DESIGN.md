# S2S Geospatial Adapter - System Design Document

## 1. Requirements (Hello Interview Format)

### Functional Requirements

| Requirement | Details |
|-------------|---------|
| **Real-Time Location Tracking** | Accept GPS/IMU data at 10 Hz, smooth with Kalman filter |
| **Context-Aware Responses** | Inject environmental context (road, traffic, elevation) into Gemini |
| **Modular Plugins** | Support Cycling, Dating, Delivery, etc. without code changes |
| **S2S Integration** | Bidirectional audio streaming with Gemini Live API |
| **Persistence** | Location service runs independently of AI session |
| **Boundary Detection** | Detect when user crosses S2 cell boundaries in < 100ms |

### Non-Functional Requirements

| Requirement | Target |
|-------------|--------|
| **Latency** | Voice-to-voice response < 500ms |
| **Consistency** | "Last Known Location" is always authoritative |
| **Scalability** | Support 10M+ concurrent users (DAU) |
| **Battery Efficiency** | Adaptive polling (high when moving, low when idle) |
| **Availability** | Location tracking persists even if S2S session crashes |
| **Accuracy** | GPS ±10m, PDR steps ±5cm |

---

## 2. Core Entities

```
User
  - userId: string
  - activePlugin: "cycling" | "dating" | "delivery"
  - accuracyLevel: 0.0 - 1.0

LocationFix
  - lat, lon, altitude: double
  - accuracy: double (meters)
  - speed, heading: double
  - accel_x, accel_y, accel_z: double
  - gyro_x, gyro_y, gyro_z: double
  - timestamp: int64_ms

WorldState
  - smoothed_lat, smoothed_lon: double (Kalman filtered)
  - s2_cell_id: uint64
  - context_json: string
  - is_moving: bool
  - step_count: uint32
  - estimated_distance_m: double
  - last_update_ms: int64

ContextFrame
  - road_name, road_type: string
  - traffic_level: string ("light" | "moderate" | "heavy")
  - elevation_gain_m, gradient_percent: double
  - hazards: json array
  - timestamp_ms: int64
```

---

## 3. API & System Interface

### Location Ingestion Endpoint

```
Daemon → Shared Memory (Lock-Free Ring Buffer)
Method: Atomic write (< 1 μs latency)

Format:
  RingBufferEntry {
    sequence: atomic<uint32>
    state: WorldState
    context: ContextFrame
  }
```

### AI Session Stream

```
Adapter ↔ Gemini Live
Protocol: WebSocket (WSS)
Content: PCM audio (16kHz, 16-bit mono) + context updates

Message Format (JSON):
{
  "client_content": {
    "turns": [
      {
        "parts": [
          {"inline_data": {"mime_type": "audio/pcm", "data": "...base64..."}},
          {"text": "context_update": {...}}
        ]
      }
    ]
  }
}
```

### Plugin Interface

```cpp
class IContextProvider {
  virtual ContextFrame getContext(double lat, double lon) = 0;
  virtual void prefetchContext(double lat, double lon, 
                               double heading, double distance) = 0;
};
```

---

## 4. Architecture & Components

### Layer 1: Location Daemon

**Responsibility**: Persistent background location tracking

**Flow**:
1. Poll GPS → LocationFix
2. Apply Kalman filter → Smoothed position
3. Detect S2 cell change → Trigger context query
4. Query active plugin → ContextFrame
5. Write to shared memory (atomic)

**Threading**:
- Main thread: Event loop (100ms)
- Optional: Background context fetcher (async)

**Error Handling**:
- GPS timeout → Use last known position
- Plugin error → Use cached context
- IPC write failure → Log and retry

### Layer 2: Shared Memory IPC

**Data Structure**: Lock-Free Single-Producer Multi-Consumer (SPMC) Ring Buffer

```cpp
struct SharedMemoryHeader {
  atomic<uint32_t> write_index;      // Producer writes here
  atomic<uint32_t> read_index;       // Consumers read from here
  atomic<uint32_t> global_sequence;  // Version counter
  atomic<bool> location_service_alive;
  char active_plugin[64];
  double accuracy_level;
};

RingBufferEntry buffer[1024];  // Slots for entries
```

**Write Path** (Daemon):
```cpp
uint32_t idx = write_index.load(acquire);
buffer[idx] = {state, context};
write_index.store((idx+1) % SIZE, release);
```

**Read Path** (Adapter):
```cpp
uint32_t idx = write_index.load(acquire) - 1;
entry = buffer[idx];  // Latest entry
```

**Latency**: < 1 microsecond (atomic operations, no locks)

### Layer 3: S2S Adapter

**Responsibility**: Manage Gemini Live session with context injection

**Flow**:
1. Connect to Gemini Live WebSocket
2. Monitor shared memory for location updates
3. Detect context changes (hash comparison)
4. Inject updated system prompt
5. Stream audio to/from Gemini

**Threading**:
- Main thread: Gemini connection + audio I/O
- Context monitor thread: Poll shared memory (100ms intervals)
- Prefetch thread: Async context queries

---

## 5. Deep Dives

### Deep Dive 1: Kalman Filter for GPS Smoothing

**Problem**: GPS noise (±10m), jitter causes poor user experience

**Solution**: 2D Constant Velocity Kalman Filter

**State Vector**: [lat, lon, lat_vel, lon_vel]

**Process Model**:
```
x[k+1] = A * x[k]
where A = [1  0  dt  0 ]
         [0  1  0   dt]
         [0  0  1   0 ]
         [0  0  0   1 ]
```

**Measurement**: GPS provides [lat, lon]

**Noise Adaptation**:
- If GPS accuracy is low (> 20m), increase R (trust GPS less)
- If movement is high (speed > 5 m/s), increase Q (model less)

**Step Detection** (PDR fusion):
- Detect peaks in Z-axis acceleration
- Estimate step length: 0.5-0.8m
- Refine position between GPS fixes

**Result**: 
- Smoothed position within ±5m
- Responsive to actual movement
- Fills gaps during GPS signal loss

### Deep Dive 2: S2 Cell Boundary Detection

**Problem**: Need to efficiently detect when user crosses from one region to another

**Solution**: Google S2 Geometry Hierarchical Cells

**S2 Cell Hierarchy**:
- Level 24: 60cm (ultra-fine, step-level)
- Level 16: 600m (cycling routes)
- Level 10: 5km (neighborhood)

**Algorithm**:
```cpp
uint64_t current_cell = s2.latLonToCell(lat, lon, LEVEL_16);
if (current_cell != last_cell) {
  // Boundary crossed → fetch new context
  context = provider->getContext(lat, lon);
  IPCWriter::writeState(state, context);
  last_cell = current_cell;
}
```

**Boundary Detection Latency**: < 0.1ms

**Optimization**:
- Prefetch data for all 4 neighbors (frustum caching)
- Skip context query if within same cell

### Deep Dive 3: Plugin Architecture & Modularity

**Design Pattern**: Strategy Pattern + Factory Pattern

**Cycling Plugin**:
- Queries: Google Maps (roads, traffic), Google Elevation (grade)
- Caches: Road surface, traffic for 1km radius
- Updates: When crossing S2 cell boundary or elevation changes > 10m

**Dating Plugin**:
- Queries: Dating API (nearby profiles), Venue database
- Caches: User profiles for 5-minute window
- Updates: When user moves > 50m

**Example Usage**:
```cpp
// Daemon startup
PluginRegistry& registry = PluginRegistry::getInstance();
registry.registerProvider("cycling", []() {
  return std::make_unique<CyclingContextProvider>();
});

// Command processing
CommandDispatcher::processCommand("Start cycling");
// → Activates CyclingContextProvider
// → Sets accuracy_level = 1.0 (high precision)
```

**Extensibility**:
- Add new plugin: Implement `IContextProvider`
- Register in daemon: `registry.registerProvider(name, factory)`
- Activate via command: `CommandDispatcher::processCommand(name)`
- No daemon restart required

### Deep Dive 4: Context Injection into Gemini

**Challenge**: Gemini context window is finite; must inject only relevant data

**Solution**: Differential Updates + System Message Injection

**Context Frame Hash**:
```cpp
uint64_t hash = 0;
for (char c : road_type)
  hash = hash * 31 + c;
hash = hash * 31 + (int)(gradient * 10);
```

**Injection on Change**:
```
If hash != last_hash:
  → Send system message update to Gemini
  → Include: location, road, gradient, traffic, hazards
  → Gemini can now answer context-aware questions
```

**Example System Prompt**:
```
You are a cycling guide. User is at 37.7749, -122.4194.
Road: Market Street (asphalt).
Gradient: 5.5%, Elevation: 45m gain.
Traffic: Heavy. 
Last 234 steps covered 156.8m.
```

**Latency Impact**:
- Context query: 50-200ms (API dependent)
- Injection: < 10ms (JSON serialization + send)
- Total: 50-210ms (acceptable for voice responses)

---

## 6. Data Flow Scenarios

### Scenario 1: Cyclist Asks "Is this going uphill?"

```
1. [T=0ms] User speaks question via AirPods
2. [T=50ms] Audio captured → PCM stream to Gemini
3. [T=100ms] Gemini receives audio, processes intent
4. [T=100ms] Adapter checks shared memory for latest context
   - Reads WorldState: elevation = 50m, gradient = 5.5%
   - Gradient increased from last reading
5. [T=150ms] Adapter recognizes context change
   - Sends system message update: "Gradient: 5.5%"
6. [T=200ms] Gemini generates response: 
   - "Yes, you're on a 5.5% climb. Stay steady!"
7. [T=300ms] Audio response queued to AirPods
```

**Total Latency**: 250ms (< 500ms target) ✓

### Scenario 2: Location Service Detects Cell Boundary

```
1. [T=0ms] User moves from S2 Cell A to Cell B
2. [T=10ms] Daemon detects cell change
3. [T=20ms] Daemon queries CyclingContextProvider
   - Calls Google Maps API: elevation, traffic
4. [T=150ms] API responds with: road="Valencia St", gradient=2%
5. [T=160ms] Daemon writes to shared memory
6. [T=170ms] Adapter reads new context (polling)
7. [T=180ms] Adapter detects context hash changed
8. [T=190ms] Adapter sends system message to Gemini
9. [T=200ms] Gemini has updated context for next user query
```

**Cell Boundary → Gemini Awareness**: 200ms ✓

### Scenario 3: S2S Session Crashes (Persistence)

```
1. [T=0ms] S2S session (Gemini) disconnects
2. [T=0ms] Daemon continues running, polling GPS
3. [T=100ms] Daemon updates shared memory with fresh location
4. [T=150ms] User restarts S2S Adapter
5. [T=160ms] Adapter connects to shared memory
6. [T=170ms] Adapter reads latest WorldState
   - Last 150ms of location data preserved!
7. [T=200ms] New Gemini session starts with current context
```

**Data Loss**: None (daemon persists across adapter crashes) ✓

---

## 7. Consistency & Reliability

### Strong Consistency Model

**Principle**: "Last Known Location" is the single source of truth

**Implementation**:
```cpp
// Daemon (Producer)
WorldState state = kalman.getSmoothedState();
IPCWriter::writeState(state, context);  // Atomic append

// Adapter (Consumer)
WorldState latest = IPCReader::readLatestState();  // Always fresh
```

**Guarantees**:
- No stale reads (read_index = write_index - 1)
- No data loss (ring buffer never overwrites unread data)
- Atomic updates (sequence counter detects partial reads)

### Fault Tolerance

| Failure Mode | Handling |
|--------------|----------|
| GPS signal loss | Use last known position, enable PDR |
| Plugin API timeout | Use cached context, degrade accuracy |
| Shared memory full | Overwrite oldest entry (sliding window) |
| Daemon crash | Adapter detects via `location_service_alive` flag |
| Adapter crash | Daemon continues, session can restart anytime |
| Network latency (Gemini) | Buffer audio locally, stream asynchronously |

---

## 8. Performance Characteristics

### Throughput

| Operation | Rate |
|-----------|------|
| GPS updates | 10 Hz (100ms intervals) |
| S2 cell checks | 100 Hz (1ms) |
| Context queries | ~2-5 Hz (cell boundary dependent) |
| Gemini context injections | ~1 Hz (significant changes) |

### Latency

| Component | Latency |
|-----------|---------|
| GPS poll | < 100ms |
| Kalman filter | < 1ms |
| S2 cell lookup | < 0.1ms |
| Plugin context fetch | 50-200ms (API dependent) |
| IPC write (atomic) | < 1 μs |
| IPC read (atomic) | < 1 μs |
| Gemini injection | < 10ms |
| Gemini inference | 200-400ms |
| **Total (GPS → Voice Response)** | **300-600ms** |

### Memory Usage

| Component | Size |
|-----------|------|
| Shared memory segment | 1 MB |
| Ring buffer (1024 entries) | ~2 MB |
| Kalman filter state | 64 bytes |
| S2 index cache | ~1 MB (configurable) |
| Context cache | ~100 KB (1 context) |
| **Total per-daemon** | **~5 MB** |

---

## 9. Security & Privacy

### Data Protection

- **Location data**: Stored locally in shared memory (no network transmission except to Gemini)
- **API keys**: Loaded from environment variables, not hardcoded
- **Context data**: Ephemeral (overwritten when user moves)

### Permissions Required

- **GPS**: Location access (iOS/Android)
- **Microphone**: Audio capture (AirPods)
- **File system**: Shared memory (process-level isolation)

---

## 10. Testing Strategy

### Unit Tests

```
TestKalmanFilter: Single update, multiple updates, noise reduction, reset
TestS2Geometry: latLonToCell, consistency, boundary detection, distance
TestIPC: Server init, client connect, write-read, alive signal, metadata
```

### Integration Tests

- Daemon ↔ Adapter communication
- Plugin activation ↔ Context injection
- Location change ↔ Gemini context update

### Load Tests

- 100 concurrent locations/second
- Ring buffer wraparound (overflow)
- Plugin response latency under load

---

## 11. Deployment & Operations

### Daemon Configuration

```json
{
  "gps_poll_interval_ms": 100,
  "kalman_process_noise": 0.1,
  "kalman_measurement_noise": 100.0,
  "s2_cell_level": 16,
  "context_cache_ttl_ms": 5000,
  "plugins": ["cycling", "dating"]
}
```

### Adapter Configuration

```json
{
  "gemini_api_key": "${GEMINI_API_KEY}",
  "shared_memory_name": "s2sgeo_shm",
  "context_poll_interval_ms": 100,
  "max_context_window": 1000
}
```

### Monitoring

- Location service alive check (periodic)
- Context query latency (p95, p99)
- Ring buffer fill rate
- Plugin cache hit rate
- Gemini API error rate

---

## 12. Future Roadmap

### Phase 1 (Current)
- ✅ Daemon + Adapter architecture
- ✅ Kalman filtering + S2 geometry
- ✅ Plugin system (Cycling, Dating)
- ✅ Gemini Live integration

### Phase 2
- [ ] Mobile SDKs (Swift, Kotlin)
- [ ] Real-time map matching
- [ ] Multi-user support (horizontal scaling)
- [ ] Waze data integration

### Phase 3
- [ ] ML-based step length estimation
- [ ] Traffic prediction (ETA improvement)
- [ ] Crowd-sourced hazard detection
- [ ] Offline context mode

---

## References

- [Hello Interview System Design](https://www.hellointerview.com/learn/system-design/in-a-hurry/delivery)
- [Google S2 Geometry](https://s2geometry.io/)
- [Kalman Filter Tutorial](https://www.bzarg.com/p/how-a-kalman-filter-works-in-pictures/)
- [Gemini Live API](https://ai.google.dev/docs/gemini_live)
