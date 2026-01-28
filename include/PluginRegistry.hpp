/**
 * @file PluginRegistry.hpp
 * @brief Registry for loading and managing context provider plugins
 */

#ifndef S2SGEO_PLUGIN_REGISTRY_HPP
#define S2SGEO_PLUGIN_REGISTRY_HPP

#include "IGeoProvider.hpp"
#include <map>
#include <memory>
#include <string>
#include <functional>

namespace s2sgeo {

/**
 * @class PluginRegistry
 * @brief Factory pattern for plugin management
 */
class PluginRegistry {
public:
    using ProviderFactory = std::function<std::unique_ptr<IContextProvider>()>;
    
    static PluginRegistry& getInstance();
    
    /**
     * @brief Register a new provider plugin
     */
    void registerProvider(const std::string& name, ProviderFactory factory);
    
    /**
     * @brief Activate a provider by name
     */
    bool activateProvider(const std::string& name);
    
    /**
     * @brief Get the active provider
     */
    IContextProvider* getActiveProvider();
    
    /**
     * @brief List all available providers
     */
    std::vector<std::string> listProviders() const;
    
    /**
     * @brief Get provider by name (without activating)
     */
    IContextProvider* getProvider(const std::string& name);
    
private:
    PluginRegistry() = default;
    
    std::map<std::string, ProviderFactory> factories_;
    std::map<std::string, std::unique_ptr<IContextProvider>> instances_;
    IContextProvider* active_provider_ = nullptr;
    std::string active_provider_name_;
};

} // namespace s2sgeo

#endif // S2SGEO_PLUGIN_REGISTRY_HPP
