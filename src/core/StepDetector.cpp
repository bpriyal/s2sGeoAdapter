/**
 * @file StepDetector.cpp
 * @brief Step detection for Pedestrian Dead Reckoning
 */

#include <cmath>

namespace s2sgeo {

// Step detection utilities for PDR

class StepDetector {
public:
    static constexpr double STEP_THRESHOLD = 1.5;  // m/s^2
    static constexpr double STEP_MIN_INTERVAL = 0.3;  // seconds
    
    bool detectStep(double accel_z, double current_time_s) {
        // Simple peak detection
        if (current_time_s - last_step_time_s_ > STEP_MIN_INTERVAL) {
            if (accel_z > STEP_THRESHOLD && last_accel_z_ < STEP_THRESHOLD) {
                last_step_time_s_ = current_time_s;
                return true;
            }
        }
        last_accel_z_ = accel_z;
        return false;
    }
    
private:
    double last_accel_z_ = 0.0;
    double last_step_time_s_ = 0.0;
};

} // namespace s2sgeo
