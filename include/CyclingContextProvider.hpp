/**
 * @file CyclingContextProvider.hpp
 * @brief Context provider for cycling applications
 */

#ifndef S2SGEO_CYCLING_CONTEXT_PROVIDER_HPP
#define S2SGEO_CYCLING_CONTEXT_PROVIDER_HPP

#include "IGeoProvider.hpp"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace s2sgeo {

/**
 * @class CyclingContextProvider
 * @brief Fetches road surface, elevation, traffic for cyclists
 * 
 * Data sources:
 * - Google Maps Routes API: Traffic, road info
 * - Google Maps Elevation API: Grade, elevation gain
 * - OpenStreetMap: Surface type (paved, gravel, dirt)
 */
class CyclingContextProvider : public IContextProvider {
public:
    CyclingContextProvider();
    
    void initialize(const std::string& config) override;
    ContextFrame getContext(double lat, double lon) override;
    void prefetchContext(double lat, double lon, double heading, double distance) override;
    std::string getName() const override { return "cycling"; }
    
private:
    std::string google_maps_api_key_;
    std::string osm_api_endpoint_;
    
    // Cached context to avoid excessive API calls
    ContextFrame cached_context_;
    double cached_lat_ = 0.0;
    double cached_lon_ = 0.0;
    int64_t cached_timestamp_ms_ = 0;
    
    static constexpr int64_t CACHE_TTL_MS = 5000;  // 5 second cache
    
    /**
     * @brief Fetch elevation data from Google Maps API
     */
    ContextFrame fetchElevation(double lat, double lon);
    
    /**
     * @brief Fetch traffic from Google Maps Routes API
     */
    void fetchTraffic(double lat, double lon, ContextFrame& ctx);
    
    /**
     * @brief Fetch road surface from OSM
     */
    void fetchSurface(double lat, double lon, ContextFrame& ctx);
    
    /**
     * @brief Parse OSM data and extract surface type
     */
    std::string parseOSMSurface(const std::string& osm_response);
};

} // namespace s2sgeo

#endif // S2SGEO_CYCLING_CONTEXT_PROVIDER_HPP
