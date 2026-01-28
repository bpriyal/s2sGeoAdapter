/**
 * @file ContextInjector.cpp
 * @brief Context injector implementation
 */

#include "ContextInjector.hpp"
#include <sstream>
#include <iomanip>

namespace s2sgeo {

std::string ContextInjector::formatContextPrompt(const ContextFrame& ctx) {
    std::ostringstream oss;
    
    oss << "Road: " << ctx.road_name << " (" << ctx.road_type << ")\n";
    oss << "Traffic: " << ctx.traffic_level << "\n";
    oss << "Grade: " << std::fixed << std::setprecision(1) << ctx.gradient_percent << "%\n";
    oss << "Elevation gain: " << ctx.elevation_gain_m << "m\n";
    oss << "Current speed: " << ctx.current_speed << " m/s\n";
    oss << "Speed limit: " << ctx.speed_limit << " km/h\n";
    
    return oss.str();
}

std::string ContextInjector::buildSystemInstruction(const WorldState& state) {
    std::ostringstream oss;
    
    oss << "You are an expert cycling AI assistant. ";
    oss << "User is at coordinates " << std::fixed << std::setprecision(6)
        << state.smoothed_lat << ", " << state.smoothed_lon << ". ";
    
    if (state.is_moving) {
        oss << "User is moving. ";
        oss << "Detected " << state.step_count << " steps, "
            << std::setprecision(1) << state.estimated_distance_m << "m traveled. ";
    } else {
        oss << "User is stationary. ";
    }
    
    return oss.str();
}

} // namespace s2sgeo
