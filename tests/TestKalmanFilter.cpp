/**
 * @file TestKalmanFilter.cpp
 * @brief Unit tests for Kalman filter
 */

#include "KalmanFilter.hpp"
#include "gtest/gtest.h"
#include <chrono>

using namespace s2sgeo;

class KalmanFilterTest : public ::testing::Test {
protected:
    KalmanFilterTest() : kf_(std::make_unique<KalmanFilter>()) {}
    
    std::unique_ptr<KalmanFilter> kf_;
};

TEST_F(KalmanFilterTest, InitializationTest) {
    WorldState state = kf_->getSmoothedState();
    EXPECT_EQ(state.smoothed_lat, 0.0);
    EXPECT_EQ(state.smoothed_lon, 0.0);
}

TEST_F(KalmanFilterTest, SingleUpdateTest) {
    LocationFix fix(37.7749, -122.4194, 1000);
    fix.altitude = 50.0;
    fix.accuracy = 10.0;
    
    kf_->update(fix);
    
    WorldState state = kf_->getSmoothedState();
    // After Kalman update, should be close to input
    EXPECT_NEAR(state.smoothed_lat, 37.7749, 0.01);
    EXPECT_NEAR(state.smoothed_lon, -122.4194, 0.01);
}

TEST_F(KalmanFilterTest, MultipleUpdatesTest) {
    for (int i = 0; i < 10; ++i) {
        LocationFix fix(37.7749 + i * 0.001, -122.4194, 1000 + i * 100);
        fix.altitude = 50.0 + i * 0.5;
        fix.accuracy = 10.0;
        
        kf_->update(fix);
        
        WorldState state = kf_->getSmoothedState();
        EXPECT_TRUE(!std::isnan(state.smoothed_lat));
        EXPECT_TRUE(!std::isnan(state.smoothed_lon));
    }
}

TEST_F(KalmanFilterTest, NoiseReductionTest) {
    // Add noisy measurements
    LocationFix fix1(37.7749, -122.4194, 1000);
    LocationFix fix2(37.7749 + 0.1, -122.4194 + 0.1, 1100);  // Big jump (noise)
    LocationFix fix3(37.7749, -122.4194, 1200);  // Back to original
    
    kf_->update(fix1);
    kf_->update(fix2);
    kf_->update(fix3);
    
    WorldState state = kf_->getSmoothedState();
    // Should be closer to original than the noisy jump
    EXPECT_LT(std::abs(state.smoothed_lat - 37.7749), 0.05);
    EXPECT_LT(std::abs(state.smoothed_lon - (-122.4194)), 0.05);
}

TEST_F(KalmanFilterTest, ResetTest) {
    LocationFix fix(37.7749, -122.4194, 1000);
    kf_->update(fix);
    
    kf_->reset();
    
    WorldState state = kf_->getSmoothedState();
    EXPECT_EQ(state.smoothed_lat, 0.0);
    EXPECT_EQ(state.smoothed_lon, 0.0);
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
