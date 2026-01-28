/**
 * @file WebSocketManager.hpp
 * @brief WebSocket connection management
 */

#ifndef S2SGEO_WEBSOCKET_MANAGER_HPP
#define S2SGEO_WEBSOCKET_MANAGER_HPP

#include <string>
#include <functional>
#include <vector>

namespace s2sgeo {

/**
 * @class WebSocketManager
 * @brief Manages WebSocket lifecycle and message passing
 */
class WebSocketManager {
public:
    using OnMessageCallback = std::function<void(const std::string&)>;
    using OnConnectedCallback = std::function<void()>;
    using OnErrorCallback = std::function<void(const std::string&)>;
    
    /**
     * @brief Connect to WebSocket endpoint
     */
    bool connect(const std::string& url, OnConnectedCallback on_connected);
    
    /**
     * @brief Send message over WebSocket
     */
    void sendMessage(const std::string& message);
    
    /**
     * @brief Set message handler callback
     */
    void setMessageCallback(OnMessageCallback callback);
    
    /**
     * @brief Set error handler callback
     */
    void setErrorCallback(OnErrorCallback callback);
    
    /**
     * @brief Disconnect
     */
    void disconnect();
    
    /**
     * @brief Check connection status
     */
    bool isConnected() const;
    
private:
    std::string url_;
    bool connected_ = false;
    OnMessageCallback message_callback_;
    OnErrorCallback error_callback_;
};

} // namespace s2sgeo

#endif // S2SGEO_WEBSOCKET_MANAGER_HPP
