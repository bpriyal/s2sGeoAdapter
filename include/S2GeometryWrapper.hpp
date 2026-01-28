/**
 * @file S2GeometryWrapper.hpp
 * @brief Wrapper around Google's S2 geometry for spatial indexing
 */

#ifndef S2SGEO_S2_GEOMETRY_WRAPPER_HPP
#define S2SGEO_S2_GEOMETRY_WRAPPER_HPP

#include "IGeoProvider.hpp"
#include "s2/s2cell_id.h"
#include "s2/s2geometry.h"
#include <vector>
#include <cstdint>

namespace s2sgeo {

/**
 * @class S2GeometryIndex
 * @brief S2 Geometry spatial indexing (Google's system)
 * 
 * S2 provides hierarchical square cells:
 * - Level 24: ~60cm (for barefoot movement)
 * - Level 16: ~600m (for cycling)
 * - Level 10: ~5km (for driving)
 */
class S2GeometryIndex : public IGeometryIndex {
public:
    S2GeometryIndex();
    
    /**
     * @brief Convert Lat/Lon to S2 cell ID at given level
     */
    uint64_t latLonToCell(double lat, double lon, int level) override;
    
    /**
     * @brief Get 4 neighboring cells
     */
    std::vector<uint64_t> getNeighbors(uint64_t cellId) override;
    
    /**
     * @brief Check if user crossed a cell boundary
     */
    bool crossedBoundary(double lat1, double lon1,
                        double lat2, double lon2) override;
    
    /**
     * @brief Get cell center coordinates
     */
    void getCellCenter(uint64_t cellId, double& lat, double& lon);
    
    /**
     * @brief Get cell area in square meters
     */
    double getCellArea(uint64_t cellId);
    
    /**
     * @brief Calculate distance between two points (Haversine)
     */
    static double distanceMeters(double lat1, double lon1, 
                                 double lat2, double lon2);
};

} // namespace s2sgeo

#endif // S2SGEO_S2_GEOMETRY_WRAPPER_HPP
