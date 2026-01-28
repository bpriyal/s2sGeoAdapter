/**
 * @file PluginRegistry.cpp
 * @brief Plugin registry implementation
 */

#include "PluginRegistry.hpp"
#include <iostream>

namespace s2sgeo {

PluginRegistry& PluginRegistry::getInstance() {
    static PluginRegistry instance;
    return instance;
}

void PluginRegistry::registerProvider(const std::string& name, ProviderFactory factory) {
    factories_[name] = factory;
    std::cout << "[PluginRegistry] Registered provider: " << name << std::endl;
}

bool PluginRegistry::activateProvider(const std::string& name) {
    auto it = factories_.find(name);
    if (it == factories_.end()) {
        std::cerr << "[PluginRegistry] Provider not found: " << name << std::endl;
        return false;
    }
    
    // Create instance if not exists
    if (instances_.find(name) == instances_.end()) {
        instances_[name] = it->second();
    }
    
    active_provider_ = instances_[name].get();
    active_provider_name_ = name;
    
    std::cout << "[PluginRegistry] Activated provider: " << name << std::endl;
    return true;
}

IContextProvider* PluginRegistry::getActiveProvider() {
    return active_provider_;
}

std::vector<std::string> PluginRegistry::listProviders() const {
    std::vector<std::string> result;
    for (const auto& [name, _] : factories_) {
        result.push_back(name);
    }
    return result;
}

IContextProvider* PluginRegistry::getProvider(const std::string& name) {
    auto it = factories_.find(name);
    if (it == factories_.end()) {
        return nullptr;
    }
    
    if (instances_.find(name) == instances_.end()) {
        instances_[name] = it->second();
    }
    
    return instances_[name].get();
}

} // namespace s2sgeo
