/**
 * @file WebSocketManager.cpp
 * @brief WebSocket manager implementation
 */

#include "WebSocketManager.hpp"
#include <iostream>

namespace s2sgeo {

bool WebSocketManager::connect(const std::string& url, OnConnectedCallback on_connected) {
    url_ = url;
    std::cout << "[WebSocketManager] Connecting to: " << url << std::endl;
    
    // Mock: In production, use libwebsockets or Boost.Beast
    connected_ = true;
    
    if (on_connected) {
        on_connected();
    }
    
    std::cout << "[WebSocketManager] Connected" << std::endl;
    return true;
}

void WebSocketManager::sendMessage(const std::string& message) {
    if (!connected_) {
        if (error_callback_) {
            error_callback_("Not connected");
        }
        return;
    }
    
    std::cout << "[WebSocketManager] Sending message (" << message.size() << " bytes)" << std::endl;
}

void WebSocketManager::setMessageCallback(OnMessageCallback callback) {
    message_callback_ = callback;
}

void WebSocketManager::setErrorCallback(OnErrorCallback callback) {
    error_callback_ = callback;
}

void WebSocketManager::disconnect() {
    if (connected_) {
        std::cout << "[WebSocketManager] Disconnecting" << std::endl;
        connected_ = false;
    }
}

bool WebSocketManager::isConnected() const {
    return connected_;
}

} // namespace s2sgeo
