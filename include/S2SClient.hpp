/**
 * @file S2SClient.hpp
 * @brief Speech-to-Speech client for Gemini integration
 */

#ifndef S2SGEO_S2S_CLIENT_HPP
#define S2SGEO_S2S_CLIENT_HPP

#include "SharedMemoryStructs.hpp"
#include <string>
#include <memory>
#include <functional>

namespace s2sgeo {

/**
 * @class S2SClient
 * @brief Manages WebSocket connection to Gemini Live
 */
class S2SClient {
public:
    S2SClient();
    ~S2SClient();
    
    /**
     * @brief Initialize and connect to Gemini Live
     */
    bool connect(const std::string& api_key);
    
    /**
     * @brief Send audio chunk to Gemini
     */
    void sendAudio(const std::vector<uint8_t>& pcm_data);
    
    /**
     * @brief Send context update
     */
    void sendContext(const std::string& json_context);
    
    /**
     * @brief Disconnect from Gemini
     */
    void disconnect();
    
    /**
     * @brief Set callback for receiving audio response
     */
    void setAudioResponseCallback(
        std::function<void(const std::vector<uint8_t>&)> callback);
    
    /**
     * @brief Check if connected
     */
    bool isConnected() const { return connected_; }
    
private:
    std::string api_key_;
    bool connected_ = false;
    std::string websocket_url_;
    
    std::function<void(const std::vector<uint8_t>&)> audio_response_callback_;
};

} // namespace s2sgeo

#endif // S2SGEO_S2S_CLIENT_HPP
