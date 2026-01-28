/**
 * @file S2SClient.cpp
 * @brief S2S client implementation
 */

#include "S2SClient.hpp"
#include <iostream>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace s2sgeo {

S2SClient::S2SClient() {
    std::cout << "[S2SClient] Initialized" << std::endl;
}

S2SClient::~S2SClient() {
    disconnect();
}

bool S2SClient::connect(const std::string& api_key) {
    api_key_ = api_key;
    websocket_url_ = "wss://generativelanguage.googleapis.com/google.ai.generativelanguage.v1alpha.GenerativeService.BidiGenerateContent";
    
    std::cout << "[S2SClient] Connecting to Gemini Live..." << std::endl;
    std::cout << "[S2SClient] Endpoint: " << websocket_url_ << std::endl;
    
    // Mock: In production, use WebSocket library (libwebsockets, Boost.Beast)
    connected_ = true;
    
    std::cout << "[S2SClient] Connected to Gemini Live" << std::endl;
    return true;
}

void S2SClient::sendAudio(const std::vector<uint8_t>& pcm_data) {
    if (!connected_) {
        std::cerr << "[S2SClient] Not connected" << std::endl;
        return;
    }
    
    // Create audio input message
    json message = {
        {"client_content", {
            {"turns", json::array({
                {{"parts", json::array({
                    {{"inline_data", {
                        {"mime_type", "audio/pcm"},
                        {"data", ""}  // Would base64 encode actual audio
                    }}}
                })}}
            })}
        }}
    };
    
    std::cout << "[S2SClient] Sent audio chunk (" << pcm_data.size() << " bytes)" << std::endl;
    
    // Mock response
    if (audio_response_callback_) {
        std::vector<uint8_t> mock_response(16000, 0);  // 1s of silence
        audio_response_callback_(mock_response);
    }
}

void S2SClient::sendContext(const std::string& json_context) {
    if (!connected_) {
        std::cerr << "[S2SClient] Not connected" << std::endl;
        return;
    }
    
    // Inject context as system message update
    json message = {
        {"system_instruction", json_context}
    };
    
    std::cout << "[S2SClient] Sent context update: " << json_context << std::endl;
}

void S2SClient::disconnect() {
    if (connected_) {
        std::cout << "[S2SClient] Disconnecting from Gemini Live..." << std::endl;
        connected_ = false;
    }
}

void S2SClient::setAudioResponseCallback(
    std::function<void(const std::vector<uint8_t>&)> callback) {
    audio_response_callback_ = callback;
}

} // namespace s2sgeo
