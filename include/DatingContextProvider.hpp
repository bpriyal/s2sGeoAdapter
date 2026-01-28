/**
 * @file DatingContextProvider.hpp
 * @brief Context provider for dating applications
 */

#ifndef S2SGEO_DATING_CONTEXT_PROVIDER_HPP
#define S2SGEO_DATING_CONTEXT_PROVIDER_HPP

#include "IGeoProvider.hpp"

namespace s2sgeo {

/**
 * @class DatingContextProvider
 * @brief Fetches nearby users and venues for dating apps
 */
class DatingContextProvider : public IContextProvider {
public:
    void initialize(const std::string& config) override;
    ContextFrame getContext(double lat, double lon) override;
    void prefetchContext(double lat, double lon, double heading, double distance) override;
    std::string getName() const override { return "dating"; }
    
private:
    std::string dating_api_endpoint_;
};

} // namespace s2sgeo

#endif // S2SGEO_DATING_CONTEXT_PROVIDER_HPP
