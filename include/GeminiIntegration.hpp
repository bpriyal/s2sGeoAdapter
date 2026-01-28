/**
 * @file GeminiIntegration.hpp
 * @brief High-level Gemini integration with context injection
 */

#ifndef S2SGEO_GEMINI_INTEGRATION_HPP
#define S2SGEO_GEMINI_INTEGRATION_HPP

#include "SharedMemoryStructs.hpp"
#include "S2SClient.hpp"
#include <thread>
#include <atomic>
#include <memory>

namespace s2sgeo {

/**
 * @class GeminiIntegration
 * @brief Orchestrates S2S session with geospatial context
 */
class GeminiIntegration {
public:
    GeminiIntegration();
    ~GeminiIntegration();
    
    /**
     * @brief Initialize and start the session
     */
    bool start(const std::string& api_key);
    
    /**
     * @brief Stop the session
     */
    void stop();
    
private:
    std::unique_ptr<S2SClient> s2s_client_;
    std::atomic<bool> running_ = false;
    std::thread context_update_thread_;
    
    uint64_t last_context_hash_ = 0;
    
    /**
     * @brief Monitor location service and inject context updates
     */
    void contextUpdateLoop();
    
    /**
     * @brief Compute hash of context (for change detection)
     */
    uint64_t hashContext(const ContextFrame& ctx);
};

} // namespace s2sgeo

#endif // S2SGEO_GEMINI_INTEGRATION_HPP
