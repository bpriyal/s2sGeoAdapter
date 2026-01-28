/**
 * @file S2GeometryWrapper.cpp
 * @brief S2 Geometry implementation
 */

#include "S2GeometryWrapper.hpp"
#include <cmath>

namespace s2sgeo {

S2GeometryIndex::S2GeometryIndex() {
}

uint64_t S2GeometryIndex::latLonToCell(double lat, double lon, int level) {
    S2LatLng latlng = S2LatLng::FromDegrees(lat, lon);
    S2CellId cellid = S2CellId(latlng).parent(level);
    return cellid.id();
}

std::vector<uint64_t> S2GeometryIndex::getNeighbors(uint64_t cellId) {
    std::vector<uint64_t> neighbors;
    S2CellId cell(cellId);
    
    // Get all 4 edge neighbors
    for (int i = 0; i < 4; ++i) {
        S2CellId neighbor;
        if (cell.GetEdgeNeighbors()[i].is_valid()) {
            neighbors.push_back(cell.GetEdgeNeighbors()[i].id());
        }
    }
    
    return neighbors;
}

bool S2GeometryIndex::crossedBoundary(double lat1, double lon1,
                                      double lat2, double lon2) {
    // Use Level 16 for boundary detection (600m cells)
    uint64_t cell1 = latLonToCell(lat1, lon1, 16);
    uint64_t cell2 = latLonToCell(lat2, lon2, 16);
    return cell1 != cell2;
}

void S2GeometryIndex::getCellCenter(uint64_t cellId, double& lat, double& lon) {
    S2CellId cell(cellId);
    S2LatLng latlng = S2LatLng(cell.ToPoint());
    lat = latlng.lat().degrees();
    lon = latlng.lng().degrees();
}

double S2GeometryIndex::getCellArea(uint64_t cellId) {
    S2CellId cell(cellId);
    S2Cell s2cell(cell);
    return s2cell.GetArea() * 40680631590769;  // Convert to square meters
}

double S2GeometryIndex::distanceMeters(double lat1, double lon1,
                                       double lat2, double lon2) {
    // Haversine formula
    const double R = 6371000.0;  // Earth radius in meters
    
    double phi1 = lat1 * M_PI / 180.0;
    double phi2 = lat2 * M_PI / 180.0;
    double delta_phi = (lat2 - lat1) * M_PI / 180.0;
    double delta_lambda = (lon2 - lon1) * M_PI / 180.0;
    
    double a = std::sin(delta_phi / 2.0) * std::sin(delta_phi / 2.0) +
               std::cos(phi1) * std::cos(phi2) *
               std::sin(delta_lambda / 2.0) * std::sin(delta_lambda / 2.0);
    
    double c = 2.0 * std::asin(std::sqrt(a));
    
    return R * c;
}

} // namespace s2sgeo
