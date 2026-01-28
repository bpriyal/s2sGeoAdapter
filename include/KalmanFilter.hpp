/**
 * @file KalmanFilter.hpp
 * @brief Kalman filter for GPS location smoothing and PDR fusion
 */

#ifndef S2SGEO_KALMAN_FILTER_HPP
#define S2SGEO_KALMAN_FILTER_HPP

#include "SharedMemoryStructs.hpp"
#include "IGeoProvider.hpp"
#include <Eigen/Dense>
#include <memory>

namespace s2sgeo {

/**
 * @class KalmanFilter
 * @brief 2D position Kalman filter with adaptive process/measurement noise
 * 
 * State Vector: [x, y, vx, vy]
 * Measurement: [x, y] from GPS
 * Fusion: GPS + IMU (optional PDR)
 */
class KalmanFilter : public IKalmanFilter {
public:
    KalmanFilter();
    
    /**
     * @brief Update filter with GPS measurement
     */
    void update(const LocationFix& measurement) override;
    
    /**
     * @brief Get current smoothed state
     */
    WorldState getSmoothedState() override;
    
    /**
     * @brief Reset filter (e.g., when GPS signal is lost)
     */
    void reset() override;
    
    /**
     * @brief Set process noise (tuning parameter)
     * Higher = more responsive to changes
     */
    void setProcessNoise(double q);
    
    /**
     * @brief Set measurement noise (tuning parameter)
     * Higher = trust GPS less
     */
    void setMeasurementNoise(double r);
    
    /**
     * @brief Enable PDR (Pedestrian Dead Reckoning) fusion
     */
    void enablePDR(bool enable) { use_pdr_ = enable; }
    
    /**
     * @brief Detect steps from IMU accelerometer data
     * @return true if a step was detected
     */
    bool detectStep(const LocationFix& imu_data);
    
private:
    // Kalman matrices
    Eigen::Matrix4d A_;  // State transition matrix
    Eigen::Matrix2d H_;  // Measurement matrix
    Eigen::Matrix4d Q_;  // Process noise
    Eigen::Matrix2d R_;  // Measurement noise
    
    // State
    Eigen::Vector4d x_;  // [lat, lon, lat_vel, lon_vel]
    Eigen::Matrix4d P_;  // Covariance matrix
    
    // PDR state
    bool use_pdr_ = false;
    double last_accel_z_ = 0.0;
    int32_t step_count_ = 0;
    double step_length_m_ = 0.7;  // Default average step length
    
    // History
    int64_t last_update_ms_ = 0;
    
    /**
     * @brief Predict step (time update)
     */
    void predict(double dt);
    
    /**
     * @brief Correct step (measurement update)
     */
    void correct(const Eigen::Vector2d& z);
};

} // namespace s2sgeo

#endif // S2SGEO_KALMAN_FILTER_HPP
