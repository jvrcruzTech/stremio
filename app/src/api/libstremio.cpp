#include "api/libstremio.hpp"
#include <logger.hpp>

void StremioAPI::login(string email = "", string password = "") {
    if ((email.empty() || password.empty()) && (AppConfig::instance().getUser().email.empty() || AppConfig::instance().getUser().passwd.empty())) {
        brls::Logger::error("StremioAPI::login: email or password is empty");
        return;
    }
    StremioUtils::login(
        email.empty() ? AppConfig::instance().getUser().email : email,
        password.empty() ? AppConfig::instance().getUser().passwd : password
    );
}

void StremioAPI::init() {
    if (AppConfig::instance().getUser().email.empty() || AppConfig::instance().getUser().passwd.empty()) {
        brls::Logger::error("StremioAPI::init: user is not logged in");
        return;
    }

    
    // Fetch addons and catalogs
    AddonCollection::updateAddonCollection();
}

void StremioAPI::fetchAddons() {
    AddonCollection::updateAddonCollection();
}

list<Catalog> StremioAPI::fetchCatalogs(const std::string& type) {
    list<Addon> addons = AddonCollection::getAddons();
    list<Catalog> catalogs;

    for (const auto& addon : addons) {
        if (addon.getResources().contains("catalog")) {
            nlohmann::json catalogDescriptors = addon.getCatalogs();
            for (const auto& catalogDescriptor : catalogDescriptors) {
                if (catalogDescriptor["type"] == type) {
                    try {
                        Catalog catalog = addon.getCatalog(catalogDescriptor["type"], catalogDescriptor["id"]);
                        catalogs.push_back(catalog);
                    } catch (const std::exception& e) {
                        brls::Logger::error("StremioAPI::fetchCatalogs: {}", e.what());
                    }
                }
            }
        }
    }

    return catalogs;
}