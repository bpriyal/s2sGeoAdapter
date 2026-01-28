# S2S Geospatial Adapter - Architecture Overview

## System Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────────┐
│                         MOBILE DEVICE / DESKTOP                         │
└─────────────────────────────────────────────────────────────────────────┘

┌──────────────────────────────────────────────────────────────────────────┐
│                    HARDWARE SENSORS (OS Level)                           │
│                                                                           │
│  ┌───────────────┐  ┌───────────────┐  ┌───────────────┐                │
│  │   GPS Module  │  │   IMU (Accel) │  │   Microphone  │                │
│  │               │  │   (Gyro)      │  │   (AirPods)   │                │
│  └───────┬───────┘  └───────┬───────┘  └───────┬───────┘                │
│          │                  │                  │                         │
│          └──────────────────┴──────────────────┘                         │
│                             │                                            │
└─────────────────────────────┼────────────────────────────────────────────┘
                              │
              ┌───────────────┼───────────────┐
              │               │               │
      ┌───────▼─────────────┐ │   ┌──────────▼────────┐
      │  LOCATION DAEMON    │ │   │  S2S ADAPTER      │
      │  (Background Svc)   │ │   │  (AI Session)     │
      │                     │ │   │                   │
      │  ┌─────────────────┐│ │   │ ┌────────────────┐│
      │  │ Sensor Manager  ││ │   │ │ WebSocket Mgr  ││
      │  │ - Poll GPS      ││ │   │ │ - Connect to   ││
      │  │ - Read IMU      ││ │   │ │   Gemini Live  ││
      │  └────────┬────────┘│ │   │ └────────────────┘│
      │           │         │ │   │                   │
      │  ┌────────▼────────┐│ │   │ ┌────────────────┐│
      │  │ Kalman Filter   ││ │   │ │  Context       ││
      │  │ - Smooth GPS    ││ │   │ │  Injector      ││
      │  │ - PDR Fusion    ││ │   │ │ - Format JSON  ││
      │  └────────┬────────┘│ │   │ │ - Send to LLM  ││
      │           │         │ │   │ └────────────────┘│
      │  ┌────────▼────────┐│ │   │                   │
      │  │ S2 Geometry     ││ │   │ ┌────────────────┐│
      │  │ - Cell ID       ││ │   │ │ IPC Reader     ││
      │  │ - Boundary Det  ││ │   │ │ - Read shm     ││
      │  └────────┬────────┘│ │   │ │ - Latest state ││
      │           │         │ │   │ └────────────────┘│
      │  ┌────────▼────────┐│ │   │                   │
      │  │ Plugin Registry ││ │   │                   │
      │  │ + Active Plugin ││ │   │                   │
      │  │ (Cycling,       ││ │   │                   │
      │  │  Dating, etc)   ││ │   │                   │
      │  └────────┬────────┘│ │   │                   │
      │           │         │ │   │                   │
      │  ┌────────▼────────┐│ │   │                   │
      │  │ Context Provider││ │   │                   │
      │  │ - Query Maps    ││ │   │                   │
      │  │ - Query Elevation
      │  │ - Query Traffic ││ │   │                   │
      │  │ - Cache Data    ││ │   │                   │
      │  └────────┬────────┘│ │   │                   │
      │           │         │ │   │                   │
      │  ┌────────▼────────┐│ │   │                   │
      │  │ IPC Writer      ││ │   │                   │
      │  │ - Write to shm  ││ │   │                   │
      │  │ - Atomic ops    ││ │   │                   │
      │  └────────┬────────┘│ │   │                   │
      │           │         │ │   │                   │
      └───────────┼─────────┘ │   └──────────────────┘
                  │           │
                  └─────┬─────┘
                        │
              ┌─────────▼──────────┐
              │  SHARED MEMORY     │
              │  IPC RING BUFFER   │
              │                    │
              │ ┌────────────────┐ │
              │ │ SharedMemory   │ │
              │ │ Header:        │ │
              │ │ - write_index  │ │
              │ │ - read_index   │ │
              │ │ - sequence     │ │
              │ │ - service_alive│ │
              │ │ - accuracy_lvl │ │
              │ │ - active_plugin│ │
              │ └────────────────┘ │
              │                    │
              │ ┌────────────────┐ │
              │ │ Ring Buffer    │ │
              │ │ [1024 entries] │ │
              │ │ - WorldState   │ │
              │ │ - ContextFrame │ │
              │ │ - Sequence ID  │ │
              │ └────────────────┘ │
              │                    │
              └────────────────────┘
                        │
           ┌────────────┴────────────┐
           │                         │
    (Daemon writes)          (Adapter reads)
           │                         │
           └─ LOCK-FREE ATOMIC OPS ─┘
             < 1 microsecond latency
