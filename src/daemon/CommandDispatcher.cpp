/**
 * @file CommandDispatcher.cpp
 * @brief Command dispatcher implementation
 */

#include "CommandDispatcher.hpp"
#include "PluginRegistry.hpp"
#include "IPCManager.hpp"
#include <iostream>
#include <algorithm>
#include <cctype>

namespace s2sgeo {

static std::string toLower(const std::string& str) {
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

bool CommandDispatcher::processCommand(const std::string& command) {
    std::string lower_cmd = toLower(command);
    
    std::cout << "[CommandDispatcher] Processing command: " << lower_cmd << std::endl;
    
    if (lower_cmd.find("cycling") != std::string::npos ||
        lower_cmd.find("bike") != std::string::npos) {
        return PluginRegistry::getInstance().activateProvider("cycling");
    }
    else if (lower_cmd.find("dating") != std::string::npos ||
             lower_cmd.find("tinder") != std::string::npos) {
        return PluginRegistry::getInstance().activateProvider("dating");
    }
    else if (lower_cmd.find("delivery") != std::string::npos) {
        return PluginRegistry::getInstance().activateProvider("delivery");
    }
    else if (lower_cmd.find("running") != std::string::npos ||
             lower_cmd.find("walking") != std::string::npos) {
        // Footbased activities - use high accuracy level
        setAccuracyLevel(1.0);
        return PluginRegistry::getInstance().activateProvider("cycling");  // Reuse cycling for now
    }
    else if (lower_cmd.find("driving") != std::string::npos ||
             lower_cmd.find("car") != std::string::npos) {
        // Car-based - use lower accuracy level
        setAccuracyLevel(0.5);
        return PluginRegistry::getInstance().activateProvider("cycling");
    }
    else {
        std::cerr << "[CommandDispatcher] Unknown command: " << command << std::endl;
        return false;
    }
}

std::string CommandDispatcher::getActivePlugin() {
    auto provider = PluginRegistry::getInstance().getActiveProvider();
    if (provider) {
        return provider->getName();
    }
    return "";
}

void CommandDispatcher::setAccuracyLevel(double level) {
    // Clamp to [0.0, 1.0]
    level = std::max(0.0, std::min(1.0, level));
    
    auto& mgr = SharedMemoryManager::getInstance();
    if (mgr.isReady()) {
        auto* header = mgr.getHeader();
        if (header) {
            header->accuracy_level.store(level, std::memory_order_release);
            std::cout << "[CommandDispatcher] Set accuracy level to: " << level << std::endl;
        }
    }
}

} // namespace s2sgeo
