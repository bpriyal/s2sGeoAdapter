/**
 * @file ContextInjector.hpp
 * @brief Context injection utilities
 */

#ifndef S2SGEO_CONTEXT_INJECTOR_HPP
#define S2SGEO_CONTEXT_INJECTOR_HPP

#include "SharedMemoryStructs.hpp"
#include <string>

namespace s2sgeo {

/**
 * @class ContextInjector
 * @brief Utility for formatting and injecting context into LLM
 */
class ContextInjector {
public:
    /**
     * @brief Format context frame as prompt
     */
    static std::string formatContextPrompt(const ContextFrame& ctx);
    
    /**
     * @brief Build system instruction from world state
     */
    static std::string buildSystemInstruction(const WorldState& state);
};

} // namespace s2sgeo

#endif // S2SGEO_CONTEXT_INJECTOR_HPP