```

---

## Data Flow: User Asks Question

```
┌──────────────────────────────────────────────────────────────────┐
│ USER: "Is this going uphill?"                                    │
│ (Speaks into AirPods)                                             │
└──────────────────────┬───────────────────────────────────────────┘
                       │ [Audio → PCM 16kHz]
                       ▼
            ┌──────────────────────┐
            │  S2S Adapter         │
            │ (Listening to audio) │
            └──────────┬───────────┘
                       │ [Stream to Gemini]
                       ▼
            ┌──────────────────────┐
            │  Gemini Live API     │
            │ (Speech Recognition) │
            └──────────┬───────────┘
                       │ [Decode intent]
                       ▼
            ┌──────────────────────┐
            │  Adapter             │
            │  Check Shared Memory │
            │  Read Latest State   │
            └──────────┬───────────┘
                       │
         ┌─────────────┴─────────────┐
         ▼                           ▼
    ┌─────────────┐         ┌──────────────┐
    │ WorldState  │         │ContextFrame  │
    │             │         │              │
    │ lat: 37.77  │         │ road: Market │
    │ lon: -122.4 │         │ gradient: 5.5│
    │ moving: yes │         │ elevation:45 │
    └──────┬──────┘         └──────────────┘
           │
           └──────────┬──────────┘
                      │
           ┌──────────▼──────────┐
           │  Gemini             │
           │  (Has context)      │
           │  Generates response │
           └──────────┬──────────┘
                      │
    "Yes, you're on a 5.5% climb. Stay steady!"
                      │
                      ▼
           ┌──────────────────────┐
           │  Text-to-Speech      │
           │  Gemini Audio Output │
           └──────────┬───────────┘
                      │
                      ▼
           ┌──────────────────────┐
           │ Adapter              │
           │ (Receive audio)      │
           └──────────┬───────────┘
                      │
                      ▼
           ┌──────────────────────┐
           │ Speaker / AirPods    │
           │ (Play response)      │
           └──────────────────────┘
```

---

## Component Interactions

```
┌─────────────────────────────────────────────────────────────────┐
│                   DAEMON                                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  LocationService                                                 │
│  ├─ run() loop [100ms]                                          │
│  │  ├─ pollSensors() → LocationFix {lat, lon, alt, imu}         │
│  │  │                                                            │
│  │  ├─ kalman.update(LocationFix)                               │
│  │  │  └─ Smooth GPS, detect steps                              │
│  │  │                                                            │
│  │  ├─ geometry.latLonToCell() → S2 cell ID                     │
│  │  │                                                            │
│  │  ├─ if (cell_changed) {                                      │
│  │  │    context = provider.getContext(lat, lon)                │
│  │  │    IPCWriter::writeState(state, context)                  │
│  │  │  }                                                         │
│  │  │                                                            │
│  │  └─ sleep(100ms)                                             │
│  │                                                              │
│  CommandDispatcher (Command Handler)                             │
│  ├─ processCommand("cycling")                                    │
│  │  └─ registry.activateProvider("cycling")                     │
│  │     └─ location_service.setContextProvider(cycling_provider)  │
│  │                                                               │
│  │- setAccuracyLevel(1.0)                                        │
│  │  └─ shm.header->accuracy_level = 1.0                         │
│  │                                                               │
│  PluginRegistry                                                  │
│  ├─ cycling → CyclingContextProvider                             │
│  ├─ dating  → DatingContextProvider                              │
│  └─ active_provider → Currently selected                         │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│              SHARED MEMORY (IPC)                                 │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  Header                                                          │
│  ├─ write_index: atomic<uint32>                                 │
│  ├─ read_index: atomic<uint32>                                  │
│  ├─ global_sequence: atomic<uint32>                             │
│  ├─ location_service_alive: atomic<bool>                        │
│  ├─ active_plugin: "cycling"                                    │
│  └─ accuracy_level: 1.0                                         │
│                                                                  │
│  RingBuffer[1024]                                                │
│  └─ [idx] = {                                                    │
│       sequence: 42,                                              │
│       state: {lat: 37.77, lon: -122.4, ...},                    │
│       context: {road: "Main", gradient: 5.5, ...}               │
│     }                                                            │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘

┌─────────────────────────────────────────────────────────────────┐
│                  ADAPTER                                         │
├─────────────────────────────────────────────────────────────────┤
│                                                                  │
│  GeminiIntegration                                               │
│  ├─ start(api_key)                                              │
│  │  ├─ s2s_client.connect()                                     │
│  │  └─ contextUpdateThread.start()                              │
│  │                                                              │
│  ├─ contextUpdateLoop() [100ms]                                │
│  │  ├─ IPCReader::readLatestState(state, context)              │
│  │  ├─ hash = hashContext(context)                             │
│  │  ├─ if (hash != last_hash) {                                │
│  │  │    s2s_client.sendContext(state, context)                │
│  │  │  }                                                        │
│  │  └─ sleep(100ms)                                            │
│  │                                                              │
│  S2SClient (Gemini WebSocket)                                    │
│  ├─ connect() → WSS://gemini                                    │
│  ├─ sendAudio(pcm_data) → Gemini                                │
│  ├─ sendContext(json) → System message update                   │
│  └─ onAudioResponse() → Speaker/AirPods                         │
│                                                                  │
│  WebSocketManager                                                │
│  ├─ connect(url)                                                │
│  ├─ sendMessage(json)                                           │
│  └─ onMessage(callback)                                         │
│                                                                  │
│  ContextInjector                                                 │
│  └─ formatContextPrompt(ctx) → String                           │
│                                                                  │
└─────────────────────────────────────────────────────────────────┘
```

---

## Latency Timeline

```
T=0ms    ┌─────────────────────────────────────────────┐
         │ User speaks: "Is this going uphill?"        │
         └──────────────┬──────────────────────────────┘
                        │
