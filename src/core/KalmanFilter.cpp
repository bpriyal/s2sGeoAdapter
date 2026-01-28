/**
 * @file KalmanFilter.cpp
 * @brief Implementation of Kalman filter for GPS smoothing
 */

#include "KalmanFilter.hpp"
#include <Eigen/Cholesky>
#include <cmath>
#include <iostream>

namespace s2sgeo {

KalmanFilter::KalmanFilter() {
    // Initialize state transition matrix
    // With dt = 0.1s (typical GPS update rate)
    double dt = 0.1;
    A_ << 1, 0, dt, 0,
          0, 1, 0, dt,
          0, 0, 1, 0,
          0, 0, 0, 1;
    
    // Measurement matrix (we only observe lat/lon, not velocities)
    H_ << 1, 0, 0, 0,
          0, 1, 0, 0;
    
    // Process noise covariance
    double q = 0.1;  // Default tuning
    Q_ = Eigen::Matrix4d::Identity() * q;
    Q_(0, 0) *= 0.001;  // Less noise on position
    Q_(1, 1) *= 0.001;
    
    // Measurement noise covariance
    double r = 100.0;  // GPS accuracy ~10m std
    R_ << r, 0,
          0, r;
    
    // Initial state
    x_ = Eigen::Vector4d::Zero();
    
    // Initial covariance (high uncertainty)
    P_ = Eigen::Matrix4d::Identity() * 1e6;
}

void KalmanFilter::predict(double dt) {
    // Update A with current dt
    A_(0, 2) = dt;
    A_(1, 3) = dt;
    
    // Predict: x = A * x
    x_ = A_ * x_;
    
    // Predict: P = A * P * A^T + Q
    P_ = A_ * P_ * A_.transpose() + Q_;
}

void KalmanFilter::correct(const Eigen::Vector2d& z) {
    // Innovation
    Eigen::Vector2d y = z - H_ * x_;
    
    // Innovation covariance
    Eigen::Matrix2d S = H_ * P_ * H_.transpose() + R_;
    
    // Kalman gain
    Eigen::Matrix<double, 4, 2> K = P_ * H_.transpose() * S.inverse();
    
    // Update state
    x_ = x_ + K * y;
    
    // Update covariance
    P_ = (Eigen::Matrix4d::Identity() - K * H_) * P_;
}

void KalmanFilter::update(const LocationFix& measurement) {
    int64_t current_time_ms = measurement.timestamp_ms;
    double dt_s = 0.1;  // Default
    
    if (last_update_ms_ > 0) {
        dt_s = (current_time_ms - last_update_ms_) / 1000.0;
    }
    last_update_ms_ = current_time_ms;
    
    // Clamp dt to reasonable values
    if (dt_s < 0.01) dt_s = 0.01;
    if (dt_s > 1.0) dt_s = 1.0;
    
    // Adapt measurement noise based on accuracy
    double r = std::max(100.0, measurement.accuracy * measurement.accuracy);
    R_ << r, 0,
          0, r;
    
    // Predict
    predict(dt_s);
    
    // Correct with GPS measurement
    Eigen::Vector2d z(measurement.latitude, measurement.longitude);
    correct(z);
    
    // Optional: PDR fusion
    if (use_pdr_ && detectStep(measurement)) {
        // Step detected: update position based on heading + step length
        step_count_++;
    }
}

bool KalmanFilter::detectStep(const LocationFix& imu_data) {
    const double STEP_THRESHOLD = 15.0;  // m/s^2
    const double STEP_MIN_INTERVAL = 0.3;  // seconds
    
    // Simple peak detection on Z-axis acceleration
    double accel_magnitude = std::sqrt(
        imu_data.accel_x * imu_data.accel_x +
        imu_data.accel_y * imu_data.accel_y +
        imu_data.accel_z * imu_data.accel_z
    );
    
    // Detect peaks (upstroke of walking motion)
    bool is_step = (last_accel_z_ < STEP_THRESHOLD) && 
                   (imu_data.accel_z >= STEP_THRESHOLD);
    last_accel_z_ = imu_data.accel_z;
    
    return is_step;
}

WorldState KalmanFilter::getSmoothedState() {
    WorldState state;
    state.smoothed_lat = x_(0);
    state.smoothed_lon = x_(1);
    state.smoothed_altitude = 0.0;
    state.is_moving = (std::abs(x_(2)) > 0.1 || std::abs(x_(3)) > 0.1);
    state.step_count = step_count_;
    state.last_update_ms = last_update_ms_;
    return state;
}

void KalmanFilter::reset() {
    x_ = Eigen::Vector4d::Zero();
    P_ = Eigen::Matrix4d::Identity() * 1e6;
    step_count_ = 0;
    last_update_ms_ = 0;
}

void KalmanFilter::setProcessNoise(double q) {
    Q_ = Eigen::Matrix4d::Identity() * q;
    Q_(0, 0) *= 0.001;
    Q_(1, 1) *= 0.001;
}

void KalmanFilter::setMeasurementNoise(double r) {
    R_ << r, 0,
          0, r;
}

} // namespace s2sgeo
