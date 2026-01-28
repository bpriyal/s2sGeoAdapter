/**
 * @file IGeoProvider.hpp
 * @brief Plugin interface for modular context providers
 * @details Allows swapping between Cycling, Dating, and other providers
 */

#ifndef S2SGEO_IGEO_PROVIDER_HPP
#define S2SGEO_IGEO_PROVIDER_HPP

#include "SharedMemoryStructs.hpp"
#include <memory>
#include <string>

namespace s2sgeo {

/**
 * @class IContextProvider
 * @brief Abstract base class for location-based context providers
 * 
 * Implementations:
 * - CyclingContextProvider: Road surface, traffic, elevation
 * - DatingContextProvider: Nearby users, interests, venues
 * - DeliveryContextProvider: Traffic, delivery zones, customer info
 */
class IContextProvider {
public:
    virtual ~IContextProvider() = default;
    
    /**
     * @brief Initialize the provider with API credentials
     * @param config JSON configuration string
     */
    virtual void initialize(const std::string& config) = 0;
    
    /**
     * @brief Fetch context data for a given location
     * @param lat User's latitude
     * @param lon User's longitude
     * @return ContextFrame with environmental data
     */
    virtual ContextFrame getContext(double lat, double lon) = 0;
    
    /**
     * @brief Prefetch data for the next N cells (frustum)
     * @param lat Current latitude
     * @param lon Current longitude
     * @param heading Direction of movement (degrees)
     * @param distance How far ahead to prefetch (meters)
     */
    virtual void prefetchContext(double lat, double lon, 
                                 double heading, double distance) = 0;
    
    /**
     * @brief Get provider name
     */
    virtual std::string getName() const = 0;
};

/**
 * @class IGeometryIndex
 * @brief Abstract spatial indexing interface
 */
class IGeometryIndex {
public:
    virtual ~IGeometryIndex() = default;
    
    /**
     * @brief Convert Lat/Lon to cell ID
     */
    virtual uint64_t latLonToCell(double lat, double lon, int level) = 0;
    
    /**
     * @brief Get neighboring cell IDs
     */
    virtual std::vector<uint64_t> getNeighbors(uint64_t cellId) = 0;
    
    /**
     * @brief Check if user crossed a cell boundary
     */
    virtual bool crossedBoundary(double lat1, double lon1,
                                 double lat2, double lon2) = 0;
};

/**
 * @class IKalmanFilter
 * @brief Location smoothing filter interface
 */
class IKalmanFilter {
public:
    virtual ~IKalmanFilter() = default;
    
    /**
     * @brief Update filter with new measurement
     */
    virtual void update(const LocationFix& measurement) = 0;
    
    /**
     * @brief Get smoothed state
     */
    virtual WorldState getSmoothedState() = 0;
    
    /**
     * @brief Reset filter
     */
    virtual void reset() = 0;
};

} // namespace s2sgeo

#endif // S2SGEO_IGEO_PROVIDER_HPP
