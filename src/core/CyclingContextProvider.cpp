/**
 * @file CyclingContextProvider.cpp
 * @brief Cycling context provider implementation
 */

#include "CyclingContextProvider.hpp"
#include <iostream>
#include <chrono>
#include <cstring>

namespace s2sgeo {

CyclingContextProvider::CyclingContextProvider() {
    std::memset(cached_context_.road_name, 0, sizeof(cached_context_.road_name));
    std::memset(cached_context_.road_type, 0, sizeof(cached_context_.road_type));
    std::memset(cached_context_.traffic_level, 0, sizeof(cached_context_.traffic_level));
    std::memset(cached_context_.hazards, 0, sizeof(cached_context_.hazards));
}

void CyclingContextProvider::initialize(const std::string& config) {
    try {
        auto cfg = json::parse(config);
        if (cfg.contains("google_maps_api_key")) {
            google_maps_api_key_ = cfg["google_maps_api_key"];
        }
        if (cfg.contains("osm_api_endpoint")) {
            osm_api_endpoint_ = cfg["osm_api_endpoint"];
        }
        std::cout << "[CyclingContextProvider] Initialized with API keys" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "[CyclingContextProvider] Init error: " << e.what() << std::endl;
    }
}

ContextFrame CyclingContextProvider::getContext(double lat, double lon) {
    auto now = std::chrono::system_clock::now();
    int64_t current_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    // Check cache
    if (current_ms - cached_timestamp_ms_ < CACHE_TTL_MS &&
        std::abs(lat - cached_lat_) < 0.001 &&
        std::abs(lon - cached_lon_) < 0.001) {
        return cached_context_;
    }
    
    // Fetch fresh data
    ContextFrame ctx = fetchElevation(lat, lon);
    fetchTraffic(lat, lon, ctx);
    fetchSurface(lat, lon, ctx);
    
    ctx.timestamp_ms = current_ms;
    
    // Cache
    cached_context_ = ctx;
    cached_lat_ = lat;
    cached_lon_ = lon;
    cached_timestamp_ms_ = current_ms;
    
    return ctx;
}

void CyclingContextProvider::prefetchContext(double lat, double lon,
                                             double heading, double distance) {
    // Calculate frustum (cone ahead of cyclist)
    // Simple implementation: fetch N points ahead
    double lat_delta = (distance / 111000.0);  // ~1 degree = 111 km
    double lon_delta = lat_delta;  // Simplified
    
    // Prefetch for next few points
    for (int i = 1; i <= 3; ++i) {
        double next_lat = lat + (lat_delta * i);
        double next_lon = lon + (lon_delta * i);
        // Trigger background fetch (would be async in production)
        std::cout << "[CyclingContextProvider] Prefetching: " 
                  << next_lat << ", " << next_lon << std::endl;
    }
}

ContextFrame CyclingContextProvider::fetchElevation(double lat, double lon) {
    ContextFrame ctx;
    
    // Mock: simulate elevation gradient
    // In production, call Google Maps Elevation API
    strncpy(ctx.road_name, "Main Street", sizeof(ctx.road_name) - 1);
    strncpy(ctx.road_type, "asphalt", sizeof(ctx.road_type) - 1);
    strncpy(ctx.traffic_level, "light", sizeof(ctx.traffic_level) - 1);
    
    ctx.elevation_gain_m = 45.0;      // 45 m elevation gain
    ctx.gradient_percent = 5.5;        // 5.5% grade
    ctx.current_speed = 18.0;
    ctx.speed_limit = 50.0;
    
    return ctx;
}

void CyclingContextProvider::fetchTraffic(double lat, double lon, ContextFrame& ctx) {
    // Mock: In production, query Google Maps Routes API
    strncpy(ctx.traffic_level, "moderate", sizeof(ctx.traffic_level) - 1);
    
    // Hazard detection
    json hazards = json::array();
    hazards.push_back({{"type", "congestion"}, {"severity", "low"}});
    
    std::string hazards_str = hazards.dump();
    strncpy(ctx.hazards, hazards_str.c_str(), sizeof(ctx.hazards) - 1);
}

void CyclingContextProvider::fetchSurface(double lat, double lon, ContextFrame& ctx) {
    // Mock: In production, query OpenStreetMap Overpass API
    strncpy(ctx.road_type, "asphalt", sizeof(ctx.road_type) - 1);
}

std::string CyclingContextProvider::parseOSMSurface(const std::string& osm_response) {
    // Parse OSM JSON response to extract surface tags
    try {
        auto data = json::parse(osm_response);
        if (data.contains("elements") && data["elements"].is_array()) {
            for (const auto& elem : data["elements"]) {
                if (elem.contains("tags") && elem["tags"].contains("surface")) {
                    return elem["tags"]["surface"];
                }
            }
        }
    } catch (...) {
    }
    return "unknown";
}

} // namespace s2sgeo