T=50ms   ┌──────────────▼──────────────────────────────┐
         │ Adapter captures audio (PCM 16kHz)          │
         └──────────────┬──────────────────────────────┘
                        │
T=100ms  ┌──────────────▼──────────────────────────────┐
         │ Adapter reads latest context from shared mem│
         │ - Elevation: 50m, Gradient: 5.5%            │
         └──────────────┬──────────────────────────────┘
                        │
T=150ms  ┌──────────────▼──────────────────────────────┐
         │ Adapter sends context to Gemini             │
         │ System Prompt: "Grade: 5.5%"                │
         └──────────────┬──────────────────────────────┘
                        │
T=200ms  ┌──────────────▼──────────────────────────────┐
         │ Gemini recognizes question intent           │
         │ (via speech recognition)                    │
         └──────────────┬──────────────────────────────┘
                        │
T=300ms  ┌──────────────▼──────────────────────────────┐
         │ Gemini generates response                   │
         │ "Yes, 5.5% climb. Stay steady!"             │
         └──────────────┬──────────────────────────────┘
                        │
T=350ms  ┌──────────────▼──────────────────────────────┐
         │ Gemini converts response to audio           │
         │ (Text-to-Speech)                            │
         └──────────────┬──────────────────────────────┘
                        │
T=400ms  ┌──────────────▼──────────────────────────────┐
         │ Adapter receives audio response             │
         └──────────────┬──────────────────────────────┘
                        │
T=450ms  ┌──────────────▼──────────────────────────────┐
         │ Audio streamed to AirPods speaker           │
         └──────────────┬──────────────────────────────┘
                        │
T=500ms  ┌──────────────▼──────────────────────────────┐
         │ User hears: "Yes, 5.5% climb. Stay steady!" │
         │                                              │
         │ ✓ TOTAL LATENCY: 500ms (Target met!)        │
         └──────────────────────────────────────────────┘
```

---

## Module Dependencies

```
Core Layer
├── SharedMemoryStructs.hpp (data types)
├── IGeoProvider.hpp (interfaces)
├── WorldState.hpp
├── KalmanFilter.hpp
├── S2GeometryWrapper.hpp
├── StepDetector.hpp
├── PluginRegistry.hpp
├── IPCManager.hpp
├── IPCWriter.hpp
├── IPCReader.hpp
├── CyclingContextProvider.hpp
└── DatingContextProvider.hpp

Daemon Layer
├── LocationService.hpp (depends on: KalmanFilter, S2Geometry, PluginRegistry, IPCWriter)
├── CommandDispatcher.hpp (depends on: PluginRegistry, IPCManager)
└── SensorManager.hpp (no dependencies)

Adapter Layer
├── S2SClient.hpp (WebSocket to Gemini)
├── WebSocketManager.hpp (WebSocket lifecycle)
├── GeminiIntegration.hpp (orchestration, depends on: S2SClient, IPCReader)
└── ContextInjector.hpp (formatting, depends on: SharedMemoryStructs)
```

---

## Thread Safety Model

```
Daemon (Single-threaded + async context fetcher)
├─ Main Thread
│  ├─ Poll sensors (100Hz)
│  ├─ Run Kalman filter (1ms latency)
│  ├─ Detect S2 boundaries (0.1ms latency)
│  └─ Write to shared memory (1μs atomic)
│
└─ Optional Background Thread
   ├─ Async context queries (50-200ms)
   └─ Doesn't block main loop

Shared Memory
├─ Header (protected by atomic<> operations)
├─ Ring Buffer (lock-free SPMC)
├─ No locks needed (wait-free design)
└─ Reads never block writes

Adapter (Multi-threaded)
├─ Main Thread
│  ├─ Gemini connection management
│  └─ Audio I/O
│
└─ Context Monitor Thread
   ├─ Poll shared memory (100ms)
   ├─ Detect context changes
   └─ Inject into Gemini
```

---

## S2 Geometry Cell Hierarchy

```
Level 0 (Entire Earth)
└── Level 6 (Continental)
    └── Level 12 (Country)
        └── Level 16 (City neighborhood ~600m)
            └── Level 20 (Street block ~6m)
                └── Level 24 (House/Building ~60cm)
                    └── Level 30 (Person/Meter ~1cm)

For S2S Geospatial Adapter:
├─ Level 16 (primary): 600m cells
│  └─ Used for boundary detection
│  └─ Updates when user crosses cell
│
├─ Level 24 (secondary): 60cm cells
│  └─ Used for PDR step tracking
│  └─ High precision for barefoot movement
│
└─ Level 10 (coarse): 5km cells
   └─ Optional: prefetch for regional data
```

---

End of Architecture Documentation
