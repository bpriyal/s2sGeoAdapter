/**
 * @file TestS2Geometry.cpp
 * @brief Unit tests for S2 geometry indexing
 */

#include "S2GeometryWrapper.hpp"
#include "gtest/gtest.h"
#include <cmath>

using namespace s2sgeo;

class S2GeometryTest : public ::testing::Test {
protected:
    S2GeometryTest() : index_(std::make_unique<S2GeometryIndex>()) {}
    
    std::unique_ptr<S2GeometryIndex> index_;
};

TEST_F(S2GeometryTest, LatLonToCellTest) {
    uint64_t cell_id = index_->latLonToCell(37.7749, -122.4194, 16);
    EXPECT_NE(cell_id, 0);
}

TEST_F(S2GeometryTest, ConsistencyTest) {
    uint64_t cell_id1 = index_->latLonToCell(37.7749, -122.4194, 16);
    uint64_t cell_id2 = index_->latLonToCell(37.7749, -122.4194, 16);
    EXPECT_EQ(cell_id1, cell_id2);  // Same input should give same output
}

TEST_F(S2GeometryTest, BoundaryCrossingTest) {
    double lat1 = 37.7749;
    double lon1 = -122.4194;
    double lat2 = 37.7750;
    double lon2 = -122.4193;
    
    bool crossed = index_->crossedBoundary(lat1, lon1, lat2, lon2);
    EXPECT_FALSE(crossed);  // Small distance, same cell
}

TEST_F(S2GeometryTest, DistanceCalculationTest) {
    // SF to LA (approximately 560 km)
    double dist = S2GeometryIndex::distanceMeters(37.7749, -122.4194, 34.0522, -118.2437);
    
    // Should be approximately 560 km (560000 m)
    EXPECT_GT(dist, 500000);
    EXPECT_LT(dist, 620000);
}

TEST_F(S2GeometryTest, GetCellCenterTest) {
    double lat = 37.7749;
    double lon = -122.4194;
    uint64_t cell_id = index_->latLonToCell(lat, lon, 16);
    
    double center_lat, center_lon;
    index_->getCellCenter(cell_id, center_lat, center_lon);
    
    // Center should be close to original point
    EXPECT_NEAR(center_lat, lat, 0.01);
    EXPECT_NEAR(center_lon, lon, 0.01);
}

TEST_F(S2GeometryTest, GetNeighborsTest) {
    uint64_t cell_id = index_->latLonToCell(37.7749, -122.4194, 16);
    std::vector<uint64_t> neighbors = index_->getNeighbors(cell_id);
    
    EXPECT_LE(neighbors.size(), 4);  // At most 4 edge neighbors
    
    for (uint64_t neighbor_id : neighbors) {
        EXPECT_NE(neighbor_id, cell_id);  // Neighbor should be different
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
