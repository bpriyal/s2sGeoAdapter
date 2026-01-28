/**
 * @file CommandDispatcher.hpp
 * @brief Command parsing and plugin activation
 */

#ifndef S2SGEO_COMMAND_DISPATCHER_HPP
#define S2SGEO_COMMAND_DISPATCHER_HPP

#include "IGeoProvider.hpp"
#include <string>
#include <memory>

namespace s2sgeo {

/**
 * @class CommandDispatcher
 * @brief Parses keywords and activates appropriate plugins
 * 
 * Keywords:
 * - "cycling": Activate CyclingContextProvider
 * - "dating": Activate DatingContextProvider
 * - "delivery": Activate DeliveryContextProvider (future)
 */
class CommandDispatcher {
public:
    /**
     * @brief Process a voice command or text keyword
     */
    static bool processCommand(const std::string& command);
    
    /**
     * @brief Get current active plugin
     */
    static std::string getActivePlugin();
    
    /**
     * @brief Set accuracy level (0.0 - 1.0)
     */
    static void setAccuracyLevel(double level);
};

} // namespace s2sgeo

#endif // S2SGEO_COMMAND_DISPATCHER_HPP
